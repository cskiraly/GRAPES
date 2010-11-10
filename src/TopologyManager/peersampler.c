#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "net_helper.h"
#include "topmanager.h"
#include "peersampler_iface.h"
#include "config.h"

extern struct peersampler_iface ncast;
extern struct peersampler_iface cyclon;
extern struct peersampler_iface dummy;

struct top_context{
  struct peersampler_iface *ps;
  void *ps_context;
};

int topInit(struct nodeID *myID, void *metadata, int metadata_size, const char *config, struct top_context **context)
{
  struct top_context *tc;
  struct tag *cfg_tags;
  const char *proto;


  tc = (struct top_context*) malloc(sizeof(struct top_context));
  if (!tc) return -1;
  *context = tc;

  tc->ps = &ncast;
  cfg_tags = config_parse(config);
  proto = config_value_str(cfg_tags, "protocol");
  if (proto) {
    if (strcmp(proto, "cyclon") == 0) {
      tc->ps = &cyclon;
    }
    if (strcmp(proto, "dummy") == 0) {
      tc->ps = &dummy;
    }
  }

  return tc->ps->init(myID, metadata, metadata_size, config, &tc->ps_context);
}

int topChangeMetadata(struct top_context *tc, void *metadata, int metadata_size)
{
  return tc->ps->change_metadata(tc->ps_context, metadata, metadata_size);
}

int topAddNeighbour(struct top_context *tc, struct nodeID *neighbour, void *metadata, int metadata_size)
{
  return tc->ps->add_neighbour(tc->ps_context, neighbour, metadata, metadata_size);
}

int topParseData(struct top_context *tc, const uint8_t *buff, int len)
{
  return tc->ps->parse_data(tc->ps_context, buff, len);
}

const struct nodeID **topGetNeighbourhood(struct top_context *tc, int *n)
{
  return tc->ps->get_neighbourhood(tc->ps_context, n);
}

const void *topGetMetadata(struct top_context *tc, int *metadata_size)
{
  return tc->ps->get_metadata(tc->ps_context, metadata_size);
}

int topGrowNeighbourhood(struct top_context *tc, int n)
{
  return tc->ps->grow_neighbourhood(tc->ps_context, n);
}

int topShrinkNeighbourhood(struct top_context *tc, int n)
{
  return tc->ps->shrink_neighbourhood(tc->ps_context, n);
}

int topRemoveNeighbour(struct top_context *tc, struct nodeID *neighbour)
{
  return tc->ps->remove_neighbour(tc->ps_context, neighbour);
}
