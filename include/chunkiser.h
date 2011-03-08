/** @file chunkiser.h
 *
 * @brief Split an audio/video stream in chunks.
 *
 * The chunkisation functions (chunkiser) allow to split an A/V stream in
 * chunks, and to output the chunks payload...
 * See @link chunkiser_test.c chunkiser_test.c @endlink for an usage example
 *
 */

/** @example chunkiser_test.c
 * 
 * A test program showing how to use the chunkiser and dechunkiser API.
 *
 */
 
#ifndef CHUNKISER_H
#define CHUNKISER_H

/**
 * Opaque data type representing the context for a chunkiser
 */
struct input_stream;

/**
 * Opaque data type representing the context for a de-chunkiser
 */
struct output_stream;

/**
 * @brief Initialise a chunkiser.
 * 
 * Open an A/V stream, and prepare it for reading chunks, returning the
 * chunkiser's context.
 * 
 * @param fname name of the file containing the A/V stream.
 * @param period desired input cycle size.
 * @param config configuration string.
 * @return the pointer to the chunkiser context on success, NULL on error
 */
struct input_stream *input_stream_open(const char *fname, int *period, const char *config);

/**
 * @brief Return the FDs used for input.
 *
 * Return a "-1 terminated" array of integers containing the FDs used for
 * reading an input stream. Such an array can be directly passed to wait4data()
 * as user_fds
 *
 * @param s the pointer to the chunkiser context
 * @return the array with the input FDs on success, NULL on error or if
 *         such FDs are not available
 */
const int *input_get_fds(const struct input_stream *s);

/**
 * @brief Cleanup a chunkiser.
 * 
 * Close an A/V stream, and cleanup all the data structures related to the
 * chunkiser.
 * 
 * @param c chunkiser's context.
 */
void input_stream_close(struct input_stream *c);

/**
 * @brief Read a chunk.
 * 
 * Read some data from the A/V stream, and generate a new chunk
 * 
 * @param s chunkiser's context.
 * @param c is a pointer to the chunk structure that has to be filled by the
 *        chunkiser. In particular, the chunk payload, the playload size, and
 *        the timestamp (chunk release time) will be filled.
 * @return a negative value on error, 0 if no chunk has been generated,
 *         (and chunkise() has to be invoked again), > 0 if a chunk has
 *         been succesfully generated
 */
int chunkise(struct input_stream *s, struct chunk *c);

/**
 * @brief Initialise a dechunkiser.
 * 
 * Open an A/V stream for output , and prepare it for writing chunks,
 * returning the dechunkiser's context.
 * 
 * @param fname output file name (if NULL, output goes to stdout).
 * @param config configuration string.
 * @return the pointer to the dechunkiser context on success, NULL on error
 */
struct output_stream *out_stream_init(const char *fname, const char *config);

/**
 * @brief Write a chunk.
 * 
 * Write some data (from a chunk's payload) to the A/V stream.
 * 
 * @param out dechunkiser's context.
 * @param c pointer to the chunk structure containing the data to write in the
 *        A/V stream.
 */
void chunk_write(struct output_stream *out, const struct chunk *c);

/**
 * @brief Cleanup a dechunkiser.
 * 
 * Close an A/V stream, and cleanup all the data structures related to the
 * dechunkiser.
 * 
 * @param c dechunkiser's context.
 */
void out_stream_close(struct output_stream *c);

#endif	/* CHUNKISER_H */
