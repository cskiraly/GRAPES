/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "peerset_private.h"
#include "peer.h"
#include "peerset.h"
#include "chunkidset.h"
#include "net_helper.h"
#include "config.h"

#define DEFAULT_SIZE_INCREMENT 32

static int nodeid_peer_cmp(const void *id, const void *p)
{
  const struct peer *peer = *(struct peer *const *)p;

  return nodeid_cmp( (const struct nodeID *) id, peer->id);
}

static int peerset_check_insert_pos(const struct peerset *h, const struct nodeID *id)
{
  int a, b, c, r;

  if (! h->n_elements) {
    return 0;
  }

  a = 0;
  b = c = h->n_elements - 1;

  while ((r = nodeid_peer_cmp(id, &h->elements[b])) != 0) {
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


struct peerset *peerset_init(const char *config)
{
  struct peerset *p;
  struct tag *cfg_tags;
  int res;

  p = malloc(sizeof(struct peerset));
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
  free(cfg_tags);
  if (p->size) {
    p->elements = malloc(p->size * sizeof(struct peer *));
  } else {
    p->elements = NULL;
  }

  return p;
}

int peerset_add_peer(struct peerset *h, struct nodeID *id)
{
  struct peer *e;
  int pos;

  pos = peerset_check_insert_pos(h, id);
  if (pos < 0){
    return 0;
  }

  if (h->n_elements == h->size) {
    struct peer **res;

    res = realloc(h->elements, (h->size + DEFAULT_SIZE_INCREMENT) * sizeof(struct peer *));
    if (res == NULL) {
      return -1;
    }
    h->size += DEFAULT_SIZE_INCREMENT;
    h->elements = res;
  }

  memmove(&h->elements[pos + 1], &h->elements[pos] , ((h->n_elements++) - pos) * sizeof(struct peer *));

  e = malloc(sizeof(struct peer));
  h->elements[pos] = e;;
  e->id = nodeid_dup(id);
  gettimeofday(&e->creation_timestamp,NULL);
  e->bmap = chunkID_set_init("type=bitmap");
  timerclear(&e->bmap_timestamp);
  e->cb_size = 0;

  return h->n_elements;
}

void peerset_add_peers(struct peerset *h, struct nodeID **ids, int n)
{
  int i;

  for (i = 0; i < n; i++) {
    peerset_add_peer(h, ids[i]);
  }
}

int peerset_size(const struct peerset *h)
{
  return h->n_elements;
}

struct peer **peerset_get_peers(const struct peerset *h)
{
  return h->elements;
}

struct peer *peerset_get_peer(const struct peerset *h, const struct nodeID *id)
{
  int i = peerset_check(h,id);
  return (i<0) ? NULL : h->elements[i];
}

int peerset_remove_peer(struct peerset *h, const struct nodeID *id){
  int i = peerset_check(h,id);
  if (i >= 0) {
    struct peer *e = h->elements[i];
    nodeid_free(e->id);
    chunkID_set_free(e->bmap);
    memmove(e, e + 1, ((h->n_elements--) - (i+1)) * sizeof(struct peer *));
    free(e);

    return i;
  }
  return -1;
}

int peerset_check(const struct peerset *h, const struct nodeID *id)
{
  struct peer **p;

  p = bsearch(id, h->elements, (size_t) h->n_elements, sizeof(h->elements[0]), nodeid_peer_cmp);

  return p ? p - h->elements : -1;
}

void peerset_clear(struct peerset *h, int size)
{
  int i;

  for (i = 0; i < h->n_elements; i++) {
    struct peer *e = h->elements[i];
    nodeid_free(e->id);
    chunkID_set_free(e->bmap);
    free(e);
  }

  h->n_elements = 0;
  h->size = size;
  h->elements = realloc(h->elements, size * sizeof(struct peer *));
  if (h->elements == NULL) {
    h->size = 0;
  }
}
