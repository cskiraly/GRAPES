#ifndef CYCLON_PROTO
#define CYCLON_PROTO

int cyclon_reply(const struct peer_cache *c, struct peer_cache *local_cache);
int cyclon_query(struct peer_cache *local_cache, struct nodeID *dst);

#endif	/* CYCLON_PROTO */
