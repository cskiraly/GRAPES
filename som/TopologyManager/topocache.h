struct peer_cache;
struct cache_entry;
typedef int (*ranking_function)(const void *target, const void *p1, const void *p2);	// FIXME!

struct peer_cache *cache_init(int n, int metadata_size);
void cache_free(struct peer_cache *c);
void cache_update(struct peer_cache *c);

struct nodeID *nodeid(const struct peer_cache *c, int i);
const void *get_metadata(const struct peer_cache *c, int *size);
int cache_metadata_update(struct peer_cache *c, struct nodeID *p, const void *meta, int meta_size);
int cache_add_ranked(struct peer_cache *c, const struct nodeID *neighbour, const void *meta, int meta_size, ranking_function f, const void *tmeta);
int cache_add(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size);
int cache_del(struct peer_cache *c, struct nodeID *neighbour);

struct nodeID *rand_peer(struct peer_cache *c, void **meta);

struct peer_cache *entries_undump(const uint8_t *buff, int size);
int cache_header_dump(uint8_t *b, const struct peer_cache *c);
int entry_dump(uint8_t *b, struct peer_cache *e, int i);

struct peer_cache *merge_caches_ranked(struct peer_cache *c1, struct peer_cache *c2, int newsize, int *source, ranking_function rank, void *mymeta);
struct peer_cache *merge_caches(struct peer_cache *c1, struct peer_cache *c2, int newsize, int *source);
