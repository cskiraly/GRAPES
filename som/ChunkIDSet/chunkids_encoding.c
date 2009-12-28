#include <arpa/inet.h>
#include <string.h>
#include <stdint.h>

#include "chunkids_private.h"
#include "chunkidset.h"

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

int chunkID_set_encode(const struct chunkID_set *h, uint8_t *buff, int buff_len)
{
  int i;

  if (buff_len < h->n_ids * 4 + 8) {
    return -1;
  }
  int_cpy(buff, h->n_ids);
  int_cpy(buff + 4, 0);
  
  for (i = 0; i < h->n_ids; i++) {
    int_cpy(buff + 8 + i * 4, h->ids[i]);
  }

  return h->n_ids * 4 + 8;
}

struct chunkID_set *chunkID_set_decode(const uint8_t *buff, int buff_len)
{
  int i, size;
  struct chunkID_set *h;

  size = int_rcpy(buff);
  if (buff_len != size * 4 + 8) {
    return NULL;
  }
  h = chunkID_set_init(size);
  if (h == NULL) {
    return NULL;
  }
  if (int_rcpy(buff + 4)) {
    return h;	/* Not supported yet! */
  }

  for (i = 0; i < size; i++) {
    h->ids[i] = int_rcpy(buff + 8 + i * 4);
  }
  h->n_ids = size;

  return h;
}
