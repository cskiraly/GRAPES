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
#include "topocache.h"

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

int cache_add_ranked(struct peer_cache *c, struct nodeID *neighbour, const void *meta, int meta_size, ranking_function f, const void *tmeta)
{
  int i, pos = 0;

  if (meta_size && meta_size != c->metadata_size) {
    return -3;
  }
  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, neighbour)) {
      if (f != NULL) {
        cache_del(c,c->entries[i].id);
        if (i == c->current_size) break;
      } else return -1;
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

int cache_del(struct peer_cache *c, struct nodeID *neighbour)
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

void cache_update_tout(struct peer_cache *c)
{
  int i;
  
  for (i = 0; i < c->current_size; i++) {
    if (c->entries[i].timestamp == MAX_TIMESTAMP) {
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
      c->entries[i].timestamp++;
    }
  }
}

void cache_update(struct peer_cache *c)
{
  int i;
  
  for (i = 0; i < c->current_size; i++) {
      c->entries[i].timestamp++;
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

static int in_cache(const struct peer_cache *c, const struct cache_entry *elem)
{
  int i;

  for (i = 0; i < c->current_size; i++) {
    if (nodeid_equal(c->entries[i].id, elem->id)) {
      return i;
    }
  }

  return -1;
}

struct nodeID *rand_peer(struct peer_cache *c, void **meta)
{
  int j;

  if (c->current_size == 0) {
    return NULL;
  }
  j = ((double)rand() / (double)RAND_MAX) * c->current_size;

  if (meta) {
    *meta = c->metadata + (j * c->metadata_size);
  }

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

struct peer_cache *cache_rank (const struct peer_cache *c, ranking_function rank, const struct nodeID *target, const void *target_meta)
{
	struct peer_cache *res;
	int i,j,pos;

	res = cache_init(c->cache_size,c->metadata_size);
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

struct peer_cache *cache_union(struct peer_cache *c1, struct peer_cache *c2, int *size) {
	int n,pos;
	struct peer_cache *new_cache;
	uint8_t *meta;

	if (c1->metadata_size != c2->metadata_size) {
		return NULL;
	}

	*size = c1->current_size + c2->current_size;
	new_cache = cache_init(*size, c1->metadata_size);
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
		if (pos >= 0) {
			if (new_cache->entries[pos].timestamp > c2->entries[n].timestamp) {
				cache_del(new_cache,new_cache->entries[pos].id);
				meta -= new_cache->metadata_size;
				pos = -1;
			}
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

	return new_cache;
}

int cache_resize (struct peer_cache *c, int size) {

	int dif = size - c->cache_size;
	if (!dif) {
		return c->current_size;
	}

	c->entries = realloc(c->entries, sizeof(struct cache_entry) * size);
	if (dif > 0) {
		memset(c->entries + sizeof(struct cache_entry) * c->cache_size, 0, sizeof(struct cache_entry) * dif);
	}
	else if (c->current_size > size) {
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
  


struct peer_cache *merge_caches_ranked(struct peer_cache *c1, struct peer_cache *c2, int newsize, int *source, ranking_function rank, void *mymetadata)
{
  int n1, n2;
  struct peer_cache *new_cache;
  uint8_t *meta;

  new_cache = cache_init(newsize, c1->metadata_size);
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
      int nowFirst;

      nowFirst = 0;
      if (rank) {
        nowFirst = rank(mymetadata, c1->metadata + n1 * c1->metadata_size,
                        c2->metadata + n2 * c2->metadata_size);
      }
      if (nowFirst == 0) {
        nowFirst = c2->entries[n2].timestamp > c1->entries[n1].timestamp ? 1 : 2;
      }
      if (nowFirst == 1) {
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

struct peer_cache *merge_caches(struct peer_cache *c1, struct peer_cache *c2, int newsize, int *source)
{
  return merge_caches_ranked(c1, c2, newsize, source, NULL, NULL);
}
