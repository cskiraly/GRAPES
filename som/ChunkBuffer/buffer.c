#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "chunk.h"
#include "chunkbuffer.h"
#include "config.h"

struct chunk_buffer {
  int size;
  int num_chunks;
  struct chunk *buffer;
};

static void insert_sort(struct chunk *b, int size)
{
  int i, j;
  struct chunk tmp;

  for(i = 1; i < size; i++) {
    tmp = b[i];
    j = i-1;
    while(tmp.id < b[j].id && j >= 0) {
      b[j + 1] = b[j];
      j = j - 1;
    }
    b[j + 1] = tmp;
  }
}



static void chunk_free(struct chunk *c)
{
    free(c->data);
    free(c->attributes);
    c->id = -1;
}

static int remove_oldest_chunk(struct chunk_buffer *cb, int id)
{
  int i, min, pos_min;

  min = cb->buffer[0].id; pos_min = 0;
  for (i = 1; i < cb->num_chunks; i++) {
    if (cb->buffer[i].id < min) {
      min = cb->buffer[i].id;
      pos_min = i;
    }
  }
  if (min < id) {
    chunk_free(&cb->buffer[pos_min]);

    return pos_min;
  }

  return -1;
}

struct chunk_buffer *cb_init(const char *config)
{
  struct tag *cfg_tags;
  struct chunk_buffer *cb;
  int res, i;

  cb = malloc(sizeof(struct chunk_buffer));
  if (cb == NULL) {
    return cb;
  }
  memset(cb, 0, sizeof(struct chunk_buffer));

  cfg_tags = config_parse(config);
  res = config_value_int(cfg_tags, "size", &cb->size);
  if (!res) {
    free(cb);
    free(cfg_tags);

    return NULL;
  }
  free(cfg_tags);

  cb->buffer = malloc(sizeof(struct chunk) * cb->size);
  if (cb->buffer == NULL) {
    free(cb);
    return NULL;
  }
  for (i = 0; i < cb->size; i++) {
    cb->buffer[i].id = -1;
  }

  return cb;
}

int cb_add_chunk(struct chunk_buffer *cb, const struct chunk *c)
{
  int i;

  if (cb->num_chunks == cb->size) {
    i = remove_oldest_chunk(cb, c->id);
  } else {
    i = 0;
  }
  
  while(1) {
    if (cb->buffer[i].id < 0) {
      cb->buffer[i] = *c;
      cb->num_chunks++;

      return 0; 
    }
    i++;
  }
}

struct chunk *cb_get_chunks(const struct chunk_buffer *cb, int *n)
{
  *n = cb->num_chunks;
  if (*n == 0) {
    return NULL;
  }

  insert_sort(cb->buffer, cb->size);

  return cb->buffer;
}

int cb_clear(struct chunk_buffer *cb)
{
  int i;

  cb->num_chunks = 0;
  for (i = 0; i < cb->size; i++) {
    chunk_free(&cb->buffer[i]);
  }

  return 0;
}
