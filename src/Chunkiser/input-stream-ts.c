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
  int size;
  int bufsize;
  int pcr_period;
  uint8_t *buff;
  uint64_t old_pcr;
  int fds[2];
};
#define DEFAULT_PKTS 512
#define BUFSIZE_INCR (512 * 188)

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

  res->size = res->bufsize = 0;
  res->buff = NULL;
  res->loop = 0;
  res->pcr_period = 100000 / 100 * 9;
  res->old_pcr = 0;
  res->fds[0] = open(fname, O_RDONLY);
  if (res->fds[0] < 0) {
    free(res);

    return NULL;
  }
  res->fds[1] = -1;

  res->pkts_per_chunk = DEFAULT_PKTS;
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *access_mode;

    config_value_int(cfg_tags, "loop", &res->loop);
    config_value_int(cfg_tags, "pkts", &res->pkts_per_chunk);
    config_value_int(cfg_tags, "pcr_period", &res->pcr_period);
    access_mode = config_value_str(cfg_tags, "mode");
    if (access_mode && !strcmp(access_mode, "nonblock")) {
      fcntl(res->fds[0], F_SETFL, O_NONBLOCK);
    }
  }
  free(cfg_tags);
  if (res->pcr_period) {
    *period = res->pcr_period;
  } else {
    *period = 0;
  }

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

  if (!s->pcr_period) {
    res = malloc(s->pkts_per_chunk * 188);
    if (res == NULL) {
      *size = -1;

      return NULL;
    }
    *ts = 0;		/* FIXME: Read the PCR!!! */
    *size = read(s->fds[0], res, s->pkts_per_chunk * 188);
    if (*size && (res[0] != 0x47)) {
      int err;

      ts_resync(res, size);
      if (size) {
        err = read(s->fds[0], res + *size, s->pkts_per_chunk * 188 - *size);
        if (err > 0) {
          *size += err;
        }
      }
    }
  } else {
    int done;

    res = NULL;
    *size = 0;
    if (s->size + 188 > s->bufsize) {
      s->bufsize += BUFSIZE_INCR;
      s->buff = realloc(s->buff, s->bufsize);
    }
    done = 0;
    while(!done) {
      uint8_t *p;
      int err;

      p = s->buff + s->size;
      err = read(s->fds[0], p, 188);
      if (err == 188) {
        if (*p != 0x47) {
          ts_resync(p, &err);
          if (err) {
            err += read(s->fds[0], p + err, 188 - err);
          }
        }
        if (err != 188) {
          done = 1;
        } else {
          s->size += 188;
          if (p[3] & 0x20) {
            fprintf(stderr, "Adaptation field!!! Size: %d\n", p[4]);
            if (p[5] & 0x10) {
              unsigned long long int pcr;

              fprintf(stderr, "PCR!!!\n");
              pcr = p[6] << 24 | p[7] << 16 | p[8] << 8 | p[9];
              pcr = pcr << 1 | p[10] >> 7;
              fprintf(stderr, "%llu", pcr);
              if (pcr > s->old_pcr + s->pcr_period) {
                *ts = pcr / 9 * 100;
                *size = s->size;
                res = s->buff;
                s->old_pcr = pcr;
                s->buff = NULL;
                s->size = 0;
                s->bufsize = 0;
                done = 1;
              }
            }
          }
        }
      } else {
        done = 1;
      }
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
