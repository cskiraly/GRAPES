/** @file chunkidset.h
 *
 * @brief Structure containing a set of chunk IDs.
 *
 * The Chunk ID Set is an abstract data structure that can contain a set of
 * (chunk ID, priority) couples. Few simple operations for adding chunk IDs
 * in a set, for getting the chunk IDs present in a set, for allocating a
 * set, and for clearing a set are provided.
 * See @link chunkidset_test.c chunkidset_test.c @endlink for an usage example.
 *
 */

/** @example chunkidset_test.c
 * 
 * A test program showing how to use the Chunk ID Set API.
 *
 */
 
#ifndef CHUNKIDSET_H
#define CHUNKIDSET_H

/**
* Opaque data type representing a Chunk ID Set
*/
typedef struct chunkID_set ChunkIDSet;

#define CHUNKID_INVALID (uint32_t)-1	/**< Trying to get a chunk ID from an empty set */

 /**
  * @brief Allocate a chunk ID set.
  * 
  * Create an empty chunk ID set, and return a pointer to it.
  * 
  * @param config a configuration string containing tags which describe
  *                   the chunk ID set. For example, the "size" tag indicates
  *                   the expected number of chunk IDs that will be stored
  *                   in the set; 0 or not present if such a number is not
  *                   known.
  * @return the pointer to the new set on success, NULL on error
  */
struct chunkID_set *chunkID_set_init(const char *config);

 /**
  * @brief Add a chunk ID to the set.
  * 
  * Insert a chunk ID, and its associated priority (the priority is assumed
  * to depend on the insertion order), to the set. If the chunk
  * ID is already in the set, nothing happens.
  *
  * @param h a pointer to the set where the chunk ID has to be added
  * @param chunk_id the ID of the chunk to be inserted in the set
  * @return > 0 if the chunk ID is correctly inserted in the set, 0 if chunk_id
  *         is already in the set, < 0 on error
  */
int chunkID_set_add_chunk(struct chunkID_set *h, int chunk_id);

 /**
  * @brief Get the set size
  * 
  * Return the number of chunk IDs present in a set.
  *
  * @param h a pointer to the set
  * @return the number of chunk IDs in the set, or < 0 on error
  */
int chunkID_set_size(const struct chunkID_set *h);

 /**
  * @brief Get a chunk ID from a set
  * 
  * Return the i^th chunk ID from the set. The chunk's priority is
  * assumed to depend on i.
  *
  * @param h a pointer to the set
  * @param i the index of the chunk ID to be returned
  * @return the i^th chunk ID in the set in case of success, or < 0 on error
  *         (in case of error, priority is not meaningful)
  */
int chunkID_set_get_chunk(const struct chunkID_set *h, int i);

 /**
  * @brief Check if a chunk ID is in a set
  * 
  * @param h a pointer to the set
  * @param chunk_id the chunk ID we are searching for
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
  * @param h a pointer to the set where the chunk ID has to be added
  * @param a a pointer to the set which has to be added
  * @return > 0 if the chunk ID is correctly inserted in the set, 0 if chunk_id
  *         is already in the set, < 0 on error
  */
int chunkID_set_union(struct chunkID_set *h, struct chunkID_set *a);

 /**
  * Clear a set
  * 
  * Remove all the chunk IDs from a set.
  *
  * @param h a pointer to the set
  * @param size the expected number of chunk IDs that will be stored
  *                 in the set; 0 if such a number is not known.
  */
void chunkID_set_clear(struct chunkID_set *h, int size);

 /**
  * @brief Clear a set and free all associated memory.
  *
  * @param h a pointer to the set
  */
void chunkID_set_free(struct chunkID_set *h);

 /**
  * @brief Get the smallest chunk ID from a set
  * 
  * Return the ID of the earliest chunk from the the set.
  *
  * @param h a pointer to the set
  * @return the chunk ID in case of success, or CHUNKID_INVALID on error
  */
uint32_t chunkID_set_get_earliest(const struct chunkID_set *h);

 /**
  * @brief Get the largest chunk ID from a set
  * 
  * Return the ID of the latest chunk from the the set.
  *
  * @param h a pointer to the set
  * @return the chunk ID in case of success, or CHUNKID_INVALID on error
  */
uint32_t chunkID_set_get_latest(const struct chunkID_set *h);

void chunkID_set_trim(struct chunkID_set *h, int size);

#endif	/* CHUNKIDSET_H */
