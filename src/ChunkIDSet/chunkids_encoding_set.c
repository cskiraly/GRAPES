#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "chunkids_private.h"
#include "chunkids_iface.h"
#include "int_coding.h"
#include "chunkidset.h"

static uint8_t *bmap_encode(const struct chunkID_set *h, uint8_t *buff, int buff_len, int meta_len)
{
  int i, elements;
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
    return NULL;
  }
  int_cpy(buff + 12, c_min); //first value in the bitmap, i.e., base value
  memset(buff + 16, 0, elements);
  for (i = 0; i < h->n_elements; i++) {
    buff[16 + (h->elements[i] - c_min) / 8] |= 1 << ((h->elements[i] - c_min) % 8);
  }

  return buff + 16 + elements;
}

static const uint8_t *bmap_decode(struct chunkID_set *h, const uint8_t *buff, int buff_len, int *meta_len)
{
  int i;
  int base;
  int byte_cnt;

  byte_cnt = h->size / 8 + (h->size % 8 ? 1 : 0);
  if (buff_len < 16 + byte_cnt + *meta_len) {
    fprintf(stderr, "Error in decoding chunkid set - wrong length\n");
    chunkID_set_free(h);

    return NULL;
  }
  base = int_rcpy(buff + 12);
  for (i = h->size - 1; i >= 0; i--) {
  if (buff[16 + (i / 8)] & 1 << (i % 8))
    h->elements[h->n_elements++] = base + i;
  }

  return buff + 16 + byte_cnt;
}

struct cids_encoding_iface bmap_encoding = {
  .encode = bmap_encode,
  .decode = bmap_decode,
};
