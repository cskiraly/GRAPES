#ifndef SCHEDULER_COMMON_H
#define SCHEDULER_COMMON_H

/**
 * @file scheduler_common.h
 * @brief Common definitions for the scheduler
 *
 */


#include <stdlib.h>
/**
  * peer-chunk pair, the "atomic unit" of scheduling
  */
struct PeerChunk {
    struct peer* peer;
    struct chunk* chunk;
};

/**
  * Comodity function to convert peer and chunk lists to list of peer-chunk pairs
  */
void toPairs(struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct PeerChunk *pairs, size_t *pairs_len);	//out, inout

#endif	/* SCHEDULER_COMMON_H */
