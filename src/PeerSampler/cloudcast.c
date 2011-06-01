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
#include <assert.h>

#include "net_helper.h"
#include "peersampler_iface.h"
#include "../Cache/topocache.h"
#include "../Cache/cloudcast_proto.h"
#include "../Cache/proto.h"
#include "config.h"
#include "grapes_msg_types.h"

#define DEFAULT_CACHE_SIZE 20
#define DEFAULT_PARTIAL_VIEW_SIZE 5

#define CLOUDCAST_TRESHOLD 4
#define CLOUDCAST_PERIOD 10000000 // 10 seconds
#define CLOUDCAST_BOOTSTRAP_PERIOD 2000000 // 2 seconds

struct peersampler_context{
  uint64_t currtime;
  int cache_size;
  int sent_entries;
  struct peer_cache *local_cache;
  bool bootstrap;
  int bootstrap_period;
  int period;

  uint64_t last_cloud_contact_sec;
  int max_silence;
  double cloud_respawn_prob;
  int cloud_contact_treshold;
  struct nodeID *local_node;
  struct nodeID **cloud_nodes;

  int reserved_entries;
  struct peer_cache *flying_cache;
  struct nodeID *dst;

  struct cloudcast_proto_context *proto_context;
  struct nodeID **r;
};


/* return the time in microseconds */
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
  con->bootstrap_period = CLOUDCAST_BOOTSTRAP_PERIOD;
  con->period = CLOUDCAST_PERIOD;
  con->currtime = gettime();
  con->cloud_contact_treshold = CLOUDCAST_TRESHOLD;
  con->last_cloud_contact_sec = 0;
  con->max_silence = 0;
  con->cloud_respawn_prob = 0;

  con->r = NULL;

  return con;
}

static int time_to_send(struct peersampler_context* con)
{
  long time;
  int p = con->bootstrap ? con->bootstrap_period : con->period;

  time = gettime();
  if (time - con->currtime > p) {
    if (con->bootstrap) con->currtime = time;
    else con->currtime += p;
    return 1;
  }

  return 0;
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
    con->sent_entries = DEFAULT_PARTIAL_VIEW_SIZE;
  }
  res = config_value_int(cfg_tags, "max_silence", &(con->max_silence));
  if (!res) {
    con->max_silence = 0;
  }

  res = config_value_double(cfg_tags, "cloud_respawn_prob", &(con->cloud_respawn_prob));
  if (!res) {
    con->max_silence = 0;
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
  if (cache_add(context->local_cache, neighbour, metadata, metadata_size) < 0) {
    return -1;
  } return 0;
}

static int cloudcast_query(struct peersampler_context *context,
                           struct peer_cache *remote_cache)
{
  struct peer_cache *sent_cache;

  /* Fill up reply and send it.
     NOTE: we don't know the remote node so we cannot prevent putting it into
     the reply. Remote node should check for entries of itself */
  sent_cache = rand_cache(context->local_cache, context->sent_entries - 1);
  cloudcast_reply_peer(context->proto_context, remote_cache, sent_cache,
                       context->last_cloud_contact_sec);

  /* Insert entries in view without using space reserved by the active thread */
  cache_fill_rand(context->local_cache, remote_cache,
                    context->cache_size - context->reserved_entries);


  cache_free(remote_cache);
  cache_free(sent_cache);
  return 0;
}

static int cloudcast_reply(struct peersampler_context *context,
                           struct peer_cache *remote_cache)
{
  /* Be sure not to insert local_node in cache!!! */
  cache_del(remote_cache, context->local_node);

  /* Insert remote entries in view */
  context->reserved_entries = 0;
  cache_fill_rand(context->local_cache, remote_cache, 0);

  cache_free(remote_cache);
  return 0;
}

static int cloudcast_cloud_dialogue(struct peersampler_context *context,
                                    struct peer_cache *cloud_cache)
{
  struct peer_cache * sent_cache = NULL;
  uint64_t cloud_tstamp;
  uint64_t current_time;
  int delta;

  /* Obtain the timestamp reported by the last query_cloud operation and
     compute delta */
  cloud_tstamp = cloudcast_timestamp_cloud(context->proto_context) * 1000000ull;
  current_time = gettime();
  delta = current_time - cloud_tstamp;

  /* Fill up the request with one spot free for local node */
  sent_cache = rand_cache_except(context->local_cache, context->sent_entries - 1,
                                 context->cloud_nodes, 2);


  if (cloud_cache == NULL) {
    /* Cloud is not initialized, just set the cloud cache to send_cache */
    cloudcast_reply_cloud(context->proto_context, sent_cache);
    cloudcast_cloud_default_reply(context->local_cache, context->cloud_nodes[0]);
    return 0;
  } else {
    struct peer_cache *remote_cache = NULL;
    int err = 0;
    int g = 0;
    int i = 0;


    /* If the cloud contact frequency *IS NOT* too high save an entry for the
       cloud  */
    if (delta > (context->period / context->cloud_contact_treshold)) {
      g++;
    }

    /* If the cloud contact frequency *IS* too low save another entry for the
       cloud */
    if (delta > (context->cloud_contact_treshold * context->period)) {
      g++;
    }

    /* Fill up the reply with g spots free for cloud entries */
    remote_cache = rand_cache_except(cloud_cache, context->sent_entries-g,
                                     &context->local_node, 1);

    /* Insert cloud entries in remote cache */
    cache_resize(remote_cache,  cache_current_size(remote_cache)+g);
    for (i=0; i<g; i++) {
      err = cache_add(remote_cache, context->cloud_nodes[i], NULL, 0);
      assert(err > 0);
    }

    /* Insert remote entries in local view */
    cache_fill_rand(context->local_cache, remote_cache, 0);

    /* Insert sent entries in cloud view */
    cache_del(cloud_cache, context->local_node);
    cache_resize(cloud_cache, context->cache_size);
    assert(cache_max_size(cloud_cache) == context->cache_size);
    cache_fill_rand(cloud_cache, sent_cache, 0);

    /* Write the new view in the cloud */
    cloudcast_reply_cloud(context->proto_context, cloud_cache);

    cache_free(remote_cache);
    cache_free(cloud_cache);
    cache_free(sent_cache);
    return 0;
  }
}

static int silence_treshold(struct peersampler_context *context)
{
  int threshold;
  int delta;
  if (!context->max_silence) return 0;
  if (context->last_cloud_contact_sec == 0) return 0;

  threshold = (context->max_silence * context->period) / 1000000ull;
  delta = (gettime() / 1000000ull) - context->last_cloud_contact_sec;

  return delta > threshold &&
    ((double) rand())/RAND_MAX < context->cloud_respawn_prob;
}

static int cloudcast_active_thread(struct peersampler_context *context)
{
  int err = 0;

  /* If space permits, re-insert old entries that have been sent in the previous
     cycle */
  if (context->flying_cache) {
    cache_fill_rand(context->local_cache, context->flying_cache, 0);
    cache_free(context->flying_cache);
    context->flying_cache = NULL;

    /* If we didn't received an answer since the last cycle, forget about it */
    context->reserved_entries = 0;
  }

  /* Increase age of all nodes in the view */
  cache_update(context->local_cache);

  /* Select oldest node in the view */
  context->dst = last_peer(context->local_cache);

  /* If we remain without neighbors, forcely add a cloud entry */
  if (context->dst == NULL) {
    context->dst = context->cloud_nodes[0];
  }

  context->dst = nodeid_dup(context->dst);
  cache_del(context->local_cache, context->dst);

  /* If enabled, readd a cloud node when too much time is passed from the
     last contact (computed globally) */
  if (!cloudcast_is_cloud_node(context->proto_context, context->dst) &&
      silence_treshold(context)) {
    cache_add(context->local_cache, context->cloud_nodes[0], NULL, 0);
  }

  if (cloudcast_is_cloud_node(context->proto_context, context->dst)) {
    context->last_cloud_contact_sec = gettime() / 1000000ull;

    /* Request cloud view */
    err = cloudcast_query_cloud(context->proto_context);
  } else {
    /* Fill up request keeping space for local descriptor */
    context->flying_cache = rand_cache(context->local_cache,
                                       context->sent_entries - 1);

    context->reserved_entries = cache_current_size(context->flying_cache);

    /* Send request to remote peer */
    err = cloudcast_query_peer(context->proto_context, context->flying_cache,
                               context->dst, context->last_cloud_contact_sec);
  }

  return err;
}

static int
cloudcast_parse_data(struct peersampler_context *context, const uint8_t *buff,
                     int len)
{
  int err = 0;
  cache_check(context->local_cache);

  /* If we got data, perform the appropriate passive thread operation */
  if (len) {
    const struct topo_header *th = NULL;
    const struct cloudcast_header *ch = NULL;
    struct peer_cache *remote_cache  = NULL;
    size_t shift = 0;


    th = (const struct topo_header *) buff;
    shift += sizeof(struct topo_header);

    ch = (const struct cloudcast_header *) (buff + shift);
    shift += sizeof(struct cloudcast_header);

    if (th->protocol != MSG_TYPE_TOPOLOGY) {
      fprintf(stderr, "Peer Sampler: Wrong protocol: %d!\n", th->protocol);
      return -1;
    }

    context->bootstrap = false;

    /* Update info on global cloud contact time */
    if (ch->last_cloud_contact_sec > context->last_cloud_contact_sec) {
      context->last_cloud_contact_sec = ch->last_cloud_contact_sec;
    }

    if (len - shift > 0)
      remote_cache = entries_undump(buff + (shift * sizeof(uint8_t)), len - shift);

    if (th->type == CLOUDCAST_QUERY) {
      /* We're being queried by a remote peer */
      err = cloudcast_query(context, remote_cache);
    } else if(th->type == CLOUDCAST_REPLY){
      /* We've received the response to our query from the remote peer */
      err = cloudcast_reply(context, remote_cache);
    } else if (th->type == CLOUDCAST_CLOUD) {
      /* We've received the response form the cloud. Simulate dialogue */
      err = cloudcast_cloud_dialogue(context, remote_cache);
    } else {
      fprintf(stderr, "Cloudcast: Wrong message type: %d\n", th->type);
      return -1;
    }
  }

  /* It it's time, perform the active thread */
  if (time_to_send(context)) {
    err = cloudcast_active_thread(context);
  }
  cache_check(context->local_cache);

  return err;
}

static const struct nodeID *const *cloudcast_get_neighbourhood(struct peersampler_context *context, int *n)
{
  context->r = realloc(context->r, context->cache_size * sizeof(struct nodeID *));
  if (context->r == NULL) {
    return NULL;
  }

  for (*n = 0; nodeid(context->local_cache, *n) && (*n < context->cache_size); (*n)++) {
    context->r[*n] = nodeid(context->local_cache, *n);
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

  return (const struct nodeID *const *)context->r;
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
