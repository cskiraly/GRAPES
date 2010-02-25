/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "chunk.h"
#include "chunkbuffer.h"

const struct chunk *cb_get_chunk(const struct chunk_buffer *cb, int id)
{
  int i, n;
  const struct chunk *buffer;

  buffer = cb_get_chunks(cb, &n);
  if (buffer == NULL) {
    return NULL;
  }

  for (i = 0; i < n; i++) {
    if (buffer[i].id == id) {
      return &buffer[i];
    }
  }

  return NULL;
}
