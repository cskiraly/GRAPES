int topo_reply(const struct peer_cache *c, struct peer_cache *local_cache, int protocol, int type, int max_peers);
int topo_query_peer(struct peer_cache *local_cache, struct nodeID *dst, int protocol, int type, int max_peers);

int topo_proto_metadata_update(void *meta, int meta_size);
int topo_proto_init(struct nodeID *s, void *meta, int meta_size);
