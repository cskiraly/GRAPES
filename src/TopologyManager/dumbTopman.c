/*
 * dumbTopman.c
 *
 *  Created on: Oct 21, 2010
 *      Author: Marco Biazzini
 */

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "net_helper.h"
#include "../Cache/topocache.h"
#include "config.h"
#include "topman_iface.h"

#define DUMB_DEFAULT_MEM	20
#define DUMB_DEFAULT_CSIZE	20
#define DUMB_DEFAULT_PERIOD	10

static uint64_t currtime;
static int memory;
static int cache_size;
static int current_size;
static int mdata_size;
static int do_resize;
static int period;
static struct peer_cache *local_cache;
static uint8_t *my_mdata;
static struct nodeID *me;

static uint64_t gettime(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_usec + tv.tv_sec * 1000000ull;
}

static int time_to_run(void)
{
	if (gettime() - currtime > period) {
		currtime += period;
		return 1;
	}

	return 0;
}

static int dumbInit(struct nodeID *myID, void *metadata, int metadata_size, ranking_function rfun, const char *config)
{
	struct tag *cfg_tags;
	int res;

	cfg_tags = config_parse(config);
	res = config_value_int(cfg_tags, "cache_size", &cache_size);
	if (!res) {
		cache_size = DUMB_DEFAULT_CSIZE;
	}
	res = config_value_int(cfg_tags, "memory", &memory);
	if (!res) {
		memory = DUMB_DEFAULT_MEM;
	}
	res = config_value_int(cfg_tags, "period", &period);
	if (!res) {
		period = DUMB_DEFAULT_PERIOD;
	}
	period *= 1000000;

	local_cache = cache_init(cache_size, metadata_size, 0);
	if (local_cache == NULL) {
		return -1;
	}
	mdata_size = metadata_size;
	if (mdata_size) {
		my_mdata = malloc(mdata_size);
		if (my_mdata == NULL) {
			cache_free(local_cache);
			return -1;
		}
		memcpy(my_mdata, metadata, mdata_size);
	}
	me = myID;
	currtime = gettime();

	return 0;
}

static int dumbGivePeers (int n, struct nodeID **peers, void *metadata)
{
	int metadata_size;
	const uint8_t *mdata;
	int i;

	mdata = get_metadata(local_cache, &metadata_size);
	for (i=0; nodeid(local_cache, i) && (i < n); i++) {
		peers[i] = nodeid(local_cache,i);
		if (metadata_size)
			memcpy((uint8_t *)metadata + i * metadata_size, mdata + i * metadata_size, metadata_size);
	}

	return i;
}

static int dumbGetNeighbourhoodSize(void)
{
	int i;

	for (i = 0; nodeid(local_cache, i); i++);

	return i;
}

static int dumbAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
	if (cache_add(local_cache, neighbour, metadata, metadata_size) < 0) {
		return -1;
	}

	current_size++;
	return 1;
}

static const void *dumbGetMetadata(int *metadata_size)
{
	return get_metadata(local_cache, metadata_size);
}

static int dumbChangeMetadata(void *metadata, int metadata_size)
{
	if (metadata_size && metadata_size == mdata_size) {
		memcpy(my_mdata, (uint8_t *)metadata, mdata_size);
		return 1;
	}
	else return -1;
}

static int dumbParseData(const uint8_t *buff, int len, struct nodeID **peers, int size, const void *metadata, int metadata_size)
{
	struct peer_cache *new_cache;
	const uint8_t *m_data;
	int r,j, msize, csize, heritage;

	if (!time_to_run()) {
		return 1;
	}
	if (metadata_size != mdata_size) {
		fprintf(stderr, "DumbTopman : Metadata size mismatch with peer sampler!\n");
		return 1;
	}
	if (!size) {
		fprintf(stderr, "DumbTopman : No peer available from peer sampler!\n");
	}

	m_data = (const uint8_t *)get_metadata(local_cache, &msize);
	new_cache = cache_init(cache_size, msize, 0);
	if (!new_cache) {
		fprintf(stderr, "DumbTopman : Memory error while creating new cache!\n");
		return 1;
	}

	cache_update(local_cache);
	heritage = (cache_size * memory) / 100;
	if (heritage > current_size) {
		heritage = current_size;
	}
	for (csize = 0; csize < heritage; ) {
		if (heritage == current_size) {
			r = csize;
		} else {
			r = ((double)rand() / (double)RAND_MAX) * current_size;
			if (r == current_size) r--;
		}
		r = cache_add(new_cache, nodeid(local_cache, r), m_data + r * msize, msize);
		if (csize < r) {
			csize = r;
		}
	}
	for (j = 0; j < size && csize < cache_size; j++) {
		r = cache_add(new_cache, peers[j], (const uint8_t *)metadata + j * metadata_size,
			metadata_size);
		if (csize < r) {
			csize = r;
		}
	}
	current_size = csize;
	cache_free(local_cache);
	local_cache = new_cache;
	do_resize = 0;

	fprintf(stderr, "DumbTopman : Parse Data.\n");
	return 0;
}

// limit : at most it doubles the current cache size...
static int dumbGrowNeighbourhood(int n)
{
	if (n <= 0 || do_resize)
		return -1;
	n = n > cache_size ? cache_size : n;
	cache_size += n;
	do_resize = 1;
	return cache_size;
}

static int dumbShrinkNeighbourhood(int n)
{
	if (n <= 0 || n >= cache_size || do_resize)
		return -1;
	cache_size -= n;
	do_resize = 1;
	return cache_size;
}

static int dumbRemoveNeighbour(struct nodeID *neighbour)
{
	current_size = cache_del(local_cache, neighbour);
	return current_size;
}


struct topman_iface dumb = {
	.init = dumbInit,
	.changeMetadata = dumbChangeMetadata,
	.addNeighbour = dumbAddNeighbour,
	.parseData = dumbParseData,
	.givePeers = dumbGivePeers,
	.getMetadata = dumbGetMetadata,
	.growNeighbourhood = dumbGrowNeighbourhood,
	.shrinkNeighbourhood = dumbShrinkNeighbourhood,
	.removeNeighbour = dumbRemoveNeighbour,
	.getNeighbourhoodSize = dumbGetNeighbourhoodSize,
};
