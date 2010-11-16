#ifndef BLIST_CACHE
#define BLIST_CACHE

struct peer_cache;
struct cache_entry;
typedef int (*ranking_function)(const void *target, const void *p1, const void *p2);	// FIXME!

struct peer_cache *blist_cache_init(int n, int metadata_size, int max_timestamp);
void blist_cache_free(struct peer_cache *c);
void blist_cache_update(struct peer_cache *c);
void blist_cache_update_tout(struct peer_cache *c);

struct nodeID *blist_nodeid(const struct peer_cache *c, int i);
const void *blist_get_metadata(const struct peer_cache *c, int *size);
int blist_cache_metadata_update(struct peer_cache *c, struct nodeID *p, const void *meta, int meta_size);
int blist_cache_add_ranked(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size, ranking_function f, const void *tmeta);
int blist_cache_add(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size);
int blist_cache_del(struct peer_cache *c, struct nodeID *neighbour);

struct nodeID *blist_rand_peer(struct peer_cache *c, void **meta, int max);

struct peer_cache *blist_entries_undump(const uint8_t *buff, int size);
int blist_cache_header_dump(uint8_t *b, const struct peer_cache *c);
int blist_entry_dump(uint8_t *b, struct peer_cache *e, int i, size_t max_write_size);

struct peer_cache *blist_merge_caches(struct peer_cache *c1, struct peer_cache *c2, int newsize, int *source);
struct peer_cache *blist_cache_rank (const struct peer_cache *c, ranking_function rank, const struct nodeID *target, const void *target_meta);
struct peer_cache *blist_cache_union(struct peer_cache *c1, struct peer_cache *c2, int *size);
int blist_cache_resize (struct peer_cache *c, int size);

#endif	/* BLIST_CACHE */
