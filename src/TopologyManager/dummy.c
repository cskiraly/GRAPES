/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


#include "net_helper.h"
#include "peersampler_iface.h"

#define MAX_PEERS 5000
static struct nodeID *table[MAX_PEERS];

static int dummy_init(struct nodeID *myID, void *metadata, int metadata_size, const char *config)
{
  FILE *f;
  int i = 0;

  f = fopen("peers.txt", "r");
  while(!feof(f)) {
    int res;
    char addr[32];
    int port;

    res = fscanf(f, "%s %d\n", addr, &port);
    if ((res == 2) && (i < MAX_PEERS - 1)) {
fprintf(stderr, "Creating table[%d]\n", i);
      table[i++] = create_node(addr, port);
    }
  }
  table[i] = NULL;

  return i;
}

static int dummy_change_metadata(void *metadata, int metadata_size)
{
  /* Metadata not supported: fail! */
  return -1;
}

static int dummy_add_neighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
  int i;

  for (i = 0; table[i] != NULL; i++);
  table[i++] = neighbour;
  table[i] = NULL;

  return i;
}

static int dummy_parse_data(const uint8_t *buff, int len)
{
  /* FAKE! */
  return 0;
}

static const struct nodeID **dummy_get_neighbourhood(int *n)
{
  for (*n = 0; table[*n] != NULL; (*n)++) {
fprintf(stderr, "Checking table[%d]\n", *n);
  }
  return (const struct nodeID **)table;
}

static const void *dummy_get_metadata(int *metadata_size)
{
  /* Metadata not supported: fail! */
  *metadata_size = -1;

  return NULL;
}

static int dummy_grow_neighbourhood(int n)
{
  return -1;
}

static int dummy_shrink_neighbourhood(int n)
{
  return -1;
}

static int dummy_remove_neighbour(struct nodeID *neighbour)
{
  return -1;
}


struct peersampler_iface dummy = {
  .init = dummy_init,
  .change_metadata = dummy_change_metadata,
  .add_neighbour = dummy_add_neighbour,
  .parse_data = dummy_parse_data,
  .get_neighbourhood = dummy_get_neighbourhood,
  .get_metadata = dummy_get_metadata,
  .grow_neighbourhood = dummy_grow_neighbourhood,
  .shrink_neighbourhood = dummy_shrink_neighbourhood,
  .remove_neighbour = dummy_remove_neighbour,
};

