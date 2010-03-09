/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chunk.h"
#include "chunkbuffer.h"

static struct chunk *chunk_forge(int id)
{
  struct chunk *c;
  char buff[64];

  c = malloc(sizeof(struct chunk));
  if (c == NULL) {
    return c;
  }

  sprintf(buff, "Chunk %d", id);
  c->id = id;
  c->timestamp = 40 * id;
  c->data = strdup(buff);
  c->size = strlen(c->data) + 1;
  c->attributes_size = 0;
  c->attributes = NULL;
  return c;
}

static void chunk_add(struct chunk_buffer *cb, int id)
{
  struct chunk *c;
  int res;

  printf("Inserting %d... ", id);
  c = chunk_forge(id);
  if (c) {
    res = cb_add_chunk(cb, c);
    if (res < 0) {
      printf("not inserted (out of window)");
      free(c->data);
      free(c->attributes);
    }
  } else {
    printf("Failed to create the chunk");
  }
  printf("\n");
  free(c);
}

static void cb_print(const struct chunk_buffer *cb)
{
  struct chunk *buff;
  int i, size;

  buff = cb_get_chunks(cb, &size);
  for (i = 0; i < size; i++) {
    printf("C[%d]: %s %d\n", i, buff[i].data, buff[i].id);
  }
}

int main(int argc, char *argv[])
{
  struct chunk_buffer *b;

  b = cb_init("size=8,time=now");
  if (b == NULL) {
    printf("Error initialising the Chunk Buffer\n");

    return -1;
  }
  chunk_add(b, 10);
  chunk_add(b, 5);
  chunk_add(b, 12);
  chunk_add(b, 12);
  chunk_add(b, 40);
  cb_print(b);

  chunk_add(b, 51);
  chunk_add(b, 2);
  chunk_add(b, 13);
  chunk_add(b, 11);
  cb_print(b);

  chunk_add(b, 26);
  cb_print(b);
  chunk_add(b, 30);
  cb_print(b);
  chunk_add(b, 110);
  cb_print(b);
  chunk_add(b, 64);
  chunk_add(b, 4);
  cb_print(b);
  chunk_add(b, 7);
  chunk_add(b, 34);
  chunk_add(b, 2);
  chunk_add(b, 33);
  cb_print(b);

  cb_destroy(b);

  return 0;
}
