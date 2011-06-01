
/*
 *  Copyright (c) 2011 Andrea Zito
 *
 *  This is free software; see gpl-3.0.txt
 *
 *  This is a small test program which monitors the topology partial seen by the cloud
 *  To try the simple test: run it with
 *    ./cloud_topology_monitor -c <cloud_conf>
 *    -c parameters describe the configuration of the cloud
 *    For example, run
 *      ./cloud_topology_monitor-c "provider=delegate,delegate_lib=libfilecloud.so"
 *    to use the filecloud implementation with the default parameters
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>

#include "net_helper.h"
#include "peersampler.h"
#include "net_helpers.h"
#include "cloud_helper.h"
#include "cloud_helper_utils.h"

#define CLOUD_VIEW_KEY "view"

#include "../Cache/topocache.h"

struct context{
  struct psample_context *ps_context;
  struct cloud_helper_context *cloud_context;
  struct nodeID *net_context;
};

static const char *my_addr = "127.0.0.1";
static int port = 9999;
static const char *cloud_conf = NULL;
static char *fprefix = NULL;
static int init_cloud = 0;

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "s:c:i")) != -1) {
    switch(o) {
      case 'c':
        cloud_conf = strdup(optarg);
        break;
      case 's':
        fprefix = strdup(optarg);
        break;
      case 'i':
        init_cloud = 1;
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);
        exit(-1);
    }
  }
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

  return c;
}

static void loop(struct context *con)
{
  int done = 0;
#define BUFFSIZE 1024
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;

  if (init_cloud) {
    printf("Initializing the cloud...\n");
    put_on_cloud(con->cloud_context, CLOUD_VIEW_KEY, NULL, 0, 0);
  }

  while (!done) {
    int len;
    int news;
    int err;
    int i;
    const struct timeval tout = {1, 0};
    struct timeval t1;
    struct peer_cache *remote_cache;
    time_t timestamp, now;
    char timestamp_str[26];
    char addr[256];


    t1 = tout;
    sleep(5);
    err = get_from_cloud(con->cloud_context, CLOUD_VIEW_KEY, NULL, 0, 0);
    news = wait4cloud(con->cloud_context, &t1);
    if (news > 0) {
      len = recv_from_cloud(con->cloud_context, buff, BUFFSIZE);

      if (len == 0){
        printf("*** Empty view on cloud\n");
        continue;
      }
      timestamp = timestamp_cloud(con->cloud_context);
      remote_cache = entries_undump(buff, len);
      ctime_r(&timestamp, timestamp_str);
      timestamp_str[24] = '\0';
      now = time(NULL);
      printf("--------------------------------------\nCache (%s, last contact %d sec ago):\n", timestamp_str, (int)(now - timestamp));
      for (i=0; nodeid(remote_cache, i); i++) {
        node_addr(nodeid(remote_cache,i), addr, 256);
        printf("\t%d: %s\n", i, addr);
      }
      if (fprefix) {
        FILE *f;
        char fname[64];

        sprintf(fname, "%s.txt", fprefix);
        f = fopen(fname, "w");
        if (f) fprintf(f, "#Cache:");
        for (i=0; nodeid(remote_cache, i); i++) {
          node_addr(nodeid(remote_cache, i), addr, 256);
          if (f) fprintf(f, "\t%d\t%s\n", i, addr);
        }
        fclose(f);
      }
    } else if (news < 0) {
      printf("*** No view on cloud\n");
    } else {
      printf("*** !Cloud not responding!\n");
    }
    cnt++;
    fflush(stdout);
  }
}

int main(int argc, char *argv[])
{
  struct context *con;

  cmdline_parse(argc, argv);

  if (!cloud_conf){
    printf("No cloud configuration provided!\n");
    return 1;
  }

  con = init();
  if (!con) return 1;

  loop(con);

  return 0;
}
