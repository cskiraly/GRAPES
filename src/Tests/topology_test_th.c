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
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>

#include "net_helper.h"
#include "peersampler.h"
#include "net_helpers.h"

static struct psample_context *context;
static const char *my_addr = "127.0.0.1";
static int port = 6666;
static int srv_port;
static const char *srv_ip;
static char *fprefix;
static pthread_mutex_t neigh_lock;

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "s:p:i:P:I:")) != -1) {
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
      case 's':
        fprefix = strdup(optarg);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);

        exit(-1);
    }
  }
}

static struct nodeID *init(void)
{
  struct nodeID *myID;

  myID = net_helper_init(my_addr, port, "");
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);

    return NULL;
  }
  context = psample_init(myID, NULL, 0, "protocol=cyclon");
//  context = psample_init(myID, NULL, 0, "");

  return myID;
}

static void *cycle_loop(void *p)
{
  char addr[256];
  int done = 0;
  int cnt = 0;

  while (!done) {
    const int tout = 1;

    pthread_mutex_lock(&neigh_lock);
    psample_parse_data(context, NULL, 0);
    pthread_mutex_unlock(&neigh_lock);
    if (cnt % 10 == 0) {
      const struct nodeID *const *neighbours;
      int n, i;

      pthread_mutex_lock(&neigh_lock);
      neighbours = psample_get_cache(context, &n);
      printf("I have %d neighbours:\n", n);
      for (i = 0; i < n; i++) {
        node_addr(neighbours[i], addr, 256);
        printf("\t%d: %s\n", i, addr);
      }
      fflush(stdout);
      if (fprefix) {
        FILE *f;
        char fname[64];

        sprintf(fname, "%s-%d.txt", fprefix, port);
        f = fopen(fname, "w");
        if (f) fprintf(f, "#Cache size: %d\n", n);
        for (i = 0; i < n; i++) {
          node_addr(neighbours[i], addr, 256);
          if (f) fprintf(f, "%d\t\t%d\t%s\n", port, i, addr);
        }
        fclose(f);
      }
      pthread_mutex_unlock(&neigh_lock);
    }
    cnt++;
    sleep(tout);
  }

  return NULL;
}

static void *recv_loop(void *p)
{
  struct nodeID *s = p;
  int done = 0;
#define BUFFSIZE 1024
  static uint8_t buff[BUFFSIZE];

  while (!done) {
    int len;
    struct nodeID *remote;

    len = recv_from_peer(s, &remote, buff, BUFFSIZE);
    pthread_mutex_lock(&neigh_lock);
    psample_parse_data(context, buff, len);
    pthread_mutex_unlock(&neigh_lock);
    nodeid_free(remote);
  }

  return NULL;
}

int main(int argc, char *argv[])
{
  struct nodeID *my_sock;
  pthread_t cycle_id, recv_id;

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
    psample_add_peer(context, knownHost, NULL, 0);
  }
  pthread_mutex_init(&neigh_lock, NULL);
  pthread_create(&recv_id, NULL, recv_loop, my_sock);
  pthread_create(&cycle_id, NULL, cycle_loop, my_sock);
  pthread_join(recv_id, NULL);
  pthread_join(cycle_id, NULL);

  return 0;
}
