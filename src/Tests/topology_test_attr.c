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

#include "net_helper.h"
#include "peersampler.h"
#include "net_helpers.h"

static struct psample_context *context;
static const char *my_addr = "127.0.0.1";
static int port = 6666;
static int srv_port;
static const char *srv_ip;

static struct peer_attributes {
  enum peer_state {psleep, awake, tired} state;
  char colour[20];
  char name[40];
} my_attr;

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  my_attr.state = awake;
  sprintf(my_attr.colour, "red");
  while ((o = getopt(argc, argv, "p:i:P:I:c:ST")) != -1) {
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
      case 'c':
        strcpy(my_attr.colour, optarg);
        break;
      case 'S':
        my_attr.state = psleep;
        break;
      case 'T':
        my_attr.state = tired;
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
  char addr[256];

  myID = net_helper_init(my_addr, port, "");
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);

    return NULL;
  }

  node_addr(myID, addr, 256);
  strcpy(my_attr.name, addr);
  context = psample_init(myID, &my_attr, sizeof(struct peer_attributes), "");

  return myID;
}

static const char *status_print(enum peer_state s)
{
  switch (s) {
    case psleep:
      return "sleeping";
    case awake:
      return "awake";
    case tired:
      return "tired";
    default:
      return "Boh?";
  }
}

static void status_update(void)
{
  struct nodeID *myself;

  switch (my_attr.state) {
    case psleep:
      my_attr.state = awake;
      break;
    case awake:
      my_attr.state = tired;
      break;
    case tired:
      my_attr.state = psleep;
      break;
  }
  printf("goin' %s\n", status_print(my_attr.state));
  myself = create_node(my_addr, port);
  psample_change_metadata(context, &my_attr, sizeof(struct peer_attributes));
  nodeid_free(myself);
}

static void loop(struct nodeID *s)
{
  int done = 0;
#define BUFFSIZE 1024
  static uint8_t buff[BUFFSIZE];
  char addr[256];
  int cnt = 0;

  psample_parse_data(context, NULL, 0);
  while (!done) {
    int len;
    int news;
    struct timeval tout = {1, 0};

    news = wait4data(s, &tout, NULL);
    if (news > 0) {
      struct nodeID *remote;

      len = recv_from_peer(s, &remote, buff, BUFFSIZE);
      psample_parse_data(context, buff, len);
      nodeid_free(remote);
    } else {
      if (cnt % 30 == 0) {
        status_update();
      }
      psample_parse_data(context, NULL, 0);
      if (cnt++ % 10 == 0) {
        const struct nodeID *const *neighbourhoods;
        int n, i, size;
        const struct peer_attributes *meta;

        neighbourhoods = psample_get_cache(context, &n);
        meta = psample_get_metadata(context, &size);
        if (meta == NULL) {
          printf("No MetaData!\n");
        } else {
          if (size != sizeof(struct peer_attributes)) {
            fprintf(stderr, "Bad MetaData!\n");
            exit(-1);
          }
        }
        printf("I have %d neighbourhoods:\n", n);
        for (i = 0; i < n; i++) {
          node_addr(neighbourhoods[i], addr, 256);
          printf("\t%d: %s", i, addr);
          if (meta) {
            printf("\tPeer %s is a %s peer and is %s", meta[i].name, meta[i].colour, status_print(meta[i].state));
          }
          printf("\n");
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  struct nodeID *my_sock;

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

  loop(my_sock);

  return 0;
}
