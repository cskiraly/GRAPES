/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chunkiser_iface.h"
#include "config.h"

struct chunkiser_ctx {
  int loop;	//loop on input file infinitely
  int chunk_size;
  int fds[2];
};
#define DEFAULT_CHUNK_SIZE 2 * 1024

static struct chunkiser_ctx *dumb_open(const char *fname, int *period, const char *config)
{
  struct tag *cfg_tags;
  struct chunkiser_ctx *res;

  res = malloc(sizeof(struct chunkiser_ctx));
  if (res == NULL) {
    return NULL;
  }

  res->loop = 0;
  res->fds[0] = open(fname, O_RDONLY);
  if (res->fds[0] < 0) {
    free(res);

    return NULL;
  }
  res->fds[1] = -1;

  *period = 0;
  res->chunk_size = DEFAULT_CHUNK_SIZE;
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *access_mode;

    config_value_int(cfg_tags, "loop", &res->loop);
    config_value_int(cfg_tags, "chunk_size", &res->chunk_size);
    access_mode = config_value_str(cfg_tags, "mode");
    if (access_mode && !strcmp(access_mode, "nonblock")) {
#ifndef _WIN32
      fcntl(res->fds[0], F_SETFL, O_NONBLOCK);
#else
      fprintf(stderr, "nonblock is not implemented yet\n");
#endif
    }
  }
  free(cfg_tags);

  return res;
}

static void dumb_close(struct chunkiser_ctx *s)
{
  close(s->fds[0]);
  free(s);
}

static uint8_t *dumb_chunkise(struct chunkiser_ctx *s, int id, int *size, uint64_t *ts)
{
  uint8_t *res;

  res = malloc(s->chunk_size);
  if (res == NULL) {
    *size = -1;

    return NULL;
  }
  *ts = 0;
  *size = read(s->fds[0], res, s->chunk_size);
  if ((*size == 0) && (errno != EAGAIN)) {
    *size = -1;
    if (s->loop) {
      if (lseek(s->fds[0], 0, SEEK_SET) == 0) {
        *size = 0;
      }
    }
    free(res);
    res = NULL;
  }

  return res;
}

const int *dumb_get_fds(const struct chunkiser_ctx *s)
{
  return s->fds;
}

struct chunkiser_iface in_dumb = {
  .open = dumb_open,
  .close = dumb_close,
  .chunkise = dumb_chunkise,
  .get_fds = dumb_get_fds,
};


