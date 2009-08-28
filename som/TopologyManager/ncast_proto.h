int ncast_payload_fill(uint8_t *payload, int size, struct cache_entry *c, struct socketID *snot);
int ncast_reply(const struct connectionID *conn, const uint8_t *payload, int psize, struct cache_entry *local_cache);
int ncast_query_peer(struct cache_entry *local_cache, struct socketID *dst);
int ncast_query(struct cache_entry *local_cache);
int ncast_proto_init(struct socketID *s);
