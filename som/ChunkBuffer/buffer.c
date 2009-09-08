#include <stdint.h>

#include "chunk.h"
#include "chunkbuffer.h"
#include "config.h"

struct chunk_buffer {
  int size;
  int num_chunks;
  struct chunk **buffer;
};

struct chunk_buffer *cb_init(const char *config);
{
  struct tag *cfg_tags;
  struct chunk_buffer *cb;

  cb = malloc(sizeof(struct chunk_buffer))
  if (cb == NULL) {
    return cb;
  }

  cfg_tags = config_parse(config);
  cb->size = config_value_int(cfg_tags, "size);
  if (cb->size <= 0) {
    free(cb);
    return NULL;
  }
  free(cfg_tags);

  cb->buffer = malloc(sizeof(struct chunk *));
  if (cb->buffer == NULL) {
    free(cb);
    return NULL;
  }

  return cb;
}

int cb_add_chunk(struct chunk_buffer *cb, const struct chunk *c)
{
  int i;

  if (cb->num_chunks == cb->size) {
    remove_oldest_chunk(cb);
  }
  
  i = 0;
  while(1) {
    if (cb->buffer[i] == NULL) {
      cb->buffer[i] = c;
      cb-> size++;

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

  bubble_sort(cb->buffer, cb->size);

  return cb->buffer;
}

int cb_clear(struct chunk_buffer *cb);
{
  cb->num_chunks = 0;
  for (i = 0; i < cb->size; i++) {
    chunk_free(cb->chunk[i]);
    cb->chunk[i] = NULL;
  }

  return 0;
}
