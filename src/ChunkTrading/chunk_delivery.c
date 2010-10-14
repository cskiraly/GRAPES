/*
 *  Copyright (c) 2009 Alessandro Russo.
 *
 *  This is free software;
 *  see lgpl-2.1.txt
 */
#include <stdlib.h>
#include <stdint.h>

#include "chunk.h"
#include "net_helper.h"
#include "trade_msg_la.h"
#include "trade_msg_ha.h"
#include "grapes_msg_types.h"

static struct nodeID *localID;

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
int sendChunk(struct nodeID *to, const struct chunk *c)
{
  int buff_len;
  uint8_t *buff;
  int res;

  buff_len  = 20 + c->size + c->attributes_size;
  buff = malloc(buff_len + 1);
  if (buff == NULL) {
      return -1;
  }
  res = encodeChunk(c, buff + 1, buff_len);
  buff[0] = MSG_TYPE_CHUNK;
  send_to_peer(localID, to, buff, buff_len + 1);
  free(buff);

  return EXIT_SUCCESS;
}

int chunkDeliveryInit(struct nodeID *myID)
{
  localID = myID;

  return 1;
}

