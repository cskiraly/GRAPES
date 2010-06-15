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

int encodeChunkSignaling(const struct chunkID_set *h, const void *meta, int meta_len, uint8_t *buff, int buff_len)
{
  int i;

  if (buff_len < h->n_elements * 4 + 12 + meta_len) {
    return -1;
  }
  int_cpy(buff, h->n_elements);
  int_cpy(buff + 4, 0);
  int_cpy(buff + 8, meta_len);
  
  for (i = 0; i < h->n_elements; i++) {
    int_cpy(buff + 12 + i * 4, h->elements[i]);
  }
  if (meta_len) {
    memcpy(buff + h->n_elements * 4 + 12, meta, meta_len);
  }

  return h->n_elements * 4 + 12 + meta_len;
}

struct chunkID_set *decodeChunkSignaling(void **meta, int *meta_len, const uint8_t *buff, int buff_len)
{
  int i, val, size;
  struct chunkID_set *h;
  char cfg[32];

  size = int_rcpy(buff);
  val = int_rcpy(buff + 4);
  *meta_len = int_rcpy(buff + 8);
  if (buff_len != size * 4 + 12 + *meta_len) {
    return NULL;
  }
  sprintf(cfg, "size=%d", size);
  h = chunkID_set_init(cfg);
  if (h == NULL) {
    return NULL;
  }
  if (val) {
    return h;	/* Not supported yet! */
  }

  for (i = 0; i < size; i++) {
    h->elements[i] = int_rcpy(buff + 12 + i * 4);
  }
  h->n_elements = size;

  if (*meta_len) {
    *meta = malloc(*meta_len);
    if (*meta != NULL) {
      memcpy(*meta, buff + 12 + size * 4, *meta_len);
    } else {
      *meta_len = 0;
    }
  } else {
    *meta = NULL;
  }

  return h;
}
