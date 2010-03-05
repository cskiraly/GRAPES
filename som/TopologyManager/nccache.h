struct peer_cache;
struct cache_entry;

struct nodeID *nodeid(const struct peer_cache *c, int i);
int cache_add(struct peer_cache *c, struct nodeID *neighbour);
int cache_del(struct peer_cache *c, struct nodeID *neighbour);
struct peer_cache *cache_init(int n);
int fill_cache_entry(struct cache_entry *c, const struct nodeID *s);
int in_cache(const struct peer_cache *c, const struct cache_entry *elem);
struct nodeID *rand_peer(struct peer_cache *c);
struct peer_cache *entries_undump(const uint8_t *buff, int size);
int cache_header_dump(uint8_t *b, const struct peer_cache *c);
int entry_dump(uint8_t *b, struct peer_cache *e, int i);
struct peer_cache *merge_caches(struct peer_cache *c1, struct peer_cache *c2);
void cache_update(struct peer_cache *c);
int empty(struct peer_cache *c);
void cache_free(struct peer_cache *c);
