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

#include "msg_types.h"
#include "net_helper.h"
#include "topmanager.h"
#include "tman.h"

#define TMAN_MAX_IDLE 5

static int counter = 0;


int topoChangeMetadata(struct nodeID *peer, void *metadata, int metadata_size)
{
	// this because if my own metadata are to be changed, it shouldn't be done twice!
 	if (counter <= TMAN_MAX_IDLE)
 		return topChangeMetadata(metadata,metadata_size);
    return tmanChangeMetadata(metadata,metadata_size);
}

int topoAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size, tmanRankingFunction rfun)
{
//	int ncs;
//	topGetNeighbourhood(&ncs);
	// TODO: check this!! Just to use this function to bootstrap ncast...
	if (counter < TMAN_MAX_IDLE)
		return topAddNeighbour(neighbour,metadata,metadata_size);
	else return tmanAddNeighbour(neighbour,metadata,metadata_size);
}

int topoParseData(const uint8_t *buff, int len, tmanRankingFunction f)
{
	int res,ncs,msize;
	const struct nodeID **n; const void *m;
	res = topParseData(buff,len);
	if (res==0 && counter <= TMAN_MAX_IDLE)
		counter++;
	if (res !=0 || counter >= TMAN_MAX_IDLE)
	{
		n = topGetNeighbourhood(&ncs);
		m = topGetMetadata(&msize);
		res = tmanParseData(buff,len,n,ncs,m,msize);
	}
  return res;
}

const struct nodeID **topoGetNeighbourhood(int *n)
{
	if (counter > TMAN_MAX_IDLE) {
		struct nodeID ** neighbors; void *mdata; int msize;
		*n = tmanGetNeighbourhoodSize();
		neighbors = calloc(*n,sizeof(struct nodeID *));
		tmanGetMetadata(&msize);
		mdata = calloc(*n,msize);
		tmanGivePeers(*n,neighbors,mdata);
		free(mdata);
		return (const struct nodeID **)neighbors;
	}
	else
		return topGetNeighbourhood(n);
}

const void *topoGetMetadata(int *metadata_size)
{
	if (counter > TMAN_MAX_IDLE)
		return tmanGetMetadata(metadata_size);
	else
		return topGetMetadata(metadata_size);
}

int topoGrowNeighbourhood(int n)
{
	return tmanGrowNeighbourhood(n);
}

int topoShrinkNeighbourhood(int n)
{
  return tmanShrinkNeighbourhood(n);
}

int topoRemoveNeighbour(struct nodeID *neighbour)
{
  return topRemoveNeighbour(neighbour);
}


