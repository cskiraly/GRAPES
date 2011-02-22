/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "dechunkiser_iface.h"

enum output_type {
  chunk_id,
  stats,
};

struct dechunkiser_ctx {
  enum output_type type;
  int last_id;
};

static struct dechunkiser_ctx *dummy_open(const char *fname, const char *config)
{
  struct dechunkiser_ctx *res;
  struct tag *cfg_tags;

  res = malloc(sizeof(struct dechunkiser_ctx));
  if (res == NULL) {
    return NULL;
  }
  res->type = chunk_id;
  res->last_id = -1;
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *pt;

    pt = config_value_str(cfg_tags, "type");
    if (pt) {
      if (!strcmp(pt, "stats")) {
        res->type = stats;
      }
    }
  }
  free(cfg_tags);

  return res;
}

static void dummy_write(struct dechunkiser_ctx *o, int id, uint8_t *data, int size)
{
  switch (o->type) {
    case chunk_id:
      printf("Chunk %d: size %d\n", id, size);
      break;
    case stats:
      if (o->last_id >= 0) {
        int i;

        for (i = 1; i < id - o->last_id; i++) {
          printf("Lost chunk %d\n", o->last_id + i);
        }
      }
      break;
    default:
      fprintf(stderr, "Internal error!\n");
      exit(-1);
  }
  o->last_id = id;
}

static void dummy_close(struct dechunkiser_ctx *s)
{
  free(s);
}

struct dechunkiser_iface out_dummy = {
  .open = dummy_open,
  .write = dummy_write,
  .close = dummy_close,
};
