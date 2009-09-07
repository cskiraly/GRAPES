/** @file chunkidset.h
 *
 * Chunk ID Set
 *
 * The Chunk ID Set is an abstract data structure that can contain a set of
 * (chunk ID, priority) couples. Few simple operations for adding chunk IDs
 * in a set, for getting the chunk IDs present in a set, for allocating a
 * set, and for clearing a set are provided.
 */
 
#ifndef CHUNKIDSET_H
#define CHUNKIDSET_H

typedef struct chunkID_set ChunkIDSet;

 /**
  * Allocate a chunk ID set.
  * 
  * Create an empty chunk ID set, and return a pointer to it.
  * 
  * @return the pointer to the new set on success, NULL on error
  */
struct chunkID_set *chunkID_set_init(void);

 /**
  * Add a chunk ID to the set.
  * 
  * Insert a chunk ID, and its associated priority, to the set. If the chunk
  * ID is already in the set, nothing happens.
  *
  * @parameter h a pointer to the set where the chunk ID has to be added
  * @parameter chunk_id the ID of the chunk to be inserted in the set
  * @parameter priority the priority associated to chunk_id 
  * @return > 0 if the chunk ID is correctly inserted in the set, 0 if chunk_id
  *         is already in the set, < 0 on error
  */
int chunkID_set_add_chunk(struct chunkID_set *h, int chunk_id, int priority);

 /**
  * Get the set size
  * 
  * Return the number of chunk IDs present in a set.
  *
  * @parameter h a pointer to the set
  * @return the number of chunk IDs in the set, or < 0 on error
  */
int chunkID_size(const struct chunkID_set *h);

 /**
  * Get a chunk ID from a set
  * 
  * Return the i^th chunk ID from the set, with its associated priority.
  *
  * @parameter h a pointer to the set
  * @parameter i the index of the chunk ID to be returned
  * @parameter priority a pointer to an integer where the priority associated
  *            to the chunk ID is stored. If this parameter is NULL, the
  *            priority is not returned
  * @return the i^th chunk ID in the set in case of success, or < 0 on error
  *         (in case of error, priority is not meaningful)
  */
int chunkID_set_get_chunk(const struct chunkID_set *h, int i, int *priority);

 /**
  * Check if a chunk ID is in a set
  * 
  * @parameter h a pointer to the set
  * @parameter chunk_id the chunk ID we are searching for
  * @return the priority of the chunk ID if it is present in the set,
  *         < 0 on error or if the chunk ID is not in the set
  */
int chunkID_set_check(const struct chunkID_set *h, int chunk_id);

 /**
  * Clear a set
  * 
  * Remove all the chunk IDs from a set.
  *
  * @parameter h a pointer to the set
  */
void chunkID_clear(struct chunkID_set *h);

#endif	/* CHUNKIDSET_H */
