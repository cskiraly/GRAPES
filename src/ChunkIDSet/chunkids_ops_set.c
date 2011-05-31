#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "chunkids_private.h"
#include "chunkids_iface.h"

#define DEFAULT_SIZE_INCREMENT 32

static int int_cmp(const void *pa, const void *pb)
{
  return (*(const int *)pa - *(const int *)pb);
}

static int check_insert_pos(const struct chunkID_set *h, int id)
{
  int a, b, c, r;

  if (! h->n_elements) {
    return 0;
  }

  a = 0;
  b = c = h->n_elements - 1;

  while ((r = int_cmp(&id, &h->elements[b])) != 0) {
    if (r > 0) {
      if (b == c) {
        return b + 1;
      } else {
        a = b + 1;
      }
    } else {
      if (b == a) {
        return b;
      } else {
        c = b;
      }
    }
    b = (a + c) / 2;
  }

  return -1;
}



static int chunkID_set_add_chunk_set(struct chunkID_set *h, int chunk_id)
{
  int pos;

  pos = check_insert_pos(h, chunk_id);
  if (pos < 0){
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

  memmove(&h->elements[pos + 1], &h->elements[pos] , ((h->n_elements++) - pos) * sizeof(int));

  h->elements[pos] = chunk_id;

  return h->n_elements;
}

static int chunkID_set_check_set(const struct chunkID_set *h, int chunk_id)
{
  int *p;

  p = bsearch(&chunk_id, h->elements, (size_t) h->n_elements, sizeof(h->elements[0]), int_cmp);
  return p ? p - h->elements : -1;
}

struct cids_ops_iface set_ops = {
  .add_chunk = chunkID_set_add_chunk_set,
  .check = chunkID_set_check_set,
};
