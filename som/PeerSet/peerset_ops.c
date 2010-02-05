#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "peerset_private.h"
#include "peer.h"
#include "peerset.h"
#include "chunkidset.h"
#include "net_helper.h"

#define DEFAULT_SIZE_INCREMENT 32

struct nodeID;

struct peerset *peerset_init(int size)
{
  struct peerset *p;

  p = malloc(sizeof(struct peerset));
  if (p == NULL) {
    return NULL;
  }
  p->n_elements = 0;
  p->size = size;
  if (p->size) {
    p->elements = malloc(p->size * sizeof(struct peer));
  } else {
    p->elements = NULL;
  }

  return p;
}

int peerset_add_peer(struct peerset *h, const struct nodeID *id)
{
  struct peer *e;
  if (peerset_check(h, id) >= 0) {
    return 0;
  }

  if (h->n_elements == h->size) {
    int *res;

    res = realloc(h->elements, (h->size + DEFAULT_SIZE_INCREMENT) * sizeof(struct peer));
    if (res == NULL) {
      return -1;
    }
    h->size += DEFAULT_SIZE_INCREMENT;
    h->elements = (struct peer*) res;
  }
  e = &(h->elements[h->n_elements++]);
  e->id = nodeid_dup(id);
  gettimeofday(&e->creation_timestamp,NULL);
  e->bmap = chunkID_set_init(0);
  timerclear(&e->bmap_timestamp);
  e->cb_size = INT_MAX;

  return h->n_elements;
}

void peerset_add_peers(struct peerset *h, const struct nodeID **ids, int n)
{
  int i;

  for (i = 0; i < n; i++) {
    peerset_add_peer(h,ids[i]);
  }
}

int peerset_size(const struct peerset *h)
{
  return h->n_elements;
}

struct peer* peerset_get_peers(const struct peerset *h)
{
  return h->elements;
}

struct peer *peerset_get_peer(const struct peerset *h, const struct nodeID *id)
{
  int i = peerset_check(h,id);
  return (i<0) ? NULL : &(h->elements[i]);
}

int peerset_remove_peer(struct peerset *h, const struct nodeID *id){
  int i = peerset_check(h,id);
  if (i >= 0) {
    memmove(&h->elements[i], &h->elements[i+1], ((h->n_elements--) - (i+1)) * sizeof(struct peer));
    return i;
  }
  return -1;
}

int peerset_check(const struct peerset *h, const struct nodeID *id)
{
  int i;

  for (i = 0; i < h->n_elements; i++) {
    if (nodeid_equal(h->elements[i].id, id)) {
      return i;
    }
  }

  return -1;
}

void peerset_clear(struct peerset *h, int size)
{
  h->n_elements = 0;
  h->size = size;
  h->elements = realloc(h->elements, size * sizeof(struct peer));
  if (h->elements == NULL) {
    h->size = 0;
  }
}
