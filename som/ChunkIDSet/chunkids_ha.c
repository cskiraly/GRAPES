/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>

#include "chunkidset.h"

int chunkID_set_get_earliest(const struct chunkID_set *h)
{
  int i, min;

  min = INT_MAX;
  for (i = 0; i < chunkID_set_size(h); i++) {
    int c = chunkID_set_get_chunk(h, i);

    min = (c < min) ? c : min;
  }

  return min;
}

int chunkID_set_get_latest(const struct chunkID_set *h)
{
  int i, max;

  max = INT_MIN;
  for (i = 0; i < chunkID_set_size(h); i++) {
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
