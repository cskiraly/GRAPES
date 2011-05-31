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
#include "../Cache/ncast_proto.h"
#include "../Cache/proto.h"
#include "config.h"
#include "grapes_msg_types.h"

#define DEFAULT_CACHE_SIZE 10
#define DEFAULT_MAX_TIMESTAMP 5
#define DEFAULT_BOOTSTRAP_CYCLES 5
#define DEFAULT_BOOTSTRAP_PERIOD 2*1000*1000
#define DEFAULT_PERIOD 10*1000*1000

struct peersampler_context{
  uint64_t currtime;
  int cache_size;
  struct peer_cache *local_cache;
  bool bootstrap;
  int bootstrap_period;
  int bootstrap_cycles;
  int period;
  int counter;
  struct ncast_proto_context *tc;
  const struct nodeID **r;
};

static uint64_t gettime(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_usec + tv.tv_sec * 1000000ull;
}

static struct peersampler_context* ncast_context_init(void)
{
  struct peersampler_context* con;
  con = (struct peersampler_context*) calloc(1,sizeof(struct peersampler_context));

  //Initialize context with default values
  con->bootstrap = true;
  con->currtime = gettime();
  con->r = NULL;

  return con;
}

static int time_to_send(struct peersampler_context *context)
{
  int p = context->bootstrap ? context->bootstrap_period : context->period;
  if (gettime() - context->currtime > p) {
    context->currtime += p;

    return 1;
  }

  return 0;
}

/*
 * Exported Functions!
 */
static struct peersampler_context* ncast_init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config)
{
  struct tag *cfg_tags;
  struct peersampler_context *context;
  int res, max_timestamp;

  context = ncast_context_init();
  if (!context) return NULL;

  cfg_tags = config_parse(config);
  res = config_value_int(cfg_tags, "cache_size", &context->cache_size);
  if (!res) {
    context->cache_size = DEFAULT_CACHE_SIZE;
  }
  res = config_value_int(cfg_tags, "max_timestamp", &max_timestamp);
  if (!res) {
    max_timestamp = DEFAULT_MAX_TIMESTAMP;
  }
  res = config_value_int(cfg_tags, "period", &context->period);
  if (!res) {
    context->period = DEFAULT_PERIOD;
  }
  res = config_value_int(cfg_tags, "bootstrap_period", &context->bootstrap_period);
  if (!res) {
    context->bootstrap_period = DEFAULT_BOOTSTRAP_PERIOD;
  }
  res = config_value_int(cfg_tags, "bootstrap_cycles", &context->bootstrap_cycles);
  if (!res) {
    context->bootstrap_cycles = DEFAULT_BOOTSTRAP_CYCLES;
  }
  free(cfg_tags);
  
  context->local_cache = cache_init(context->cache_size, metadata_size, max_timestamp);
  if (context->local_cache == NULL) {
    free(context);
    return NULL;
  }

  context->tc = ncast_proto_init(myID, metadata, metadata_size);
  if (!context->tc){
    free(context->local_cache);
    free(context);
    return NULL;
  }

  return context;
}

static int ncast_change_metadata(struct peersampler_context *context, const void *metadata, int metadata_size)
{
  if (ncast_proto_metadata_update(context->tc, metadata, metadata_size) <= 0) {
    return -1;
  }

  return 1;
}

static int ncast_add_neighbour(struct peersampler_context *context, struct nodeID *neighbour, const void *metadata, int metadata_size)
{
  if (cache_add(context->local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  }
  return ncast_query_peer(context->tc, context->local_cache, neighbour);
}

static int ncast_parse_data(struct peersampler_context *context, const uint8_t *buff, int len)
{
  int dummy;

  if (len) {
    const struct topo_header *h = (const struct topo_header *)buff;
    struct peer_cache *new, *remote_cache;

    if (h->protocol != MSG_TYPE_TOPOLOGY) {
      fprintf(stderr, "NCAST: Wrong protocol!\n");

      return -1;
    }

    context->counter++;
    if (context->counter == context->bootstrap_cycles) context->bootstrap = false;

    remote_cache = entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));
    if (h->type == NCAST_QUERY) {
      ncast_reply(context->tc, remote_cache, context->local_cache);
    }
    new = merge_caches(context->local_cache, remote_cache, context->cache_size, &dummy);
    cache_free(remote_cache);
    if (new != NULL) {
      cache_free(context->local_cache);
      context->local_cache = new;
    }
  }

  if (time_to_send(context)) {
    cache_update(context->local_cache);
    return ncast_query(context->tc, context->local_cache);
  }

  return 0;
}

static const struct nodeID *const*ncast_get_neighbourhood(struct peersampler_context *context, int *n)
{
  context->r = realloc(context->r, context->cache_size * sizeof(struct nodeID *));
  if (context->r == NULL) {
    return NULL;
  }

  for (*n = 0; nodeid(context->local_cache, *n) && (*n < context->cache_size); (*n)++) {
    context->r[*n] = nodeid(context->local_cache, *n);
    //fprintf(stderr, "Checking table[%d]\n", *n);
  }

  return context->r;
}

static const void *ncast_get_metadata(struct peersampler_context *context, int *metadata_size)
{
  return get_metadata(context->local_cache, metadata_size);
}

static int ncast_grow_neighbourhood(struct peersampler_context *context, int n)
{
  context->cache_size += n;

  return context->cache_size;
}

static int ncast_shrink_neighbourhood(struct peersampler_context *context, int n)
{
  if (context->cache_size < n) {
    return -1;
  }
  context->cache_size -= n;

  return context->cache_size;
}

static int ncast_remove_neighbour(struct peersampler_context *context, const struct nodeID *neighbour)
{
  return cache_del(context->local_cache, neighbour);
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
