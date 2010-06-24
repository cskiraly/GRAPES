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
#include "chunk.h"
#include "chunkidset.h"
#include "msg_types.h"
#include "trade_sig_la.h"
#include "trade_sig_ha.h"

//Type of signaling message
//Request a ChunkIDSet
#define MSG_SIG_REQ 0
//Diliver a ChunkIDSet (reply to a request)
#define MSG_SIG_DEL 1
//Offer a ChunkIDSet
#define MSG_SIG_OFF 2
//Accept a ChunkIDSet (reply to an offer)
#define MSG_SIG_ACC 3
//Receive the BufferMap
#define MSG_SIG_BMOFF 4
//Request the BufferMap
#define MSG_SIG_BMREQ 5

struct sig_nal {
    uint8_t type;//type of signal.
    uint8_t max_deliver;//Max number of chunks to deliver.
    uint16_t cb_size;
    uint16_t trans_id;//future use...
    uint8_t third_peer;//for buffer map exchange from other peers, just the first byte!
} __attribute__((packed));

//set the local node ID
static struct nodeID *localID;

int chunkSignalingInit(struct nodeID *myID) {
    if(!myID)
        return -1;
    localID = myID;
    return 1;
}

int sendSignalling(int type, struct nodeID *to_id, const struct nodeID *owner_id, struct chunkID_set *cset, int max_deliver, int cb_size, int trans_id)
{
    int buff_len, meta_len, msg_len, ret;
    uint8_t *buff;
    struct sig_nal *sigmex;
    uint8_t *meta;
    ret = 1;
    meta = malloc(1024);

    sigmex = (struct sig_nal*) meta;
    sigmex->type = type;
    sigmex->max_deliver = max_deliver;
    sigmex->cb_size = cb_size;
    sigmex->trans_id = trans_id;
    meta_len = sizeof(*sigmex)-1;
      sigmex->third_peer = 0;
    if (owner_id) {
      meta_len += nodeid_dump(&sigmex->third_peer, owner_id);
    }

    buff_len = 1 + chunkID_set_size(cset) * 4 + 16 + meta_len; // this should be enough
    buff = malloc(buff_len);
    if (!buff) {
      fprintf(stderr, "Error allocating buffer\n");
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
 * Request a (sub)set of chunks from a Peer.
 *
 * Initiate a request for a set of Chunks towards a Peer, and specify the  maximum number of Chunks attempted to deliver
 * (attempted to deliver: i.e. the destination peer would like to receive at most this number of Chunks from the set)
 *
 * @param[in] to target peer to request the ChunkIDs from
 * @param[in] cset array of ChunkIDs
 * @param[in] cset_len length of the ChunkID set
 * @param[in] max_deliver deliver at most this number of Chunks
 * @param[in] trans_id transaction number associated with this request
 * @return 1 on success, 0 no chunks, <0 on error,
 *//*
int requestChunks(const struct nodeID *to, const ChunkIDSet *cset, int cset_len, int max_deliver, int trans_id) {
    int buff_len;
    int res;
    uint8_t *buff;
    struct sig_nal *sigmex;
    sigmex = malloc(sizeof(struct sig_nal));
    sigmex->type = MSG_SIG_REQ;
    sigmex->max_deliver = max_deliver;
    sigmex->trans_id = trans_id;
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d PP %d\n", sigmex->type, sigmex->trans_id, sigmex->third_peer);
    buff_len = cset_len * 4 + 12 + sizeof(struct sig_nal); // Signaling type + set len
    buff = malloc(buff_len + 1);    
    res = encodeChunkSignaling(cset, sigmex, sizeof(struct sig_nal), buff+1, buff_len);
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for chunks request\n");
        return -1;
    } else {
        buff[0] = MSG_TYPE_SIGNALLING; //the message type is CHUNK
        res = send_to_peer(localID, to, buff, buff_len + 1); //actual send chunk
        free(buff); //free memory allocated for the buffer
        free(sigmex);
    }
    return (1);
}

*/
/**
 * Deliver a set of Chunks to a Peer as a reply of its previous request of Chunks
 *
 * Announce to the Peer which sent a request of a set of Chunks, that we have these chunks (sub)set available
 * among all those requested and willing to deliver them.
 *
 * @param[in] to target peer to which deliver the ChunkIDs
 * @param[in] cset array of ChunkIDs
 * @param[in] cset_len length of the ChunkID set
 * @param[in] max_deliver we are able to deliver at most this many from the set
 * @param[in] trans_id transaction number associated with this request
 * @return 1 on success, <0 on error
 *//*

int deliverChunks(const struct nodeID *to, ChunkIDSet *cset, int cset_len, int max_deliver, int trans_id) {
    int buff_len;
    int res;
    uint8_t *buff;
    struct sig_nal *sigmex;
    sigmex = malloc(sizeof(struct sig_nal));
    sigmex->type = MSG_SIG_DEL;
    sigmex->max_deliver = max_deliver;
    sigmex->trans_id = trans_id;
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d PP %d\n", sigmex->type, sigmex->trans_id, sigmex->third_peer);
    buff_len = cset_len * 4 + 12 + sizeof(struct sig_nal); // Signaling type + set len
    buff = malloc(buff_len + 1);
    res = encodeChunkSignaling(cset, sigmex, sizeof(struct sig_nal), buff+1, buff_len);
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for chunks request\n");
        return -1;
    } else {
        buff[0] = MSG_TYPE_SIGNALLING; //the message type is CHUNK
        res = send_to_peer(localID, to, buff, buff_len + 1); //actual send chunk
        free(buff); //free memory allocated for the buffer
        free(sigmex);
    }
    return 1;
}
*/

/**
 * Offer a (sub)set of chunks to a Peer.
 *
 * Initiate a offer for a set of Chunks towards a Peer, and specify the  maximum number of Chunks attempted to deliver
 * (attempted to deliver: i.e. the sender peer should try to send at most this number of Chunks from the set)
 *
 * @param[in] to target peer to offer the ChunkIDs
 * @param[in] cset array of ChunkIDs
 * @param[in] cset_len length of the ChunkID set
 * @param[in] max_deliver deliver at most this number of Chunks
 * @param[in] trans_id transaction number associated with this request
 * @return 1 on success, <0 on error
 */

int offerChunks(struct nodeID *to, struct chunkID_set *cset, int max_deliver, int trans_id)
{
  return sendSignalling(MSG_SIG_OFF, to, NULL, cset, max_deliver, -1, trans_id);
}

/**
 * Accept a (sub)set of chunks from a Peer.
 *
 * Announce to accept a (sub)set of Chunks from a Peer which sent a offer before, and specify the  maximum number of Chunks attempted to receive
 * (attempted to receive: i.e. the receiver peer would like to receive at most this number of Chunks from the set offerd before)
 *
 * @param[in] to target peer to accept the ChunkIDs
 * @param[in] cset array of ChunkIDs
 * @param[in] max_deliver accept at most this number of Chunks
 * @param[in] trans_id transaction number associated with this request
 * @return 1 on success, <0 on error
 */
int acceptChunks(struct nodeID *to, struct chunkID_set *cset, int max_deliver, int trans_id) {
  return sendSignalling(MSG_SIG_ACC, to, NULL, cset, max_deliver, -1, trans_id);
}

/**
 * Send a BufferMap to a Peer.
 *
 * Send (our own or some other peer's) BufferMap to a third Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to send.
 * @param[in] bmap the BufferMap to send.
 * @param[in] bmap_len length of the buffer map
 * @param[in] trans_id transaction number associated with this send
 * @return 1 on success, <0 on error
 */
int sendBufferMap(struct nodeID *to, const struct nodeID *owner_id, struct chunkID_set *bmap, int bmap_len, int trans_id) {
  return sendSignalling(MSG_SIG_BMOFF, to, (!owner_id?localID:owner_id), bmap, 0, bmap_len, trans_id);//owner NULL -> my buffermap
}


/**
 * Request a BufferMap to a Peer.
 *
 * Request (target peer or some other peer's) BufferMap to target Peer.
 *
 * @param[in] to PeerID.
 * @param[in] owner Owner of the BufferMap to request.
 * @param[in] trans_id transaction number associated with this request
 * @return 1 on success, <0 on error
 */
//TODO Receive would require a buffer map from a given chunk, with a given length
/*
int requestBufferMap(const struct nodeID *to, const struct nodeID *owner, int trans_id) {
    int buff_len;
    int res;
    uint8_t *buff;
    struct sig_nal *sigmex;
    sigmex = malloc(sizeof(struct sig_nal));
    sigmex->type = MSG_SIG_BMREQ;
    sigmex->trans_id = trans_id;         
    //sigmex->third_peer->fd = owner->fd ;
    //sigmex->third_peer->refcnt = owner->refcnt ;
    //socklen_t raddr_size = sizeof(struct sockaddr_in);
    //memcpy(&sigmex->third_peer->addr,&owner->addr, raddr_size);
     
    fprintf(stdout, "SIG_HEADER: Type %d Tx %d\n", sigmex->type, sigmex->trans_id);
    buff_len = 12 + sizeof(struct sig_nal);
    buff = malloc(buff_len + 1);
    ChunkIDSet *bmap;
    bmap = chunkID_set_init(0);
    res = encodeChunkSignaling(bmap, sigmex, sizeof(struct sig_nal), buff, buff_len);
    if (res < 0) {
        fprintf(stderr, "Error in encoding chunk set for chunks request\n");
        return -1;
    } else {
        buff[0] = MSG_TYPE_SIGNALLING; //the message type is CHUNK
        res = send_to_peer(localID, to, buff, buff_len + 1); //actual send chunk
        free(buff); //free memory allocated for the buffer
        free(sigmex);
    }
    return 1;
}*/
