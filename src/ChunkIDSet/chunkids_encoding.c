/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "chunkids_private.h"
#include "chunkidset.h"
#include "trade_sig_la.h"
#include "int_coding.h"

int encodeChunkSignaling(const struct chunkID_set *h, const void *meta, int meta_len, uint8_t *buff, int buff_len)
{
  int i;
  uint8_t *meta_p;
  uint32_t type = h ? h->type : -1;
  
  int_cpy(buff + 4, type);
  int_cpy(buff + 8, meta_len);

  switch (type) {
    case CIST_BITMAP:
    {
      int elements;
      uint32_t c_min, c_max;

      c_min = c_max = h->n_elements ? h->elements[0] : 0;
      for (i = 1; i < h->n_elements; i++) {
        if (h->elements[i] < c_min)
          c_min = h->elements[i];
        else if (h->elements[i] > c_max)
          c_max = h->elements[i];
      }
      elements = h->n_elements ? c_max - c_min + 1 : 0;
      int_cpy(buff, elements);
      elements = elements / 8 + (elements % 8 ? 1 : 0);
      if (buff_len < elements + 16 + meta_len) {
        return -1;
      }
      int_cpy(buff + 12, c_min); //first value in the bitmap, i.e., base value
      memset(buff + 16, 0, elements);
      for (i = 0; i < h->n_elements; i++) {
        buff[16 + (h->elements[i] - c_min) / 8] |= 1 << ((h->elements[i] - c_min) % 8);
      }
      meta_p = buff + 16 + elements;
      break;
    }
    case CIST_PRIORITY:
      int_cpy(buff, h->n_elements);
      if (buff_len < h->n_elements * 4 + 12 + meta_len) {
        return -1;
      }
      for (i = 0; i < h->n_elements; i++) {
        int_cpy(buff + 12 + i * 4, h->elements[i]);
      }
      meta_p = buff + 12 + h->n_elements * 4;

      break;
    case -1:
      int_cpy(buff, 0);
      meta_p = buff + 12;
      break;
    default:
      fprintf(stderr, "Invalid ChunkID encoding type %d\n", type);

      return -1;
  }

  if (meta_len) {
    memcpy(meta_p, meta, meta_len);
  }

  return meta_p + meta_len - buff;
}

struct chunkID_set *decodeChunkSignaling(void **meta, int *meta_len, const uint8_t *buff, int buff_len)
{
  int i;
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
    {
      // uint8_t bitmap;
      int base;
      int byte_cnt;

      byte_cnt = size / 8 + (size % 8 ? 1 : 0);
      if (buff_len < 16 + byte_cnt + *meta_len) {
        fprintf(stderr, "Error in decoding chunkid set - wrong length\n");
        chunkID_set_free(h);

        return NULL;
      }
      base = int_rcpy(buff + 12);
      for (i = size - 1; i >= 0; i--) {
        if (buff[16 + (i / 8)] & 1 << (i % 8))
          h->elements[h->n_elements++] = base + i;
      }
      meta_p = buff + 16 + byte_cnt;
      break;
    }
    case CIST_PRIORITY:
      if (buff_len != size * 4 + 12 + *meta_len) {
        fprintf(stderr, "Error in decoding chunkid set - wrong length.\n");
        chunkID_set_free(h);

        return NULL;
      }
      for (i = 0; i < size; i++) {
        h->elements[i] = int_rcpy(buff + 12 + i * 4);
      }
      h->n_elements = size;
      meta_p = buff + 12 + size * 4;
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
