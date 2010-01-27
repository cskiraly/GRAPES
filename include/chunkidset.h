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
  * @parameter size the expected number of chunk IDs that will be stored
  *                 in the set; 0 if such a number is not known.
  * @return the pointer to the new set on success, NULL on error
  */
struct chunkID_set *chunkID_set_init(int size);

 /**
  * Add a chunk ID to the set.
  * 
  * Insert a chunk ID, and its associated priority (the priority is assumed
  * to depend on the insertion order), to the set. If the chunk
  * ID is already in the set, nothing happens.
  *
  * @parameter h a pointer to the set where the chunk ID has to be added
  * @parameter chunk_id the ID of the chunk to be inserted in the set
  * @return > 0 if the chunk ID is correctly inserted in the set, 0 if chunk_id
  *         is already in the set, < 0 on error
  */
int chunkID_set_add_chunk(struct chunkID_set *h, int chunk_id);

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
  * Return the i^th chunk ID from the set. The chunk's priority is
  * assumed to depend on i.
  *
  * @parameter h a pointer to the set
  * @parameter i the index of the chunk ID to be returned
  * @return the i^th chunk ID in the set in case of success, or < 0 on error
  *         (in case of error, priority is not meaningful)
  */
int chunkID_set_get_chunk(const struct chunkID_set *h, int i);

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
  * Add chunks from a chunk ID set to another one
  * 
  * Insert all chunk from a chunk ID set into another one. Priority is
  * kept in the old one. New chunks from the added one are added with
  * lower priorities, but keeping their order.
  *
  * @parameter h a pointer to the set where the chunk ID has to be added
  * @parameter a a pointer to the set which has to be added
  * @return > 0 if the chunk ID is correctly inserted in the set, 0 if chunk_id
  *         is already in the set, < 0 on error
  */
int chunkID_set_union(struct chunkID_set *h, struct chunkID_set *a);

 /**
  * Clear a set
  * 
  * Remove all the chunk IDs from a set.
  *
  * @parameter h a pointer to the set
  * @parameter size the expected number of chunk IDs that will be stored
  *                 in the set; 0 if such a number is not known.
  */
void chunkID_clear(struct chunkID_set *h, int size);

#endif	/* CHUNKIDSET_H */
