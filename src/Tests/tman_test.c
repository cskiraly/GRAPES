/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 *
 *  This is a small test program for the gossip based TopologyManager
 *  To try the simple test: run it with
 *    ./topology_test -I <network interface> -P <port> [-i <remote IP> -p <remote port>]
 *    the "-i" and "-p" parameters can be used to add an initial neighbour
 *    (otherwise, the peer risks to stay out of the overlay).
 *    For example, run
 *      ./topology_test -I eth0 -P 6666
 *    on a computer, and then
 *      ./topology_test -I eth0 -P 2222 -i <ip_address> -p 6666
 *  on another one ... Of course, one of the two peers has to use -i... -p...
 *  (in general, to be part of the overlay a peer must either use
 *  "-i<known peer IP> -p<known peer port>" or be referenced by another peer).
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "peer.h"
#include "net_helper.h"
#include "tman.h"
#include "peersampler.h"
#include "net_helpers.h"

#include "topology.h"
#include "peer_util.h"

static const char *my_addr = "127.0.0.1";
static unsigned int port = 6666;
static int srv_port=0;
int srv_metadata =0; int my_metadata;
static const char *srv_ip;
tmanRankingFunction funct = NULL;

int testRanker (const void *tin, const void *p1in, const void *p2in) {
        const int *tt, *pp1, *pp2;

      tt = ((const int *)(tin));
      pp1 = ((const int *)(p1in));
      pp2 = ((const int *)(p2in));

        return (abs(*tt-*pp1) == abs(*tt-*pp2))?0:(abs(*tt-*pp1) < abs(*tt-*pp2))?1:2;
}

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "p:i:P:I:")) != -1) {
    switch(o) {
      case 'p':
        srv_port = atoi(optarg);
        break;
      case 'i':
        srv_ip = strdup(optarg);
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
  if (srv_port) {
          srand(srv_port);
          srv_metadata = 1 + ((double)rand() / (double)RAND_MAX)*1000;
  }
  srand(port);
  my_metadata = 1 + ((double)rand() / (double)RAND_MAX)*1000;
  fprintf(stderr,"\tMy metadata = %d\n",my_metadata);
}

static void change_metadata (struct nodeID *id) {

        my_metadata = 1 + ((double)rand() / (double)RAND_MAX)*1000;
        fprintf(stderr,"\tMy metadata ARE CHANGED = %d\n",my_metadata);

        tmanChangeMetadata(&my_metadata, sizeof(my_metadata));
}

static struct nodeID *init()
{
  struct nodeID *myID;

  myID = net_helper_init(my_addr, port, "");
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);

    return NULL;
  }
  context = psample_init(myID,&my_metadata, sizeof(my_metadata),NULL);
  tmanInit(myID,&my_metadata, sizeof(my_metadata),funct,NULL);

  return myID;
}

static void loop(struct nodeID *s)
{
  int done = 0, now = -1;
#define BUFFSIZE 1524
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;

  topoParseData(NULL, 0);
  while (!done) {
    int len;
    int news;
    const struct timeval tout = {1, 0};
    struct timeval t1;

    t1 = tout;
    news = wait4data(s, &t1,NULL);
    if (news > 0) {
      struct nodeID *remote;
      len = recv_from_peer(s, &remote, buff, BUFFSIZE);
      topoParseData(buff, len);
      nodeid_free(remote);
    } else
      topoParseData(NULL, 0);
    if (++cnt % 1 == 0) {
        const uint8_t *mdata;
        const struct nodeID **neighbours;
        char addr[256];
        int n, i, msize;
        mdata = topoGetMetadata(&msize);
        neighbours = topoGetNeighbourhood(&n);
        fprintf(stderr, "\tMy metadata = %d\nIteration # %d -- Cache size now is : %d -- I have %d neighbours:\n",my_metadata,cnt,now,n);
        for (i = 0; i < n; i++) {
                const int *d;
                d = (const int*)((mdata+i*msize));
                node_addr(neighbours[i], addr, 256);
                fprintf(stderr, "\t%d: %s -- %d\n", i, addr, *d);
        }
    }
    if (cnt % 20 == 0) {
        change_metadata(s);
    }
//    if (cnt % 13 == 0) {
//      more = ((double)rand() / (double)RAND_MAX)*10;
//      now = topoGrowNeighbourhood(more);
//      printf("Growing : +%d -- Cache size now is : %d\n", more,now);
//    }
//    if (cnt % 10 == 0) {
//      less = ((double)rand() / (double)RAND_MAX)*10;
//      now = topoShrinkNeighbourhood(less);
//      printf("Shrinking : -%d -- Cache size now is : %d\n", less,now);
//    }
  }

}

int main(int argc, char *argv[])
{
  struct nodeID *my_sock;
  funct = testRanker;
  cmdline_parse(argc, argv);

  my_sock = init();
  if (my_sock == NULL) {
    return -1;
  }

  if (srv_port != 0) {
    struct nodeID *knownHost;

    knownHost = create_node(srv_ip, srv_port);
    if (knownHost == NULL) {
      fprintf(stderr, "Error creating knownHost socket (%s:%d)!\n", srv_ip, srv_port);

      return -1;
    }
    topoAddNeighbour(knownHost,NULL,0);
  }

  loop(my_sock);

  return 0;
}
