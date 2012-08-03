/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#undef NDEBUG
#include <assert.h>

#include "net_helper.h"
#include "topocache.h"
#include "int_coding.h"

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
  int max_timestamp;
};

static int cache_insert(struct peer_cache *c, struct cache_entry *e, const void *meta)
{
  int i, position;

  if (c->current_size == c->cache_size) {
    return -2;
  }
  position = 0;
  for (i = 0; i < c->current_size; i++) {
    assert(e->id);
    assert(c->entries[i].id);

    if (nodeid_equal(e->id, c->entries[i].id)) {
      if (c->entries[i].timestamp > e->timestamp) {
        assert(i >= position);
        nodeid_free(c->entries[i].id);

        if (position != i) {
          memmove(c->entries + position + 1, c->entries + position, sizeof(struct cache_entry) * (i - position));
          memmove(c->metadata + (position + 1) * c->metadata_size, c->metadata + position * c->metadata_size, (i -position) * c->metadata_size);
        }

        c->entries[position] = *e;
        memcpy(c->metadata + position * c->metadata_size, meta, c->metadata_size);

        return position;
      }

      return -1;
    }

    if (c->entries[i].timestamp <= e->timestamp) {
      position = i + 1;
    }
  }

  if (position != c->current_size) {
    memmove(c->entries + position + 1, c->entries + position, sizeof(struct cache_entry) * (c->current_size - position));
    memmove(c->metadata + (position + 1) * c->metadata_size, c->metadata + position * c->metadata_size, (c->current_size - position) * c->metadata_size);
  }
  c->current_size++;
  c->entries[position] = *e;
  memcpy(c->metadata + position * c->metadata_size, meta, c->metadata_size);

  return position;
}

int cache_add_cache(struct peer_cache *dst, const struct peer_cache *src)
{
  struct cache_entry *e_orig;
  int count, j;
  struct cache_entry e_dup;
cache_check(dst);
cache_check(src);

  count = 0;
  j=0;
  while(dst->current_size < dst->cache_size && src->current_size > count) {
    count++;

    e_orig = src->entries + j;

    e_dup.id = nodeid_dup(e_orig->id);
    e_dup.timestamp = e_orig->timestamp;

    if (cache_insert(dst, &e_dup, src->metadata + src->metadata_size * j) < 0) {
      nodeid_free(e_dup.id);
    }
    j++;
  }
cache_check(dst);
cache_check(src);

  return dst->current_size;
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

int cache_metadata_update(struct peer_cache *c, const struct nodeID *p, const void *meta, int meta_size)
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

int cache_add_ranked(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size, ranking_function f, const void *tmeta)
{
  int i, pos = 0;

  if (meta_size && meta_size != c->metadata_size) {
    return -3;
  }
  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, neighbour)) {
      if (f != NULL) {
        cache_del(c,neighbour);
        if (i == c->current_size) break;
      } else {
          cache_metadata_update(c,neighbour,meta,meta_size);
          return -1;
      }
    }
    if ((f != NULL) && f(tmeta, meta, c->metadata+(c->metadata_size * i)) == 2) {
      pos++;
    }
  }
  if (c->current_size == c->cache_size) {
    return -2;
  }
  if (c->metadata_size) {
    memmove(c->metadata + (pos + 1) * c->metadata_size, c->metadata + pos * c->metadata_size, (c->current_size - pos) * c->metadata_size);
    if (meta_size) {
      memcpy(c->metadata + pos * c->metadata_size, meta, meta_size);
    } else {
      memset(c->metadata + pos * c->metadata_size, 0, c->metadata_size);
    }
  }
  for (i = c->current_size; i > pos; i--) {
    c->entries[i] = c->entries[i - 1];
  }
  c->entries[pos].id = nodeid_dup(neighbour);
  c->entries[pos].timestamp = 1;
  c->current_size++;

  return c->current_size;
}

int cache_add(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size)
{
  return cache_add_ranked(c, neighbour, meta, meta_size, NULL, NULL);
}

int cache_del(struct peer_cache *c, const struct nodeID *neighbour)
{
  int i;
  int found = 0;

  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, neighbour)) {
      nodeid_free(c->entries[i].id);
      c->current_size--;
      found = 1;
      if (c->metadata_size && (i < c->current_size)) {
        memmove(c->metadata + c->metadata_size * i,
                c->metadata + c->metadata_size * (i + 1),
                c->metadata_size * (c->current_size - i));
      }
    }
    if (found && (i < c->current_size)) {
      c->entries[i] = c->entries[i + 1];
    }
  }

  return c->current_size;
}

void cache_delay(struct peer_cache *c, int dts)
{
  int i;
  
  for (i = 0; i < c->current_size; i++) {
    if (c->max_timestamp && (c->entries[i].timestamp + dts > c->max_timestamp)) {
      int j = i;

      while(j < c->current_size && c->entries[j].id) {
        nodeid_free(c->entries[j].id);
        c->entries[j++].id = NULL;
      }
      c->current_size = i;	/* The cache is ordered by timestamp...
				   all the other entries wiil be older than
				   this one, so remove all of them
				*/
    } else {
      c->entries[i].timestamp = c->entries[i].timestamp + dts > 0 ? c->entries[i].timestamp + dts : 0;
    }
  }
}

void cache_update(struct peer_cache *c)
{
  cache_delay(c, 1);
}


struct peer_cache *cache_init(int n, int metadata_size, int max_timestamp)
{
  struct peer_cache *res;

  res = malloc(sizeof(struct peer_cache));
  if (res == NULL) {
    return NULL;
  }
  res->max_timestamp = max_timestamp;
  res->cache_size = n;
  res->current_size = 0;
  res->entries = malloc(sizeof(struct cache_entry) * n);
  if (res->entries == NULL) {
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

struct peer_cache *cache_copy(const struct peer_cache *c1)
{
  int n;
  struct peer_cache *new_cache;

  new_cache = cache_init(c1->current_size, c1->metadata_size, c1->max_timestamp);
  if (new_cache == NULL) {
    return NULL;
  }

  for (n = 0; n < c1->current_size; n++) {
    new_cache->entries[new_cache->current_size].id = nodeid_dup(c1->entries[n].id);
    new_cache->entries[new_cache->current_size++].timestamp = c1->entries[n].timestamp;
  }
  if (new_cache->metadata_size) {
    memcpy(new_cache->metadata, c1->metadata, c1->metadata_size * c1->current_size);
  }

  return new_cache;
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

int cache_pos(const struct peer_cache *c, const struct nodeID *n)
{
  int i;

  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, n)) {
      return i;
    }
  }

  return -1;
}

static int in_cache(const struct peer_cache *c, const struct cache_entry *elem)
{
  return cache_pos(c, elem->id);
}

struct nodeID *rand_peer(const struct peer_cache *c, void **meta, int max)
{
  int j;

  if (c->current_size == 0) {
    return NULL;
  }
  if (!max || max >= c->current_size)
    max = c->current_size;
  else
    ++max;
  j = ((double)rand() / (double)RAND_MAX) * max;

  if (meta) {
    *meta = c->metadata + (j * c->metadata_size);
  }

  return c->entries[j].id;
}

struct nodeID *last_peer(const struct peer_cache *c)
{
  if (c->current_size == 0) {
    return NULL;
  }

  return c->entries[c->current_size - 1].id;
}


int cache_fill_ordered(struct peer_cache *dst, const struct peer_cache *src, int target_size)
{
  struct cache_entry *e_orig, e_dup;
  int count, j, err;
cache_check(dst);
cache_check(src);
  if (target_size <= 0 || target_size > dst->cache_size) {
    target_size = dst->cache_size;
  }

  if (dst->current_size >= target_size) return -1;

  count = 0;
  j=0;
  while(dst->current_size < target_size && src->current_size > count) {
    count++;

    e_orig = src->entries + j;
    e_dup.id = nodeid_dup(e_orig->id);
    e_dup.timestamp = e_orig->timestamp;

    err = cache_insert(dst, &e_dup, src->metadata + src->metadata_size * j);
    if (err == -1) {
      /* Cache entry is fresher */
      nodeid_free(e_dup.id);
    }

    j++;
  }
cache_check(dst);
cache_check(src);
 return dst->current_size;
}

int cache_fill_rand(struct peer_cache *dst, const struct peer_cache *src, int target_size)
{
  int added[src->current_size];
  struct cache_entry *e_orig, e_dup;
  int count, j, err;
cache_check(dst);
cache_check(src);
  if (target_size <= 0 || target_size > dst->cache_size) {
    target_size = dst->cache_size;
  }

  if (dst->current_size >= target_size) return -1;

  memset(added, 0, sizeof(int) * src->current_size);
  count = 0;
  while(dst->current_size < target_size && src->current_size > count) {
    j = ((double)rand() / (double)RAND_MAX) * src->current_size;
    if (added[j] != 0) continue;
    added[j] = 1;
    count++;

    e_orig = src->entries + j;

    e_dup.id = nodeid_dup(e_orig->id);
    e_dup.timestamp = e_orig->timestamp;

    err = cache_insert(dst, &e_dup, src->metadata + src->metadata_size * j);
    if (err == -1) {
      /* Cache entry is fresher */
      nodeid_free(e_dup.id);
    }
  }
cache_check(dst);
cache_check(src);
 return dst->current_size;
}


struct peer_cache *rand_cache_except(struct peer_cache *c, int n,
                                     struct nodeID *except[], int len)
{
  struct peer_cache *res;
  struct cache_entry *e;
  int present[len];
  int count = 0;

cache_check(c);

 memset(present, 0, sizeof(int) * len);

  if (c->current_size < n) {
    n = c->current_size;
  }
  res = cache_init(n, c->metadata_size, c->max_timestamp);
  while(res->current_size < (n - count)) {
    int j,i,flag;

    j = ((double)rand() / (double)RAND_MAX) * c->current_size;
    e = c->entries +j;

    flag = 0;
    for (i=0; i<len; i++) {
      if (nodeid_equal(e->id, except[i])) {
        if (present[i] == 0) {
          count++;
          present[i] = 1;
        }
        flag = 1;
        break;
      }
    }
    if (flag) continue;

    cache_insert(res, c->entries + j, c->metadata + c->metadata_size * j);
    c->current_size--;
    memmove(c->entries + j, c->entries + j + 1, sizeof(struct cache_entry) * (c->current_size - j));
    memmove(c->metadata + c->metadata_size * j, c->metadata + c->metadata_size * (j + 1), c->metadata_size * (c->current_size - j));
    c->entries[c->current_size].id = NULL;
cache_check(c);
  }

  return res;
}

struct peer_cache *rand_cache(struct peer_cache *c, int n)
{
  struct peer_cache *res;

cache_check(c);
  if (c->current_size < n) {
    n = c->current_size;
  }
  res = cache_init(n, c->metadata_size, c->max_timestamp);

  while(res->current_size < n) {
    int j;

    j = ((double)rand() / (double)RAND_MAX) * c->current_size;
    cache_insert(res, c->entries + j, c->metadata + c->metadata_size * j);
    c->current_size--;
    memmove(c->entries + j, c->entries + j + 1, sizeof(struct cache_entry) * (c->current_size - j));
    memmove(c->metadata + c->metadata_size * j, c->metadata + c->metadata_size * (j + 1), c->metadata_size * (c->current_size - j));
    c->entries[c->current_size].id = NULL;
cache_check(c);
  }

  return res;
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
  res = cache_init(cache_size, metadata_size, 0);
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
  assert(p - buff == size);

  return res;
}

int cache_header_dump(uint8_t *b, const struct peer_cache *c, int include_me)
{
  int_cpy(b, c->cache_size + (include_me ? 1 : 0));
  int_cpy(b + 4, c->metadata_size);

  return 8;
}

int entry_dump(uint8_t *b, const struct peer_cache *c, int i, size_t max_write_size)
{
  int res;
  int size = 0;
 
  if (i && (i >= c->cache_size - 1)) {
    return 0;
  }
  int_cpy(b, c->entries[i].timestamp);
  size = +4;
  res = nodeid_dump(b + size, c->entries[i].id, max_write_size - size);
  if (res < 0 ) {
    return -1;
  }
  size += res;
  if (c->metadata_size) {
    if (c->metadata_size > max_write_size - size) {
      return -1;
    }
    memcpy(b + size, c->metadata + c->metadata_size * i, c->metadata_size);
    size += c->metadata_size;
  }

  return size;
}

struct peer_cache *cache_rank (const struct peer_cache *c, ranking_function rank, const struct nodeID *target, const void *target_meta)
{
  struct peer_cache *res;
  int i,j,pos;

  res = cache_init(c->cache_size, c->metadata_size, c->max_timestamp);
  if (res == NULL) {
    return res;
  }

  for (i = 0; i < c->current_size; i++) {
    if (!target || !nodeid_equal(c->entries[i].id,target)) {
      pos = 0;
      for (j=0; j<res->current_size;j++) {
        if (((rank != NULL) && rank(target_meta, c->metadata+(c->metadata_size * i), res->metadata+(res->metadata_size * j)) == 2) ||
            ((rank == NULL) && res->entries[j].timestamp < c->entries[i].timestamp)) {
          pos++;
        }
      }
      if (c->metadata_size) {
        memmove(res->metadata + (pos + 1) * res->metadata_size, res->metadata + pos * res->metadata_size, (res->current_size - pos) * res->metadata_size);
        memcpy(res->metadata + pos * res->metadata_size, c->metadata+(c->metadata_size * i), res->metadata_size);
      }
      for (j = res->current_size; j > pos; j--) {
        res->entries[j] = res->entries[j - 1];
      }
      res->entries[pos].id = nodeid_dup(c->entries[i].id);
      res->entries[pos].timestamp = c->entries[i].timestamp;
      res->current_size++;
    }
  }

  return res;
}

struct peer_cache *cache_union(const struct peer_cache *c1, const struct peer_cache *c2, int *size)
{
  int n, pos;
  struct peer_cache *new_cache;
  uint8_t *meta;

  if (c1->metadata_size != c2->metadata_size) {
    return NULL;
  }

  new_cache = cache_init(c1->current_size + c2->current_size, c1->metadata_size, c1->max_timestamp);
  if (new_cache == NULL) {
    return NULL;
  }

  meta = new_cache->metadata;

  for (n = 0; n < c1->current_size; n++) {
    if (new_cache->metadata_size) {
      memcpy(meta, c1->metadata + n * c1->metadata_size, c1->metadata_size);
      meta += new_cache->metadata_size;
    }
    new_cache->entries[new_cache->current_size++] = c1->entries[n];
    c1->entries[n].id = NULL;
  }
  
  for (n = 0; n < c2->current_size; n++) {
    pos = in_cache(new_cache, &c2->entries[n]);
    if (pos >= 0 && new_cache->entries[pos].timestamp > c2->entries[n].timestamp) {
      cache_metadata_update(new_cache, c2->entries[n].id, c2->metadata + n * c2->metadata_size, c2->metadata_size);
      new_cache->entries[pos].timestamp = c2->entries[n].timestamp;
    }
    if (pos < 0) {
      if (new_cache->metadata_size) {
        memcpy(meta, c2->metadata + n * c2->metadata_size, c2->metadata_size);
        meta += new_cache->metadata_size;
      }
      new_cache->entries[new_cache->current_size++] = c2->entries[n];
      c2->entries[n].id = NULL;
    }
  }
  *size = new_cache->current_size;

  return new_cache;
}


int cache_max_size(const struct peer_cache *c)
{
  return c->cache_size;
}

int cache_current_size(const struct peer_cache *c)
{
  return c->current_size;
}

int cache_resize (struct peer_cache *c, int size)
{
  int dif = size - c->cache_size;

  if (!dif) {
    return c->current_size;
  }

  c->entries = realloc(c->entries, sizeof(struct cache_entry) * size);
  if (dif > 0) {
    memset(c->entries + c->cache_size, 0, sizeof(struct cache_entry) * dif);
  } else if (c->current_size > size) {
    c->current_size = size;
  }

  if (c->metadata_size) {
    c->metadata = realloc(c->metadata, c->metadata_size * size);
    if (dif > 0) {
      memset(c->metadata + c->metadata_size * c->cache_size, 0, c->metadata_size * dif);
    }
  }

  c->cache_size = size;

  return c->current_size;
}
  
struct peer_cache *merge_caches(const struct peer_cache *c1, const struct peer_cache *c2, int newsize, int *source)
{
  int n1, n2;
  struct peer_cache *new_cache;
  uint8_t *meta;

  new_cache = cache_init(newsize, c1->metadata_size, c1->max_timestamp);
  if (new_cache == NULL) {
    return NULL;
  }

  meta = new_cache->metadata;
  *source = 0;
  for (n1 = 0, n2 = 0; new_cache->current_size < new_cache->cache_size;) {
    if ((n1 == c1->current_size) && (n2 == c2->current_size)) {
      return new_cache;
    }
    if (n1 == c1->current_size) {
      if (in_cache(new_cache, &c2->entries[n2]) < 0) {
        if (new_cache->metadata_size) {
          memcpy(meta, c2->metadata + n2 * c2->metadata_size, c2->metadata_size);
          meta += new_cache->metadata_size;
        }
        new_cache->entries[new_cache->current_size++] = c2->entries[n2];
        c2->entries[n2].id = NULL;
        *source |= 0x02;
      }
      n2++;
    } else if (n2 == c2->current_size) {
      if (in_cache(new_cache, &c1->entries[n1]) < 0) {
        if (new_cache->metadata_size) {
          memcpy(meta, c1->metadata + n1 * c1->metadata_size, c1->metadata_size);
          meta += new_cache->metadata_size;
        }
        new_cache->entries[new_cache->current_size++] = c1->entries[n1];
        c1->entries[n1].id = NULL;
        *source |= 0x01;
      }
      n1++;
    } else {
      if (c2->entries[n2].timestamp > c1->entries[n1].timestamp) {
        if (in_cache(new_cache, &c1->entries[n1]) < 0) {
          if (new_cache->metadata_size) {
            memcpy(meta, c1->metadata + n1 * c1->metadata_size, c1->metadata_size);
            meta += new_cache->metadata_size;
          }
          new_cache->entries[new_cache->current_size++] = c1->entries[n1];
          c1->entries[n1].id = NULL;
          *source |= 0x01;
        }
        n1++;
      } else {
        if (in_cache(new_cache, &c2->entries[n2]) < 0) {
          if (new_cache->metadata_size) {
            memcpy(meta, c2->metadata + n2 * c2->metadata_size, c2->metadata_size);
            meta += new_cache->metadata_size;
          }
          new_cache->entries[new_cache->current_size++] = c2->entries[n2];
          c2->entries[n2].id = NULL;
          *source |= 0x02;
        }
        n2++;
      }
    }
  }

  return new_cache;
}

static int swap_entries(const struct peer_cache *c, int i, int j)
{
  struct cache_entry t;
  uint8_t *metadata;

  if (i == j) {
    return 1;
  }

  if (c->metadata_size) {
    metadata = malloc(c->metadata_size);
    if (! metadata) {
      return -1;
    }

    memcpy(metadata,                           c->metadata + i * c->metadata_size, c->metadata_size);
    memcpy(c->metadata + i * c->metadata_size, c->metadata + j * c->metadata_size, c->metadata_size);
    memcpy(c->metadata + j * c->metadata_size, metadata,                           c->metadata_size);

    free(metadata);
  }

  t = c->entries[i];
  c->entries[i] = c->entries[j];
  c->entries[j] = t;

  return 1;
}

void cache_randomize(const struct peer_cache *c)
{
  int i;

  for (i = 0; i < c->current_size - 1; i++) {
    int j, k;
    uint32_t ts = c->entries[i].timestamp;

    for (j = i + 1; j < c->current_size; j++) {
      if (c->entries[j].timestamp != ts) {
        break;
      }
    }

    //permutate entries from i to j-1
    if (j - 1 > i) {
      for (k = i; k < j - 1; k++) {
        int r;
        r = (rand() / (RAND_MAX + 1.0)) * (j - k);
        swap_entries(c, k, k + r);
      }
    }

    i = j - 1;
  }
}

void cache_check(const struct peer_cache *c)
{
  int i, j;
  int ts = 0;

  for (i = 0; i < c->current_size; i++) {
    if (c->entries[i].timestamp < ts) {
      fprintf(stderr, "WTF!!!! %d.ts=%d > %d.ts=%d!!!\n",
              i-1, ts, i, c->entries[i].timestamp);
      *((char *)0) = 1;
    }
    ts = c->entries[i].timestamp;
    for (j = i + 1; j < c->current_size; j++) {
      assert(!nodeid_equal(c->entries[i].id, c->entries[j].id));
    }
  }
}

int cache_entries(const struct peer_cache *c)
{
  return c->current_size;
}

void cache_log(const struct peer_cache *c, const char *name){
  char addr[256];
  int i;
  const char default_name[] = "none";
  const char *actual_name = (name)? name : default_name;
  fprintf(stderr, "### dumping cache (%s)\n", actual_name);
  fprintf(stderr, "\tcache_size=%d, current_size=%d\n", c->cache_size, c->current_size);
  for (i = 0; i < c->current_size; i++) {
    node_addr(c->entries[i].id, addr, 256);
    fprintf(stderr, "\t%d: %s[%d]\n", i, addr, c->entries[i].timestamp);
  }
  fprintf(stderr, "\n-----------------------------\n");
}
