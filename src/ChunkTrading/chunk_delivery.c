/*
 *  Copyright (c) 2009 Alessandro Russo.
 *
 *  This is free software;
 *  see lgpl-2.1.txt
 */
#include <stdlib.h>
#include <stdint.h>

#include "int_coding.h"
#include "chunk.h"
#include "net_helper.h"
#include "trade_msg_la.h"
#include "trade_msg_ha.h"
#include "grapes_msg_types.h"

static struct nodeID *localID;

int parseChunkMsg(const uint8_t *buff, int buff_len, struct chunk *c, uint16_t *transid)
{
  int res;

  if (c == NULL) {
    return -1;
  }

  res = decodeChunk(c, buff + sizeof(*transid), buff_len - sizeof(*transid));
  if (res < 0) {
    return -1;
  }

  *transid = int16_rcpy(buff);

  return 1;
}

/**
 * Send a Chunk to a target Peer
 *
 * Send a single Chunk to a given Peer
 *
 * @param[in] to destination peer
 * @param[in] c Chunk to send
 * @return 0 on success, <0 on error
 */
//TO CHECK AND CORRECT
//XXX Send data is in char while our buffer is in uint8
int sendChunk(struct nodeID *to, const struct chunk *c, uint16_t transid)
{
  int buff_len;
  uint8_t *buff;
  int res;

  buff_len  = CHUNK_HEADER_SIZE + sizeof(transid) + c->size + c->attributes_size;
  buff = malloc(buff_len + 1);
  if (buff == NULL) {
      return -1;
  }
  buff[0] = MSG_TYPE_CHUNK;
  int16_cpy(buff + 1, transid);
  res = encodeChunk(c, buff + 1 + sizeof(transid), buff_len);
  send_to_peer(localID, to, buff, buff_len + 1);
  free(buff);

  return EXIT_SUCCESS;
}

int chunkDeliveryInit(struct nodeID *myID)
{
  localID = myID;

  return 1;
}

