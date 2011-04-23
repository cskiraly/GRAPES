/*
 *  Copyright (c) 2010 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net_helper.h"
#include "topocache.h"
#include "proto.h"
#include "topo_proto.h"
#include "cloudcast_proto.h"
#include "grapes_msg_types.h"

// TODO: This should not be duplicated here. should inherit from topo_proto.c
#define MAX_MSG_SIZE 1500

#define CLOUD_VIEW_KEY "view"
#define CLOUDCAST_MESSAGE_HEADER 10 /* PROTOCOL MSG_TYPE LAST_CLOUD_CONTACT */

uint8_t cloud_header[10] = { MSG_TYPE_TOPOLOGY, CLOUDCAST_CLOUD, 0llu};

struct cloudcast_proto_context {
  struct peer_cache* myEntry;
  uint8_t def_cloudcache[128];
  int def_cloudcache_len;
  struct topo_context *topo_context;
  struct cloud_helper_context *cloud_context;
};

int cloudcast_payload_fill(struct cloudcast_proto_context *context, uint8_t *payload, int size, struct peer_cache *c, int max_peers, int include_me);

struct cloudcast_proto_context* cloudcast_proto_init(struct nodeID *s, const void *meta, int meta_size)
{
  struct cloudcast_proto_context *con;
  struct peer_cache *tmp;
  con = malloc(sizeof(struct cloudcast_proto_context));

  if (!con) {
    fprintf(stderr, "cloudcast_proto: Error initializing context. ENOMEM\n");
    return NULL;
  }

  con->topo_context = topo_proto_init(s, meta, meta_size);
  if (!con->topo_context){
    fprintf(stderr, "cloudcast_proto: Error initializing topo_proto.\n");
    free(con);
    return NULL;
  }

  con->cloud_context = get_cloud_helper_for(s);
  if (!con->cloud_context) {
    fprintf(stderr, "cloudcast_proto: Error retrieving cloud_helper for current node\n");
    free(con);
    return NULL;
  }

  con->myEntry = cache_init(1, meta_size, 0);
  tmp = cache_init(0, meta_size, 0);
  con->def_cloudcache_len = cloudcast_payload_fill(con, con->def_cloudcache, sizeof(con->def_cloudcache), tmp, 0, 0);
  cache_free(tmp);
  cache_add(con->myEntry, s, meta, meta_size);

  return con;
}

struct nodeID* cloudcast_get_cloud_node(struct cloudcast_proto_context *con)
{
  return get_cloud_node(con->cloud_context, 0);
}

struct nodeID** cloudcast_get_cloud_nodes(struct cloudcast_proto_context *con, uint8_t number)
{
  int i;
  struct nodeID* *cloud_nodes;

  if (number == 0) return NULL;
  cloud_nodes = calloc(number, sizeof(struct nodeID*));

  for (i=0; i< number; i++)
    cloud_nodes[i] = get_cloud_node(con->cloud_context, i);
  return cloud_nodes;
}

int cloudcast_is_cloud_node(struct cloudcast_proto_context *con, struct nodeID* node)
{
  return is_cloud_node(con->cloud_context, node);
}

int cloudcast_reply_peer(struct cloudcast_proto_context *context, const struct peer_cache *c, struct peer_cache *local_cache, uint64_t last_cloud_contact_sec)
{
  uint8_t *header = (uint8_t *) &last_cloud_contact_sec;
  int header_len = sizeof(uint64_t);
  return topo_reply_header(context->topo_context, c, local_cache, MSG_TYPE_TOPOLOGY, CLOUDCAST_REPLY, header, header_len, 0, 0);
}

int cloudcast_query_peer(struct cloudcast_proto_context *context, struct peer_cache *sent_cache, struct nodeID *dst, uint64_t last_cloud_contact_sec)
{
  uint8_t *header = (uint8_t *) &last_cloud_contact_sec;
  int header_len = sizeof(uint64_t);
  return topo_query_peer_header(context->topo_context, sent_cache, dst, MSG_TYPE_TOPOLOGY, CLOUDCAST_QUERY, header, header_len, 0);
}


int cloudcast_payload_fill(struct cloudcast_proto_context *context, uint8_t *payload, int size, struct peer_cache *c, int max_peers, int include_me)
{
  int i;
  uint8_t *p = payload;

  if (!max_peers) max_peers = MAX_MSG_SIZE; // FIXME: we should use topo_proto for this
  p += cache_header_dump(p, c, include_me);
  if (include_me) {
    p += entry_dump(p, context->myEntry, 0, size - (p - payload));
    max_peers--;
  }
  for (i = 0; nodeid(c, i) && max_peers; i++) {
    if (!is_cloud_node(context->cloud_context, nodeid(c, i))) {
      int res;
      res = entry_dump(p, c, i, size - (p - payload));
      if (res < 0) {
        fprintf(stderr, "too many entries!\n");
        return -1;
      }
      p += res;
      --max_peers;
    }
  }

  return p - payload;
}


int cloudcast_reply_cloud(struct cloudcast_proto_context *context, struct peer_cache *cloud_cache)
{
  uint8_t headerless_pkt[MAX_MSG_SIZE - CLOUDCAST_MESSAGE_HEADER];
  int len, res;

  len = cloudcast_payload_fill(context, headerless_pkt, MAX_MSG_SIZE - CLOUDCAST_MESSAGE_HEADER, cloud_cache, 0, 1);

  if (len > 0)
    res = put_on_cloud(context->cloud_context, CLOUD_VIEW_KEY, headerless_pkt, len, 0);
  else
    res = 0;
  return res;
}

int cloudcast_query_cloud(struct cloudcast_proto_context *context)
{
  return get_from_cloud_default(context->cloud_context, CLOUD_VIEW_KEY,
                                cloud_header, CLOUDCAST_MESSAGE_HEADER,
                                0, context->def_cloudcache,
                                context->def_cloudcache_len, 0);
}

struct peer_cache * cloudcast_cloud_default_reply(struct peer_cache *template, struct nodeID *cloud_entry)
{
  int size;
  struct peer_cache *cloud_reply;
  get_metadata(template, &size);
  cloud_reply = cache_init(1, size, 0);
  cache_add(cloud_reply, cloud_entry, NULL, 0);
  return cloud_reply;
}

time_t cloudcast_timestamp_cloud(struct cloudcast_proto_context *context)
{
  return timestamp_cloud(context->cloud_context);
}

int cloudcast_proto_change_metadata(struct cloudcast_proto_context *context, const void *metadata, int metadata_size)
{
  if (topo_proto_metadata_update(context->topo_context, metadata, metadata_size) <= 0) {
    return -1;
  }

  return 1;
}
