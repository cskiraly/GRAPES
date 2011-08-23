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
#include <limits.h>

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
  int cache_size_threshold;
  struct peer_cache *local_cache;
  bool bootstrap;
  struct nodeID *bootstrap_node;
  int bootstrap_period;
  int bootstrap_cycles;
  int period;
  int counter;
  struct ncast_proto_context *tc;
  const struct nodeID **r;
  int query_tokens;
  int reply_tokens;
  int first_ts;
  int adaptive;
  int restart;
  int randomize;
  int slowstart;
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
  con->bootstrap_node = NULL;
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

static void cache_size_threshold_init(struct peersampler_context* context)
{
  context->cache_size_threshold = (context->cache_size - 1 / 2);
}

/*
 * Exported Functions!
 */
static struct peersampler_context* init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config, int plus_features)
{
  struct tag *cfg_tags;
  struct peersampler_context *context;
  int res, max_timestamp;

  context = ncast_context_init();
  if (!context) return NULL;

  cfg_tags = config_parse(config);
  res = config_value_int_default(cfg_tags, "cache_size", &context->cache_size, DEFAULT_CACHE_SIZE);
  res = config_value_int_default(cfg_tags, "max_timestamp", &max_timestamp, DEFAULT_MAX_TIMESTAMP);
  res = config_value_int_default(cfg_tags, "period", &context->period, DEFAULT_PERIOD);
  res = config_value_int_default(cfg_tags, "bootstrap_period", &context->bootstrap_period, DEFAULT_BOOTSTRAP_PERIOD);
  res = config_value_int_default(cfg_tags, "bootstrap_cycles", &context->bootstrap_cycles, DEFAULT_BOOTSTRAP_CYCLES);
  res = config_value_int_default(cfg_tags, "adaptive", &context->adaptive, plus_features);
  res = config_value_int_default(cfg_tags, "restart", &context->restart, plus_features);
  res = config_value_int_default(cfg_tags, "randomize", &context->randomize, plus_features);
  res = config_value_int_default(cfg_tags, "slowstart", &context->slowstart, plus_features);
  free(cfg_tags);

  context->local_cache = cache_init(context->cache_size, metadata_size, max_timestamp);
  if (context->local_cache == NULL) {
    free(context);
    return NULL;
  }

  cache_size_threshold_init(context);

  context->tc = ncast_proto_init(myID, metadata, metadata_size);
  if (!context->tc){
    free(context->local_cache);
    free(context);
    return NULL;
  }

  context->query_tokens = 0;
  context->reply_tokens = 0;
  context->first_ts = (max_timestamp + 1) / 2;
  // increase timestamp for initial message, since that is out of the normal cycle of the bootstrap peer
  ncast_proto_myentry_update(context->tc, NULL, context->first_ts, NULL, 0);

  return context;
}

static int ncast_change_metadata(struct peersampler_context *context, const void *metadata, int metadata_size)
{
  if (ncast_proto_metadata_update(context->tc, metadata, metadata_size) <= 0) {
    return -1;
  }

  return 1;
}

static struct peersampler_context* ncast_init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config)
{
    return init(myID, metadata, metadata_size, config, 0);
}

static struct peersampler_context* ncastplus_init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config)
{
    return init(myID, metadata, metadata_size, config, 1);
}

static int ncast_add_neighbour(struct peersampler_context *context, struct nodeID *neighbour, const void *metadata, int metadata_size)
{
  if (cache_add(context->local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  }
  if (!context->bootstrap_node) {	//save the first added nodeid as bootstrap nodeid
    context->bootstrap_node = nodeid_dup(neighbour);
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
    if (context->counter == context->bootstrap_cycles) {
      context->bootstrap = false;
      ncast_proto_myentry_update(context->tc, NULL , - context->first_ts, NULL, 0);  // reset the timestamp of our own ID, we are in normal cycle, we will not disturb the algorithm
    }

    remote_cache = entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));
    if (h->type == NCAST_QUERY) {
      context->reply_tokens--;	//sending a reply to someone who presumably receives it
      cache_randomize(context->local_cache);
      ncast_reply(context->tc, remote_cache, context->local_cache);
    } else {
     context->query_tokens--;	//a query was successful
    }
    cache_randomize(context->local_cache);
    cache_randomize(remote_cache);
    new = merge_caches(context->local_cache, remote_cache, context->cache_size, &dummy);
    cache_free(remote_cache);
    if (new != NULL) {
      cache_free(context->local_cache);
      context->local_cache = new;
    }
  }

  if (time_to_send(context)) {
    int ret = INT_MIN;
    int i;
    int entries = cache_entries(context->local_cache);

    if (context->bootstrap_node &&
        (entries <= context->cache_size_threshold) &&
        (cache_pos(context->local_cache, context->bootstrap_node) < 0)) {
      cache_add(context->local_cache, context->bootstrap_node, NULL, 0);
	  entries = cache_entries(context->local_cache);
    }

    context->query_tokens++;
    if (context->reply_tokens++ > 0) {//on average one reply is sent, if not, do something
      context->query_tokens += context->reply_tokens;
      context->reply_tokens = 0;
    }
    if (context->query_tokens > entries) context->query_tokens = entries;	//don't be too aggressive

    cache_update(context->local_cache);
    for (i = 0; i < context->query_tokens; i++) {
      int r;

      r = ncast_query(context->tc, context->local_cache);
      r = r > ret ? r : ret;
    }
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
  cache_size_threshold_init(context);

  return context->cache_size;
}

static int ncast_shrink_neighbourhood(struct peersampler_context *context, int n)
{
  if (context->cache_size < n) {
    return -1;
  }
  context->cache_size -= n;
  cache_size_threshold_init(context);

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

struct peersampler_iface ncastplus = {
  .init = ncastplus_init,
  .change_metadata = ncast_change_metadata,
  .add_neighbour = ncast_add_neighbour,
  .parse_data = ncast_parse_data,
  .get_neighbourhood = ncast_get_neighbourhood,
  .get_metadata = ncast_get_metadata,
  .grow_neighbourhood = ncast_grow_neighbourhood,
  .shrink_neighbourhood = ncast_shrink_neighbourhood,
  .remove_neighbour = ncast_remove_neighbour,
};
