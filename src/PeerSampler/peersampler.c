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
extern struct peersampler_iface cloudcast;
extern struct peersampler_iface dummy;

struct psample_context{
  struct peersampler_iface *ps;
  struct peersampler_context *ps_context;
};

struct psample_context* psample_init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config)
{
  struct psample_context *tc;
  struct tag *cfg_tags;
  const char *proto;


  tc = malloc(sizeof(struct psample_context));
  if (!tc) return NULL;

  tc->ps = &cyclon;
  cfg_tags = config_parse(config);
  proto = config_value_str(cfg_tags, "protocol");
  if (proto) {
    if (strcmp(proto, "newscast") == 0) {
      tc->ps = &ncast;
    } else if (strcmp(proto, "cyclon") == 0) {
      tc->ps = &cyclon;
    } else if (strcmp(proto, "cloudcast") == 0) {
      tc->ps = &cloudcast;
    }else if (strcmp(proto, "dummy") == 0) {
      tc->ps = &dummy;
    } else {
      free(tc);
      return NULL;
    }
  }
  free(cfg_tags);
  
  tc->ps_context = tc->ps->init(myID, metadata, metadata_size, config);
  if (!tc->ps_context){
    free(tc);
    return NULL;
  }
  
  return tc;
}

int psample_change_metadata(struct psample_context *tc, const void *metadata, int metadata_size)
{
  return tc->ps->change_metadata(tc->ps_context, metadata, metadata_size);
}

int psample_add_peer(struct psample_context *tc, struct nodeID *neighbour, const void *metadata, int metadata_size)
{
  return tc->ps->add_neighbour(tc->ps_context, neighbour, metadata, metadata_size);
}

int psample_parse_data(struct psample_context *tc, const uint8_t *buff, int len)
{
  return tc->ps->parse_data(tc->ps_context, buff, len);
}

const struct nodeID *const *psample_get_cache(struct psample_context *tc, int *n)
{
  return tc->ps->get_neighbourhood(tc->ps_context, n);
}

const void *psample_get_metadata(struct psample_context *tc, int *metadata_size)
{
  return tc->ps->get_metadata(tc->ps_context, metadata_size);
}

int psample_grow_cache(struct psample_context *tc, int n)
{
  return tc->ps->grow_neighbourhood(tc->ps_context, n);
}

int psample_shrink_cache(struct psample_context *tc, int n)
{
  return tc->ps->shrink_neighbourhood(tc->ps_context, n);
}

int psample_remove_peer(struct psample_context *tc, const struct nodeID *neighbour)
{
  return tc->ps->remove_neighbour(tc->ps_context, neighbour);
}
