/** @file trade_sig_ha.h
 *
 * @brief Chunk Signaling API - Higher Abstraction (HA).
 *
 * The trade signaling HA provides a set of primitives for chunks signaling negotiation with other peers, in order to collect information for the effective chunk exchange with other peers. <br>
 * This is a part of the Data Exchange Protocol which provides high level abstraction for chunks' negotiations, like requesting and proposing chunks.
 *
 */


#ifndef TRADE_SIG_HA_H
#define TRADE_SIG_HA_H

#include "net_helper.h"
#include "chunkidset.h"

/**
 * Header used to define a signaling message.
 */
typedef struct sig_nal SigType;

/**
 * Set current node identifier.
 *
 * @param[in] current node indentifier.
 * @return 1 on success, <0 on error.
 */
int chunkSignalingInit(struct nodeID *myID);

/**
 * Request a set of chunks from a Peer.
 *
 * Request a set of Chunks towards a Peer, and specify the  maximum number of Chunks attempted to receive
 * (i.e., the number of chunks the destination peer would like to receive among those requested).
 *
 * @param[in] to target peer to request the ChunkIDs from.
 * @param[in] cset array of ChunkIDs.
 * @param[in] cset_len length of the ChunkID set.
 * @param[in] max_deliver deliver at most this number of Chunks.
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 on success, <0 on error.
 */
//int requestChunks(const struct nodeID *to, const struct chunkID_set *cset, int cset_len, int max_deliver, int trans_id);

/**
 * Deliver a set of Chunks to a Peer as a reply of its previous request of Chunks.
 *
 * Announce to the Peer which sent a request of a set of Chunks, that we have these chunks (sub)set available
 * among all those requested and willing to deliver them.
 *
 * @param[in] to target peer to which deliver the ChunkIDs.
 * @param[in] cset array of ChunkIDs.
 * @param[in] cset_len length of the ChunkID set.
 * @param[in] max_deliver we are able to deliver at most this many from the set.
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 on success, <0 on error.
 */
//int deliverChunks(const struct nodeID *to, struct chunkID_set *cset, int cset_len, int max_deliver, int trans_id);

/**
 * Offer a (sub)set of chunks to a Peer.
 *
 * Initiate a offer for a set of Chunks towards a Peer, and specify the  maximum number of Chunks attempted to deliver
 * (attempted to deliver: i.e., the sender peer should try to send at most this number of Chunks from the set).
 *
 * @param[in] to target peer to offer the ChunkIDs.
 * @param[in] cset array of ChunkIDs.
 * @param[in] max_deliver deliver at most this number of Chunks.
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 on success, <0 on error.
 */
int offerChunks(struct nodeID *to, struct chunkID_set *cset, int max_deliver, int trans_id);

/**
 * Accept a (sub)set of chunks from a Peer.
 *
 * Announce to accept a (sub)set of Chunks from a Peer which sent a offer before, and specify the  maximum number of Chunks attempted to receive
 * (attempted to receive: i.e., the receiver peer would like to receive at most this number of Chunks from the set offered before).
 *
 * @param[in] to target peer to accept the ChunkIDs.
 * @param[in] cset array of ChunkIDs. 
 * @param[in] max_deliver accept at most this number of Chunks.
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 on success, <0 on error.
 */
int acceptChunks(struct nodeID *to, struct chunkID_set *cset, int max_deliver, int trans_id);

/**
 * Send a BufferMap to a Peer.
 *
 * Send (our own or some other peer's) BufferMap to a third Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to send.
 * @param[in] bmap the BufferMap to send.
 * @param[in] bmap_len length of the buffer map.
 * @param[in] trans_id transaction number associated with this send.
 * @return 1 Success, <0 on error.
 */
int sendBufferMap(struct nodeID *to, const struct nodeID *owner, struct chunkID_set *bmap, int bmap_len, int trans_id);

/**
 * Request a BufferMap to a Peer.
 *
 * Request (target peer or some other peer's) BufferMap to target Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to request.
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 Success, <0 on error.
 */
//int requestBufferMap(const struct nodeID *to, const struct nodeID *owner, int trans_id);

#endif //TRADE_SIG_HA_H 
