/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "payload.h"
#include "config.h"
#include "dechunkiser_iface.h"

struct output_stream {
  int fd;
  int payload_type;
};

static struct output_stream *raw_open(const char *fname, const char *config)
{
  struct output_stream *res;
  struct tag *cfg_tags;

  res = malloc(sizeof(struct output_stream));
  if (res == NULL) {
    return NULL;
  }
  res->fd = 1;
  res->payload_type = 0;
  if (fname) {
    res->fd = open(fname, O_WRONLY | O_CREAT, S_IROTH | S_IWUSR | S_IRUSR);
    if (res->fd < 0) {
      res->fd = 1;
    }
  }
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *pt;

    pt = config_value_str(cfg_tags, "payload");
    if (pt) {
      if (strcmp(pt, "avf")) {
        res->payload_type = 1;
      }
    }
  }

  return res;
}

static void raw_write(struct output_stream *o, int id, uint8_t *data, int size)
{
  int offset;

  if (o->payload_type == 1) {
    const int header_size = VIDEO_PAYLOAD_HEADER_SIZE;
    int width, height, frame_rate_n, frame_rate_d, frames;
    int i;
    uint8_t codec;

    payload_header_parse(data, &codec, &width, &height, &frame_rate_n, &frame_rate_d);
    if (codec != 1) {
      fprintf(stderr, "Error! Non video chunk: %x!!!\n", codec);
      return;
    }
//    dprintf("Frame size: %dx%d -- Frame rate: %d / %d\n", width, height, frame_rate_n, frame_rate_d);
    frames = data[9];
    for (i = 0; i < frames; i++) {
      int frame_size;
      int64_t pts, dts;

      frame_header_parse(data, &frame_size, &pts, &dts);
//      dprintf("Frame %d has size %d\n", i, frame_size);
    }
    offset = header_size + frames * FRAME_HEADER_SIZE;
  } else {
    offset = 0;
  }

  write(o->fd, data + offset, size - offset);
}

struct dechunkiser_iface out_raw = {
  .open = raw_open,
  .write = raw_write,
};
