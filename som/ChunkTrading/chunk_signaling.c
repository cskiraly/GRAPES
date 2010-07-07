/*
 *  Copyright (c) 2009 Alessandro Russo.
 *
 *  This is free software;
 *  see lgpl-2.1.txt
 *
 * Chunk Signaling API - Higher Abstraction
 *
 * The Chunk Signaling HA provides a set of primitives for chunks signaling negotiation with other peers, in order to collect information for the effective chunk exchange with other peers. <br>
 * This is a part of the Data Exchange Protocol which provides high level abstraction for chunks' negotiations, like requesting and proposing chunks.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "chunk.h"
#include "msg_types.h"
#include "chunkidset.h"
#include "trade_sig_la.h"
#include "trade_sig_ha.h"

//Type of signaling message
//Request a ChunkIDSet
#define MSG_SIG_REQ 1
//Diliver a ChunkIDSet (reply to a request)
#define MSG_SIG_DEL 2
//Offer a ChunkIDSet
#define MSG_SIG_OFF 4
//Accept a ChunkIDSet (reply to an offer)
#define MSG_SIG_ACC 6
//Receive the BufferMap
#define MSG_SIG_BMOFF 10
//Request the BufferMap
#define MSG_SIG_BMREQ 12

struct sig_nal {
    uint8_t type;//type of signal.
    uint8_t max_deliver;//Max number of chunks to deliver.
    //uint16_t cb_size;//size of the buffer map to send
    uint16_t trans_id;//future use...
    uint8_t third_peer;//for buffer map exchange from other peers, just the first byte!
} __attribute__((packed));

//set the local node ID
static struct nodeID *localID;


static inline void int_cpy(uint8_t *p, int v)
{
  int tmp;

  tmp = htonl(v);
  memcpy(p, &tmp, 4);
}

static inline int int_rcpy(const uint8_t *p)
{
  int tmp;

  memcpy(&tmp, p, 4);
  tmp = ntohl(tmp);

  return tmp;
}

/**
 * @brief Set current node identifier.
 *
 * Initialize the node identifier of the peer
 *
 * @param[in] current node indentifier.
 * @return 1 on success, <0 on error.
 */
int chunkSignalingInit(struct nodeID *myID) {
    if(!myID)
        return -1;
    localID = myID;
    return 1;
}

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
int parseSignaling(uint8_t *buff, int buff_len, struct nodeID **owner_id, struct chunkID_set **cset,
        int *max_deliver, int *trans_id, int *sig_type) {
    struct sig_nal *signal;
    int ret, third_peer, meta_len;
    void *meta;

    third_peer = meta_len = 0;
    *cset = decodeChunkSignaling(&meta, &meta_len, buff, buff_len);
    if (meta_len) {
        signal = (struct sig_nal *) meta;
        switch (signal->type) {
            case MSG_SIG_OFF:
                *sig_type = sig_offer;
                break;
            case MSG_SIG_ACC:
                *sig_type = sig_accept;
                break;
            case MSG_SIG_REQ:
                *sig_type = sig_request;
                break;
            case MSG_SIG_DEL:
                *sig_type = sig_deliver;
                break;
            case MSG_SIG_BMOFF:
                *sig_type = sig_send_buffermap;
                break;
            case MSG_SIG_BMREQ:
                *sig_type = sig_request_buffermap;
                break;
            default:
                fprintf(stderr, "Error invalid signaling message\n");                
                return -1;
        }
        *max_deliver = signal->max_deliver;
        *trans_id = signal->trans_id;
        *owner_id = (signal->third_peer ? nodeid_undump(&(signal->third_peer), &third_peer) : NULL);
        free(meta);
    }
    ret = (!*cset || !meta_len) ? -1 : 1;
    if (ret < 0) {
        fprintf(stderr, "Error parsing invalid signaling message\n");
    }
    return ret;
}


static int sendSignaling(int type, struct nodeID *to_id, const struct nodeID *owner_id, const struct chunkID_set *cset, int max_deliver, int trans_id)
{
    int buff_len, meta_len, msg_len, ret;
    uint8_t *buff;
    struct sig_nal *sigmex;
    uint8_t *meta;
    ret = 1;
    meta = malloc(1024);
    if (!meta) {
        fprintf(stderr, "Error allocating meta-buffer\n");
        return -1;
    }
    sigmex = (struct sig_nal*) meta;
    sigmex->type = type;
    sigmex->max_deliver = max_deliver;    
    sigmex->trans_id = trans_id;
    sigmex->third_peer = 0;
    meta_len = sizeof(*sigmex) - 1;
    if (owner_id) {
        meta_len += nodeid_dump(&sigmex->third_peer, owner_id);
    }
    buff_len = 1 + (cset ? chunkID_set_size(cset):0) * 4 + 12 + meta_len; // this should be enough
    buff = malloc(buff_len);
    if (!buff) {
        fprintf(stderr, "Error allocating buffer\n");
        free(meta);
        return -1;
    }

    buff[0] = MSG_TYPE_SIGNALLING;
    msg_len = 1 + encodeChunkSignaling(cset, meta, meta_len, buff+1, buff_len-1);
    free(meta);
    if (msg_len <= 0) {
      fprintf(stderr, "Error in encoding chunk set for sending a buffermap\n");
      ret = -1;
    } else {
      send_to_peer(localID, to_id, buff, msg_len);
    }    
    free(buff);
    return ret;
}

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
int requestChunks(struct nodeID *to, const ChunkIDSet *cset, int max_deliver, int trans_id) {
    return sendSignaling(MSG_SIG_REQ, to, NULL, cset, max_deliver, trans_id);
}


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
int deliverChunks(struct nodeID *to, ChunkIDSet *cset, int trans_id) {
    return sendSignaling(MSG_SIG_DEL, to, NULL, cset, 0, trans_id);
}

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
int offerChunks(struct nodeID *to, struct chunkID_set *cset, int max_deliver, int trans_id){
  return sendSignaling(MSG_SIG_OFF, to, NULL, cset, max_deliver, trans_id);
}

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
int acceptChunks(struct nodeID *to, struct chunkID_set *cset, int trans_id) {
  return sendSignaling(MSG_SIG_ACC, to, NULL, cset, 0, trans_id);
}

/**
 * @brief Send a BufferMap to a Peer.
 *
 * Send (our own or some other peer's) BufferMap to a third Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to send.
 * @param[in] bmap the BufferMap to send.
 * @param[in] trans_id transaction number associated with this send.
 * @return 1 Success, <0 on error.
 */

int sendBufferMap(struct nodeID *to, const struct nodeID *owner, struct chunkID_set *bmap, int trans_id) {
  return sendSignaling(MSG_SIG_BMOFF, to, (!owner?localID:owner), bmap, 0, trans_id);
}

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

int requestBufferMap(struct nodeID *to, const struct nodeID *owner, int trans_id) {
    return sendSignaling(MSG_SIG_BMREQ, to, (!owner?localID:owner), NULL, 0, trans_id);
}
