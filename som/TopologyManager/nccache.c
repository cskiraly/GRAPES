/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "net_helper.h"
#include "nccache.h"

struct cache_entry {
  struct nodeID *id;
  uint32_t timestamp;
};

struct peer_cache {
  struct cache_entry *entries;
  int cache_size;
  int current_size;
  int metadata_size;
  uint8_t *metadata; 
};

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

struct nodeID *nodeid(const struct peer_cache *c, int i)
{
  if (i < c->current_size) {
    return c->entries[i].id;
  }

  return NULL;
}

int cache_add(struct peer_cache *c, struct nodeID *neighbour)
{
  int i;

  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, neighbour)) {
      return -1;
    }
  }
  if (c->current_size == c->cache_size) {
    return -2;
  }
  c->entries[c->current_size].id = nodeid_dup(neighbour);
  c->entries[c->current_size++].timestamp = 1;

  return c->current_size;
}

int cache_del(struct peer_cache *c, struct nodeID *neighbour)
{
  int i;
  int found = 0;

  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, neighbour)) {
      nodeid_free(c->entries[i].id);
      c->current_size--;
      found = 1;
    }
    if (found) {
      c->entries[i] = c->entries[i+1];
    }
  }

  return c->current_size;
}

void cache_update(struct peer_cache *c)
{
  int i;
  
  for (i = 0; i < c->current_size; i++) {
    c->entries[i].timestamp++;
  }
}

struct peer_cache *cache_init(int n)
{
  struct peer_cache *res;

  res = malloc(sizeof(struct peer_cache));
  if (res == NULL) {
    return NULL;
  }
  res->cache_size = n;
  res->current_size = 0;
  res->entries = malloc(sizeof(struct cache_entry) * n);
  if (res == NULL) {
    free(res);

    return NULL;
  }
  
  memset(res->entries, 0, sizeof(struct cache_entry) * n);

  return res;
}

void cache_free(struct peer_cache *c)
{
  int i;

  for (i = 0; i < c->current_size; i++) {
    nodeid_free(c->entries[i].id);
  }
  free(c->entries);
  free(c);
}

int fill_cache_entry(struct cache_entry *c, const struct nodeID *s)
{
  c->id = nodeid_dup(s);
  c->timestamp = 1;
#warning Timestamps are probably wrong...
  return 1;
}

int in_cache(const struct peer_cache *c, const struct cache_entry *elem)
{
  int i;

  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, elem->id)) {
      return 1;
    }
  }

  return 0;
}

struct nodeID *rand_peer(struct peer_cache *c)
{
  int j;

  if (c->current_size == 0) {
    return NULL;
  }
  j = ((double)rand() / (double)RAND_MAX) * c->current_size;

  return c->entries[j].id;
}

struct peer_cache *entries_undump(const uint8_t *buff, int size)
{
  struct peer_cache *res;
  int i = 0;
  const uint8_t *p = buff;
  int cache_size;

  cache_size = int_rcpy(buff);
  p = buff + 4;
  res = cache_init(cache_size);
  
  while (p - buff < size) {
    int len;

    res->entries[i].timestamp = int_rcpy(p);
    p += sizeof(uint32_t);
    res->entries[i++].id = nodeid_undump(p, &len);
    p += len;
  }
  res->current_size = i;
if (p - buff != size) { fprintf(stderr, "Waz!! %d != %d\n", p - buff, size); exit(-1);}

  return res;
}

int cache_header_dump(uint8_t *b, const struct peer_cache *c)
{
  int_cpy(b, c->cache_size);

  return 4;
}
 
int entry_dump(uint8_t *b, struct peer_cache *c, int i)
{
  int res;
  
  int_cpy(b, c->entries[i].timestamp);
  res = 4;
  res += nodeid_dump(b + res, c->entries[i].id);

  return res;
}

struct peer_cache *merge_caches(struct peer_cache *c1, struct peer_cache *c2)
{
  int n1, n2;
  struct peer_cache *new_cache;

  new_cache = cache_init(c1->cache_size);
  if (new_cache == NULL) {
    return NULL;
  }

  for (n1 = 0, n2 = 0; new_cache->current_size < new_cache->cache_size;) {
    if ((n1 == c1->current_size) && (n2 == c2->current_size)) {
      return new_cache;
    }
    if (n1 == c1->current_size) {
      if (!in_cache(new_cache, &c2->entries[n2])) {
        new_cache->entries[new_cache->current_size++] = c2->entries[n2];
        c2->entries[n2].id = NULL;
      }
      n2++;
    } else if (n2 == c2->current_size) {
      if (!in_cache(new_cache, &c1->entries[n1])) {
        new_cache->entries[new_cache->current_size++] = c1->entries[n1];
        c1->entries[n1].id = NULL;
      }
      n1++;
    } else {
      if (c2->entries[n2].timestamp > c1->entries[n1].timestamp) {
        if (!in_cache(new_cache, &c1->entries[n1])) {
          new_cache->entries[new_cache->current_size++] = c1->entries[n1];
          c1->entries[n1].id = NULL;
        }
        n1++;
      } else {
        if (!in_cache(new_cache, &c2->entries[n2])) {
          new_cache->entries[new_cache->current_size++] = c2->entries[n2];
          c2->entries[n2].id = NULL;
        }
        n2++;
      }
    }
  }

  return new_cache;
}
