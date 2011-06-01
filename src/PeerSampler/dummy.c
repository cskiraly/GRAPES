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

//TODO: context support not implemented
struct peersampler_context{
  const struct nodeID **r;
};

static struct peersampler_context* dummy_init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config)
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

  //return i;
  //TODO: Returning the context may break some tests
  return NULL;
}

static int dummy_change_metadata(struct peersampler_context *context, const void *metadata, int metadata_size)
{
  /* Metadata not supported: fail! */
  return -1;
}

static int dummy_add_neighbour(struct peersampler_context *context, struct nodeID *neighbour, const void *metadata, int metadata_size)
{
  int i;

  for (i = 0; table[i] != NULL; i++);
  table[i++] = neighbour;
  table[i] = NULL;

  return i;
}

static int dummy_parse_data(struct peersampler_context *context, const uint8_t *buff, int len)
{
  /* FAKE! */
  return 0;
}

static const struct nodeID *const *dummy_get_neighbourhood(struct peersampler_context *context, int *n)
{
  if (context->r == NULL) {
    context->r = malloc(MAX_PEERS * sizeof(struct nodeID *));
    if (context->r == NULL) {
      return NULL;
    }
  }

  for (*n = 0; table[*n] != NULL; (*n)++) {
    context->r[*n] = table[*n];
fprintf(stderr, "Checking table[%d]\n", *n);
  }
  return context->r;
}

static const void *dummy_get_metadata(struct peersampler_context *context, int *metadata_size)
{
  /* Metadata not supported: fail! */
  *metadata_size = -1;

  return NULL;
}

static int dummy_grow_neighbourhood(struct peersampler_context *context, int n)
{
  return -1;
}

static int dummy_shrink_neighbourhood(struct peersampler_context *context, int n)
{
  return -1;
}

static int dummy_remove_neighbour(struct peersampler_context *context, const struct nodeID *neighbour)
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

