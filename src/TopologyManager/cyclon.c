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
#include "cyclon_proto.h"
#include "proto.h"
#include "config.h"
#include "grapes_msg_types.h"

#define DEFAULT_CACHE_SIZE 10

static uint64_t currtime;
static int cache_size;
static int sent_entries;
static struct peer_cache *local_cache;
static bool bootstrap = true;
static int bootstrap_period = 2000000;
static int period = 10000000;

static struct peer_cache *flying_cache;
static struct nodeID *dst;

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
 * Public Functions!
 */
static int cyclon_init(struct nodeID *myID, void *metadata, int metadata_size, const char *config)
{
  struct tag *cfg_tags;
  int res;

  cfg_tags = config_parse(config);
  res = config_value_int(cfg_tags, "cache_size", &cache_size);
  if (!res) {
    cache_size = DEFAULT_CACHE_SIZE;
  }
  res = config_value_int(cfg_tags, "sent_entries", &sent_entries);
  if (!res) {
    sent_entries = cache_size / 2;
  }

  local_cache = cache_init(cache_size, metadata_size, 0);
  if (local_cache == NULL) {
    return -1;
  }
  topo_proto_init(myID, metadata, metadata_size);
  currtime = gettime();
  bootstrap = true;

  return 1;
}

static int cyclon_change_metadata(void *metadata, int metadata_size)
{
  if (topo_proto_metadata_update(metadata, metadata_size) <= 0) {
    return -1;
  }

  return 1;
}

static int cyclon_add_neighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
  if (!flying_cache) {
    flying_cache = rand_cache(local_cache, sent_entries - 1);
  }
  if (cache_add(local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  }

  return cyclon_query(flying_cache, neighbour);
}

static int cyclon_parse_data(const uint8_t *buff, int len)
{
  cache_check(local_cache);
  if (len) {
    const struct topo_header *h = (const struct topo_header *)buff;
    struct peer_cache *new, *remote_cache;
    struct peer_cache *sent_cache = NULL;
    int dummy;

    if (h->protocol != MSG_TYPE_TOPOLOGY) {
      fprintf(stderr, "Peer Sampler: Wrong protocol!\n");

      return -1;
    }

    bootstrap = false;

    remote_cache = entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));
    if (h->type == CYCLON_QUERY) {
      sent_cache = rand_cache(local_cache, sent_entries);
      cyclon_reply(remote_cache, sent_cache);
      dst = NULL;
    }
    cache_check(local_cache);
    new = merge_caches(local_cache, remote_cache, cache_size, &dummy);
    cache_free(remote_cache);
    if (new != NULL) {
      cache_free(local_cache);
      local_cache = new;
    }
    if (sent_cache) {
      int dummy;

      new = merge_caches(local_cache, sent_cache, cache_size, &dummy);
      if (new != NULL) {
        cache_free(local_cache);
        local_cache = new;
      }
      cache_free(sent_cache);
    } else {
      if (flying_cache) {
        cache_free(flying_cache);
        flying_cache = NULL;
      }
    }
  }

  if (time_to_send()) {
    if (flying_cache) {
      struct peer_cache *new;
      int dummy;

      new = merge_caches(local_cache, flying_cache, cache_size, &dummy);
      if (new != NULL) {
        cache_free(local_cache);
        local_cache = new;
      }
      cache_free(flying_cache);
      flying_cache = NULL;
    }
    cache_update(local_cache);
    dst = last_peer(local_cache);
    if (dst == NULL) {
      return 0;
    }
    dst = nodeid_dup(dst);
    cache_del(local_cache, dst);
    flying_cache = rand_cache(local_cache, sent_entries - 1);
    cyclon_query(flying_cache, dst);
  }
  cache_check(local_cache);

  return 0;
}

static const struct nodeID **cyclon_get_neighbourhood(int *n)
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
  if (flying_cache) {
    int i;

    for (i = 0; nodeid(flying_cache, i) && (*n < cache_size); (*n)++, i++) {
      r[*n] = nodeid(flying_cache, i);
    }
  }
  if (dst && (*n < cache_size)) {
    r[*n] = dst;
    (*n)++;
  }

  return (const struct nodeID **)r;
}

static const void *cyclon_get_metadata(int *metadata_size)
{
  return get_metadata(local_cache, metadata_size);
}

static int cyclon_grow_neighbourhood(int n)
{
  cache_size += n;

  return cache_size;
}

static int cyclon_shrink_neighbourhood(int n)
{
  if (cache_size < n) {
    return -1;
  }
  cache_size -= n;

  return cache_size;
}

static int cyclon_remove_neighbour(struct nodeID *neighbour)
{
  return cache_del(local_cache, neighbour);
}

struct peersampler_iface cyclon = {
  .init = cyclon_init,
  .change_metadata = cyclon_change_metadata,
  .add_neighbour = cyclon_add_neighbour,
  .parse_data = cyclon_parse_data,
  .get_neighbourhood = cyclon_get_neighbourhood,
  .get_metadata = cyclon_get_metadata,
  .grow_neighbourhood = cyclon_grow_neighbourhood,
  .shrink_neighbourhood = cyclon_shrink_neighbourhood,
  .remove_neighbour = cyclon_remove_neighbour,
};
