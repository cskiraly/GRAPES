/** @file trade_sig_ha.h
 *
 * @brief Chunk Signaling API - Higher Abstraction (HA).
 *
 * The trade signaling HA provides a set of primitives for chunks signaling negotiation with other peers, in order to collect information for the effective chunk exchange with other peers. This is a part of the Data Exchange Protocol which provides high level abstraction for chunks' negotiations, like requesting and proposing chunks.
 *
 */


#ifndef TRADE_SIG_HA_H
#define TRADE_SIG_HA_H

#include "net_helper.h"
#include "chunkidset.h"

enum signaling_type {
  sig_offer, sig_accept, sig_request, sig_deliver, sig_send_buffermap, sig_request_buffermap
};

/**
 * @brief Set current node identifier.
 *
 * Initialize the node identifier of the peer
 *
 * @param[in] current node indentifier.
 * @return 1 on success, <0 on error.
 */
int chunkSignalingInit(struct nodeID *myID);

/**
 * @brief Parse an incoming signaling message, providing the signal type and the information of the signaling message.
 *
 * Parse an incoming signaling message provided in the buffer, giving the information of the message received.
 *
 * @param[in] buffer containing the incoming message.
 * @param[in] buff_len length of the buffer.
 * @param[out] owner_id identifier of the node on which refer the message just received.
 * @param[out] cset array of chunkIDs.
 * @param[out] max_deliver deliver at most this number of Chunks.
 * @param[out] trans_id transaction number associated with this message.
 * @param[out] sig_type Type of signaling message.
 * @return 1 on success, <0 on error.
 */
int parseSignaling(uint8_t *buff, int buff_len, struct nodeID **owner_id,
                   struct chunkID_set **cset, int *max_deliver, int *trans_id,
                   enum signaling_type *sig_type);

/**
 * @brief Request a set of chunks from a Peer.
 *
 * Request a set of Chunks towards a Peer, and specify the  maximum number of Chunks attempted to receive
 * (i.e., the number of chunks the destination peer would like to receive among those requested).
 *
 * @param[in] to target peer to request the ChunkIDs from.
 * @param[in] cset array of ChunkIDs.
 * @param[in] max_deliver deliver at most this number of Chunks.
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 on success, <0 on error.
 */
int requestChunks(struct nodeID *to, const struct chunkID_set *cset, int max_deliver, int trans_id);

/**
 * @brief Deliver a set of Chunks to a Peer as a reply of its previous request of Chunks.
 *
 * Announce to the Peer which sent a request of a set of Chunks, that we have these chunks (sub)set available
 * among all those requested and willing to deliver them.
 *
 * @param[in] to target peer to which deliver the ChunkIDs.
 * @param[in] cset array of ChunkIDs.  
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 on success, <0 on error.
 */
int deliverChunks(struct nodeID *to, struct chunkID_set *cset, int trans_id);

/**
 * @brief Offer a (sub)set of chunks to a Peer.
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
 * @brief Accept a (sub)set of chunks from a Peer.
 *
 * Announce to accept a (sub)set of Chunks from a Peer which sent a offer before, and specify the  maximum number of Chunks attempted to receive
 * (attempted to receive: i.e., the receiver peer would like to receive at most this number of Chunks from the set offered before).
 *
 * @param[in] to target peer to accept the ChunkIDs.
 * @param[in] cset array of ChunkIDs. 
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 on success, <0 on error.
 */
int acceptChunks(struct nodeID *to, struct chunkID_set *cset, int trans_id);

/**
 * @brief Send a BufferMap to a Peer.
 *
 * Send (our own or some other peer's) BufferMap to a third Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to send.
 * @param[in] bmap the BufferMap to send.
 * @param[in] cb_size the size of the chunk buffer (not the size of the buffer map sent, but that of the chunk buffer).
 * @param[in] trans_id transaction number associated with this send.
 * @return 1 Success, <0 on error.
 */
int sendBufferMap(struct nodeID *to, const struct nodeID *owner, struct chunkID_set *bmap, int cb_size, int trans_id);

/**
 * @brief Request a BufferMap to a Peer.
 *
 * Request (target peer or some other peer's) BufferMap to target Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to request.
 * @param[in] trans_id transaction number associated with this request.
 * @return 1 Success, <0 on error.
 */
int requestBufferMap(struct nodeID *to, const struct nodeID *owner, int trans_id);

#endif //TRADE_SIG_HA_H 
