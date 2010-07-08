/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/select.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "net_helper.h"
#include "net_helpers.h"
#include "chunk.h"
#include "trade_msg_la.h"
#include "trade_msg_ha.h"
#include "chunkidset.h"
#include "msg_types.h"
#include "trade_sig_ha.h"

static const char *my_addr = "127.0.0.1";
static int port = 6666;
static int dst_port;
static const char *dst_ip;

#define BUFFSIZE 1024

static struct nodeID *init(void)
{
  struct nodeID *myID;

  myID = net_helper_init(my_addr, port);
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);

    return NULL;
  }  
  chunkSignalingInit(myID);
  return myID;
}

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "p:i:P:I:")) != -1) {
    switch(o) {
      case 'p':
        dst_port = atoi(optarg);
        break;
      case 'i':
        dst_ip = strdup(optarg);
        break;
      case 'P':
        port =  atoi(optarg);
        break;
      case 'I':
        my_addr = iface_addr(optarg);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);

        exit(-1);
    }
  }
}

int main(int argc, char *argv[])
{
  struct nodeID *my_sock;
  struct chunkID_set *cset;
  cmdline_parse(argc, argv);
  my_sock = init();
  if (dst_port != 0) {//sender
    struct nodeID *dst;        
    int res;
    cset = chunkID_set_init("type=1,size=10");
    res = chunkID_set_add_chunk(cset, 3);
    fprintf(stdout,"Adding chunk %d\n",3);
    res = chunkID_set_add_chunk(cset, 4);
    fprintf(stdout,"Adding chunk %d\n",4);
    res = chunkID_set_add_chunk(cset, 5);
    fprintf(stdout,"Adding chunk %d\n",5);
    res = chunkID_set_add_chunk(cset, 8);
    fprintf(stdout,"Adding chunk %d\n",8);
    res = chunkID_set_add_chunk(cset, 9);
    fprintf(stdout,"Adding chunk %d\n",9);
    res = chunkID_set_size(cset);
    fprintf(stdout,"Creating c_set with %d chunks\n",res);
    fprintf(stdout,"Sending signal ");
        
    dst = create_node(dst_ip, dst_port);
    //res = requestChunks(dst,cset,10,44);
    res = deliverChunks(dst,cset,33);
    //res = offerChunks(dst,cset,8,22);
    //res = acceptChunks(dst,cset,11);
    //res = sendBufferMap(dst,dst,cset,88);
    //res = requestBufferMap(dst,NULL,99);
    fprintf(stdout,"done: %d\n",res);
    chunkID_set_free(cset);
    nodeid_free(dst);    
  } else {
    /* Receive a chunk and print it! */        
    struct nodeID *owner;
    int max_deliver, trans_id;
    enum signaling_type sig_type;
    int ret;
    struct nodeID *remote;    
    static uint8_t buff[BUFFSIZE];

    max_deliver = trans_id = ret = 0;
    ret = recv_from_peer(my_sock, &remote, buff, BUFFSIZE);   
    /* TODO: Error check! */
    if (buff[0] != MSG_TYPE_SIGNALLING) {
      fprintf(stderr, "Wrong message type!!!\n");
      return -1;
    }    
    fprintf(stdout,"Received message of %d bytes\n",ret);
    ret = parseSignaling(buff+1,ret-1, &owner, &cset, &max_deliver, &trans_id, &sig_type);
    if(cset){
    fprintf(stdout, "Chunk %d= %d\n",3, chunkID_set_check(cset,3));
    fprintf(stdout, "Chunk %d= %d\n",4, chunkID_set_check(cset,4));
    fprintf(stdout, "Chunk %d= %d\n",5, chunkID_set_check(cset,5));
    fprintf(stdout, "Chunk %d= %d\n",8, chunkID_set_check(cset,8));
    fprintf(stdout, "Chunk %d= %d\n",9, chunkID_set_check(cset,9));
    }    
    switch(sig_type){
        case sig_offer:
            fprintf(stdout, "Offer of %d chunks\n", chunkID_set_size(cset));
            break;
        case sig_accept:
            fprintf(stdout, "Accept of %d chunks\n", chunkID_set_size(cset));
            break;
        case sig_request:
            fprintf(stdout, "Request of %d chunks\n", chunkID_set_size(cset));
            break;
        case sig_deliver:
            fprintf(stdout, "Deliver of %d chunks\n", chunkID_set_size(cset));
            break;
        case sig_send_buffermap:
            fprintf(stdout, "Send buffer map of %d chunks\n", chunkID_set_size(cset));
            break;
        case sig_request_buffermap:
            fprintf(stdout, "Request buffer map %d\n", (cset == NULL));
            break;
    }
    fprintf(stdout, "Trans_id = %d; Max_del = %d\n", trans_id,max_deliver);
    nodeid_free(remote);
    chunkID_set_free(cset);
  }
  nodeid_free(my_sock);
  
  return 0;
}

