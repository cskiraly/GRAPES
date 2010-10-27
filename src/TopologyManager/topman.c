#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

#include "net_helper.h"
#include "tman.h"
#include "topman_iface.h"

extern struct topman_iface tman;
extern struct topman_iface dumb;

static struct topman_iface *tm;


int tmanInit(struct nodeID *myID, void *metadata, int metadata_size, tmanRankingFunction rfun, const char *config)
{
	tm = &dumb;
	return tm->init(myID, metadata, metadata_size, rfun, config);
}


int tmanAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
	return tm->addNeighbour(neighbour, metadata, metadata_size);
}


int tmanParseData(const uint8_t *buff, int len, struct nodeID **peers, int size, const void *metadata, int metadata_size)
{
	return tm->parseData(buff, len, peers, size, metadata, metadata_size);
}


int tmanChangeMetadata(void *metadata, int metadata_size)
{
	return tm->changeMetadata(metadata, metadata_size);
}


const void *tmanGetMetadata(int *metadata_size)
{
	return tm->getMetadata(metadata_size);
}


int tmanGetNeighbourhoodSize(void)
{
	return tm->getNeighbourhoodSize();
}


int tmanGivePeers (int n, struct nodeID **peers, void *metadata)
{
	return tm->givePeers (n, peers, metadata);
}


int tmanGrowNeighbourhood(int n)
{
	return tm->growNeighbourhood(n);
}


int tmanShrinkNeighbourhood(int n)
{
	return tm->shrinkNeighbourhood(n);
}


int tmanRemoveNeighbour(struct nodeID *neighbour)
{
	return tm->removeNeighbour(neighbour);
}
