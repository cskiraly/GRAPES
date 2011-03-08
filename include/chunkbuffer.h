#ifndef CHUNKBUFFER_H
#define CHUNKBUFFER_H

/** 
 * @file chunkbuffer.h
 *
 * @brief This is the chunk buffer of the peer.
 * 
 * The chunks buffer is responsible for storing the chunks received by a
 * peer. Each chunk is stored until the chunk buffer decides to discard it
 * (for example, when some kind of playout delay is expired), then it
 * is removed from the buffer. The buffer makes its chunks available to
 * the output module and to the scheduler, in form of an ordered list (chunks
 * are ordered by timestamp). Since every chunk has a timestamp and a
 * sequence number (the chunk ID), the chunk buffer's clients
 * (scheduler and output module) can easily check if there are gaps in the
 * list.
 * See @link cb_test.c cb_test.c @endlink for an usage example
 *
 */

/** @example cb_test.c
 * 
 * A test program showing how to use the chunk buffer API.
 *
 */

#define E_CB_OLD -1		/**< The chunk is too old */
#define E_CB_DUPLICATE -2	/**< The chunk is already in the buffer */

/**
 * Structure describing a chunk buffer. This is an opaque type.
 */
typedef struct chunk_buffer ChunkBuffer;


/**
 * Allocate a chunk buffer.
 *
 * Allocate and initialise a chunk buffer structure, and return a pointer to
 * it.
 *
 * @param config a text string containing some configuration parameters for
 *        the buffer, such as the playout delay and maybe some additional
 *        parameters (estimated size of the buffer, etc...)
 * @return a pointer to the allocated chunk buffer in case of success, NULL
 *         otherwise
 */
struct chunk_buffer *cb_init(const char *config);

/**
 * Add a chunk to a buffer.
 *
 * Insert a chunk in the given buffer. One or more chunks can be removed
 * from the buffer (if necessary, and according to the internal logic of
 * the chunk buffer) to create space for the new one.
 *
 * @param cb a pointer to the chunk buffer
 * @param c a pointer to the descriptor of the chunk to be inserted in the
 *        buffer
 * @return >=0 in case of success, < 0 in case of failure
 */
int cb_add_chunk(struct chunk_buffer *cb, const struct chunk *c);

/** 
 * Get the chunks from a buffer.
 *
 * Provide an (ordered) list of the chunks which are currently stored in
 * the specified chunk buffer. Such list is stored in a C arrary (so,
 * after calling chunks_array = cb_get_chunks(cb), chunks_array[i]
 * contains the i^th chunk).
 * Chunks are ordered by increasing chunk ID.
 *
 * @param cb a pointer to the chunks buffer
 * @param n a pointer to an integer variable where number of chunks
 *        will be stored
 * @return the chunks array if there are no failures and the buffer is not
 *         empty, NULL if the buffer is empty or in case of error (in case
 *         of error, n < 0; if the buffer is empty, n = 0).
 */
struct chunk *cb_get_chunks(const struct chunk_buffer *cb, int *n);

/**
 * Clear a chunk buffer
 *
 * Remove all the chunks from the specified chunk buffer.
 *
 * @param cb a pointer to the chunk buffer
 * @return >= 0 in case of success, < 0 in case of error.
 */
int cb_clear(struct chunk_buffer *cb);

/**
 * Destroy a chunk buffer
 *
 * Remove all the chunks from the specified chunk buffer,
 * and free all the memory dynamically allocated to it.
 *
 * @param cb a pointer to the chunk buffer
 */
void cb_destroy(struct chunk_buffer *cb);


/*
 * HA Functions
 */

/**
 * Get a specific chunk from a buffer
 *
 * Provide one single chunk from the specified chunkbuffer,
 * with the requested identifier.
 *
 * @param cb a pointer to the chunk buffer
 * @param id the identifier of the chunk to be returned
 * @return a pointer to the requested chunk
*/
const struct chunk *cb_get_chunk(const struct chunk_buffer *cb, int id);

#endif	/* CHUNKBUFFER_H */
