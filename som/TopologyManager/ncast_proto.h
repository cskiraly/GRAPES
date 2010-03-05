int ncast_reply(const uint8_t *payload, int psize, struct peer_cache *local_cache);
int ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst);
int ncast_query(struct peer_cache *local_cache);
int ncast_proto_init(struct nodeID *s, void *meta, int meta_size);
