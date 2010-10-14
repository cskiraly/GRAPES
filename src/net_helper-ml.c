/*
 *  Copyright (c) 2009 Marco Biazzini
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <event2/event.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "net_helper.h"
#include "ml.h"
#include "config.h"

#include "grapes_msg_types.h"


#ifdef MONL
#include "mon.h"
#include "grapes_log.h"
#include "repoclient.h"
#include "grapes.h"
#endif


/**
 * libevent pointer
 */
struct event_base *base;

#define NH_BUFFER_SIZE 1000
#define NH_LOOKUP_SIZE 1000
#define NH_PACKET_TIMEOUT {0, 500*1000}
#define NH_ML_INIT_TIMEOUT {1, 0}

#define FDSSIZE 16

static int sIdx = 0;
static int rIdxML = 0;	//reveive from ML to this buffer position
static int rIdxUp = 0;	//hand up to layer above at this buffer position

typedef struct nodeID {
	socketID_handle addr;
	int connID;	// connection associated to this node, -1 if myself
	int refcnt;
#ifdef MONL
	//n quick and dirty static vector for measures TODO: make it dinamic
	MonHandler mhs[20];
	int n_mhs;
#endif
//	int addrSize;
//	int addrStringSize;
} nodeID;

typedef struct msgData_cb {
	int bIdx;	// index of the message in the proper buffer
	unsigned char msgType; // message type
	int mSize;	// message size
	bool conn_cb_called;
	bool cancelled;
} msgData_cb;

static struct nodeID **lookup_array;
static int lookup_max = NH_LOOKUP_SIZE;
static int lookup_curr = 0;

static nodeID *me; //TODO: is it possible to get rid of this (notwithstanding ml callback)??
static int timeoutFired = 0;
static bool fdTriggered = false;

// pointers to the msgs to be send
static uint8_t *sendingBuffer[NH_BUFFER_SIZE];
// pointers to the received msgs + sender nodeID
struct receivedB {
	struct nodeID *id;
	int len;
	uint8_t *data;
};
static struct receivedB receivedBuffer[NH_BUFFER_SIZE];
/**/ static int recv_counter =0;


static void connReady_cb (int connectionID, void *arg);
static struct nodeID *new_node(socketID_handle peer) {
	send_params params = {0,0,0,0};
	struct nodeID *res = malloc(sizeof(struct nodeID));
	if (!res) {
		 fprintf(stderr, "Net-helper : memory error\n");
		 return NULL;
	}
	memset(res, 0, sizeof(struct nodeID));

	res->addr = malloc(SOCKETID_SIZE);
	if (! res->addr) {
		free (res);
		fprintf(stderr, "Net-helper : memory error while creating a new nodeID \n");
		return NULL;
	}
	memset(res->addr, 0, SOCKETID_SIZE);
	memcpy(res->addr, peer ,SOCKETID_SIZE);

	res->refcnt = 1;

	res->connID = mlOpenConnection(peer, &connReady_cb, NULL, params);

	return res;
}


static struct nodeID **id_lookup(socketID_handle target) {

	int i,here=-1;
	for (i=0;i<lookup_curr;i++) {
		if (lookup_array[i] == NULL) {
			if (here < 0) {
				here = i;
			}
		} else if (!mlCompareSocketIDs(lookup_array[i]->addr,target)) {
			return &lookup_array[i];
		}
	}

	if (here == -1) {
		here = lookup_curr++;
	}

	if (lookup_curr > lookup_max) {
		lookup_max *= 2;
		lookup_array = realloc(lookup_array,lookup_max*sizeof(struct nodeID*));
	}

	lookup_array[here] = new_node(target);
	return &lookup_array[here];
}

static struct nodeID *id_lookup_dup(socketID_handle target) {
	return nodeid_dup(*id_lookup(target));
}


/**
 * Look for a free slot in the received buffer and allocates it for immediate use
 * @return the index of a free slot in the received msgs buffer, -1 if no free slot available.
 */
static int next_R() {
	if (receivedBuffer[rIdxML].data==NULL) {
		int ret = rIdxML;
		rIdxML = (rIdxML+1)%NH_BUFFER_SIZE;
		return ret;
	} else {
		//TODO: handle receive overload situation!
		return -1;
	}
}

/**
 * Look for a free slot in the sending buffer and allocates it for immediate use
 * @return the index of a free slot in the sending msgs buffer, -1 if no free slot available.
 */
static int next_S() {
	if (sendingBuffer[sIdx]) {
		int count;
		for (count=0;count<NH_BUFFER_SIZE;count++) {
			sIdx = (sIdx+1)%NH_BUFFER_SIZE;
			if (sendingBuffer[sIdx]==NULL)
				break;
		}
		if (count==NH_BUFFER_SIZE) {
			return -1;
		}
	}
	return sIdx;
}


/**
 * Callback used by ml to confirm its initialization. Create a valid self nodeID and register to receive data from remote peers.
 * @param local_socketID
 * @param errorstatus
 */
static void init_myNodeID_cb (socketID_handle local_socketID,int errorstatus) {
	switch (errorstatus) {
	case 0:
		me->addr = malloc(SOCKETID_SIZE);
		if (! me->addr) {
			fprintf(stderr, "Net-helper : memory error while creating my new nodeID \n");
			return;
		}

		memcpy(me->addr,local_socketID,SOCKETID_SIZE);
	//	me->addrSize = SOCKETID_SIZE;
	//	me->addrStringSize = SOCKETID_STRING_SIZE;
		me->connID = -1;
		me->refcnt = 1;
	//	fprintf(stderr,"Net-helper init : received my own socket: %s.\n",node_addr(me));
		break;
	case -1:
		//
		fprintf(stderr,"Net-helper init : socket error occurred in ml while creating socket\n");
		exit(1);
		break;
	case 1:
		//
		fprintf(stderr,"Net-helper init : NAT traversal failed while creating socket\n");
		exit(1);
		break;
	case 2:
	    fprintf(stderr,"Net-helper init : NAT traversal timeout while creating socket\n");
	    fprintf(stderr,"Net-helper init : Retrying without STUN\n");
	    mlSetStunServer(0,NULL);
	    break;
	default :	// should never happen
		//
		fprintf(stderr,"Net-helper init : Unknown error in ml while creating socket\n");
	}

}

/**
 * Timeout callback to be set in the eventlib loop as needed
 * @param socket
 * @param flag
 * @param arg
 */
static void t_out_cb (int socket, short flag, void* arg) {

	timeoutFired = 1;
//	fprintf(stderr,"TIMEOUT!!!\n");
//	event_base_loopbreak(base);
}

/**
 * File descriptor readable callback to be set in the eventlib loop as needed
 */
static void fd_cb (int fd, short flag, void* arg)
{
  //fprintf(stderr, "\twait4data: fd %d triggered\n", fd);
  fdTriggered = true;
  *((bool*)arg) = true;
}

/**
 * Callback called by ml when a remote node ask for a connection
 * @param connectionID
 * @param arg
 */
static void receive_conn_cb(int connectionID, void *arg) {
//    fprintf(stderr, "Net-helper : remote peer opened the connection %d with arg = %d\n", connectionID,(int)arg);

}

void free_sending_buffer(int i)
{
	free(sendingBuffer[i]);
	sendingBuffer[i] = NULL;
}

/**
 * Callback called by the ml when a connection is ready to be used to send data to a remote peer
 * @param connectionID
 * @param arg
 */
static void connReady_cb (int connectionID, void *arg) {

	msgData_cb *p;
	p = (msgData_cb *)arg;
	if (p == NULL) return;
	if (p->cancelled) {
	    free(p);
	    return;
	}
	mlSendData(connectionID,(char *)(sendingBuffer[p->bIdx]),p->mSize,p->msgType,NULL);
	free_sending_buffer(p->bIdx);
//	fprintf(stderr,"Net-helper: Message # %d for connection %d sent!\n ", p->bIdx,connectionID);
	//	event_base_loopbreak(base);
	p->conn_cb_called = true;
}

/**
 * Callback called by ml when a connection error occurs
 * @param connectionID
 * @param arg
 */
static void connError_cb (int connectionID, void *arg) {
	// simply get rid of the msg in the buffer....
	msgData_cb *p;
	p = (msgData_cb *)arg;
	if (p != NULL) {
		fprintf(stderr,"Net-helper: Connection %d could not be established to send msg %d.\n ", connectionID,p->bIdx);
		if (p->cancelled) {
			free(p);//p->mSize = -1;
		} else {
			p->conn_cb_called = true;
		}
	}
	//	event_base_loopbreak(base);
}


/**
 * Callback to receive data from ml
 * @param buffer
 * @param buflen
 * @param msgtype
 * @param arg
 */
static void recv_data_cb(char *buffer, int buflen, unsigned char msgtype, recv_params *arg) {
// TODO: lacks a void* arg... moreover: recv_params has a msgtype, but there is also a msgtype explicit argument...
	char str[SOCKETID_STRING_SIZE];
//	fprintf(stderr, "Net-helper : called back with some news...\n");
/**/ ++recv_counter;
	if (arg->remote_socketID != NULL)
		mlSocketIDToString(arg->remote_socketID,str,SOCKETID_STRING_SIZE);
	else
		sprintf(str,"!Unknown!");
	if (arg->nrMissingBytes || !arg->firstPacketArrived) {
	    fprintf(stderr, "Net-helper : corrupted message arrived from %s\n",str);
/**/    fprintf(stderr, "\tMessage # %d -- Message type: %hhd -- Missing # %d bytes%s\n",
			recv_counter, buffer[0],arg->nrMissingBytes, arg->firstPacketArrived?"":", Missing first!");
	}
	else {
	//	fprintf(stderr, "Net-helper : message arrived from %s\n",str);
		// buffering the received message only if possible, otherwise ignore it...
		int index = next_R();
		if (index<0) {
			fprintf(stderr,"Net-helper: receive buffer full\n ");
			return;
		} else {
			receivedBuffer[index].data = malloc(buflen);
			if (receivedBuffer[index].data == NULL) {
				fprintf(stderr,"Net-helper: memory full, can't receive!\n ");
				return;
			}
			receivedBuffer[index].len = buflen;
			memcpy(receivedBuffer[index].data,buffer,buflen);
			  // save the socketID of the sender
			receivedBuffer[index].id = id_lookup_dup(arg->remote_socketID);
		}
  }
//	event_base_loopbreak(base);
}


struct nodeID *net_helper_init(const char *IPaddr, int port, const char *config) {

	struct timeval tout = NH_ML_INIT_TIMEOUT;
	int s, i;
	struct tag *cfg_tags;
	const char *res;
	const char *stun_server = "stun.ekiga.net";
	int stun_port = 3478;
	const char *repo_address = "79.120.193.115:9832";
	int publish_interval = 60;

	signal(SIGPIPE, SIG_IGN); // workaround for a known issue in libevent2 with SIGPIPE on TPC connections
	base = event_base_new();
	lookup_array = calloc(lookup_max,sizeof(struct nodeID *));

	cfg_tags = config_parse(config);
	if (!cfg_tags) {
		return NULL;
	}

	res = config_value_str(cfg_tags, "stun_server");
	if (res) {
		stun_server = res;
	}
	config_value_int(cfg_tags, "stun_port", &stun_port);

	res = config_value_str(cfg_tags, "repo_address");
	if (res) {
		repo_address = res;
	}
	
	config_value_int(cfg_tags, "publish_interval", &publish_interval);

	me = malloc(sizeof(nodeID));
	if (me == NULL) {
		return NULL;
	}
	memset(me,0,sizeof(nodeID));
	me->connID = -10;	// dirty trick to spot later if the ml has called back ...
	me->refcnt = 1;

	for (i=0;i<NH_BUFFER_SIZE;i++) {
		sendingBuffer[i] = NULL;
		receivedBuffer[i].data = NULL;
	}

	mlRegisterErrorConnectionCb(&connError_cb);
	mlRegisterRecvConnectionCb(&receive_conn_cb);
	s = mlInit(1, tout, port, IPaddr, stun_port, stun_server, &init_myNodeID_cb, base);
	if (s < 0) {
		fprintf(stderr, "Net-helper : error initializing ML!\n");
		free(me);
		return NULL;
	}

#ifdef MONL
{
	void *repoclient;
	eventbase = base;

	// Initialize logging
	grapesInitLog(DCLOG_WARNING, NULL, NULL);

	repInit("");
	repoclient = repOpen(repo_address, publish_interval);	//repository.napa-wine.eu
	if (repoclient == NULL) fatal("Unable to initialize repoclient");
	monInit(base, repoclient);
}
#endif
	free(cfg_tags);

	while (me->connID<-1) {
	//	event_base_once(base,-1, EV_TIMEOUT, &t_out_cb, NULL, &tout);
		event_base_loop(base,EVLOOP_ONCE);
	}
	timeoutFired = 0;
//	fprintf(stderr,"Net-helper init : back from init!\n");

	return me;
}


void bind_msg_type (unsigned char msgtype) {

			mlRegisterRecvDataCb(&recv_data_cb,msgtype);
}


void send_to_peer_cb(int fd, short event, void *arg)
{
	msgData_cb *p = (msgData_cb *) arg;
	if (p->conn_cb_called) {
		free(p);
	}
	else { //don't send it anymore
		free_sending_buffer(p->bIdx);
		p->cancelled = true;
		// don't free p, the other timeout will do it
	}
}

/**
 * Called by the application to send data to a remote peer
 * @param from
 * @param to
 * @param buffer_ptr
 * @param buffer_size
 * @return The dimension of the buffer or -1 if a connection error occurred.
 */
int send_to_peer(const struct nodeID *from, struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size)
{
	msgData_cb *p;
	int current;
	send_params params = {0,0,0,0};

	if (buffer_size <= 0) {
		fprintf(stderr,"Net-helper: message size problematic: %d\n", buffer_size);
		return buffer_size;
	}

	// if buffer is full, discard the message and return an error flag
	int index = next_S();
	if (index<0) {
		// free(buffer_ptr);
		fprintf(stderr,"Net-helper: send buffer full\n ");
		return -1;
	}
	sendingBuffer[index] = malloc(buffer_size);
	if (! sendingBuffer[index]){
		fprintf(stderr,"Net-helper: memory full, can't send!\n ");
		return -1;
	}
	memset(sendingBuffer[index],0,buffer_size);
	memcpy(sendingBuffer[index],buffer_ptr,buffer_size);
	// free(buffer_ptr);
	p = malloc(sizeof(msgData_cb));
	p->bIdx = index; p->mSize = buffer_size; p->msgType = (unsigned char)buffer_ptr[0]; p->conn_cb_called = false; p->cancelled = false;
	current = p->bIdx;

	to->connID = mlOpenConnection(to->addr,&connReady_cb,p, params);
	if (to->connID<0) {
		free_sending_buffer(current);
		fprintf(stderr,"Net-helper: Couldn't get a connection ID to send msg %d.\n ", p->bIdx);
		free(p);
		return -1;
	}
	else {
		struct timeval timeout = NH_PACKET_TIMEOUT;
		event_base_once(base, -1, EV_TIMEOUT, send_to_peer_cb, (void *) p, &timeout);
		return buffer_size; //p->mSize;
	}

}


/**
 * Called by an application to receive data from remote peers
 * @param local
 * @param remote
 * @param buffer_ptr
 * @param buffer_size
 * @return The number of received bytes or -1 if some error occurred.
 */
int recv_from_peer(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size)
{
	int size;
	if (receivedBuffer[rIdxUp].data==NULL) {	//block till first message arrives
		wait4data(local, NULL, NULL);
	}

	assert(receivedBuffer[rIdxUp].data && receivedBuffer[rIdxUp].id);

	(*remote) = receivedBuffer[rIdxUp].id;
	// retrieve a msg from the buffer
	size = receivedBuffer[rIdxUp].len;
	if (size>buffer_size) {
		fprintf(stderr, "Net-helper : recv_from_peer: buffer too small (size:%d > buffer_size: %d)!\n",size,buffer_size);
		return -1;
	}
	memcpy(buffer_ptr, receivedBuffer[rIdxUp].data, size);
	free(receivedBuffer[rIdxUp].data);
	receivedBuffer[rIdxUp].data = NULL;
	receivedBuffer[rIdxUp].id = NULL;

	rIdxUp = (rIdxUp+1)%NH_BUFFER_SIZE;

//	fprintf(stderr, "Net-helper : I've got mail!!!\n");

	return size;
}


int wait4data(const struct nodeID *n, struct timeval *tout, int *fds) {

	struct event *timeout_ev = NULL;
	struct event *fd_ev[FDSSIZE];
	bool fd_triggered[FDSSIZE] = { false };
	int i;

//	fprintf(stderr,"Net-helper : Waiting for data to come...\n");
	if (tout) {	//if tout==NULL, loop wait infinitely
	  timeout_ev = event_new(base, -1, EV_TIMEOUT, &t_out_cb, NULL);
	  event_add(timeout_ev, tout);
	}
	for (i = 0; fds && fds[i] != -1; i ++) {
	  if (i >= FDSSIZE) {
	    fprintf(stderr, "Can't listen on more than %d file descriptors!\n", FDSSIZE);
	    break;
	  }
	  fd_ev[i] = event_new(base, fds[i], EV_READ, &fd_cb, &fd_triggered[i]);
	  event_add(fd_ev[i], NULL);
	}

	while(receivedBuffer[rIdxUp].data==NULL && timeoutFired==0 && fdTriggered==0) {
	//	event_base_dispatch(base);
		event_base_loop(base,EVLOOP_ONCE);
	}

	//delete one-time events
	if (timeout_ev) {
	  if (!timeoutFired) event_del(timeout_ev);
	  event_free(timeout_ev);
	}
	for (i = 0; fds && fds[i] != -1; i ++) {
	  if (! fd_triggered[i]) {
	    fds[i] = -2;
	    event_del(fd_ev[i]);
	  //} else {
	    //fprintf(stderr, "\twait4data: fd %d triggered\n", fds[i]);
	  }
	  event_free(fd_ev[i]);
	}

	if (fdTriggered) {
	  fdTriggered = false;
	  //fprintf(stderr, "\twait4data: fd event\n");
	  return 2;
	} else if (timeoutFired) {
	  timeoutFired = 0;
	  //fprintf(stderr, "\twait4data: timed out\n");
	  return 0;
	} else if (receivedBuffer[rIdxUp].data!=NULL) {
	  //fprintf(stderr, "\twait4data: ML receive\n");
	  return 1;
	} else {
	  fprintf(stderr, "BUG in wait4data\n");
	  exit(EXIT_FAILURE);
	}
}

socketID_handle getRemoteSocketID(const char *ip, int port) {
	char str[SOCKETID_STRING_SIZE];
	socketID_handle h;

	snprintf(str, SOCKETID_STRING_SIZE, "%s:%d-%s:%d", ip, port, ip, port);
	h = malloc(SOCKETID_SIZE);
	mlStringToSocketID(str, h);

	return h;
}

struct nodeID *create_node(const char *rem_IP, int rem_port) {
	socketID_handle s;
	struct nodeID *remote;

	s = getRemoteSocketID(rem_IP, rem_port);
	remote = id_lookup_dup(s);
	free(s);

	return remote;
}

const char *node_ip(const struct nodeID *s) {
	static char ip[64];
	int len;
	char *start, *end;
	const char *tmp = node_addr(s);
	start = strstr(tmp, "-") + 1;
	end = strstr(start, ":");
	len = end - start;
	memcpy(ip, start, len);
	ip[len] = 0;

	return (const char *)ip;
}


// TODO: check why closing the connection is annoying for the ML
void nodeid_free(struct nodeID *n) {
	if (n && (--(n->refcnt) == 1)) {
/*
		struct nodeID **npos;
	//	mlCloseConnection(n->connID);
		npos = id_lookup(n->addr);
		*npos = NULL;
		mlCloseSocket(n->addr);
		free(n);
*/
	}
}


const char *node_addr(const struct nodeID *s)
{
  static char addr[256];
  // TODO: mlSocketIDToString always return 0 !!!
  int r = mlSocketIDToString(s->addr,addr,256);
  if (!r)
	  return addr;
  else
	  return "";
}

struct nodeID *nodeid_dup(struct nodeID *s)
{
	s->refcnt++;
	return s;
}

int nodeid_equal(const struct nodeID *s1, const struct nodeID *s2)
{
	return (mlCompareSocketIDs(s1->addr,s2->addr) == 0);
}

int nodeid_dump(uint8_t *b, const struct nodeID *s, size_t max_write_size)
{
  if (max_write_size < SOCKETID_STRING_SIZE) return -1;

  mlSocketIDToString(s->addr,(char *)b,SOCKETID_STRING_SIZE);
  //fprintf(stderr,"Dumping nodeID : ho scritto %s (%d bytes)\n",b, strlen((char *)b));

//	memcpy(b, s->addr,SOCKETID_SIZE);//sizeof(struct sockaddr_in6)*2
//	return SOCKETID_SIZE;//sizeof(struct sockaddr_in6)*2;

  return 1 + strlen((char *)b);	//terminating \0 IS included in the size
}

struct nodeID *nodeid_undump(const uint8_t *b, int *len)
{
  uint8_t sid[SOCKETID_SIZE];
  socketID_handle h = (socketID_handle) sid;
  mlStringToSocketID((char *)b,h);
  *len = strlen((char*)b) + 1;
  return id_lookup_dup(h);
}
