#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "config.h"
#include "chunkiser.h"
#include "chunkiser_iface.h"

extern struct chunkiser_iface in_avf;
extern struct chunkiser_iface in_dummy;
extern struct chunkiser_iface in_dumb;
extern struct chunkiser_iface in_udp;
extern struct chunkiser_iface in_ts;

struct input_stream {
  struct chunkiser_ctx *c;
  struct chunkiser_iface *in;
};

struct input_stream *input_stream_open(const char *fname, int *period, const char *config)
{
  struct tag *cfg_tags;
  struct input_stream *res;

  res = malloc(sizeof(struct input_stream));
  if (res == NULL) {
    return res;
  }

#ifdef AVF
  res->in = &in_avf;
#else
  res->in = &in_dumb;
#endif
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    const char *type;

    type = config_value_str(cfg_tags, "chunkiser");
    if (type && !strcmp(type, "dummy")) {
      res->in = &in_dummy;
    }
    if (type && !strcmp(type, "dumb")) {
      res->in = &in_dumb;
    }
    if (type && !strcmp(type, "ts")) {
      res->in = &in_ts;
    }
    if (type && !strcmp(type, "udp")) {
      res->in = &in_udp;
    }
    if (type && !strcmp(type, "avf")) {
#ifdef AVF
      res->in = &in_avf;
#else
      free(res);
      free(cfg_tags);

      return NULL;
#endif
    }
  }
  free(cfg_tags);

  res->c = res->in->open(fname, period, config);
  if (res->c == NULL) {
    free(res);

    return NULL;
  }

  return res;
}

void input_stream_close(struct input_stream *s)
{
  s->in->close(s->c);
  free(s);
}

int chunkise(struct input_stream *s, struct chunk *c)
{
  c->data = s->in->chunkise(s->c, c->id, &c->size, &c->timestamp);
  if (c->data == NULL) {
    if (c->size < 0) {
      return -1;
    }

    return 0;
  }

  return 1;
}

const int *input_get_fds(const struct input_stream *s)
{
  if (s->in->get_fds) {
    return s->in->get_fds(s->c);
  }

  return NULL;
}

