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
#include "grapes_msg_types.h"
#include "chunkidset.h"
#include "trade_sig_la.h"
#include "trade_sig_ha.h"
#include "int_coding.h"

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

#define SIG_META_LEN 1024
#define SIG_BUF_LEN 2048

struct sig_nal {
  uint8_t type;//type of signal.
  uint8_t max_deliver;//Max number of chunks to deliver.
  //uint16_t cb_size;//size of the buffer map to send
  uint16_t trans_id;//future use...
  uint8_t third_peer;//for buffer map exchange from other peers, just the first byte!
} __attribute__((packed));

//set the local node ID
static struct nodeID *localID;

int chunkSignalingInit(struct nodeID *myID)
{
  if(!myID)
      return -1;
  localID = myID;

  return 1;
}

int parseSignaling(uint8_t *buff, int buff_len, struct nodeID **owner_id,
                   struct chunkID_set **cset, int *max_deliver, int *trans_id,
                   enum signaling_type *sig_type)
{
  int meta_len = 0;
  void *meta;

  *cset = decodeChunkSignaling(&meta, &meta_len, buff, buff_len);
  if (meta_len) {
    struct sig_nal *signal = meta;
    int dummy;

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
        fprintf(stderr, "Error invalid signaling message: type %d\n", signal->type);
        return -1;
    }
    *max_deliver = signal->max_deliver;
    *trans_id = signal->trans_id;
    *owner_id = (meta_len > sizeof(struct sig_nal) - 1 ? nodeid_undump(&(signal->third_peer), &dummy) : NULL);
    free(meta);
  } else {
    return -1;
  }

  return 1;
}

static int sendSignaling(int type, struct nodeID *to_id,
                         const struct nodeID *owner_id,
                         const struct chunkID_set *cset, int max_deliver,
                         int trans_id)
{
  int meta_len, msg_len;
  uint8_t *buff;
  struct sig_nal *sigmex;

  sigmex = malloc(SIG_META_LEN);
  if (!sigmex) {
    fprintf(stderr, "Error allocating meta-buffer\n");

    return -1;
  }
  sigmex->type = type;
  sigmex->max_deliver = max_deliver;    
  sigmex->trans_id = trans_id;
  sigmex->third_peer = 0;
  meta_len = sizeof(*sigmex) - 1;
  if (owner_id) {
    meta_len += nodeid_dump(&sigmex->third_peer, owner_id, SIG_META_LEN - meta_len);
  }
  buff = malloc(SIG_BUF_LEN);
  if (!buff) {
    fprintf(stderr, "Error allocating buffer\n");
    free(sigmex);

    return -1;
  }

  buff[0] = MSG_TYPE_SIGNALLING;
  msg_len = 1 + encodeChunkSignaling(cset, sigmex, meta_len, buff+1, SIG_BUF_LEN-1);
  free(sigmex);
  if (msg_len <= 0) {
    fprintf(stderr, "Error in encoding chunk set for sending a buffermap\n");
    free(buff);

    return -1;
  } else {
    send_to_peer(localID, to_id, buff, msg_len);
  }    
  free(buff);

  return 1;
}

int requestChunks(struct nodeID *to, const ChunkIDSet *cset,
                  int max_deliver, int trans_id)
{
  return sendSignaling(MSG_SIG_REQ, to, NULL, cset, max_deliver, trans_id);
}

int deliverChunks(struct nodeID *to, ChunkIDSet *cset, int trans_id)
{
  return sendSignaling(MSG_SIG_DEL, to, NULL, cset, 0, trans_id);
}

int offerChunks(struct nodeID *to, struct chunkID_set *cset,
                int max_deliver, int trans_id)
{
  return sendSignaling(MSG_SIG_OFF, to, NULL, cset, max_deliver, trans_id);
}

int acceptChunks(struct nodeID *to, struct chunkID_set *cset, int trans_id)
{
  return sendSignaling(MSG_SIG_ACC, to, NULL, cset, 0, trans_id);
}

int sendBufferMap(struct nodeID *to, const struct nodeID *owner,
                  struct chunkID_set *bmap, int cb_size, int trans_id)
{
  return sendSignaling(MSG_SIG_BMOFF, to, (!owner ? localID : owner), bmap,
                       cb_size, trans_id);
}

int requestBufferMap(struct nodeID *to, const struct nodeID *owner,
                     int trans_id)
{
  return sendSignaling(MSG_SIG_BMREQ, to, (!owner?localID:owner), NULL,
                       0, trans_id);
}
