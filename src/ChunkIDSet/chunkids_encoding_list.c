#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "chunkids_private.h"
#include "chunkids_iface.h"
#include "int_coding.h"
#include "chunkidset.h"

static uint8_t *prio_encode(const struct chunkID_set *h, uint8_t *buff, int buff_len, int meta_len)
{
  int i;

  int_cpy(buff, h->n_elements);
  if (buff_len < h->n_elements * 4 + 12 + meta_len) {
    return NULL;
  }
  for (i = 0; i < h->n_elements; i++) {
    int_cpy(buff + 12 + i * 4, h->elements[i]);
  }

  return buff + 12 + h->n_elements * 4;
}

static const uint8_t *prio_decode(struct chunkID_set *h, const uint8_t *buff, int buff_len, int *meta_len)
{
  int i;

  if (buff_len != h->size * 4 + 12 + *meta_len) {
    fprintf(stderr, "Error in decoding chunkid set - wrong length.\n");
    chunkID_set_free(h);

    return NULL;
  }
  for (i = 0; i < h->size; i++) {
    h->elements[i] = int_rcpy(buff + 12 + i * 4);
  }
  h->n_elements = h->size;

  return buff + 12 + h->size * 4;
}

struct cids_encoding_iface prio_encoding = {
  .encode = prio_encode,
  .decode = prio_decode,
};
