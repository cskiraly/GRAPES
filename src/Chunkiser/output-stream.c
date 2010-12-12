#include <stdint.h>
#include <stdlib.h>

#include "chunk.h"
#include "chunkiser.h"
#include "dechunkiser_iface.h"

extern struct dechunkiser_iface out_avf;
extern struct dechunkiser_iface out_raw;
static struct dechunkiser_iface *out;

struct output_stream *out_stream_init(const char *fname, const char *config)
{
#ifdef AVF
  out = &out_avf;
#else
  out = &out_raw;
#endif

  return out->open(fname, config);
}

void chunk_write(struct output_stream *o, const struct chunk *c)
{
  out->write(o, c->id, c->data, c->size);
}
