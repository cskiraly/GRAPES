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

#include "int_coding.h"
#include "payload.h"
#include "config.h"
#include "dechunkiser_iface.h"

enum pt {
  raw,
  avf,
  udp,
  rtp,
};

struct dechunkiser_ctx {
  int fd;
  enum pt payload_type;
};

static struct dechunkiser_ctx *raw_open(const char *fname, const char *config)
{
  struct dechunkiser_ctx *res;
  struct tag *cfg_tags;

  res = malloc(sizeof(struct dechunkiser_ctx));
  if (res == NULL) {
    return NULL;
  }
  res->fd = 1;
  res->payload_type = raw;
  if (fname) {
#ifndef _WIN32
    res->fd = open(fname, O_WRONLY | O_CREAT, S_IROTH | S_IWUSR | S_IRUSR);
#else
    res->fd = open(fname, O_WRONLY | O_CREAT);
#endif

    if (res->fd < 0) {
      res->fd = 1;
    }
  }
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *pt;

    pt = config_value_str(cfg_tags, "payload");
    if (pt) {
      if (!strcmp(pt, "avf")) {
        res->payload_type = avf;
      } else if (!strcmp(pt, "udp")) {
        res->payload_type = udp;
      } else if (!strcmp(pt, "rtp")) {
        res->payload_type = rtp;
      }
    }
  }
  free(cfg_tags);

  return res;
}

static void raw_write(struct dechunkiser_ctx *o, int id, uint8_t *data, int size)
{
  int offset;

  if (o->payload_type == avf) {
    int header_size;
    int frames;
    int i;
    uint8_t codec;

    if (data[0] == 0) {
      fprintf(stderr, "Error! Strange chunk: %x!!!\n", codec);
      return;
    } else if (data[0] < 127) {
      int width, height, frame_rate_n, frame_rate_d;

      header_size = VIDEO_PAYLOAD_HEADER_SIZE;
      video_payload_header_parse(data, &codec, &width, &height, &frame_rate_n, &frame_rate_d);
//    dprintf("Frame size: %dx%d -- Frame rate: %d / %d\n", width, height, frame_rate_n, frame_rate_d);
    } else {
      uint8_t channels;
      int sample_rate, frame_size;

      header_size = AUDIO_PAYLOAD_HEADER_SIZE;
      audio_payload_header_parse(data, &codec, &channels, &sample_rate, &frame_size);
//    dprintf("Frame size: %d Sample rate: %d Channels: %d\n", frame_size, sample_rate, channels);
    }

    frames = data[header_size - 1];
    for (i = 0; i < frames; i++) {
      int frame_size;
      int64_t pts, dts;

      frame_header_parse(data, &frame_size, &pts, &dts);
//      dprintf("Frame %d has size %d\n", i, frame_size);
    }
    offset = header_size + frames * FRAME_HEADER_SIZE;
  } else if (o->payload_type == udp) {
    offset = UDP_PAYLOAD_HEADER_SIZE;
  } else if (o->payload_type == rtp) {
    offset = UDP_PAYLOAD_HEADER_SIZE + 12;
  } else {
    offset = 0;
  }

  write(o->fd, data + offset, size - offset);
}

static void raw_close(struct dechunkiser_ctx *s)
{
  close(s->fd);
  free(s);
}

struct dechunkiser_iface out_raw = {
  .open = raw_open,
  .write = raw_write,
  .close = raw_close,
};
