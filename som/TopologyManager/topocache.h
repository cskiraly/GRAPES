struct peer_cache;
struct cache_entry;
typedef int (*ranking_function)(const void *target, const void *p1, const void *p2);	// FIXME!


struct nodeID *nodeid(const struct peer_cache *c, int i);
const void *get_metadata(const struct peer_cache *c, int *size);
int cache_metadata_update(struct peer_cache *c, struct nodeID *p, const void *meta, int meta_size);
int cache_add_ranked(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size, ranking_function f, struct peer_cache *target);
int cache_add(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size);
int cache_del(struct peer_cache *c, struct nodeID *neighbour);
struct peer_cache *cache_init(int n, int metadata_size);
int fill_cache_entry(struct cache_entry *c, const struct nodeID *s);
int in_cache(const struct peer_cache *c, const struct cache_entry *elem);
struct nodeID *rand_peer(struct peer_cache *c);
struct peer_cache *entries_undump(const uint8_t *buff, int size);
int cache_header_dump(uint8_t *b, const struct peer_cache *c);
int entry_dump(uint8_t *b, struct peer_cache *e, int i);
struct peer_cache *merge_caches_ranked(struct peer_cache *c1, struct peer_cache *c2, int newsize, ranking_function rank, struct peer_cache *me);
struct peer_cache *merge_caches(struct peer_cache *c1, struct peer_cache *c2, int newsize);
void cache_update(struct peer_cache *c);
int empty(struct peer_cache *c);
void cache_free(struct peer_cache *c);
