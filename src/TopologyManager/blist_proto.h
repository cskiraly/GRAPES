#ifndef BLIST_PROTO
#define BLIST_PROTO

int blist_ncast_reply(const struct peer_cache *c, struct peer_cache *local_cache);
int blist_tman_reply(const struct peer_cache *c, struct peer_cache *local_cache, int max_peers);
int blist_ncast_query(struct peer_cache *local_cache);
int blist_tman_query(struct peer_cache *local_cache);
int blist_tman_query_peer(struct peer_cache *local_cache, struct nodeID *dst, int max_peers);
int blist_ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst);
int blist_proto_metadata_update(void *meta, int meta_size);
int blist_proto_init(struct nodeID *s, void *meta, int meta_size);

#endif	/* BLIST_PROTO */
