int ncast_reply(const struct peer_cache *c, struct peer_cache *local_cache);
int tman_reply(const struct peer_cache *c, struct peer_cache *local_cache, int max_peers);
int ncast_query(struct peer_cache *local_cache);
int tman_query(struct peer_cache *local_cache);
int tman_query_peer(struct peer_cache *local_cache, struct nodeID *dst, int max_peers);
int ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst);
int topo_proto_metadata_update(void *meta, int meta_size);
int topo_proto_init(struct nodeID *s, void *meta, int meta_size);
