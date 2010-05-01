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
#include <string.h>


#include "net_helper.h"
#include "ml.h"

#include "msg_types.h"/**/


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

static int sIdx = 0;
static int rIdx = 0;

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
static int lookup_max = 1000;
static int lookup_curr = 0;

static nodeID *me; //TODO: is it possible to get rid of this (notwithstanding ml callback)??
static int timeoutFired = 0;

// pointers to the msgs to be send
static uint8_t *sendingBuffer[NH_BUFFER_SIZE];
// pointers to the received msgs + sender nodeID
struct receivedB {
	struct nodeID *id;
	int len;
	uint8_t *data;
};
static struct receivedB receivedBuffer[NH_BUFFER_SIZE];
/**/ static int recv_counter =0; static int snd_counter =0;


static struct nodeID *new_node(socketID_handle peer, int conn_id) {
	 struct nodeID *res = malloc(sizeof(struct nodeID));
	 if (!res)
		 return NULL;
	 memset(res, 0, sizeof(struct nodeID));
	 res->addr = malloc(SOCKETID_SIZE);
	 if (res->addr) {
		 memset(res->addr, 0, SOCKETID_SIZE);
		 memcpy(res->addr, peer ,SOCKETID_SIZE);
		 //	remote->addrSize = SOCKETID_SIZE;
		 //	remote->addrStringSize = SOCKETID_STRING_SIZE;
		 res->connID = conn_id;
		 res->refcnt = 1;
	 }
	 else {
		 free (res);
		 fprintf(stderr, "Net-helper : memory error while creating a new nodeID \n");
		 return NULL;
	 }

	 return res;
}


static struct nodeID *id_lookup(socketID_handle target, int conn_id) {

	int i,here=-1;
	for (i=0;i<lookup_curr;i++) {
		if (lookup_array[i] == NULL) {
			if (here < 0)
				here = i;
		}
		else if (!mlCompareSocketIDs(lookup_array[i]->addr,target)) {
			return nodeid_dup(lookup_array[i]);
		}
		else {
			if (lookup_array[i]->refcnt == 1) {
				nodeid_free(lookup_array[i]);
				lookup_array[i] = NULL;
				if (here < 0)
					here = i;
			}
		}
	}

	if (here >= 0) {
		lookup_array[here] = new_node(target,conn_id);
		return nodeid_dup(lookup_array[here]);
	}

	if (lookup_curr == lookup_max) {
		lookup_max *= 2;
		lookup_array = realloc(lookup_array,lookup_max*sizeof(struct nodeID*));
	}

	lookup_array[lookup_curr] = new_node(target,conn_id);
	return nodeid_dup(lookup_array[lookup_curr++]);

}


/**
 * Look for a free slot in the received buffer and allocates it for immediate use
 * @return the index of a free slot in the received msgs buffer, -1 if no free slot available.
 */
static int next_R() {
	const int size = 1024;
	if (receivedBuffer[rIdx].data==NULL) {
		receivedBuffer[rIdx].data = malloc(size);
	}
	else {
		int count;
		for (count=0;count<NH_BUFFER_SIZE;count++) {
			rIdx = (++rIdx)%NH_BUFFER_SIZE;
			if (receivedBuffer[rIdx].data==NULL)
				break;
		}
		if (count==NH_BUFFER_SIZE)
			return -1;
		else {
			receivedBuffer[rIdx].data = malloc(size);
		}
	}
	memset(receivedBuffer[rIdx].data,0,size);
	return rIdx;
}

/**
 * Look for a free slot in the sending buffer and allocates it for immediate use
 * @return the index of a free slot in the sending msgs buffer, -1 if no free slot available.
 */
static int next_S() {
	const int size = 1024;
	if (sendingBuffer[sIdx]==NULL) {
		sendingBuffer[sIdx] = malloc(size);
	}
	else {
		int count;
		for (count=0;count<NH_BUFFER_SIZE;count++) {
			sIdx = (++sIdx)%NH_BUFFER_SIZE;
			if (sendingBuffer[sIdx]==NULL)
				break;
		}
		if (count==NH_BUFFER_SIZE)
			return -1;
		else {
			sendingBuffer[sIdx] = malloc(size);
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
		//
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
/**/char mt = ((char*)sendingBuffer[p->bIdx])[0]; ++snd_counter;
	if (mt!=MSG_TYPE_TOPOLOGY &&
		mt!=MSG_TYPE_CHUNK && mt!=MSG_TYPE_SIGNALLING) {
			fprintf(stderr,"Net-helper ERROR! Sent message # %d of type %c and size %d\n",
				snd_counter,mt+'0', p->mSize);}
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
//	fprintf(stderr, "Net-helper : called back with some news...\n");
/**/ ++recv_counter;
	char str[SOCKETID_STRING_SIZE];
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
		if (index >=0) {
		//	receivedBuffer[index][0] = malloc(buflen);
			if (receivedBuffer[index].data == NULL) {
				fprintf(stderr, "Net-helper : memory error while creating a new message buffer \n");
				return;
			}
			// creating a new sender nodedID
			receivedBuffer[index].id = id_lookup(arg->remote_socketID, arg->connectionID);
				receivedBuffer[index].data = realloc(receivedBuffer[index].data,buflen);
				memset(receivedBuffer[index].data,0,buflen);
				receivedBuffer[index].len = buflen;
				//*(receivedBuffer[index][0]) = buflen;
				memcpy(receivedBuffer[index].data,buffer,buflen);
				  // get the socketID of the sender
		}
  }
//	event_base_loopbreak(base);
}


struct nodeID *net_helper_init(const char *IPaddr, int port) {

	struct timeval tout = NH_ML_INIT_TIMEOUT;
	int s;
	base = event_base_new();
	lookup_array = calloc(lookup_max,sizeof(struct nodeID *));

	me = malloc(sizeof(nodeID));
	if (me == NULL) {
		return NULL;
	}
	memset(me,0,sizeof(nodeID));
	me->addr = malloc(SOCKETID_SIZE);
	if (me->addr == NULL) {
		free(me);
		return NULL;
	}
	memset(me->addr,0,SOCKETID_SIZE);
	me->connID = -10;	// dirty trick to spot later if the ml has called back ...
	me->refcnt = 1;

	int i;
	for (i=0;i<NH_BUFFER_SIZE;i++) {
		sendingBuffer[i] = NULL;
		receivedBuffer[i].data = NULL;
	}

	mlRegisterErrorConnectionCb(&connError_cb);
	mlRegisterRecvConnectionCb(&receive_conn_cb);
	s = mlInit(1,tout,port,IPaddr,3478,"stun.ekiga.net",&init_myNodeID_cb,base);
	if (s < 0) {
		fprintf(stderr, "Net-helper : error initializing ML!\n");
		free(me);
		return NULL;
	}

#ifdef MONL
	eventbase = base;
	void *repoclient;

	// Initialize logging
	grapesInitLog(DCLOG_WARNING, NULL, NULL);

	repInit("");
	repoclient = repOpen("repository.napa-wine.eu:9832",60);
	if (repoclient == NULL) fatal("Unable to initialize repoclient");
	monInit(base, repoclient);
#endif

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
	// if buffer is full, discard the message and return an error flag
	int index = next_S();
	if (index<0) {
		// free(buffer_ptr);
		fprintf(stderr,"Net-helper: buffer full\n ");
		return -1;
	}
	sendingBuffer[index] = realloc(sendingBuffer[index],buffer_size);
	memset(sendingBuffer[index],0,buffer_size);
	memcpy(sendingBuffer[index],buffer_ptr,buffer_size);
	// free(buffer_ptr);
	msgData_cb *p = malloc(sizeof(msgData_cb));
	p->bIdx = index; p->mSize = buffer_size; p->msgType = (unsigned char)buffer_ptr[0]; p->conn_cb_called = false; p->cancelled = false;
	int current = p->bIdx;
	send_params params = {0,0,0,0};
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
	if (receivedBuffer[rIdx].id==NULL) {	//block till first message arrives
		wait4data(local, NULL, NULL);
	}

	(*remote) = receivedBuffer[rIdx].id;
	// retrieve a msg from the buffer
	size = receivedBuffer[rIdx].len;
	if (size>buffer_size) {
		fprintf(stderr, "Net-helper : recv_from_peer: buffer too small (size:%d > buffer_size: %d)!\n",size,buffer_size);
		return -1;
	}
	memcpy(buffer_ptr, receivedBuffer[rIdx].data, size);
	free(receivedBuffer[rIdx].data);
	receivedBuffer[rIdx].data = NULL;
	receivedBuffer[rIdx].id = NULL;

//	fprintf(stderr, "Net-helper : I've got mail!!!\n");

	return size;
}


int wait4data(const struct nodeID *n, struct timeval *tout, fd_set *dummy) {

//	fprintf(stderr,"Net-helper : Waiting for data to come...\n");
	if (tout) {	//if tout==NULL, loop wait infinitely
		event_base_once(base,-1, EV_TIMEOUT, &t_out_cb, NULL, tout);
	}
	while(receivedBuffer[rIdx].data==NULL && timeoutFired==0) {
	//	event_base_dispatch(base);
		event_base_loop(base,EVLOOP_ONCE);
	}
	timeoutFired = 0;
//	fprintf(stderr,"Back from eventlib loop.\n");

	if (receivedBuffer[rIdx].data!=NULL)
		return 1;
	else
		return 0;
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
	struct nodeID *remote = malloc(sizeof(nodeID));
	if (remote == NULL) {
		return NULL;
	}
	remote->addr = getRemoteSocketID(rem_IP, rem_port);
	send_params params = {0,0,0,0};
	remote->connID = mlOpenConnection(remote->addr,&connReady_cb,NULL, params);
	remote->refcnt = 1;
	return remote;
}

// TODO: check why closing the connection is annoying for the ML
void nodeid_free(struct nodeID *n) {

//	mlCloseConnection(n->connID);
//	mlCloseSocket(n->addr);
//	free(n);
	if (n && (--(n->refcnt) == 0)) {
	//	mlCloseConnection(n->connID);
		mlCloseSocket(n->addr);
		free(n);
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
//  struct nodeID *res;
//
//  res = malloc(sizeof(struct nodeID));
//  if (res != NULL) {
//	  res->addr = malloc(SOCKETID_SIZE);
//	  if (res->addr != NULL) {
//		 memcpy(res->addr, s->addr, SOCKETID_SIZE);
//	//	 res->addrSize = SOCKETID_SIZE;
//	//	 res->addrStringSize = SOCKETID_STRING_SIZE;
//		 res->connID = s->connID;
//	  }
//	  else {
//		free(res);
//		res = NULL;
//		fprintf(stderr,"Net-helper : Error while duplicating nodeID...\n");
//	  }
//  }
//	return res;
	s->refcnt++;
	return s;
}

int nodeid_equal(const struct nodeID *s1, const struct nodeID *s2)
{
	return (mlCompareSocketIDs(s1->addr,s2->addr) == 0);
}

int nodeid_dump(uint8_t *b, const struct nodeID *s)
{
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
  return id_lookup(h,-1);
}
