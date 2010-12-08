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

#include "chunk.h"
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
    int res;
    struct chunk c;

    c.id = id;
    res = chunkise(input, &c);
    if (res > 0) {
      fprintf(stderr,"chunk %d: %d %llu\n", id++, c.size, c.timestamp);
      chunk_write(output, c.id, c.data, c.size);
    } else if (res < 0) {
      done = 1;
    }
    free(c.data);
  }
  input_stream_close(input);

  return 0;
}
