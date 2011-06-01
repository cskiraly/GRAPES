/*
 *  Copyright (c) 2009 Luca Abeni
 *  Copyright (c) 2011 Andrea Zito
 *
 *  This is free software; see gpl-3.0.txt
 *
 *  This is a small test program for the gossip based TopologyManager (cloudcast protocol)
 *  To try the simple test: run it with
 *    ./cloudcast_topology_test -I <network interface> -P <port> -c <cloud_conf>
 *    -c parameters describe the configuration of the cloud which is
 *  used to bootstrap the topology
 *    For example, run
 *      ./topology_test -I eth0 -P 6666 -c "provider=delegate,delegate_lib=libfilecloud.so"
 *    to use the filecloud implementation with the default parameters
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "net_helper.h"
#include "peersampler.h"
#include "net_helpers.h"
#include "cloud_helper.h"
#include "cloud_helper_utils.h"

struct context{
  struct psample_context *ps_context;
  struct cloud_helper_context *cloud_context;
  struct nodeID *net_context;
};

static const char *my_addr = "127.0.0.1";
static int port = 6666;
static const char *cloud_conf = NULL;
static char *fprefix = NULL;

static int cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "s:P:I:c:i:p:")) != -1) {
    printf("%c", o);
    if (optarg) printf(" -> %s\n", optarg);
    else printf("\n");
    switch(o) {
      case 'c':
        cloud_conf = strdup(optarg);
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
      case 'i':
      case 'p':
        fprintf(stderr, "Ignoring option %c, just for compatibility\n", o);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);
        return 1;
    }
  }

  return 0;
}

static struct context *init(void)
{
  struct context *c;
  c = malloc(sizeof(struct context));
  c->net_context = net_helper_init(my_addr, port, "");
  if (c->net_context == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);
    return NULL;
  }

  c->cloud_context = cloud_helper_init(c->net_context, cloud_conf);
  if (c->cloud_context == NULL) {
    fprintf(stderr, "Error initiating cloud_helper\n");
    return NULL;
  }

  c->ps_context = psample_init(c->net_context, NULL, 0, "protocol=cloudcast");
  if (c->ps_context == NULL) {
    fprintf(stderr, "Error initiating peer sampler\n");
    return NULL;
  }
  return c;
}



static void loop(struct context *con)
{
  int done = 0;
#define BUFFSIZE 1024
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;

  psample_parse_data(con->ps_context, NULL, 0);
  while (!done) {
    int len;
    int news;
    int data_source;
    const struct timeval tout = {1, 0};
    struct timeval t1;

    t1 = tout;
    news = wait4any_threaded(con->net_context, con->cloud_context, &t1, NULL, &data_source);
    if (news > 0) {
      struct nodeID *remote = NULL;
      //      printf("Got data from: %d\n", data_source);
      if (data_source == DATA_SOURCE_NET)
        len = recv_from_peer(con->net_context, &remote, buff, BUFFSIZE);
      else if (data_source == DATA_SOURCE_CLOUD)
        len = recv_from_cloud(con->cloud_context, buff, BUFFSIZE);
      psample_parse_data(con->ps_context, buff, len);
      if (remote) nodeid_free(remote);
    } else {
      if (psample_parse_data(con->ps_context, NULL, 0) != 0){
        fprintf(stderr, "Error parsing data... quitting\n");
        exit(1);
      }
      if (cnt % 10 == 0) {
        const struct nodeID *const *neighbourhoods;
        char addr[256];
        int n, i;

        neighbourhoods = psample_get_cache(con->ps_context, &n);
        printf("I have %d neighbours:\n", n);
        for (i = 0; i < n; i++) {
          node_addr(neighbourhoods[i], addr, 256);
          printf("\t%d: %s\n", i, addr);
        }
        fflush(stdout);
        if (fprefix) {
          FILE *f;
          char fname[64];
          fprintf(stderr, "ci sono??? %s\n", fprefix);
          sprintf(fname, "%s-%d.txt", fprefix, port);
          f = fopen(fname, "w");
          if (f) fprintf(f, "#Cache size: %d\n", n);
          for (i = 0; i < n; i++) {
            node_addr(neighbourhoods[i], addr, 256);
            if (f) fprintf(f, "%d\t\t%d\t%s\n", port, i, addr);
          }
          fclose(f);
        }
      }
      cnt++;
    }
  }
}

int main(int argc, char *argv[])
{
  struct nodeID *cloudHost;
  struct context *con;

  if (cmdline_parse(argc, argv) != 0){
    fprintf(stderr, "Error parsing parameters!\n");
    return 1;
  }

  if (!cloud_conf){
    fprintf(stderr, "No cloud configuration provided!\n");
    return 1;
  }

  con = init();
  if (!con){
    fprintf(stderr, "Error initializing!\n");
    return 1;
  }

  cloudHost = get_cloud_node(con->cloud_context, 0);
  psample_add_peer(con->ps_context, cloudHost, NULL, 0);
  nodeid_free(cloudHost);
  loop(con);
  fprintf(stderr, "Quitting!\n");
  return 0;
}
