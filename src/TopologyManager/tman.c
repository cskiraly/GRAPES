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

#include "net_helper.h"
#include "blist_cache.h"
#include "blist_proto.h"
#include "proto.h"
#include "grapes_msg_types.h"
#include "tman.h"

#define TMAN_INIT_PEERS 10 // max # of neighbors in local cache (should be >= than the next)
#define TMAN_MAX_PREFERRED_PEERS 10 // # of peers to choose a receiver among (should be <= than the previous)
#define TMAN_MAX_GOSSIPING_PEERS 20 // # size of the view to be sent to receiver peer (should be <= than the previous)
#define TMAN_STD_PERIOD 1000000
#define TMAN_INIT_PERIOD 1000000
#define TMAN_RESTART_COUNT 20;

static  int max_preferred_peers = TMAN_MAX_PREFERRED_PEERS;
static  int max_gossiping_peers = TMAN_MAX_GOSSIPING_PEERS;
static	int restart_countdown = TMAN_RESTART_COUNT;

static uint64_t currtime;
static int cache_size = TMAN_INIT_PEERS;
static struct peer_cache *local_cache;
static int period = TMAN_INIT_PERIOD;
static int active;
static int do_resize;
static void *mymeta;
static int mymeta_size;
static struct nodeID *restart_peer;
static uint8_t *zero;

static tmanRankingFunction userRankFunct;

static int tmanRankFunct (const void *target, const void *p1, const void *p2) {

	if (memcmp(target,zero,mymeta_size) == 0 || (memcmp(p1,zero,mymeta_size) == 0 && memcmp(p2,zero,mymeta_size) == 0))
		return 0;
	if (memcmp(p1,zero,mymeta_size) == 0)
		return 2;
	if (memcmp(p2,zero,mymeta_size) == 0)
		return 1;
	return userRankFunct(target, p1, p2);
}

static uint64_t gettime(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_usec + tv.tv_sec * 1000000ull;
}

int tmanInit(struct nodeID *myID, void *metadata, int metadata_size, ranking_function rfun, int gossip_peers)
{
	userRankFunct = rfun;
	blist_proto_init(myID, metadata, metadata_size);
	mymeta = metadata;
	mymeta_size = metadata_size;
	zero = calloc(mymeta_size,1);

	local_cache = blist_cache_init(cache_size, metadata_size, 0);
	if (local_cache == NULL) {
		return -1;
	}
	if (gossip_peers) {
		max_gossiping_peers = gossip_peers;
	}
	max_preferred_peers = TMAN_MAX_PREFERRED_PEERS;
	active = -1;
	currtime = gettime();

	return 0;
}

int tmanGivePeers (int n, struct nodeID **peers, void *metadata)
{
	int metadata_size;
	const uint8_t *mdata;
	int i;

	mdata = blist_get_metadata(local_cache, &metadata_size);
	for (i=0; blist_nodeid(local_cache, i) && (i < n); i++) {
		peers[i] = blist_nodeid(local_cache,i);
		if (metadata_size)
			memcpy((uint8_t *)metadata + i * metadata_size, mdata + i * metadata_size, metadata_size);
	}

	return i;
}

int tmanGetNeighbourhoodSize(void)
{
	int i;

	for (i = 0; blist_nodeid(local_cache, i); i++);

	return i;
}

static int time_to_send(void)
{
	if (gettime() - currtime > period) {
		currtime += period;
		return 1;
	}

	return 0;
}

int tmanAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
	if (!metadata_size) {
		blist_tman_query_peer(local_cache, neighbour, max_gossiping_peers);
		return -1;
	}
	if (blist_cache_add_ranked(local_cache, neighbour, metadata, metadata_size, tmanRankFunct, mymeta) < 0) {
		return -1;
	}

	return 1;
}


// not self metadata, but neighbors'.
const void *tmanGetMetadata(int *metadata_size)
{
	return blist_get_metadata(local_cache, metadata_size);
}


int tmanChangeMetadata(void *metadata, int metadata_size)
{
	struct peer_cache *new = NULL;

	if (blist_proto_metadata_update(metadata, metadata_size) <= 0) {
		return -1;
	}
	mymeta = metadata;

	if (active >= 0) {
		new = blist_cache_rank(local_cache, tmanRankFunct, NULL, mymeta);
		if (new) {
			blist_cache_free(local_cache);
			local_cache = new;
		}
	}

	return 1;
}


int tmanParseData(const uint8_t *buff, int len, struct nodeID **peers, int size, const void *metadata, int metadata_size)
{
	int msize,s;
	const uint8_t *mdata;
	struct peer_cache *new = NULL, *temp;

	if (len && active >= 0) {
		const struct topo_header *h = (const struct topo_header *)buff;
		struct peer_cache *remote_cache;

	    if (h->protocol != MSG_TYPE_TMAN) {
	      fprintf(stderr, "TMAN: Wrong protocol!\n");
	      return -1;
	    }

		remote_cache = blist_entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));
		mdata = blist_get_metadata(remote_cache,&msize);
		blist_get_metadata(local_cache,&s);

		if (msize != s) {
			fprintf(stderr, "TMAN: Metadata size mismatch! -> local (%d) != received (%d)\n",
				s, msize);
			return 1;
		}

		if (h->type == TMAN_QUERY) {
			new = blist_cache_rank(local_cache, tmanRankFunct, blist_nodeid(remote_cache, 0), blist_get_metadata(remote_cache, &msize));
			if (new) {
				blist_tman_reply(remote_cache, new, max_gossiping_peers);
				blist_cache_free(new);
				new = NULL;
				// TODO: put sender in tabu list (check list size, etc.), if any...
			}
		}

		if (restart_peer && nodeid_equal(restart_peer, blist_nodeid(remote_cache,0))) { // restart phase : receiving new cache from chosen alive peer...
			new = blist_cache_rank(remote_cache,tmanRankFunct,NULL,mymeta);
			if (new) {
				cache_size = TMAN_INIT_PEERS;
				blist_cache_resize(new,cache_size);
				period = TMAN_STD_PERIOD;
				fprintf(stderr,"RESTARTING TMAN!!!\n");
			}
			nodeid_free(restart_peer);
			restart_peer = NULL;
			active = 1;
		}
		else {	// normal phase
			temp = blist_cache_union(local_cache,remote_cache,&s);
			if (temp) {
				new = blist_cache_rank(temp,tmanRankFunct,NULL,mymeta);
				cache_size = ((s/2)*2.5) > cache_size ? ((s/2)*2.5) : cache_size;
				blist_cache_resize(new,cache_size);
				blist_cache_free(temp);
			}
			if (restart_peer) {
				restart_countdown--;
				if (restart_countdown <= 0) {
					nodeid_free(restart_peer);
					restart_peer = NULL;
				}
			}
		}

		blist_cache_free(remote_cache);
		if (new!=NULL) {
		  blist_cache_free(local_cache);
		  local_cache = new;
                  do_resize = 0;
		}
	}

  if (time_to_send()) {
	uint8_t *meta;
	struct nodeID *chosen;

	blist_cache_update(local_cache);

	if (active > 0 && tmanGetNeighbourhoodSize() < size && !restart_countdown) {
		fprintf(stderr, "TMAN: Too few peers in cache! Triggering a restart...\n");
		active = 0;
		period = TMAN_INIT_PERIOD;
	}

	if (active <= 0) {	// active < 0 -> bootstrap phase ; active = 0 -> restart phase
		struct peer_cache *ncache;
		int j,nsize;

		nsize = TMAN_INIT_PEERS > size ? TMAN_INIT_PEERS : size + 1;
		if (size) ncache = blist_cache_init(nsize, metadata_size, 0);
		else {return 1;}
		for (j=0;j<size;j++)
			blist_cache_add_ranked(ncache, peers[j],(const uint8_t *)metadata + j * metadata_size, metadata_size, tmanRankFunct, mymeta);
		if (blist_nodeid(ncache, 0)) {
			restart_peer = nodeid_dup(blist_nodeid(ncache, 0));
			restart_countdown = TMAN_RESTART_COUNT;
			mdata = blist_get_metadata(ncache, &msize);
			new = blist_cache_rank(active < 0 ? ncache : local_cache, tmanRankFunct, restart_peer, mdata);
			if (new) {
				blist_tman_query_peer(new, restart_peer, max_gossiping_peers);
				blist_cache_free(new);
			}
		if (active < 0) { // bootstrap
			fprintf(stderr,"BOOTSTRAPPING TMAN!!!\n");
			blist_cache_free(local_cache);
			local_cache = ncache;
			cache_size = nsize;
			active = 0;
		} else { // restart
			blist_cache_free(ncache);
		}
		}
		else {
			blist_cache_free(ncache);
			fprintf(stderr, "TMAN: No peer available from peer sampler!\n");
			return 1;
		}
	}
	else { // normal phase
	chosen = blist_rand_peer(local_cache, (void **)&meta, max_preferred_peers);
	new = blist_cache_rank(local_cache, tmanRankFunct, chosen, meta);
	if (new==NULL) {
		fprintf(stderr, "TMAN: No cache could be sent to remote peer!\n");
		return 1;
	}
	blist_tman_query_peer(new, chosen, max_gossiping_peers);
	blist_cache_free(new);
	}
  }

  return 0;
}


// limit : at most it doubles the current cache size...
int tmanGrowNeighbourhood(int n)
{
	if (n<=0 || do_resize)
		return -1;
	n = n>cache_size?cache_size:n;
	cache_size += n;
	do_resize = 1;
	return cache_size;
}


int tmanShrinkNeighbourhood(int n)
{
	if (n<=0 || n>=cache_size || do_resize)
		return -1;
	cache_size -= n;
	do_resize = 1;
	return cache_size;
}

