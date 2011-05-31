/*
 *  Copyright (c) 2010 Marco Biazzini
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "grapes_msg_types.h"
#include "net_helper.h"
#include "peersampler.h"
#include "tman.h"

#define TMAN_MAX_IDLE 5

static int counter = 0;

struct psample_context *context;

int topoChangeMetadata(void *metadata, int metadata_size)
{
	int res = psample_change_metadata(context,metadata,metadata_size);
	if (counter >= TMAN_MAX_IDLE)
		return tmanChangeMetadata(metadata,metadata_size);
	else return res;
}

int topoAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
	// TODO: check this!! Just to use this function to bootstrap ncast...
	if (counter < TMAN_MAX_IDLE)
		return psample_add_peer(context,neighbour,metadata,metadata_size);
	else return tmanAddNeighbour(neighbour,metadata,metadata_size);
}

int topoParseData(const uint8_t *buff, int len)
{
	int res,ncs,msize;
	const struct nodeID *const *n; const void *m;
	if (!buff || buff[0] == MSG_TYPE_TOPOLOGY) {
		res = psample_parse_data(context,buff,len);
		if (counter <= TMAN_MAX_IDLE)
			counter++;
	}
	if (counter >= TMAN_MAX_IDLE && (!buff || buff[0] == MSG_TYPE_TMAN))
	{
		n = psample_get_cache(context,&ncs);
		m = psample_get_metadata(context,&msize);
		res = tmanParseData(buff,len,n,ncs,m,msize);
	}
  return res;
}

const struct nodeID *const *topoGetNeighbourhood(int *n)
{
	struct nodeID ** neighbors; void *mdata; int msize;
	*n = tmanGetNeighbourhoodSize();
	if (*n) {
		neighbors = calloc(*n,sizeof(struct nodeID *));
		tmanGetMetadata(&msize);
		mdata = calloc(*n,msize);
		tmanGivePeers(*n,neighbors,mdata);
		free(mdata);
		return (const struct nodeID *const *)neighbors;
	}
	else
		return psample_get_cache(context,n);
}

const void *topoGetMetadata(int *metadata_size)
{
	int n = tmanGetNeighbourhoodSize();
	if (n)
		return tmanGetMetadata(metadata_size);
	else
		return psample_get_metadata(context,metadata_size);
}

int topoGrowNeighbourhood(int n)
{
	if (counter < TMAN_MAX_IDLE)
		return psample_grow_cache(context,n);
	else
	  return tmanGrowNeighbourhood(n);
}

int topoShrinkNeighbourhood(int n)
{
	if (counter < TMAN_MAX_IDLE)
		return psample_shrink_cache(context,n);
	else
		return tmanShrinkNeighbourhood(n);
}

int topoRemoveNeighbour(struct nodeID *neighbour)
{
	if (counter < TMAN_MAX_IDLE)
		return psample_remove_peer(context,neighbour);
	else
		return tmanRemoveNeighbour(neighbour);
}
