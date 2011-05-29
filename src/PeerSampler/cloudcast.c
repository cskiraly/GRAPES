/*
 *  Copyright (c) 2010 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 *
 * Implementation of the Cloudcast peer sampling protocol.
 *
 * If thread are used calls to psample_init which result in calls to
 * cloudcast_init must be syncronized with calls to cloud_helper_init
 * and get_cloud_helper_for.
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
#include "../Cache/cloudcast_proto.h"
#include "../Cache/proto.h"
#include "config.h"
#include "grapes_msg_types.h"

#define DEFAULT_CACHE_SIZE 10
#define DEFAULT_CLOUD_CONTACT_TRESHOLD 4000000

struct peersampler_context{
  uint64_t currtime;
  int cache_size;
  int sent_entries;
  int keep_cache_full;
  struct peer_cache *local_cache;
  bool bootstrap;
  int bootstrap_period;
  int period;

  int cloud_contact_treshold;
  struct nodeID *local_node;
  struct nodeID **cloud_nodes;

  struct peer_cache *flying_cache;
  struct nodeID *dst;

  struct cloudcast_proto_context *proto_context;
  struct nodeID **r;
};


static uint64_t gettime(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_usec + tv.tv_sec * 1000000ull;
}

static struct peersampler_context* cloudcast_context_init(void){
  struct peersampler_context* con;
  con = (struct peersampler_context*) calloc(1,sizeof(struct peersampler_context));
  if (!con) {
    fprintf(stderr, "cloudcast: Error! could not create context. ENOMEM\n");
    return NULL;
  }
  memset(con, 0, sizeof(struct peersampler_context));

  //Initialize context with default values
  con->bootstrap = true;
  con->bootstrap_period = 2000000;
  con->period = 10000000;
  con->currtime = gettime();
  con->r = NULL;
  con->cloud_contact_treshold = DEFAULT_CLOUD_CONTACT_TRESHOLD;

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

static int cache_add_resize(struct peer_cache *dst, struct nodeID *n, const int cache_max_size)
{
  int err;
  err = cache_add(dst, n, NULL, 0);
  if (err == -2){
    // maybe the cache is full... try resizing it to cache_max
    cache_resize(dst, cache_max_size);
    err = cache_add(dst, n, NULL, 0);
  }
  return (err > 0)? 0 : 1;
}

/*
 * Public Functions!
 */
static struct peersampler_context* cloudcast_init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config)
{
  struct tag *cfg_tags;
  struct peersampler_context *con;
  int res;

  con = cloudcast_context_init();
  if (!con) return NULL;
  con->local_node = myID;

  cfg_tags = config_parse(config);
  res = config_value_int(cfg_tags, "cache_size", &(con->cache_size));
  if (!res) {
    con->cache_size = DEFAULT_CACHE_SIZE;
  }
  res = config_value_int(cfg_tags, "sent_entries", &(con->sent_entries));
  if (!res) {
    con->sent_entries = con->cache_size / 2;
  }
  res = config_value_int(cfg_tags, "keep_cache_full", &(con->keep_cache_full));
  if (!res) {
    con->keep_cache_full = 1;
  }

  con->local_cache = cache_init(con->cache_size, metadata_size, 0);
  if (con->local_cache == NULL) {
    fprintf(stderr, "cloudcast: Error initializing local cache\n");
    free(con);
    return NULL;
  }

  con->proto_context = cloudcast_proto_init(myID, metadata, metadata_size);
  if (!con->proto_context){
    free(con->local_cache);
    free(con);
    return NULL;
  }
  con->cloud_nodes = cloudcast_get_cloud_nodes(con->proto_context, 2);

  return con;
}

static int cloudcast_add_neighbour(struct peersampler_context *context, struct nodeID *neighbour, const void *metadata, int metadata_size)
{
  if (!context->flying_cache) {
    context->flying_cache = rand_cache(context->local_cache, context->sent_entries - 1);
  }
  if (cache_add(context->local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  }

  if (cloudcast_is_cloud_node(context->proto_context, neighbour) == 0)
    return cloudcast_query_peer(context->proto_context, context->flying_cache, neighbour);
  else
    return cloudcast_query_cloud(context->proto_context);
}

static int cloudcast_parse_data(struct peersampler_context *context, const uint8_t *buff, int len)
{
  cache_check(context->local_cache);
  if (len) {
    // Passive thread
    const struct topo_header *h = (const struct topo_header *)buff;
    struct peer_cache *remote_cache  = NULL;
    struct peer_cache *sent_cache = NULL;

    if (h->protocol != MSG_TYPE_TOPOLOGY) {
      fprintf(stderr, "Peer Sampler: Wrong protocol!\n");

      return -1;
    }

    context->bootstrap = false;

    if (len - sizeof(struct topo_header) > 0)
      remote_cache = entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));


    if (h->type == CLOUDCAST_QUERY) {
      sent_cache = rand_cache(context->local_cache, context->sent_entries);
      cloudcast_reply_peer(context->proto_context, remote_cache, sent_cache);
      context->dst = NULL;
    } else if (h->type == CLOUDCAST_CLOUD) {
      int g = 0;
      int i = 0;
      struct peer_cache *reply_cache = NULL;

      // Obtain the timestamp reported by the last query_cloud operation
      uint64_t cloud_time_stamp = cloudcast_timestamp_cloud(context->proto_context) * 1000000ull;
      uint64_t current_time = gettime();
      int delta = current_time - cloud_time_stamp;

      // local view with one spot free for the local nodeID
      sent_cache = rand_cache(context->local_cache, context->sent_entries - 1);
      for (i=0; i<2; i++) cache_del(sent_cache, context->cloud_nodes[i]);

      if (remote_cache == NULL) {
        // Cloud is not initialized, just set the cloud cache to send_cache
        cloudcast_reply_cloud(context->proto_context, sent_cache);
        remote_cache = cloudcast_cloud_default_reply(context->local_cache, context->cloud_nodes[0]);
      } else {
        // Locally perform exchange of cache entries
        int err;
        struct peer_cache *cloud_cache = NULL;

        // if the cloud contact frequency *IS NOT* too high save an entry for the cloud
        if (delta > (context->period / context->cloud_contact_treshold)) g++;

        // if the cloud contact frequency *IS* too low save another entry for the cloud
        if (delta > (context->cloud_contact_treshold * context->period)) g++;

        // Remove mentions of local node from the cache
        cache_del(remote_cache, context->local_node);

        // cloud view with g spot free for cloud nodeID
        reply_cache = rand_cache(remote_cache, context->sent_entries - g);

        // building new cloud view
        cloud_cache = merge_caches(remote_cache, sent_cache, context->cache_size, &err);
        free(remote_cache);
        cache_add_cache(cloud_cache, sent_cache);
        if (context->keep_cache_full){
          cache_add_cache(cloud_cache, reply_cache);
        }

        err = cloudcast_reply_cloud(context->proto_context, cloud_cache);
        free(cloud_cache);

        //Add the actual cloud nodes to the cache
        for (i = 0; i<g; i++){
          if (cache_add_resize(reply_cache, context->cloud_nodes[i], context->sent_entries) != 0){
            fprintf(stderr, "WTF!!! cannot add to cache\n");
            return 1;
          }
        }

        remote_cache = reply_cache;
        context->dst = NULL;
      }
    }
    cache_check(context->local_cache);

    if (remote_cache){
      // Remote cache may be NULL due to a non initialize cloud
      cache_add_cache(context->local_cache, remote_cache);
      cache_free(remote_cache);
    }
    if (sent_cache) {
      if (context->keep_cache_full)
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

  // Active thread
  if (time_to_send(context)) {
    if (context->flying_cache) {
      cache_add_cache(context->local_cache, context->flying_cache);
      cache_free(context->flying_cache);
      context->flying_cache = NULL;
    }
    cache_update(context->local_cache);
    context->dst = last_peer(context->local_cache);
    if (context->dst == NULL) {
      // If we remain without neighbors, forcely add a cloud entry
      context->dst = cloudcast_get_cloud_node(context->proto_context);
      cache_add(context->local_cache, context->dst, NULL, 0);
    }
    context->dst = nodeid_dup(context->dst);
    cache_del(context->local_cache, context->dst);

    if (cloudcast_is_cloud_node(context->proto_context, context->dst)) {
      int err;
      err = cloudcast_query_cloud(context->proto_context);
    } else {
      context->flying_cache = rand_cache(context->local_cache, context->sent_entries - 1);
      cloudcast_query_peer(context->proto_context, context->flying_cache, context->dst);
    }
  }
  cache_check(context->local_cache);

  return 0;
  }

static const struct nodeID **cloudcast_get_neighbourhood(struct peersampler_context *context, int *n)
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
    int i,j,dup;

    for (i = 0; nodeid(context->flying_cache, i) && (*n < context->cache_size); (*n)++, i++) {
      dup = 0;
      for (j = 0; j<*n; j++){
        if (nodeid_equal(context->r[j], nodeid(context->flying_cache, i))){
          dup = 1;
          continue;
        }
      }
      if (dup) (*n)--;
      else context->r[*n] = nodeid(context->flying_cache, i);
    }
  }

  if (context->dst && (*n < context->cache_size)) {
    int j,dup;
    dup = 0;
    for (j = 0; j<*n; j++){
      if (nodeid_equal(context->r[j], context->dst)){
        dup = 1;
        continue;
      }
    }
    if (!dup){
      context->r[*n] = context->dst;
      (*n)++;
    }
  }

  return (const struct nodeID **)context->r;
}

static const void *cloudcast_get_metadata(struct peersampler_context *context, int *metadata_size)
{
  return get_metadata(context->local_cache, metadata_size);
}

static int cloudcast_grow_neighbourhood(struct peersampler_context *context, int n)
{
  context->cache_size += n;

  return context->cache_size;
}

static int cloudcast_shrink_neighbourhood(struct peersampler_context *context, int n)
{
  if (context->cache_size < n) {
    return -1;
  }
  context->cache_size -= n;

  return context->cache_size;
}

static int cloudcast_remove_neighbour(struct peersampler_context *context, const struct nodeID *neighbour)
{
  return cache_del(context->local_cache, neighbour);
}

static int cloudcast_change_metadata(struct peersampler_context *context, const void *metadata, int metadata_size)
{
  return cloudcast_proto_change_metadata(context->proto_context, metadata, metadata_size);
}

struct peersampler_iface cloudcast = {
  .init = cloudcast_init,
  .change_metadata = cloudcast_change_metadata,
  .add_neighbour = cloudcast_add_neighbour,
  .parse_data = cloudcast_parse_data,
  .get_neighbourhood = cloudcast_get_neighbourhood,
  .get_metadata = cloudcast_get_metadata,
  .grow_neighbourhood = cloudcast_grow_neighbourhood,
  .shrink_neighbourhood = cloudcast_shrink_neighbourhood,
  .remove_neighbour = cloudcast_remove_neighbour,
};
