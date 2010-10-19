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
#include <stdbool.h>
#include <string.h>

#include "net_helper.h"
#include "peersampler_iface.h"
#include "topocache.h"
#include "topo_proto.h"
#include "ncast_proto.h"
#include "proto.h"
#include "config.h"
#include "grapes_msg_types.h"

#define BOOTSTRAP_CYCLES 5
#define DEFAULT_CACHE_SIZE 10
#define DEFAULT_MAX_TIMESTAMP 5

static uint64_t currtime;
static int cache_size;
static struct peer_cache *local_cache;
static bool bootstrap = true;
static int bootstrap_period = 2000000;
static int period = 10000000;
static int counter;

static uint64_t gettime(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_usec + tv.tv_sec * 1000000ull;
}

static int time_to_send(void)
{
  int p = bootstrap ? bootstrap_period : period;
  if (gettime() - currtime > p) {
    currtime += p;

    return 1;
  }

  return 0;
}

/*
 * Exported Functions!
 */
static int ncast_init(struct nodeID *myID, void *metadata, int metadata_size, const char *config)
{
  struct tag *cfg_tags;
  int res, max_timestamp;

  cfg_tags = config_parse(config);
  res = config_value_int(cfg_tags, "cache_size", &cache_size);
  if (!res) {
    cache_size = DEFAULT_CACHE_SIZE;
  }
  res = config_value_int(cfg_tags, "max_timestamp", &max_timestamp);
  if (!res) {
    max_timestamp = DEFAULT_MAX_TIMESTAMP;
  }
  free(cfg_tags);
  
  local_cache = cache_init(cache_size, metadata_size, max_timestamp);
  if (local_cache == NULL) {
    return -1;
  }
  topo_proto_init(myID, metadata, metadata_size);
  currtime = gettime();
  bootstrap = true;

  return 1;
}

static int ncast_change_metadata(void *metadata, int metadata_size)
{
  if (topo_proto_metadata_update(metadata, metadata_size) <= 0) {
    return -1;
  }

  return 1;
}

static int ncast_add_neighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
  if (cache_add(local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  }
  return ncast_query_peer(local_cache, neighbour);
}

static int ncast_parse_data(const uint8_t *buff, int len)
{
  int dummy;

  if (len) {
    const struct topo_header *h = (const struct topo_header *)buff;
    struct peer_cache *new, *remote_cache;

    if (h->protocol != MSG_TYPE_TOPOLOGY) {
      fprintf(stderr, "NCAST: Wrong protocol!\n");

      return -1;
    }

    counter++;
    if (counter == BOOTSTRAP_CYCLES) bootstrap = false;

    remote_cache = entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));
    if (h->type == NCAST_QUERY) {
      ncast_reply(remote_cache, local_cache);
    }
    new = merge_caches(local_cache, remote_cache, cache_size, &dummy);
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

static const struct nodeID **ncast_get_neighbourhood(int *n)
{
  static struct nodeID **r;

  r = realloc(r, cache_size * sizeof(struct nodeID *));
  if (r == NULL) {
    return NULL;
  }

  for (*n = 0; nodeid(local_cache, *n) && (*n < cache_size); (*n)++) {
    r[*n] = nodeid(local_cache, *n);
    //fprintf(stderr, "Checking table[%d]\n", *n);
  }

  return (const struct nodeID **)r;
}

static const void *ncast_get_metadata(int *metadata_size)
{
  return get_metadata(local_cache, metadata_size);
}

static int ncast_grow_neighbourhood(int n)
{
  cache_size += n;

  return cache_size;
}

static int ncast_shrink_neighbourhood(int n)
{
  if (cache_size < n) {
    return -1;
  }
  cache_size -= n;

  return cache_size;
}

static int ncast_remove_neighbour(struct nodeID *neighbour)
{
  return cache_del(local_cache, neighbour);
}

struct peersampler_iface ncast = {
  .init = ncast_init,
  .change_metadata = ncast_change_metadata,
  .add_neighbour = ncast_add_neighbour,
  .parse_data = ncast_parse_data,
  .get_neighbourhood = ncast_get_neighbourhood,
  .get_metadata = ncast_get_metadata,
  .grow_neighbourhood = ncast_grow_neighbourhood,
  .shrink_neighbourhood = ncast_shrink_neighbourhood,
  .remove_neighbour = ncast_remove_neighbour,
};
