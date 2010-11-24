#include <stdint.h>

#include "chunkiser.h"
#include "chunkiser_iface.h"

extern struct chunkiser_iface in_avf;

static struct chunkiser_iface *in;

struct input_stream *input_stream_open(const char *fname, int *period, const char *config)
{
  in = &in_avf;

  return in->open(fname, period, config);
}

void input_stream_close(struct input_stream *s)
{
  return in->close(s);
}

uint8_t *chunkise(struct input_stream *s, int id, int *size, uint64_t *ts)
{
  return in->chunkise(s, id, size, ts);
}
