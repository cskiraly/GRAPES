#ifndef NCAST_PROTO
#define NCAST_PROTO

int ncast_reply(const struct peer_cache *c, struct peer_cache *local_cache);
int ncast_query(struct peer_cache *local_cache);
int ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst);

#endif	/* NCAST_PROTO */
