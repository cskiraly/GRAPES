/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "net_helper.h"
#include "topmanager.h"
#include "nccache.h"
#include "ncast_proto.h"
#include "proto.h"
#include "msg_types.h"

#define MAX_PEERS 30

static uint64_t currtime;
static int cache_size = MAX_PEERS;
static struct peer_cache *local_cache;
static int period = 10000000;

static uint64_t gettime(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_usec + tv.tv_sec * 1000000ull;
}

static int time_to_send(void)
{
  if (gettime() - currtime > period) {
    currtime += period;

    return 1;
  }

  return 0;
}

/*
 * Public Functions!
 */
int topInit(struct nodeID *myID, void *metadata, int metadata_size)
{
  local_cache = cache_init(cache_size, metadata_size);
  if (local_cache == NULL) {
    return -1;
  }
  ncast_proto_init(myID, metadata, metadata_size);
  currtime = gettime();

  return 1;
}

int topAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
  if (cache_add(local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  }
  return ncast_query_peer(local_cache, neighbour);
}

int topParseData(const uint8_t *buff, int len)
{
  if (len) {
    const struct ncast_header *h = (const struct ncast_header *)buff;
    struct peer_cache *new, *remote_cache;

    if (h->protocol != MSG_TYPE_TOPOLOGY) {
      fprintf(stderr, "NCAST: Wrong protocol!\n");

      return -1;
    }

    if (h->type == NCAST_QUERY) {
      ncast_reply(buff + sizeof(struct ncast_header), len - sizeof(struct ncast_header), local_cache);
    }
    remote_cache = entries_undump(buff + sizeof(struct ncast_header), len - sizeof(struct ncast_header));
    new = merge_caches(local_cache, remote_cache);
    cache_free(remote_cache);
    if (new != NULL) {
      cache_free(local_cache);
      local_cache = new;
    }
  }

  if (time_to_send()) {
    cache_update(local_cache);
    ncast_query(local_cache);
  }

  return 0;
}

const struct nodeID **topGetNeighbourhood(int *n)
{
  static struct nodeID *r[MAX_PEERS];

  for (*n = 0; nodeid(local_cache, *n); (*n)++) {
    r[*n] = nodeid(local_cache, *n);
    //fprintf(stderr, "Checking table[%d]\n", *n);
  }
  return (const struct nodeID **)r;
}

const void *topGetMetadata(int *metadata_size)
{
  fprintf(stderr, "getMetaData is not implemented yet!!!\n");

  return NULL;
}

int topGrowNeighbourhood(int n)
{
  cache_size += n;

  return cache_size;
}

int topShrinkNeighbourhood(int n)
{
  if (cache_size < n) {
    return -1;
  }
  cache_size -= n;

  return cache_size;
}

int topRemoveNeighbour(struct nodeID *neighbour)
{
  return cache_del(local_cache, neighbour);
}

