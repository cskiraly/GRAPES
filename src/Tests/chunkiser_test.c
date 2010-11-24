/*
 *  Copyright (c) 2010 Csaba Kiraly
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include "chunkiser.h"

int main(int argc, char *argv[])
{
  int period, done, id;
  struct input_stream *input;
  struct output_stream *output;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);

    return -1;
  }
  input = input_stream_open(argv[1], &period, NULL);
  if (input == NULL) {
    fprintf(stderr, "Cannot open input %s\n", argv[1]);

    return -1;
  }
  output = out_stream_init(argv[2]);
  if (output == NULL) {
    fprintf(stderr, "Cannot open output %s\n", argv[2]);

    return -1;
  }

  done = 0; id = 0;
  while(!done) {
    int size;
    uint64_t ts;
    uint8_t *c;

    c = chunkise(input, id, &size, &ts);
    if (c) {
      fprintf(stderr,"chunk %d: %d %llu\n", id++, size, ts);
      chunk_write(output, id, c, size);
    } else if (size < 0) {
      done = 1;
    }
  }

  return 0;
}
