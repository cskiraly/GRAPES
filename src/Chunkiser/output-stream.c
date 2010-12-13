#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "config.h"
#include "chunkiser.h"
#include "dechunkiser_iface.h"

extern struct dechunkiser_iface out_avf;
extern struct dechunkiser_iface out_raw;
static struct dechunkiser_iface *out;

struct output_stream *out_stream_init(const char *fname, const char *config)
{
  struct tag *cfg_tags;

#ifdef AVF
  out = &out_avf;
#else
  out = &out_raw;
#endif
  cfg_tags = config_parse(config);

  if (cfg_tags) {
    const char *type;

    type = config_value_str(cfg_tags, "dechunkiser");
    if (type && !strcmp(type, "raw")) {
      out = &out_raw;
    }
  }
  free(cfg_tags);

  return out->open(fname, config);
}

void out_stream_close(struct output_stream *s)
{
  return out->close(s);
}

void chunk_write(struct output_stream *o, const struct chunk *c)
{
  out->write(o, c->id, c->data, c->size);
}
