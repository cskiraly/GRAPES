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

int topInit(struct nodeID *myID, void *metadata, int metadata_size, const char *config, struct topContext **context)
{
  struct topContext *tc;
  struct tag *cfg_tags;
  const char *proto;


  tc = (struct topContext*) malloc(sizeof(struct topContext));
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

  return tc->ps->init(myID, metadata, metadata_size, config, &tc->psContext);
}

int topChangeMetadata(struct topContext *tc, void *metadata, int metadata_size)
{
  return tc->ps->change_metadata(tc->psContext, metadata, metadata_size);
}

int topAddNeighbour(struct topContext *tc, struct nodeID *neighbour, void *metadata, int metadata_size)
{
  return tc->ps->add_neighbour(tc->psContext, neighbour, metadata, metadata_size);
}

int topParseData(struct topContext *tc, const uint8_t *buff, int len)
{
  return tc->ps->parse_data(tc->psContext, buff, len);
}

const struct nodeID **topGetNeighbourhood(struct topContext *tc, int *n)
{
  return tc->ps->get_neighbourhood(tc->psContext, n);
}

const void *topGetMetadata(struct topContext *tc, int *metadata_size)
{
  return tc->ps->get_metadata(tc->psContext, metadata_size);
}

int topGrowNeighbourhood(struct topContext *tc, int n)
{
  return tc->ps->grow_neighbourhood(tc->psContext, n);
}

int topShrinkNeighbourhood(struct topContext *tc, int n)
{
  return tc->ps->shrink_neighbourhood(tc->psContext, n);
}

int topRemoveNeighbour(struct topContext *tc, struct nodeID *neighbour)
{
  return tc->ps->remove_neighbour(tc->psContext, neighbour);
}
