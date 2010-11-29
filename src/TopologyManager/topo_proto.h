#ifndef TOPO_PROTO
#define TOPO_PROTO

struct topo_context;

struct topo_context* topo_proto_init(struct nodeID *s, void *meta, int meta_size);

int topo_reply(struct topo_context *context, const struct peer_cache *c, struct peer_cache *local_cache, int protocol, int type, int max_peers, int include_me);
int topo_query_peer(struct topo_context *context, struct peer_cache *local_cache, struct nodeID *dst, int protocol, int type, int max_peers);

int topo_proto_metadata_update(struct topo_context *context, void *meta, int meta_size);

int topo_payload_fill(struct topo_context *context, uint8_t *payload, int size, struct peer_cache *c, struct nodeID *snot, int max_peers, int include_me);
#endif	/* TOPO_PROTO */
