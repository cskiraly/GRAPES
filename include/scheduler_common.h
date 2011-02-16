#ifndef SCHEDULER_COMMON_H
#define SCHEDULER_COMMON_H

/**
 * @file scheduler_common.h
 * @brief Common definitions for the scheduler.
 *
 */


#include <stdlib.h>


/**
  * @brief Peer identifier used for scheduling
  */
typedef struct peer *schedPeerID ;
/**
  * @brief Chunk identifier used for scheduling
  */
typedef int schedChunkID ;

/**
  * @brief The peer-chunk pair, the "atomic unit" of scheduling
  */
struct PeerChunk {
    schedPeerID  peer;
    schedChunkID  chunk;
};

/**
  * @brief Comodity function to convert peer and chunk lists to list of peer-chunk pairs
  */
void toPairs(schedPeerID  *peers, size_t peers_len, schedChunkID  *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *pairs, size_t *pairs_len);	//out, inout

#endif	/* SCHEDULER_COMMON_H */
