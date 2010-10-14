#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

#include "net_helper.h"
#include "topmanager.h"
#include "peersampler_iface.h"

extern struct peersampler_iface ncast;

int topInit(struct nodeID *myID, void *metadata, int metadata_size, const char *config)
{
  return ncast.init(myID, metadata, metadata_size, config);
}

int topChangeMetadata(void *metadata, int metadata_size)
{
  return ncast.change_metadata(metadata, metadata_size);
}

int topAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
  return ncast.add_neighbour(neighbour, metadata, metadata_size);
}

int topParseData(const uint8_t *buff, int len)
{
  return ncast.parse_data(buff, len);
}

const struct nodeID **topGetNeighbourhood(int *n)
{
  return ncast.get_neighbourhood(n);
}

const void *topGetMetadata(int *metadata_size)
{
  return ncast.get_metadata(metadata_size);
}

int topGrowNeighbourhood(int n)
{
  return ncast.grow_neighbourhood(n);
}

int topShrinkNeighbourhood(int n)
{
  return ncast.shrink_neighbourhood(n);
}

int topRemoveNeighbour(struct nodeID *neighbour)
{
  return ncast.remove_neighbour(neighbour);
}
