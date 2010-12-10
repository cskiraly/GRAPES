/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "dechunkiser_iface.h"

struct output_stream {
  int fd;
};

static struct output_stream *raw_open(const char *config)
{
  struct output_stream *res;

  res = malloc(sizeof(struct output_stream));
  if (res == NULL) {
    return NULL;
  }
  res->fd = 1;

  return res;
}

static void raw_write(struct output_stream *o, int id, uint8_t *data, int size)
{
  write(o->fd, data, size);
}

struct dechunkiser_iface out_raw = {
  .open = raw_open,
  .write = raw_write,
};
