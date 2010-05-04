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

#define MAX_TIMESTAMP 5
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

const void *get_metadata(const struct peer_cache *c, int *size)
{
  *size = c->metadata_size;
  return c->metadata;
}

int cache_metadata_update(struct peer_cache *c, struct nodeID *p, const void *meta, int meta_size)
{
  int i;

  if (!meta_size || meta_size != c->metadata_size) {
    return -3;
  }
  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, p)) {
      memcpy(c->metadata + i * meta_size, meta, meta_size);
      return 1;
    }
  }

  return 0;
}

int cache_add(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size)
{
  int i;

  if (meta_size && meta_size != c->metadata_size) {
    return -3;
  }
  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, neighbour)) {
      return -1;
    }
  }
  if (c->current_size == c->cache_size) {
    return -2;
  }
  if (meta_size) {
    memmove(c->metadata + meta_size, c->metadata, c->current_size * meta_size);
    memcpy(c->metadata, meta, meta_size);
  }
  for (i = 0; i < c->current_size; i++) {
    c->entries[i + 1] = c->entries[i];
  }
  c->entries[0].id = nodeid_dup(neighbour);
  c->entries[0].timestamp = 1;
  c->current_size++;

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
    if (found && (i < c->current_size)) {
      c->entries[i] = c->entries[i + 1];
    }
  }

  return c->current_size;
}

void cache_update(struct peer_cache *c)
{
  int i;
  
  for (i = 0; i < c->current_size; i++) {
    if (c->entries[i].timestamp == MAX_TIMESTAMP) {
      c->current_size = i;	/* The cache is ordered by timestamp...
				   all the other entries wiil be older than
				   this one, so remove all of them
				*/
    } else {
      c->entries[i].timestamp++;
    }
  }
}

struct peer_cache *cache_init(int n, int metadata_size)
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
  if (metadata_size) {
    res->metadata = malloc(metadata_size * n);
  } else {
    res->metadata = NULL;
  }

  if (res->metadata) {
    res->metadata_size = metadata_size;
    memset(res->metadata, 0, metadata_size * n);
  } else {
    res->metadata_size = 0;
  }

  return res;
}

void cache_free(struct peer_cache *c)
{
  int i;

  for (i = 0; i < c->current_size; i++) {
    if(c->entries[i].id) {
      nodeid_free(c->entries[i].id);
    }
  }
  free(c->entries);
  free(c->metadata);
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
  uint8_t *meta;
  int cache_size, metadata_size;

  cache_size = int_rcpy(buff);
  metadata_size = int_rcpy(buff + 4);
  p = buff + 8;
  res = cache_init(cache_size, metadata_size);
  meta = res->metadata;
  while (p - buff < size) {
    int len;

    res->entries[i].timestamp = int_rcpy(p);
    p += sizeof(uint32_t);
    res->entries[i++].id = nodeid_undump(p, &len);
    p += len;
    if (metadata_size) {
      memcpy(meta, p, metadata_size);
      p += metadata_size;
      meta += metadata_size;
    }
  }
  res->current_size = i;
if (p - buff != size) { fprintf(stderr, "Waz!! %d != %d\n", p - buff, size); exit(-1);}

  return res;
}

int cache_header_dump(uint8_t *b, const struct peer_cache *c)
{
  int_cpy(b, c->cache_size);
  int_cpy(b + 4, c->metadata_size);

  return 8;
}

int entry_dump(uint8_t *b, struct peer_cache *c, int i)
{
  int res;
 
  if (i && (i >= c->cache_size - 1)) {
    return 0;
  }
  int_cpy(b, c->entries[i].timestamp);
  res = 4;
  res += nodeid_dump(b + res, c->entries[i].id);
  if (c->metadata_size) {
    memcpy(b + res, c->metadata + c->metadata_size * i, c->metadata_size);
    res += c->metadata_size;
  }

  return res;
}

struct peer_cache *merge_caches(struct peer_cache *c1, struct peer_cache *c2, int newsize)
{
  int n1, n2;
  struct peer_cache *new_cache;
  uint8_t *meta;

  new_cache = cache_init(newsize, c1->metadata_size);
  if (new_cache == NULL) {
    return NULL;
  }

  meta = new_cache->metadata;
  for (n1 = 0, n2 = 0; new_cache->current_size < new_cache->cache_size;) {
    if ((n1 == c1->current_size) && (n2 == c2->current_size)) {
      return new_cache;
    }
    if (n1 == c1->current_size) {
      if (!in_cache(new_cache, &c2->entries[n2])) {
        if (new_cache->metadata_size) {
          memcpy(meta, c2->metadata + n2 * c2->metadata_size, c2->metadata_size);
          meta += new_cache->metadata_size;
        }
        new_cache->entries[new_cache->current_size++] = c2->entries[n2];
        c2->entries[n2].id = NULL;
      }
      n2++;
    } else if (n2 == c2->current_size) {
      if (!in_cache(new_cache, &c1->entries[n1])) {
        if (new_cache->metadata_size) {
          memcpy(meta, c1->metadata + n1 * c1->metadata_size, c1->metadata_size);
          meta += new_cache->metadata_size;
        }
        new_cache->entries[new_cache->current_size++] = c1->entries[n1];
        c1->entries[n1].id = NULL;
      }
      n1++;
    } else {
      if (c2->entries[n2].timestamp > c1->entries[n1].timestamp) {
        if (!in_cache(new_cache, &c1->entries[n1])) {
          if (new_cache->metadata_size) {
            memcpy(meta, c1->metadata + n1 * c1->metadata_size, c1->metadata_size);
            meta += new_cache->metadata_size;
          }
          new_cache->entries[new_cache->current_size++] = c1->entries[n1];
          c1->entries[n1].id = NULL;
        }
        n1++;
      } else {
        if (!in_cache(new_cache, &c2->entries[n2])) {
          if (new_cache->metadata_size) {
            memcpy(meta, c2->metadata + n2 * c2->metadata_size, c2->metadata_size);
            meta += new_cache->metadata_size;
          }
          new_cache->entries[new_cache->current_size++] = c2->entries[n2];
          c2->entries[n2].id = NULL;
        }
        n2++;
      }
    }
  }

  return new_cache;
}
