#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "net_helper.h"
#include "nccache.h"

#define MAX_PEERS 50
struct cache_entry {
  struct nodeID *id;
  uint64_t timestamp;
};

struct nodeID *nodeid(const struct cache_entry *c, int i)
{
  if (c[i].timestamp == 0) {
    return NULL;
  }

  //fprintf(stderr, "Returning ID for TS=%lld: %p\n", c[i].timestamp, c[i].id);

  return c[i].id;
}

int cache_add(struct cache_entry *c, struct nodeID *neighbour)
{
  int i;

  for (i = 0; c->timestamp != 0; i++);
  c[i].id = nodeid_dup(neighbour);
  c[i++].timestamp = 1;
  c[i].timestamp = 0;
  
  return i;
}

int cache_del(struct cache_entry *c, struct nodeID *neighbour)
{
  int i;
  int found = 0;

  for (i = 0; c->timestamp != 0; i++) {
    if (nodeid_equal(c[i].id, neighbour)) {
      found = 1;
    }
    if (found) {
      c[i] = c[i+1];
    }
  }

  return i;
}

void cache_update(struct cache_entry *c)
{
  int i;
  
  for (i = 0; c[i].timestamp != 0; i++) {
      c[i].timestamp++;
  }
}

struct cache_entry *cache_init(int n)
{
  struct cache_entry *res;

  res = malloc(sizeof(struct cache_entry) * n);
  if (res) {
    memset(res, 0, sizeof(struct cache_entry) * n);
  }

  return res;
}

int fill_cache_entry(struct cache_entry *c, const struct nodeID *s)
{
  c->id = nodeid_dup(s);
  c->timestamp = 1;
#warning Timestamps are probably wrong...
  return 1;
}

int in_cache(const struct cache_entry *c, const struct cache_entry *elem)
{
  int i;

  for (i = 0; c[i].timestamp != 0; i++) {
    if (nodeid_equal(c[i].id, elem->id)) {
      return 1;
    }
  }

  return 0;
}

struct nodeID *rand_peer(struct cache_entry *c)
{
  int i, j;

  for (i = 0; c[i].timestamp != 0; i++);
  if (i == 0) {
    return NULL;
  }
  j = ((double)rand() / (double)RAND_MAX) * i;

  return c[j].id;
}

struct cache_entry *entries_undump(const uint8_t *buff, int size)
{
  struct cache_entry *res;
  int i = 0;
  const uint8_t *p = buff;
  
  res = malloc(sizeof(struct cache_entry) * MAX_PEERS);
  memset(res, 0, sizeof(struct cache_entry) * MAX_PEERS);
  while (p - buff < size) {
    int len;

    memcpy(&res[i].timestamp, p, sizeof(uint64_t));
    p += sizeof(uint64_t);
    res[i++].id = nodeid_undump(p, &len);
    p += len;
  }
if (p - buff != size) { fprintf(stderr, "Waz!! %d != %d\n", p - buff, size); exit(-1);}

  return res;
}

int entry_dump(uint8_t *b, struct cache_entry *e, int i)
{
  int res;
  
  res = sizeof(uint64_t);
  memcpy(b, &e[i].timestamp, sizeof(uint64_t));
  res += nodeid_dump(b + res, e[i].id);

  return res;
}

struct cache_entry *merge_caches(const struct cache_entry *c1, const struct cache_entry *c2, int cache_size)
{
  int i, n1, n2;
  struct cache_entry *new_cache;

  new_cache = malloc(sizeof(struct cache_entry) * cache_size);
  if (new_cache == NULL) {
    return NULL;
  }
  memset(new_cache, 0, sizeof(struct cache_entry) * cache_size);

  for (i = 0, n1 = 0, n2 = 0; i < cache_size;) {
    if ((c1[n1].timestamp == 0) && (c2[n2].timestamp == 0)) {
      return new_cache;
    }
    if (c1[n1].timestamp == 0) {
      if (!in_cache(new_cache, &c2[n2])) {
        new_cache[i++] = c2[n2];
      } else {
        free(c2[n2].id);
      }
      n2++;
    } else if (c2[n2].timestamp == 0) {
      if (!in_cache(new_cache, &c1[n1])) {
        new_cache[i++] = c1[n1];
      } else {
        free(c1[n1].id);
      }
      n1++;
    } else {
      if (c2[n2].timestamp > c1[n1].timestamp) {
        if (!in_cache(new_cache, &c1[n1])) {
          new_cache[i++] = c1[n1];
        } else {
          free(c1[n1].id);
        }
        n1++;
      } else {
        if (!in_cache(new_cache, &c2[n2])) {
          new_cache[i++] = c2[n2];
        } else {
          free(c2[n2].id);
        }
        n2++;
      }
    }
  }

  while (c1[n1].timestamp != 0) {
    free(c1[n1++].id);
  }
  while (c2[n2].timestamp != 0) {
    free(c2[n2++].id);
  }

  return new_cache;
}
