/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "chunkidset.h"

uint32_t chunkID_set_get_earliest(const struct chunkID_set *h)
{
  int i;
  uint32_t min;

  if (chunkID_set_size(h) == 0) {
    return CHUNKID_INVALID;
  }
  min = chunkID_set_get_chunk(h, 0);
  for (i = 1; i < chunkID_set_size(h); i++) {
    int c = chunkID_set_get_chunk(h, i);

    min = (c < min) ? c : min;
  }

  return min;
}

uint32_t chunkID_set_get_latest(const struct chunkID_set *h)
{
  int i;
  uint32_t  max;

  if (chunkID_set_size(h) == 0) {
    return CHUNKID_INVALID;
  }
  max = chunkID_set_get_chunk(h, 0);
  for (i = 1; i < chunkID_set_size(h); i++) {
    int c = chunkID_set_get_chunk(h, i);

    max = (c > max) ? c : max;
  }

  return max;
}

int chunkID_set_union(struct chunkID_set *h, struct chunkID_set *a)
{
  int i;

  for (i = 0; i < chunkID_set_size(a); i++) {
    int ret = chunkID_set_add_chunk(h, chunkID_set_get_chunk(a, i));
    if (ret < 0) return ret;
  }

  return chunkID_set_size(h);
}
