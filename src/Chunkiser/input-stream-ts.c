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
#include <string.h>
#include <stdio.h>

#include "chunkiser_iface.h"
#include "config.h"

struct chunkiser_ctx {
  int loop;	//loop on input file infinitely
  int pkts_per_chunk;
  int fds[2];
};
#define DEFAULT_PKTS 512

static void ts_resync(uint8_t *buff, int *size)
{
  uint8_t *p = buff;

  fprintf(stderr, "Resynch!\n");
  while((p - buff < *size) && (*p != 0x47)) {
    p++;
  }
  if (p - buff < *size) {
    memmove(buff, p, *size - (p - buff));
    *size -= (p - buff);
  } else {
    *size = 0;
  }
}

static struct chunkiser_ctx *ts_open(const char *fname, int *period, const char *config)
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
  res->pkts_per_chunk = DEFAULT_PKTS;
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *access_mode;

    config_value_int(cfg_tags, "loop", &res->loop);
    config_value_int(cfg_tags, "pkts", &res->pkts_per_chunk);
    access_mode = config_value_str(cfg_tags, "mode");
    if (access_mode && !strcmp(access_mode, "nonblock")) {
      fcntl(res->fds[0], F_SETFL, O_NONBLOCK);
    }
  }
  free(cfg_tags);

  return res;
}

static void ts_close(struct chunkiser_ctx *s)
{
  close(s->fds[0]);
  free(s);
}

static uint8_t *ts_chunkise(struct chunkiser_ctx *s, int id, int *size, uint64_t *ts)
{
  uint8_t *res;

  res = malloc(s->pkts_per_chunk * 188);
  if (res == NULL) {
    *size = -1;

    return NULL;
  }
  *ts = 0;		/* FIXME: Read the PCR!!! */
  *size = read(s->fds[0], res, s->pkts_per_chunk * 188);
  if (res[0] != 0x47) {
    int err;

    ts_resync(res, size);
    err = read(s->fds[0], res + *size, s->pkts_per_chunk * 188 - *size);
    if (err > 0) {
      *size += err;
    }
  }
  if (*size % 188) {
    fprintf(stderr, "WARNING!!! Strange input size!\n");
    *size = (*size / 188) * 188;
  }
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

const int *ts_get_fds(const struct chunkiser_ctx *s)
{
  return s->fds;
}

struct chunkiser_iface in_ts = {
  .open = ts_open,
  .close = ts_close,
  .chunkise = ts_chunkise,
  .get_fds = ts_get_fds,
};
