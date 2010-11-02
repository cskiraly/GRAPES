#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "net_helper.h"
#include "peersampler.h"
#include "peersampler_iface.h"
#include "config.h"

extern struct peersampler_iface ncast;
extern struct peersampler_iface cyclon;
extern struct peersampler_iface dummy;
static struct peersampler_iface *ps;

int psample_init(struct nodeID *myID, void *metadata, int metadata_size, const char *config)
{
  struct tag *cfg_tags;
  const char *proto;

  ps = &ncast;
  cfg_tags = config_parse(config);
  proto = config_value_str(cfg_tags, "protocol");
  if (proto) {
    if (strcmp(proto, "cyclon") == 0) {
      ps = &cyclon;
    }
    if (strcmp(proto, "dummy") == 0) {
      ps = &dummy;
    }
  }

  return ps->init(myID, metadata, metadata_size, config);
}

int psample_change_metadata(void *metadata, int metadata_size)
{
  return ps->change_metadata(metadata, metadata_size);
}

int psample_add_peer(struct nodeID *neighbour, void *metadata, int metadata_size)
{
  return ps->add_neighbour(neighbour, metadata, metadata_size);
}

int psample_parse_data(const uint8_t *buff, int len)
{
  return ps->parse_data(buff, len);
}

const struct nodeID **psample_get_cache(int *n)
{
  return ps->get_neighbourhood(n);
}

const void *psample_get_metadata(int *metadata_size)
{
  return ps->get_metadata(metadata_size);
}

int psample_grow_cache(int n)
{
  return ps->grow_neighbourhood(n);
}

int psample_shrink_cache(int n)
{
  return ps->shrink_neighbourhood(n);
}

int psample_remove_peer(struct nodeID *neighbour)
{
  return ps->remove_neighbour(neighbour);
}
