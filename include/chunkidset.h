#ifndef CHUNKIDSET_H
#define CHUNKIDSET_H

typedef struct chunkID_set ChunkIDSet;

struct chunkID_set *chunkID_set_init(void);
int chunkID_set_add_chunk(struct chunkID_set *h, int chunk_id, int priority);
int chunkID_size(const struct chunkID_set *h);
int chunkID_set_get_chunk(const struct chunkID_set *h, int i, int *priority);
void chunkID_clear(struct chunkID_set *h);

#endif	/* CHUNKIDSET_H */
