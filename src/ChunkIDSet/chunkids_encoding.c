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
#include "chunkids_iface.h"
#include "chunkidset.h"
#include "trade_sig_la.h"
#include "int_coding.h"

static const char *type_name(uint32_t type)
{
  if (type == CIST_PRIORITY) {
    return "priority";
  } else if (type == CIST_BITMAP) {
    return "bitmap";
  }

  return "Unknown";
}

int encodeChunkSignaling(const struct chunkID_set *h, const void *meta, int meta_len, uint8_t *buff, int buff_len)
{
  uint8_t *meta_p;
  uint32_t type = h ? h->type : -1;
  
  int_cpy(buff + 4, type);
  int_cpy(buff + 8, meta_len);

  if (h) {
    meta_p = h->enc->encode(h, buff, buff_len, meta_len);
  } else {
    int_cpy(buff, 0);
    meta_p = buff + 12;
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
    sprintf(cfg, "size=%d,type=%s", size, type_name(type));
    h = chunkID_set_init(cfg);
    if (h == NULL) {
      fprintf(stderr, "Error in decoding chunkid set - not enough memory to create a chunkID set.\n");

      return NULL;
    }
    meta_p = h->enc->decode(h, buff, buff_len, meta_len);
  } else {
    h = NULL;
    meta_p = buff + 12;
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
