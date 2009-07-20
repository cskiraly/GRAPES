#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "msglayer.h"
#include "topmanager.h"
#include "nccache.h"
#include "ncast_proto.h"
#include "proto.h"

#define MAX_PEERS 30

static uint64_t currtime;
static int cache_size = MAX_PEERS;
static struct cache_entry *local_cache;
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
int topInit(struct nodeID *myID)
{
  local_cache = cache_init(cache_size);
  if (local_cache == NULL) {
    return -1;
  }
  ncast_proto_init(myID);
  currtime = gettime();

  return 1;
}

int topAddNeighbour(struct nodeID *neighbour)
{
  cache_add(local_cache, neighbour);
  return ncast_query_peer(local_cache, neighbour);
}

int topParseData(const uint8_t *buff, int len)
{
  if (len) {
    struct ncast_header *h = (struct ncast_header *)buff;
    struct cache_entry *new, *remote_cache;

    if (h->protocol != PROTO_NCAST) {
      fprintf(stderr, "NCAST: Wrong protocol!\n");

      return -1;
    }

    if (h->type == NCAST_QUERY) {
      ncast_reply(buff + sizeof(struct ncast_header), len - sizeof(struct ncast_header), local_cache);
    }
    remote_cache = entries_undump(buff + sizeof(struct ncast_header), len - sizeof(struct ncast_header));
    new = merge_caches(local_cache, remote_cache, cache_size);
    free(remote_cache);
    if (new != NULL) {
      free(local_cache);
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

