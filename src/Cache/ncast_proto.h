#ifndef NCAST_PROTO
#define NCAST_PROTO

struct ncast_proto_context;

struct ncast_proto_context* ncast_proto_init(struct nodeID *s, const void *meta, int meta_size);

int ncast_reply(struct ncast_proto_context *context, const struct peer_cache *c, const struct peer_cache *local_cache);
int ncast_query(struct ncast_proto_context *context, const struct peer_cache *local_cache);
int ncast_query_peer(struct ncast_proto_context *context, const struct peer_cache *local_cache, struct nodeID *dst);
int ncast_proto_metadata_update(struct ncast_proto_context *context, const void *meta, int meta_size);
int ncast_proto_myentry_update(struct ncast_proto_context *context, struct nodeID *s, int dts, const void *meta, int meta_size);

#endif	/* NCAST_PROTO */
