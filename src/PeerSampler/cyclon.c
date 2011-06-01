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
#include "../Cache/topocache.h"
#include "../Cache/cyclon_proto.h"
#include "../Cache/proto.h"
#include "config.h"
#include "grapes_msg_types.h"

#define DEFAULT_CACHE_SIZE 10

struct peersampler_context{
  uint64_t currtime;
  int cache_size;
  int sent_entries;
  struct peer_cache *local_cache;
  bool bootstrap;
  int bootstrap_period;
  int period;
  
  struct peer_cache *flying_cache;
  struct nodeID *dst;

  struct cyclon_proto_context *pc;
  const struct nodeID **r;
};


static uint64_t gettime(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_usec + tv.tv_sec * 1000000ull;
}

static struct peersampler_context* cyclon_context_init(void)
{
  struct peersampler_context* con;
  con = (struct peersampler_context*) calloc(1,sizeof(struct peersampler_context));

  //Initialize context with default values
  con->bootstrap = true;
  con->bootstrap_period = 2000000;
  con->period = 10000000;
  con->currtime = gettime();

  return con;
}

static int time_to_send(struct peersampler_context* con)
{
  int p = con->bootstrap ? con->bootstrap_period : con->period;
  if (gettime() - con->currtime > p) {
    con->currtime += p;

    return 1;
  }

  return 0;
}

/*
 * Public Functions!
 */
static struct peersampler_context* cyclon_init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config)
{
  struct tag *cfg_tags;
  struct peersampler_context *con;
  int res;

  con = cyclon_context_init();
  if (!con) return NULL;

  cfg_tags = config_parse(config);
  res = config_value_int(cfg_tags, "cache_size", &(con->cache_size));
  if (!res) {
    con->cache_size = DEFAULT_CACHE_SIZE;
  }
  res = config_value_int(cfg_tags, "sent_entries", &(con->sent_entries));
  if (!res) {
    con->sent_entries = con->cache_size / 2;
  }
  free(cfg_tags);

  con->local_cache = cache_init(con->cache_size, metadata_size, 0);
  if (con->local_cache == NULL) {
    free(con);
    return NULL;
  }

  con->pc = cyclon_proto_init(myID, metadata, metadata_size);
  if (!con->pc){
    free(con->local_cache);
    free(con);
    return NULL;
  }

  return con;
}

static int cyclon_add_neighbour(struct peersampler_context *context, struct nodeID *neighbour, const void *metadata, int metadata_size)
{
  if (!context->flying_cache) {
    context->flying_cache = rand_cache(context->local_cache, context->sent_entries - 1);
  }
  if (cache_add(context->local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  }

  return cyclon_query(context->pc, context->flying_cache, neighbour);
}

static int cyclon_parse_data(struct peersampler_context *context, const uint8_t *buff, int len)
{
  cache_check(context->local_cache);
  if (len) {
    const struct topo_header *h = (const struct topo_header *)buff;
    struct peer_cache *remote_cache;
    struct peer_cache *sent_cache = NULL;

    if (h->protocol != MSG_TYPE_TOPOLOGY) {
      fprintf(stderr, "Peer Sampler: Wrong protocol!\n");

      return -1;
    }

    context->bootstrap = false;

    remote_cache = entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));
    if (h->type == CYCLON_QUERY) {
      sent_cache = rand_cache(context->local_cache, context->sent_entries);
      cyclon_reply(context->pc, remote_cache, sent_cache);
      nodeid_free(context->dst);
      context->dst = NULL;
    }
    cache_check(context->local_cache);
    cache_add_cache(context->local_cache, remote_cache);
    cache_free(remote_cache);
    if (sent_cache) {
      cache_add_cache(context->local_cache, sent_cache);
      cache_free(sent_cache);
    } else {
      if (context->flying_cache) {
        cache_add_cache(context->local_cache, context->flying_cache);
        cache_free(context->flying_cache);
        context->flying_cache = NULL;
      }
    }
  }

  if (time_to_send(context)) {
    if (context->flying_cache) {
      cache_add_cache(context->local_cache, context->flying_cache);
      cache_free(context->flying_cache);
      context->flying_cache = NULL;
    }
    cache_update(context->local_cache);
    nodeid_free(context->dst);
    context->dst = last_peer(context->local_cache);
    if (context->dst == NULL) {
      return 0;
    }
    context->dst = nodeid_dup(context->dst);
    cache_del(context->local_cache, context->dst);
    context->flying_cache = rand_cache(context->local_cache, context->sent_entries - 1);
    return cyclon_query(context->pc, context->flying_cache, context->dst);
  }
  cache_check(context->local_cache);

  return 0;
}

static const struct nodeID *const *cyclon_get_neighbourhood(struct peersampler_context *context, int *n)
{
  context->r = realloc(context->r, context->cache_size * sizeof(struct nodeID *));
  if (context->r == NULL) {
    return NULL;
  }

  for (*n = 0; nodeid(context->local_cache, *n) && (*n < context->cache_size); (*n)++) {
    context->r[*n] = nodeid(context->local_cache, *n);
    //fprintf(stderr, "Checking table[%d]\n", *n);
  }
  if (context->flying_cache) {
    int i;

    for (i = 0; nodeid(context->flying_cache, i) && (*n < context->cache_size); (*n)++, i++) {
      context->r[*n] = nodeid(context->flying_cache, i);
    }
  }
  if (context->dst && (*n < context->cache_size)) {
    context->r[*n] = context->dst;
    (*n)++;
  }

  return context->r;
}

static const void *cyclon_get_metadata(struct peersampler_context *context, int *metadata_size)
{
  return get_metadata(context->local_cache, metadata_size);
}

static int cyclon_grow_neighbourhood(struct peersampler_context *context, int n)
{
  context->cache_size += n;

  return context->cache_size;
}

static int cyclon_shrink_neighbourhood(struct peersampler_context *context, int n)
{
  if (context->cache_size < n) {
    return -1;
  }
  context->cache_size -= n;

  return context->cache_size;
}

static int cyclon_remove_neighbour(struct peersampler_context *context, const struct nodeID *neighbour)
{
  return cache_del(context->local_cache, neighbour);
}

static int cyclon_change_metadata(struct peersampler_context *context, const void *metadata, int metadata_size)
{
  return cyclon_proto_change_metadata(context->pc, metadata, metadata_size);
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
