/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "net_helper.h"
#include "peersampler.h"

static int cache_size = 100;

int main(int argc, char *argv[])
{
  struct nodeID *myID;
  struct psample_context *context;
  int port, res;
  char psample_cfg[16];

  myID = net_helper_init("127.0.0.1", 5555, "");
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (127.0.0.1:5555)!\n");

    return -1;
  }
  memset(psample_cfg, 0, sizeof(psample_cfg));
  sprintf(psample_cfg, "cache_size=%d", cache_size);
  context = psample_init(myID, NULL, 0, psample_cfg);

  /* Fill the cache */
  for (port = 6666; port < 6666 + cache_size; port++) {
    struct nodeID *knownHost;

    knownHost = create_node("127.0.0.1", port);
    if (knownHost == NULL) {
      fprintf(stderr, "Error creating knownHost socket (127.0.0.1:%d)!\n", port);

      return -1;
    }
    res = psample_add_peer(context, knownHost, NULL, 0);
    if (res < 0) {
      fprintf(stderr, "Error adding 127.0.0.1:%d: %d\n", port, res);
    }
    nodeid_free(knownHost);
  }
  printf("Waiting 5s...\n");
  usleep(5000);
  printf("Trying to send a gossiping message...\n");
  res = psample_parse_data(context, NULL, 0);
  if (res < 0) {
    fprintf(stderr, "Error sending a gossiping message: %d\n", res);
  }

  nodeid_free(myID);

  return 0;
}
