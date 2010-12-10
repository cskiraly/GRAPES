#include <stdint.h>
#include <stdlib.h>

#include "chunk.h"
#include "chunkiser.h"
#include "chunkiser_iface.h"

extern struct chunkiser_iface in_avf;
extern struct chunkiser_iface in_dummy;

static struct chunkiser_iface *in;

struct input_stream *input_stream_open(const char *fname, int *period, const char *config)
{
#ifdef AVF
  in = &in_avf;
#else
  in = &in_dummy;
#endif

  return in->open(fname, period, config);
}

void input_stream_close(struct input_stream *s)
{
  return in->close(s);
}

int chunkise(struct input_stream *s, struct chunk *c)
{
  c->data = in->chunkise(s, c->id, &c->size, &c->timestamp);
  if (c->data == NULL) {
    if (c->size < 0) {
      return -1;
    }

    return 0;
  }

  return 1;
}
