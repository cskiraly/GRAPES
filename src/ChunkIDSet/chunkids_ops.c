/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
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
  const char *type;

  p = malloc(sizeof(struct chunkID_set));
  if (p == NULL) {
    return NULL;
  }
  p->n_elements = 0;
  cfg_tags = config_parse(config);
  if (!cfg_tags) {
    free(p);
    return NULL;
  }
  res = config_value_int(cfg_tags, "size", &p->size);
  if (!res) {
    p->size = 0;
  }
  if (p->size) {
    p->elements = malloc(p->size * sizeof(int));
    if (p->elements == NULL) {
      p->size = 0;
    }
  } else {
    p->elements = NULL;
  }
  p->type = CIST_PRIORITY;
  type = config_value_str(cfg_tags, "type");
  if (type) {
    if (!memcmp(type, "priority", strlen(type) - 1)) {
      p->type = CIST_PRIORITY;
    } else if (!memcmp(type, "bitmap", strlen(type) - 1)) {
      p->type = CIST_BITMAP;
    } else {
      chunkID_set_free(p);
      free(cfg_tags);

      return NULL; 
    }
  }
  free(cfg_tags);
  assert(p->type == CIST_PRIORITY || p->type == CIST_BITMAP);

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
