/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>

#include "chunkids_private.h"
#include "chunkidset.h"
#include "config.h"

#define DEFAULT_SIZE_INCREMENT 32

struct chunkID_set *chunkID_set_init(const char *config)
{
  struct chunkID_set *p;
  struct tag *cfg_tags;
  int res;

  p = malloc(sizeof(struct chunkID_set));
  if (p == NULL) {
    return NULL;
  }
  p->n_elements = 0;
  cfg_tags = config_parse(config);
  res = config_value_int(cfg_tags, "size", &p->size);
  if (!res) {
    p->size = 0;
  }
  if (p->size) {
    p->elements = malloc(p->size * sizeof(int));
  } else {
    p->elements = NULL;
  }
  res = config_value_int(cfg_tags, "type", &p->type);
  if (!res) {
    p->type = CIST_PRIORITY;
  }
  assert(p->type == CIST_PRIORITY || p->type == CIST_BITMAP);
  free(cfg_tags);
  return p;
}

int chunkID_set_add_chunk(struct chunkID_set *h, int chunk_id)
{
  if (chunkID_set_check(h, chunk_id) >= 0) {
    return 0;
  }

  if (h->n_elements == h->size) {
    int *res;

    res = realloc(h->elements, (h->size + DEFAULT_SIZE_INCREMENT) * sizeof(int));
    if (res == NULL) {
      return -1;
    }
    h->size += DEFAULT_SIZE_INCREMENT;
    h->elements = res;
  }
  h->elements[h->n_elements++] = chunk_id;

  return h->n_elements;
}

int chunkID_set_size(const struct chunkID_set *h)
{
  return h->n_elements;
}

int chunkID_set_get_chunk(const struct chunkID_set *h, int i)
{
  if (i < h->n_elements) {
    return h->elements[i];
  }

  return -1;
}

int chunkID_set_check(const struct chunkID_set *h, int chunk_id)
{
  int i;

  for (i = 0; i < h->n_elements; i++) {
    if (h->elements[i] == chunk_id) {
      return i;
    }
  }

  return -1;
}

int chunkID_set_get_earliest(const struct chunkID_set *h)
{
  int i, min;

  min = INT_MAX;
  for (i = 0; i < h->n_elements; i++) {
    min = (h->elements[i] < min) ? h->elements[i] : min;
  }

  return min;
}

int chunkID_set_get_latest(const struct chunkID_set *h)
{
  int i, max;

  max = INT_MIN;
  for (i = 0; i < h->n_elements; i++) {
    max = (h->elements[i] > max) ? h->elements[i] : max;
  }

  return max;
}

int chunkID_set_union(struct chunkID_set *h, struct chunkID_set *a)
{
  int i;

  for (i = 0; i < a->n_elements; i++) {
    int ret = chunkID_set_add_chunk(h,a->elements[i]);
    if (ret < 0) return ret;
  }
  return h->n_elements;
}

void chunkID_set_clear(struct chunkID_set *h, int size)
{
  h->n_elements = 0;
  h->size = size;
  h->elements = realloc(h->elements, size * sizeof(int));
  if (h->elements == NULL) {
    h->size = 0;
  }
}

void chunkID_set_free(struct chunkID_set *h)
{
  chunkID_set_clear(h,0);
  free(h->elements);
  free(h);
}
