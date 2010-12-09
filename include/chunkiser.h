/** @file chunkiser.h
 *
 * @brief Split an audio/video stream in chunks.
 *
 * The chunkisation functions (chunkiser) allow to split an A/V stream in
 * chunks, and to output the chunks payload...
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
 * @param config configuration string.
 * @return the pointer to the dechunkiser context on success, NULL on error
 */
struct output_stream *out_stream_init(const char *config);

/**
 * @brief Write a chunk.
 * 
 * Write some data (from a chunk's payload) to the A/V stream.
 * 
 * @param out dechunkiser's context.
 * @param id the chunk id.
 * @param data pointer to the chunk's payload.
 * @param size chunk size.
 */
void chunk_write(struct output_stream *out, const struct chunk *c);
#endif	/* CHUNKISER_H */
