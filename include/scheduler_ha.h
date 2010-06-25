#ifndef SCHEDULER_LA_H
#define SCHEDULER_LA_H

#include "scheduler_common.h"

/** @file scheduler_ha.h

  @brief Scheduling functions for chunk and peer selection.

  The chunk & peer scheduler (or simply ``scheduler'') is
  responsible for selecting a list of peer-chunk pairs whenever chunks or trade
  messages are to be sent. It can e.g. select destination peers and chunks to be sent
  when push streaming is used, or the peer to request from and the list of chunks
  to ask for when pull mode operation is required.
  Note that in this design the scheduler has nothing to do with the timing (it
  does not decide when to send chunks, but only decides which chunks
  to send/reqest/offer/propose/accept and the destination/source peer).
  Hence, the scheduler is non-blocking and has to be invoked at the correct time.

  The scheduler uses:
	- information about the neighbourhood and its state: hence, it
   directly communicates with the NeighborhoodList;
	- information about the chunks currently contained in the chunk
   buffer: hence, it directly interacts with the chunk buffer.

*/

/**
  Initialize the scheduler

  @param[in] cfg configuration string
*/
int schedInit(char *cfg);

/**
  Select a list of peer-chunk pairs for sending.

  In a simple push model, chunks are sent to peers without further messaging. This selection function selects the chunk-peer
  pairs for this simple pull operation.

  @param[in] peers list of peers to choose from.
  @param[in] peers_len length of the peers list
  @param[in] chunks list of chunks to choose from.
  @param[in] chunks_len length of the chunks list
  @param[out] selected ordered list of peer-chunk pairs selected
  @param[in,out] selected_len in: maximum number of selectable pairs, also defines the allocated space for selected. out: length of the selected list
*/
void schedSelectPushList(schedPeerID  *peers, int peers_len, schedChunkID  *chunks, int chunks_len, 	//in
                     struct PeerChunk *selected, int *selected_len);	//out, inout

/**
  Select what chunks to request from which peers.

  In a pull model, chunk requests shuold be explicitly signaled. This selection function serves to select the peer-chunk combinations for such
  requests. E.g. one well known implementation of this selection creates a partitioning of the missing chunk set and assigns a peer to each of
  these subsets.

  @param[in] peers list of peers to choose from.
  @param[in] peers_len length of the peers list
  @param[in] chunks list of chunks to choose from.
  @param[in] chunks_len length of the chunks list
  @param[out] selected ordered list of peer-chunk pairs selected
  @param[in,out] selected_len in: maximum number of selectable pairs, also defines the allocated space for selected. out: length of the selected list
*/
void schedSelectRequestList(schedPeerID  *peers, int peers_len, schedChunkID  *chunks, int chunks_len, 	//in
                     struct PeerChunk *selected, int *selected_len);	//out, inout


/**
  Select what to offer based on the requests that arrived.

  In a simple pull model, all requests are served, therefore no selection is needed on the  sender side. In a more sophisicated model, the sender side should
  evaluate requests that have arrived and select the ones that are satisfied. This selection funciton is to 

  @param[in] peers list of peers to choose from.
  @param[in] peers_len length of the peers list
  @param[in] chunks list of chunks to choose from.
  @param[in] chunks_len length of the chunks list
  @param[out] selected ordered list of peer-chunk pairs selected
  @param[in,out] selected_len in: maximum number of OCselectable pairs, also defines the allocated space for selected. out: length of the selected list
*/
void schedSelectOfferList(schedPeerID  *peers, int peers_len, schedChunkID  *chunks, int chunks_len, 	//in
                     struct PeerChunk *selected, int *selected_len);	//out, inout

/**
  Select chunks to propose to other peers.

  This selection function decides which chunks to propose to which peers. In this case first a decision is made at the peer initiating the transaction about
  the set of peers it is willing to send to the other peer. 

  @param[in] peers list of peers to choose from.
  @param[in] peers_len length of the peers list
  @param[in] chunks list of chunks to choose from.
  @param[in] chunks_len length of the chunks list
  @param[out] selected ordered list of peer-chunk pairs selected
  @param[in,out] selected_len in: maximum number of selectable pairs, also defines the allocated space for selected. out: length of the selected list
*/
void schedSelectProposeList(schedPeerID  *peers, int peers_len, schedChunkID  *chunks, int chunks_len, 	//in
                     struct PeerChunk *selected, int *selected_len);	//out, inout

/**
  Select chunks to accept based on proposals.

  This selection function which proposed chunks to ask for.

  @param[in] peers list of peers to choose from.
  @param[in] peers_len length of the peers list
  @param[in] chunks list of chunks to choose from.
  @param[in] chunks_len length of the chunks list
  @param[out] selected ordered list of peer-chunk pairs selected
  @param[in,out] selected_len in: maximum number of selectable pairs, also defines the allocated space for selected. out: length of the selected list
*/
void schedSelectAcceptList(schedPeerID  *peers, int peers_len, schedChunkID  *chunks, int chunks_len, 	//in
                     struct PeerChunk *selected, int *selected_len);	//out, inout

#endif /* SCHEDULER_LA_H */
