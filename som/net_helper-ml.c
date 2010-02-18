/*
 *  Copyright (c) 2009 Marco Biazzini
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <event2/event.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "net_helper.h"
#include "ml.h"
#include "ml_helpers.h"

/**
 * libevent pointer
 */
struct event_base *base;

#define NH_BUFFER_SIZE 100

static int sIdx = 0;
static int rIdx = 0;

typedef struct nodeID {
	socketID_handle addr;
	int connID;	// connection associated to this node, -1 if myself
	int refcnt;
//	int addrSize;
//	int addrStringSize;
} nodeID;

typedef struct msgData_cb {
	int bIdx;	// index of the message in the proper buffer
	unsigned char msgType; // message type
	int mSize;	// message size
} msgData_cb;

static nodeID *me; //TODO: is it possible to get rid of this (notwithstanding ml callback)??
static int timeoutFired = 0;

// pointers to the msgs to be send
static uint8_t *sendingBuffer[NH_BUFFER_SIZE];
// pointers to the received msgs + sender nodeID
static uint8_t *receivedBuffer[NH_BUFFER_SIZE][2];


/**
 * Look for a free slot in the received buffer and allocates it for immediate use
 * @return the index of a free slot in the received msgs buffer, -1 if no free slot available.
 */
static int next_R() {
	const int size = 1024;
	if (receivedBuffer[rIdx][0]==NULL) {
		receivedBuffer[rIdx][0] = malloc(size);
	}
	else {
		int count;
		for (count=0;count<NH_BUFFER_SIZE;count++) {
			rIdx = (++rIdx)%NH_BUFFER_SIZE;
			if (receivedBuffer[rIdx][0]==NULL)
				break;
		}
		if (count==NH_BUFFER_SIZE)
			return -1;
		else {
			receivedBuffer[rIdx][0] = malloc(size);
		}
	}
	memset(receivedBuffer[rIdx][0],0,size);
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
	    exit(2);
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

/**
 * Callback called by the ml when a connection is ready to be used to send data to a remote peer
 * @param connectionID
 * @param arg
 */
static void connReady_cb (int connectionID, void *arg) {

	msgData_cb *p;
	p = (msgData_cb *)arg;
	if (p == NULL) return;
	send_params params = {0,0,0,0};
	send_Data(connectionID,(char *)(sendingBuffer[p->bIdx]),p->mSize,p->msgType,&params);
	free(sendingBuffer[p->bIdx]);
	sendingBuffer[p->bIdx] = NULL;
//	fprintf(stderr,"Net-helper: Message # %d for connection %d sent!\n ", p->bIdx,connectionID);
	//	event_base_loopbreak(base);
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
		free(sendingBuffer[p->bIdx]);
		sendingBuffer[p->bIdx] = NULL;
		p->mSize = -1;
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
	char str[SOCKETID_STRING_SIZE];
	if (arg->remote_socketID != NULL)
		socketID_To_String(arg->remote_socketID,str,SOCKETID_STRING_SIZE);
	else
		sprintf(str,"!Unknown!");
	if (arg->nrMissingBytes || !arg->firstPacketArrived) {
	    fprintf(stderr, "Net-helper : corrupted message arrived from %s\n",str);
	}
	else {
	//	fprintf(stderr, "Net-helper : message arrived from %s\n",str);
		// buffering the received message only if possible, otherwise ignore it...
		int index = next_R();
		if (index >=0) {
		//	receivedBuffer[index][0] = malloc(buflen);
			if (receivedBuffer[index][0] == NULL) {
				fprintf(stderr, "Net-helper : memory error while creating a new message buffer \n");
				return;
			}
			// creating a new sender nodedID
			receivedBuffer[index][1] = malloc(sizeof(nodeID));
			if (receivedBuffer[index][1]==NULL) {
				free (receivedBuffer[index][0]);
				receivedBuffer[index][0] = NULL;
				fprintf(stderr, "Net-helper : memory error while creating a new nodeID. Message from %s is lost.\n", str);
				return;
			}
			else {
				memset(receivedBuffer[index][1], 0, sizeof(struct nodeID));
				nodeID *remote; remote = (nodeID*)(receivedBuffer[index][1]);
				receivedBuffer[index][0] = realloc(receivedBuffer[index][0],buflen+sizeof(int));
				memset(receivedBuffer[index][0],0,buflen+sizeof(int));
				memcpy(receivedBuffer[index][0],&buflen,sizeof(int));
				//*(receivedBuffer[index][0]) = buflen;
				memcpy((receivedBuffer[index][0])+sizeof(int),buffer,buflen);
				  // get the socketID of the sender
				remote->addr = malloc(SOCKETID_SIZE);
				if (remote->addr == NULL) {
					  free (remote);
					  fprintf(stderr, "Net-helper : memory error while creating a new nodeID \n");
					  return;
				}
				else {
					memset(remote->addr, 0, SOCKETID_SIZE);
					memcpy(remote->addr, arg->remote_socketID ,SOCKETID_SIZE);
				//	remote->addrSize = SOCKETID_SIZE;
				//	remote->addrStringSize = SOCKETID_STRING_SIZE;
					remote->connID = arg->connectionID;
					remote->refcnt = 1;
				}
			}
		}
  }
//	event_base_loopbreak(base);
}


struct nodeID *net_helper_init(const char *IPaddr, int port) {

	struct timeval tout = {1, 0};
	base = event_base_new();

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
		receivedBuffer[i][0] = NULL;
	}

	register_Error_connection_cb(&connError_cb);
	register_Recv_connection_cb(&receive_conn_cb);
	init_messaging_layer(1,tout,port,IPaddr,0,NULL,&init_myNodeID_cb,base);
	while (me->connID<-1) {
	//	event_base_once(base,-1, EV_TIMEOUT, &t_out_cb, NULL, &tout);
		event_base_loop(base,EVLOOP_ONCE);
	}
	timeoutFired = 0;
//	fprintf(stderr,"Net-helper init : back from init!\n");

	return me;
}


void bind_msg_type (unsigned char msgtype) {

			register_Recv_data_cb(&recv_data_cb,msgtype);
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
		return -1;
	}
	sendingBuffer[index] = realloc(sendingBuffer[index],buffer_size);
	memset(sendingBuffer[index],0,buffer_size);
	memcpy(sendingBuffer[index],buffer_ptr,buffer_size);
	// free(buffer_ptr);
	msgData_cb *p = malloc(sizeof(msgData_cb));
	p->bIdx = index; p->mSize = buffer_size; p->msgType = (unsigned char)buffer_ptr[0];
	int current = p->bIdx;
	to->connID = open_Connection(to->addr,&connReady_cb,p);
	if (to->connID<0) {
		free(sendingBuffer[current]);
		sendingBuffer[current] = NULL;
		fprintf(stderr,"Net-helper: Couldn't get a connection ID to send msg %d.\n ", p->bIdx);
		free(p);
		return -1;
	}
	else {
	//	fprintf(stderr,"Net-helper: Got a connection ID to send msg %d to %s.\n ",current,node_addr(to));
	//		struct timeval tout = {0,500};
	//		event_base_once(base,0, EV_TIMEOUT, &t_out_cb, NULL, &tout);
		while (sendingBuffer[current] != NULL)
			event_base_loop(base,EVLOOP_ONCE);//  EVLOOP_NONBLOCK
//		fprintf(stderr,"Net-helper: Back from eventlib loop with status %d.\n", ok);
		int size = p->mSize;
		free(p);
		return size;
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
	// this should never happen... if it does, index handling is faulty...
	if (receivedBuffer[rIdx][1]==NULL) {
		fprintf(stderr, "Net-helper : memory error while creating a new nodeID \n");
		return -1;
	}

	(*remote) = (nodeID*)(receivedBuffer[rIdx][1]);
	// retrieve a msg from the buffer
	memcpy(buffer_ptr, (receivedBuffer[rIdx][0])+sizeof(int), buffer_size);
	int size = (int)(*(receivedBuffer[rIdx][0]));
	free(receivedBuffer[rIdx][0]);
	receivedBuffer[rIdx][0] = NULL;
	receivedBuffer[rIdx][1] = NULL;

//	fprintf(stderr, "Net-helper : I've got mail!!!\n");

	return size;
}


int wait4data(const struct nodeID *n, struct timeval *tout) {

//	fprintf(stderr,"Net-helper : Waiting for data to come...\n");
	event_base_once(base,-1, EV_TIMEOUT, &t_out_cb, NULL, tout);
	while(receivedBuffer[rIdx][0]==NULL && timeoutFired==0) {
	//	event_base_dispatch(base);
		event_base_loop(base,EVLOOP_ONCE);
	}
	timeoutFired = 0;
//	fprintf(stderr,"Back from eventlib loop.\n");

	if (receivedBuffer[rIdx][0]!=NULL)
		return 1;
	else
		return 0;
}




struct nodeID *create_node(const char *rem_IP, int rem_port) {
	struct nodeID *remote = malloc(sizeof(nodeID));
	if (remote == NULL) {
		return NULL;
	}
//	remote->addr = malloc(sizeof(SOCKETID_SIZE));
//	if (remote->addr == NULL) {
//		free(remote);
//		return NULL;
//	}
//	remote->addrSize = SOCKETID_SIZE;
//	remote->addrStringSize = SOCKETID_STRING_SIZE;
	remote->addr = getRemoteSocketID(rem_IP, rem_port);
	remote->connID = open_Connection(remote->addr,&connReady_cb,NULL);
	remote->refcnt = 1;
	return remote;
}

// TODO: check why closing the connection is annoying for the ML
void nodeid_free(struct nodeID *n) {

//	close_Connection(n->connID);
//	close_Socket(n->addr);
//	free(n);
	if (n && (--(n->refcnt) == 0)) {
	//	close_Connection(n->connID);
		close_Socket(n->addr);
		free(n);
	}
}


const char *node_addr(const struct nodeID *s)
{
  static char addr[256];
  // TODO: socketID_To_String always return 0 !!!
  int r = socketID_To_String(s->addr,addr,256);
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
	return (compare_socketIDs(s1->addr,s2->addr) == 0);
}

int nodeid_dump(uint8_t *b, const struct nodeID *s)
{
  socketID_To_String(s->addr,(char *)b,SOCKETID_STRING_SIZE);
  //fprintf(stderr,"Dumping nodeID : ho scritto %s (%d bytes)\n",b, strlen((char *)b));
  return strlen((char *)b);

//	memcpy(b, s->addr,SOCKETID_SIZE);//sizeof(struct sockaddr_in6)*2
//	return SOCKETID_SIZE;//sizeof(struct sockaddr_in6)*2;

}

struct nodeID *nodeid_undump(const uint8_t *b, int *len)
{
  struct nodeID *res;
  res = malloc(sizeof(struct nodeID));
  if (res != NULL) {
	  memset(res,0,sizeof(struct nodeID));
	  res->addr = malloc(SOCKETID_SIZE);
	  if (res->addr != NULL) {
		  memset(res->addr,0,SOCKETID_SIZE);
		  //memcpy(res->addr, b, SOCKETID_SIZE);
		  //*len = SOCKETID_SIZE;
		  *len = strlen((char*)b);
		  string_To_SocketID((char *)b,res->addr);
	//	  fprintf(stderr,"Node undumped : %s\n",node_addr(res));
	//	  res->addrSize = SOCKETID_SIZE;
	//	  res->addrStringSize = SOCKETID_STRING_SIZE;
		  res->connID = -1;
		  res->refcnt = 1;
	  }
	  else {
		  free(res);
		  res = NULL;
		  // TODO: what about *len in this case???
		  fprintf(stderr,"Net-helper : Error while 'undumping' nodeID...\n");
	  }
  }


  return res;
}
