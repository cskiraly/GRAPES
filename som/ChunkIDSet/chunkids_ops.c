#include <stdlib.h>
#include <stdint.h>

#include "chunkids_private.h"
#include "chunkidset.h"

#define DEFAULT_SIZE_INCREMENT 32

struct chunkID_set *chunkID_set_init(int size)
{
  struct chunkID_set *p;

  p = malloc(sizeof(struct chunkID_set));
  if (p == NULL) {
    return NULL;
  }
  p->n_ids = 0;
  p->size = size;
  if (p->size) {
    p->ids = malloc(p->size * sizeof(int));
  } else {
    p->ids = NULL;
  }

  return p;
}

int chunkID_set_add_chunk(struct chunkID_set *h, int chunk_id)
{
  if (chunkID_set_check(h, chunk_id) < 0) {
    return 0;
  }

  if (h->n_ids == h->size) {
    int *res;

    res = realloc(h->ids, h->size + DEFAULT_SIZE_INCREMENT);
    if (res == NULL) {
      return -1;
    }
    h->size += DEFAULT_SIZE_INCREMENT;
    h->ids = res;
  }
  h->ids[h->n_ids++] = chunk_id;

  return h->n_ids;
}

int chunkID_size(const struct chunkID_set *h)
{
  return h->n_ids;
}

int chunkID_set_get_chunk(const struct chunkID_set *h, int i)
{
  if (i < h->n_ids) {
    return h->ids[i];
  }

  return -1;
}

int chunkID_set_check(const struct chunkID_set *h, int chunk_id)
{
  int i;

  for (i = 0; i < h->n_ids; i++) {
    if (h->ids[i] == chunk_id) {
      return i;
    }
  }

  return -1;
}

void chunkID_clear(struct chunkID_set *h, int size)
{
  h->n_ids = 0;
  h->size = size;
  h->ids = realloc(h->ids, size);
  if (h->ids == NULL) {
    h->size = 0;
  }
}
