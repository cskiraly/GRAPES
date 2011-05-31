#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "chunkids_private.h"
#include "chunkids_iface.h"

#define DEFAULT_SIZE_INCREMENT 32

static int chunkID_set_check_list(const struct chunkID_set *h, int chunk_id)
{
  int i;

  for (i = 0; i < h->n_elements; i++) {
    if (h->elements[i] == chunk_id) {
      return i;
    }
  }

  return -1;
}

static int chunkID_set_add_chunk_list(struct chunkID_set *h, int chunk_id)
{
  if (chunkID_set_check_list(h, chunk_id) >= 0) {
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

struct cids_ops_iface list_ops = {
  .add_chunk = chunkID_set_add_chunk_list,
  .check = chunkID_set_check_list,
};
