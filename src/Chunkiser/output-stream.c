#include <stdint.h>
#include <stdlib.h>

#include "chunkiser.h"
#include "dechunkiser_iface.h"

extern struct dechunkiser_iface out_avf;
static struct dechunkiser_iface *out;

struct output_stream *out_stream_init(const char *config)
{
#ifdef AVF
  out = &out_avf;
#else
  return NULL;
#endif

  return out->open(config);
}

void chunk_write(struct output_stream *o, int id, const uint8_t *data, int size)
{
  out->write(o, id, data, size);
}
