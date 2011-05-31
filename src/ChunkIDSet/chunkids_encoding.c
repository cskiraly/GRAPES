/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "chunkids_private.h"
#include "chunkidset.h"
#include "trade_sig_la.h"
#include "int_coding.h"

uint8_t *bmap_encode(const struct chunkID_set *h, uint8_t *buff, int buff_len, int meta_len);
uint8_t *prio_encode(const struct chunkID_set *h, uint8_t *buff, int buff_len, int meta_len);
const uint8_t *bmap_decode(struct chunkID_set *h, const uint8_t *buff, int buff_len, int *meta_len);
const uint8_t *prio_decode(struct chunkID_set *h, const uint8_t *buff, int buff_len, int *meta_len);

int encodeChunkSignaling(const struct chunkID_set *h, const void *meta, int meta_len, uint8_t *buff, int buff_len)
{
  uint8_t *meta_p;
  uint32_t type = h ? h->type : -1;
  
  int_cpy(buff + 4, type);
  int_cpy(buff + 8, meta_len);

  switch (type) {
    case CIST_BITMAP:
      meta_p =  bmap_encode(h, buff, buff_len, meta_len);
      break;
    case CIST_PRIORITY:
      meta_p =  prio_encode(h, buff, buff_len, meta_len);
      break;
    case -1:
      int_cpy(buff, 0);
      meta_p = buff + 12;
      break;
    default:
      fprintf(stderr, "Invalid ChunkID encoding type %d\n", type);

      return -1;
  }
  if (meta_p == NULL) {
    return -1;
  }

  if (meta_len) {
    memcpy(meta_p, meta, meta_len);
  }

  return meta_p + meta_len - buff;
}

struct chunkID_set *decodeChunkSignaling(void **meta, int *meta_len, const uint8_t *buff, int buff_len)
{
  uint32_t size;
  uint32_t type;
  struct chunkID_set *h;
  const uint8_t *meta_p;

  size = int_rcpy(buff);
  type = int_rcpy(buff + 4);
  *meta_len = int_rcpy(buff + 8);

  if (type != -1) {
    char cfg[32];

    memset(cfg, 0, sizeof(cfg));
    sprintf(cfg, "size=%d", size);
    h = chunkID_set_init(cfg);
    if (h == NULL) {
      fprintf(stderr, "Error in decoding chunkid set - not enough memory to create a chunkID set.\n");

      return NULL;
    }
    h->type = type;
  } else {
    h = NULL;
  }

  switch (type) {
    case CIST_BITMAP:
      meta_p = bmap_decode(h, buff, buff_len, meta_len);
      break;
    case CIST_PRIORITY:
      meta_p = prio_decode(h, buff, buff_len, meta_len);
      break;
    case -1:
      meta_p = buff + 12;
      break;
    default:
      fprintf(stderr, "Error in decoding chunkid set - wrong type %d\n", type);
      chunkID_set_free(h);

      return NULL;
  }

  if (*meta_len) {
    *meta = malloc(*meta_len);
    if (*meta != NULL) {
      memcpy(*meta, meta_p, *meta_len);
    } else {
      *meta_len = 0;
    }
  } else {
    *meta = NULL;
  }

  return h;
}
