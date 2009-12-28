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
  * Clear a set
  * 
  * Remove all the chunk IDs from a set.
  *
  * @parameter h a pointer to the set
  * @parameter size the expected number of chunk IDs that will be stored
  *                 in the set; 0 if such a number is not known.
  */
void chunkID_clear(struct chunkID_set *h, int size);

 /**
  * Encode a chunk ID set, filling the buffer with the corresponding bit stream.
  * 
  * Encode a chunk ID set given as parameter and fills a buffer (given as
  * parameter) with the corresponding bit stream, which can be sent over the
  * network.
  * The main reason to encode and return the bit stream is the possibility
  * to either send directly a packet with the encoded bit stream, or 
  * add this bit stream in piggybacking
  * 
  * @param[in] h Chunk ID set to send 
  * @param[in] buff Buffer that will be filled with the bit stream obtained as a coding of the above parameters
  * @param[in] buff_len length of the buffer that will contain the bit stream
  * @return the lenght of the encoded bitstream (in bytes) on success, <0 on error
  */
int chunkID_set_encode(const struct chunkID_set *h, uint8_t *buff, int buff_len);

/**
  * Decode the bit stream contained int the buffer, transforming it in a chunk
  * ID set.
  *  
  * @param[in] buff Buffer which contain the bit stream to decode
  * @param[in] buff_len length of the buffer that contain the bit stream
  * @return a pointer to the chunk ID set on success, NULL on error
  */
struct chunkID_set *chunkID_set_decode(const uint8_t *buff, int buff_len);

#endif	/* CHUNKIDSET_H */
