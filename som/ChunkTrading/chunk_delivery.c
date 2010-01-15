/*
 *  Copyright (c) 2009 Alessandro Russo.
 *
 *  This is free software;
 *  see GPL.txt
 */
 //header to clean
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "chunk.h"
#include "net_helper.h"
#include "trade_msg_la.h"
#include "trade_msg_ha.h"

static struct nodeID *localID;

static void chunk_print(FILE *f, const struct chunk *c)
{
  const uint8_t *p;

  fprintf(f, "Chunk %d:\n", c->id);
  fprintf(f, "\tTS: %llu\n", c->timestamp);
  fprintf(f, "\tPayload size: %d\n", c->size);
  fprintf(f, "\tAttributes size: %d\n", c->attributes_size);
  p = c->data;
  fprintf(f, "\tPayload:\n");
  fprintf(f, "\t\t%c %c %c %c ...:\n", p[0], p[1], p[2], p[3]);
  if (c->attributes_size > 0) {
    p = c->attributes;
    fprintf(f, "\tAttributes:\n");
    fprintf(f, "\t\t%c %c %c %c ...:\n", p[0], p[1], p[2], p[3]);
  }
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
int sendChunk(const struct nodeID *to, struct chunk *c){
    int buff_len;
    uint8_t *buff;
    int res;

    buff_len  = 20 + c->size + c->attributes_size;
    buff = malloc(buff_len + 1);
    if (buff == NULL) {
        return -1;
    }
    res = encodeChunk(c, buff + 1, buff_len);
    fprintf(stdout, "Encoding chunk %d in %d bytes\n",res, buff_len);
    chunk_print(stdout, c);
    buff[0] = 12;
    send_data(localID, to, buff, buff_len + 1);
    free(buff);

    return (EXIT_SUCCESS);
}

int chunkInit(struct nodeID *myID)
{
  localID = myID;
  return 1;
}

