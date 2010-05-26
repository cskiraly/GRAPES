int ncast_reply(const struct peer_cache *c, struct peer_cache *local_cache);
int tman_reply(const struct peer_cache *c, struct peer_cache *local_cache);
int ncast_query(struct peer_cache *local_cache);
int tman_query(struct peer_cache *local_cache);
int tman_query_peer(struct peer_cache *local_cache, struct nodeID *dst);
int ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst);
//int topo_query_peer(struct peer_cache *local_cache, struct nodeID *dst);
int topo_proto_metadata_update(struct nodeID *peer, void *meta, int meta_size);
int topo_proto_init(struct nodeID *s, void *meta, int meta_size);
