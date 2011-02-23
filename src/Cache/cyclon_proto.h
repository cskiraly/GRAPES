#ifndef CYCLON_PROTO
#define CYCLON_PROTO

struct cyclon_proto_context;

struct cyclon_proto_context* cyclon_proto_init(struct nodeID *s, const void *meta, int meta_size);


int cyclon_reply(struct cyclon_proto_context *context, const struct peer_cache *c, const struct peer_cache *local_cache);
int cyclon_query(struct cyclon_proto_context *context, const struct peer_cache *local_cache, struct nodeID *dst);

int cyclon_proto_change_metadata(struct cyclon_proto_context *context, const void *metadata, int metadata_size);
#endif	/* CYCLON_PROTO */
