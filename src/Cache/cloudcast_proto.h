#ifndef CLOUDCAST_PROTO
#define CLOUDCAST_PROTO

#include <time.h>
#include "cloud_helper.h"

struct cloudcast_proto_context;

struct cloudcast_header {
  uint64_t last_cloud_contact_sec;
};

struct cloudcast_proto_context* cloudcast_proto_init(struct nodeID *s, const void *meta, int meta_size);

int cloudcast_reply_peer(struct cloudcast_proto_context *context, const struct peer_cache *c, struct peer_cache *local_cache, uint64_t last_cloud_contact_sec);
int cloudcast_query_peer(struct cloudcast_proto_context *context, struct peer_cache *local_cache, struct nodeID *dst, uint64_t last_cloud_contact_sec);

int cloudcast_reply_cloud(struct cloudcast_proto_context *context, struct peer_cache *cloud_cache);

struct peer_cache * cloudcast_cloud_default_reply(struct peer_cache *template, struct nodeID *cloud_entry);

int cloudcast_query_cloud(struct cloudcast_proto_context *context);
time_t cloudcast_timestamp_cloud(struct cloudcast_proto_context *context);

int cloudcast_proto_change_metadata(struct cloudcast_proto_context *context, const void *metadata, int metadata_size);

struct nodeID** cloudcast_get_cloud_nodes(struct cloudcast_proto_context *context, uint8_t number);

struct nodeID* cloudcast_get_cloud_node(struct cloudcast_proto_context *context);

int cloudcast_is_cloud_node(struct cloudcast_proto_context *context, struct nodeID* node);
#endif  /* CLOUDCAST_PROTO */
