struct cache_entry;

struct nodeID *nodeid(const struct cache_entry *c, int i);
int cache_add(struct cache_entry *c, struct nodeID *neighbour);
int cache_del(struct cache_entry *c, struct nodeID *neighbour);
struct cache_entry *cache_init(int n);
int fill_cache_entry(struct cache_entry *c, const struct nodeID *s);
int in_cache(const struct cache_entry *c, const struct cache_entry *elem);
struct nodeID *rand_peer(struct cache_entry *c);
struct cache_entry *entries_undump(const uint8_t *buff, int size);
int entry_dump(uint8_t *b, struct cache_entry *e, int i);
struct cache_entry *merge_caches(const struct cache_entry *c1, const struct cache_entry *c2, int cache_size);
void cache_update(struct cache_entry *c);
int empty(struct cache_entry *c);

