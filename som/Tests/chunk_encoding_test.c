/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "chunk.h"
#include "trade_msg_la.h"

static void chunk_print(FILE *f, const struct chunk *c)
{
  const uint8_t *p;

  fprintf(f, "Chunk %d:\n", c->id);
  fprintf(f, "\tTS: %"PRIu64"\n", c->timestamp);
  fprintf(f, "\tPayload size: %d\n", c->size);
  fprintf(f, "\tAttributes size: %d\n", c->attributes_size);
  p = c->data;
  fprintf(f, "\tPayload:\n");
  fprintf(f, "\t\t%c %c %c %c ...:\n", p[0], p[1], p[2], p[3]);
  if (c->attributes_size > 0) {
    p = c->attributes;
    fprintf(f, "\tAttributes:\n");
    fprintf(f, "\t\t%c %c %c %c ...:\n", p[0], p[1], p[2], p[3]);
  }
}

int main(int argc, char *argv[])
{
  struct chunk src_c;
  struct chunk dst_c;
  uint8_t buff[100];
  int res;

  src_c.id = 666;
  src_c.timestamp = 1000000000ULL;
  src_c.size = strlen("ciao") + 1;
  src_c.data = strdup("ciao");
  src_c.attributes_size = 0;

  chunk_print(stdout, &src_c);

  res = encodeChunk(&src_c, buff, 15);
  fprintf(stdout, "Encoding in 15 bytes: %d\n", res);
  res = encodeChunk(&src_c, buff, 23);
  fprintf(stdout, "Encoding in 23 bytes: %d\n", res);
  res = encodeChunk(&src_c, buff, sizeof(buff));
  fprintf(stdout, "Encoding in %d bytes: %d\n", sizeof(buff), res);
  free(src_c.data);

  res = decodeChunk(&dst_c, buff, res);
  fprintf(stdout, "Decoding it: %d\n", res);
  chunk_print(stdout, &dst_c);
  free(dst_c.data);

  return 0;
}
