#ifndef SCHEDULER_LA_H
#define SCHEDULER_LA_H

#include "scheduler_common.h"

/** @file scheduler_la.h
  @brief Low level scheduling functions for chunk and peer selection.

  The interface contains highly customizable selector functions. Customization is supported along the following concepts:
	-# evaluator functions: selector functions can be parameterized by evaluator functions.
		By using evaluator functions, the decision process can be decoupled into two parts:
		-# a generic selector function operating on abstract peer and chunk weights
		-# the evaluator function linking the decision process to the peer's knowledge about other peers and chunks
	-# selection policy: ChunkFirst, PeerFirst, Composed and Hybrid selection policies are supported
	-# ordering method: two kinds of ordering methods are supported:
		-# Best: strict ordering accorging to the given evaluator functions
		-# Weighted: Weighted random selection accorging to the given weight functions
	-# filter functions: selections are typically filtered by functions such as whether a given peer (according to local knowledge) needs a given chunk.
		The abstraction of the filter concept allows for easy modification of these filter conditions.
*/

/**
  * Scheduler ordering methods
  */
typedef enum {SCHED_BEST,SCHED_WEIGHTED} SchedOrdering;

/**
  * @brief Prototype for filter functions that select useful peer-chunk combinations
  * @return true if the combination is valid, false otherwise
  */
typedef int (*filterFunction)(schedPeerID ,schedChunkID );

/**
  * @brief Prototype for function assigning a weigth to a peer
  * @return the weight associated to the peer
  */
typedef double (*peerEvaluateFunction)(schedPeerID*);

/**
  * @brief Prototype for function assigning a weigth to a given chunk
  * @return the weight associated to the chunk
  */
typedef double (*chunkEvaluateFunction)(schedChunkID*);

/**
  * @brief Prototype for function assigning a weigth to a peer-chunk pair
  * @return the weight associated to the peer-chunk pair
  */
typedef double (*pairEvaluateFunction)(struct PeerChunk*);

/**
  * @brief Prototype for a two operand double function used to combine weights
  */
typedef double (*double2op)(double,double);



/**
  @brief Low level scheduler function for selecting peers.

  If called with chunks_len=0, it will not consider chunks in the selection.
  Otherwise, if chunks_len>0, only those peers will be selected that could be interesting for at least one of the given chunks.

  @param [in] ordering the ordering method to be used
  @param [in] peers list of peers to select from
  @param [in] peers_len length of peers list
  @param [in] chunk list of chunks to select from
  @param [in] chunks_len length of chunks list
  @param [out] selected list of peers selected by the function
  @param [in,out] selected_len in: maximum number of peers to select; out: number of peers selected
  @param [in] filter only peers that satisfy the filter condition can be selected
  @param [in] peerevaluate peers are selected based on the weight assigned by this evaluator function
 */
void schedSelectPeers(SchedOrdering ordering, schedPeerID  *peers, int peers_len, schedChunkID  *chunks, int chunks_len, 	//in
                     schedPeerID  *selected, int *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate);

/**
  @brief Low level scheduler function for selecting chunks.

  If called with peers_len=0, it will not consider peers in the selection.
  Otherwise, if peers_len>0, only those chunks will be selected that could be interesting for at least one of the given peers.

 */
void schedSelectChunks(SchedOrdering ordering, schedPeerID  *peers, int peers_len, schedChunkID  *chunks, int chunks_len, 	//in
                     schedChunkID  *selected, int *selected_len,	//out, inout
                     filterFunction filter,
                     chunkEvaluateFunction chunkevaluate);

/*---PeerFirst----------------*/

/**
  @brief Low level scheduler function for selecting peer-chunk pairs in chunk first order.

  First a peer is selected based on weights assigned by the peerevaluate function.
  Then, maximum selected_len chunks are selected for the given peer based on weights assigned by the chunkevaluate function.
  */
void schedSelectPeerFirst(SchedOrdering ordering, schedPeerID  *peers, size_t peers_len, schedChunkID  *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate);

/*---ChunkFirst----------------*/

/**
  * @brief Low level scheduler function for selecting peer-chunk pairs in peer first order.

  First a chunk is selected based on weights assigned by the chunkevaluate function.
  Then, maximum selected_len peers are selected for the given chunk based on weights assigned by the peerevaluate function.
  */
void schedSelectChunkFirst(SchedOrdering ordering, schedPeerID  *peers, size_t peers_len, schedChunkID  *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate);

/*---Composed----------------*/

/**
  * @brief Low level scheduler function for selecting peer-chunk pairs based on a composed evaluation function.

  A maximum of selected_len peer-chunk pairs are selected based on a composed weight function.
  @param [in] chunkevaluate function to assign a weight to each chunk
  @param [in] peerevaluate function to assign a weight to each peer
  @param [in] weightcombine operation to combine peer and chunk weight into one weight
  */
void schedSelectComposed(SchedOrdering ordering, schedPeerID  *peers, size_t peers_len, schedChunkID  *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate, double2op weightcombine);

/*---PeersForChunks----------------*/
/** 
  * @brief Added by Arpad without knowing what he is doing
   Documentation: see above

  */
void schedSelectPeersForChunks(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len,        //in
                     schedPeerID *selected, size_t *selected_len,       //out, inout
                     filterFunction filter,
                     peerEvaluateFunction evaluate);

/*---Hybrid----------------*/

/**
  * @brief Low level scheduler function for selecting peer-chunk pairs based on a hybrid evaluation function.

  A maximum of selected_len peer-chunk pairs are selected based on the given evaluation function.
  @param [in] pairevaluate function to assign a weight to each peer-chunk pair
  */
void schedSelectHybrid(SchedOrdering ordering, schedPeerID  *peers, size_t peers_len, schedChunkID  *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     pairEvaluateFunction pairevaluate);


/*---selector function----------------*/
/**
  * casted evaluator for generic use in generic selector functions
  */
typedef double (*evaluateFunction)(void*);

/**
  * Select best N of K with the given ordering method
  */
void selectWithOrdering(SchedOrdering ordering, size_t size, unsigned char *base, size_t nmemb, double(*evaluate)(void *), unsigned char *selected,size_t *selected_len);

#endif /* SCHEDULER_LA_H */
