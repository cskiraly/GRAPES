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

typedef struct cyclon_context{
  uint64_t currtime;
  int cache_size;
  int sent_entries;
  struct peer_cache *local_cache;
  bool bootstrap;
  int bootstrap_period;
  int period;
  
  struct peer_cache *flying_cache;
  struct nodeID *dst;
} cyclon_context_t;

static cyclon_context_t* cyclon_context_init(){
  cyclon_context_t* con = (cyclon_context_t*) calloc(1,sizeof(cyclon_context_t));

  //Initialize context with default values
  con->bootstrap = true;
  con->bootstrap_period = 2000000;
  con->period = 10000000;

  return con;
}

static cyclon_context_t* get_cyclon_context(void *context){
  return (cyclon_context_t*) context;
}

static uint64_t gettime(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_usec + tv.tv_sec * 1000000ull;
}

static int time_to_send(cyclon_context_t* con)
{
  int p = con->bootstrap ? con->bootstrap_period : con->period;
  if (gettime() - con->currtime > p) {
    con->currtime += p;

    return 1;
  }

  return 0;
}

static void cache_add_cache(struct peer_cache *dst, const struct peer_cache *add)
{
  int i, meta_size;
  const uint8_t *meta;

  meta = get_metadata(add, &meta_size);
  for (i = 0; nodeid(add, i); i++) {
    cache_add(dst,  nodeid(add, i), meta + (meta_size * i), meta_size);
  }
}


/*
 * Public Functions!
 */
static int cyclon_init(struct nodeID *myID, void *metadata, int metadata_size, const char *config, void **cyclon_context)
{
  struct tag *cfg_tags;
  cyclon_context_t *con;
  int res;

  con = cyclon_context_init();
  if (!con) return -1;
  *cyclon_context = (void*) con;

  cfg_tags = config_parse(config);
  res = config_value_int(cfg_tags, "cache_size", &(con->cache_size));
  if (!res) {
    con->cache_size = DEFAULT_CACHE_SIZE;
  }
  res = config_value_int(cfg_tags, "sent_entries", &(con->sent_entries));
  if (!res) {
    con->sent_entries = con->cache_size / 2;
  }

  con->local_cache = cache_init(con->cache_size, metadata_size, 0);
  if (con->local_cache == NULL) {
    return -1;
  }
  topo_proto_init(myID, metadata, metadata_size);
  con->currtime = gettime();
  con->bootstrap = true;

  return 1;
}

static int cyclon_change_metadata(void *context, void *metadata, int metadata_size)
{
  if (topo_proto_metadata_update(metadata, metadata_size) <= 0) {
    return -1;
  }

  return 1;
}

static int cyclon_add_neighbour(void *context, struct nodeID *neighbour, void *metadata, int metadata_size)
{
  cyclon_context_t *con = get_cyclon_context(context);
  if (!con->flying_cache) {
    con->flying_cache = rand_cache(con->local_cache, con->sent_entries - 1);
  }
  if (cache_add(con->local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  }

  return cyclon_query(con->flying_cache, neighbour);
}

static int cyclon_parse_data(void *context, const uint8_t *buff, int len)
{
  cyclon_context_t *con = get_cyclon_context(context);
  cache_check(con->local_cache);
  if (len) {
    const struct topo_header *h = (const struct topo_header *)buff;
    struct peer_cache *remote_cache;
    struct peer_cache *sent_cache = NULL;

    if (h->protocol != MSG_TYPE_TOPOLOGY) {
      fprintf(stderr, "Peer Sampler: Wrong protocol!\n");

      return -1;
    }

    con->bootstrap = false;

    remote_cache = entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));
    if (h->type == CYCLON_QUERY) {
      sent_cache = rand_cache(con->local_cache, con->sent_entries);
      cyclon_reply(remote_cache, sent_cache);
      con->dst = NULL;
    }
    cache_check(con->local_cache);
    cache_add_cache(con->local_cache, remote_cache);
    cache_free(remote_cache);
    if (sent_cache) {
      cache_add_cache(con->local_cache, sent_cache);
      cache_free(sent_cache);
    } else {
      if (con->flying_cache) {
        cache_add_cache(con->local_cache, con->flying_cache);
        cache_free(con->flying_cache);
        con->flying_cache = NULL;
      }
    }
  }

  if (time_to_send(con)) {
    if (con->flying_cache) {
      cache_add_cache(con->local_cache, con->flying_cache);
      cache_free(con->flying_cache);
      con->flying_cache = NULL;
    }
    cache_update(con->local_cache);
    con->dst = last_peer(con->local_cache);
    if (con->dst == NULL) {
      return 0;
    }
    con->dst = nodeid_dup(con->dst);
    cache_del(con->local_cache, con->dst);
    con->flying_cache = rand_cache(con->local_cache, con->sent_entries - 1);
    cyclon_query(con->flying_cache, con->dst);
  }
  cache_check(con->local_cache);

  return 0;
}

static const struct nodeID **cyclon_get_neighbourhood(void* context, int *n)
{
  static struct nodeID **r;
  cyclon_context_t *con = get_cyclon_context(context);

  r = realloc(r, con->cache_size * sizeof(struct nodeID *));
  if (r == NULL) {
    return NULL;
  }

  for (*n = 0; nodeid(con->local_cache, *n) && (*n < con->cache_size); (*n)++) {
    r[*n] = nodeid(con->local_cache, *n);
    //fprintf(stderr, "Checking table[%d]\n", *n);
  }
  if (con->flying_cache) {
    int i;

    for (i = 0; nodeid(con->flying_cache, i) && (*n < con->cache_size); (*n)++, i++) {
      r[*n] = nodeid(con->flying_cache, i);
    }
  }
  if (con->dst && (*n < con->cache_size)) {
    r[*n] = con->dst;
    (*n)++;
  }

  return (const struct nodeID **)r;
}

static const void *cyclon_get_metadata(void *context, int *metadata_size)
{
  cyclon_context_t *con = get_cyclon_context(context);
  return get_metadata(con->local_cache, metadata_size);
}

static int cyclon_grow_neighbourhood(void *context, int n)
{
  cyclon_context_t *con = get_cyclon_context(context);
  con->cache_size += n;

  return con->cache_size;
}

static int cyclon_shrink_neighbourhood(void *context, int n)
{
  cyclon_context_t *con = get_cyclon_context(context);
  if (con->cache_size < n) {
    return -1;
  }
  con->cache_size -= n;

  return con->cache_size;
}

static int cyclon_remove_neighbour(void *context, struct nodeID *neighbour)
{
  cyclon_context_t *con = get_cyclon_context(context);
  return cache_del(con->local_cache, neighbour);
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
