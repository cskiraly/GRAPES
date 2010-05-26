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
#include "topmanager.h"
#include "topocache.h"
#include "topo_proto.h"
#include "proto.h"
#include "msg_types.h"

#define TMAN_MAX_PEERS 5 // max # of neighbors in local cache (should be > than the next)
#define TMAN_MAX_PREFERRED_PEERS 5 // # of peers to choose a receiver among (should be < than the previous)
#define TMAN_MAX_GOSSIPING_PEERS 5 // # size of the view to be sent to receiver peer (should be <= than the previous)
#define TMAN_IDLE_TIME 10 // # of iterations to wait before switching to inactive state
#define TMAN_STD_PERIOD 1000000

static const int TMAN_FLAGS = 2;
static const int TMAN_BLACK = 0;
static const int TMAN_STICKY = 1;

//static const int MAX_PEERS = TMAN_MAX_PEERS;
static  int MAX_PREFERRED_PEERS = TMAN_MAX_PREFERRED_PEERS;
static  int MAX_GOSSIPING_PEERS = TMAN_MAX_GOSSIPING_PEERS;
static  int IDLE_TIME = TMAN_IDLE_TIME;

static uint64_t currtime;
static int cache_size = TMAN_MAX_PEERS;
static struct peer_cache *local_cache = NULL;
static int period = TMAN_STD_PERIOD;
static int active = 0;
static ranking_function rfun;
static void *mymeta;

static struct peer_cache *rank_cache (const struct peer_cache *c, const struct nodeID *target, const void *target_meta)
{
	struct peer_cache *res;
	int i,msize,max = MAX_GOSSIPING_PEERS;
	const uint8_t *mdata;

        mdata = get_metadata(c,&msize);
	res = cache_init(cache_size,msize);
        if (res == NULL) {
          return res;
        }

        for (i=0;nodeid(c,i) && i<max;i++) {
				if (!nodeid_equal(nodeid(c,i),target))
					cache_add_ranked(res,nodeid(c,i),mdata+i*msize,msize, rfun, target_meta);
				else
					max++;
	}

	return res;
}

static uint64_t gettime(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_usec + tv.tv_sec * 1000000ull;
}

int tmanInit(struct nodeID *myID, void *metadata, int metadata_size, ranking_function f) //, int max_peers, int max_idle, int periodicity
{
  rfun = f;
  topo_proto_init(myID, metadata, metadata_size);
  mymeta = metadata;
//  if (max_peers) {
//	  cache_size = max_peers;
//  }
//  else
	  cache_size = TMAN_MAX_PEERS;
  local_cache = cache_init(cache_size, metadata_size);
  if (local_cache == NULL) {
    return -1;
  }
//  if (max_idle)
//	  IDLE_TIME = max_idle;
//  else
	  IDLE_TIME = TMAN_IDLE_TIME;
//  if (periodicity>=1000000)
//	  period = periodicity;
//  else
	  period = TMAN_STD_PERIOD;

  currtime = gettime();

  return 0;
}


int tmanStart(struct nodeID **peers, int size, const void *metadata, int metadata_size,
	int best_peers, int gossip_peers)
{
	int j,res=0;

	if (best_peers)
		MAX_PREFERRED_PEERS = best_peers;
	else MAX_PREFERRED_PEERS = TMAN_MAX_PREFERRED_PEERS;
	if (gossip_peers)
		MAX_GOSSIPING_PEERS = gossip_peers;
	else
		MAX_GOSSIPING_PEERS = TMAN_MAX_GOSSIPING_PEERS;
	// TODO: empty tabu list, if any...
	for (j=0;j<size && res!=-3;j++) // TODO: conditions to keep some peers if app needs...
		res = cache_add_ranked(local_cache,peers[j],(const uint8_t *)metadata+j*metadata_size,metadata_size,rfun, mymeta);
	if (res == -3)
		return -1;
	active = TMAN_IDLE_TIME;
	return 0;
}

static int time_to_send()
{
	if (gettime() - currtime > period) {
		currtime += period;
		if (active > 0)
			return 1;
	}

  return 0;
}


int tmanChangeMetadata(struct nodeID *peer, void *metadata, int metadata_size)
{
  if (topo_proto_metadata_update(peer, metadata, metadata_size) <= 0) {
    return -1;
  }
  mymeta = metadata;

  return 1;
}

int tmanAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size)
{
  // TODO: change this for flagging...
	if (cache_add_ranked(local_cache, neighbour, metadata, metadata_size, rfun, mymeta) < 0) {
    return -1;
  }
  return 1;//topo_query_peer(local_cache, neighbour, TMAN_QUERY);
}

int tmanParseData(const uint8_t *buff, int len)
{
        int msize,s;
        const uint8_t *mdata;
	struct peer_cache *new;

        new = NULL;
	  // TODO: change this for flagging...
	if (local_cache == NULL) return 1;
	if (len) {
		const struct topo_header *h = (const struct topo_header *)buff;
		struct peer_cache *remote_cache; int idx;
                int from;

	    if (h->protocol != MSG_TYPE_TOPOLOGY) {
	      fprintf(stderr, "TMAN: Wrong protocol!\n");
	      return -1;
	    }

	    if (h->type != TMAN_QUERY && h->type != TMAN_REPLY) {
	      return -1;
	    }
		remote_cache = entries_undump(buff + sizeof(struct topo_header), len - sizeof(struct topo_header));
		mdata = get_metadata(remote_cache,&msize);
		get_metadata(local_cache,&s);

		if (msize != s) {
			fprintf(stderr, "TMAN: Metadata size mismatch! -> local (%d) != received (%d)\n",
				s, msize);
			return 1;
		}

		if (h->type == TMAN_QUERY) {
//			fprintf(stderr, "\tTman: Parsing received msg to reply...\n");
			new = rank_cache(local_cache, nodeid(remote_cache, 0), get_metadata(remote_cache, &msize));
			if (new) {
				tman_reply(remote_cache, new);
				cache_free(new);
				// TODO: put sender in tabu list (check list size, etc.), if any...
			}
		}
		idx = cache_add_ranked(local_cache, nodeid(remote_cache,0), mdata, msize,rfun, mymeta);
//		fprintf(stderr, "\tidx = %d\n",idx);
		new = merge_caches_ranked(local_cache, remote_cache, cache_size, &from, rfun, mymeta);

//		  newsize = new_cache->current_size>c1->cache_size?new_cache->current_size:c1->cache_size;
//		  newsize = cache_shrink(new_cache,(new_cache->cache_size-newsize));
//		  // TODO: check newsize??
//		  return new_cache;

		cache_free(remote_cache);
		if (new!=NULL) {
		  cache_free(local_cache);
		  local_cache = new;
                }
	
                if (from == 1) {
		     	if (active > 0) active = TMAN_IDLE_TIME;
                }	else if (active > 0){
		     	active--;
		}
  }

  if (time_to_send()) {
	void *meta;
	struct nodeID *chosen;

	cache_update(local_cache);
	mdata = get_metadata(local_cache,&msize);
	chosen = rand_peer(local_cache, &meta);		//MAX_PREFERRED_PEERS
		 // TODO: check for chosen peer against tabu list, if any...
		new = rank_cache(local_cache, chosen, meta);
		if (new==NULL) {
			fprintf(stderr, "TMAN: No cache could be sent to remote peer!\n");
			return 1;
	}
	if (new) {
		tman_query_peer(new, chosen);
		cache_free(new);
	}
  }

  return 0;
}

const struct nodeID **tmanGetNeighbourhood(int *n)
{
  static struct nodeID *r[2000 /*FIXME!*/];
  for (*n = 0; nodeid(local_cache, *n); (*n)++) {
    r[*n] = nodeid(local_cache, *n);
    //fprintf(stderr, "Checking table[%d]\n", *n);
  }
  fprintf(stderr, "\tTman : active = %d\n",active);
  return (const struct nodeID **)r;
}

// not self metadata, but neighbors'.
const void *tmanGetMetadata(int *metadata_size)
{
  return get_metadata(local_cache, metadata_size);
}

//assuming peers are always ordered
int tmanGivePeers (int n, struct nodeID **peers, uint8_t *metadata) {

//	void *m[n];
	int msize;
	const uint8_t *mdata = get_metadata(local_cache,&msize);
	int i;
	for (i=0; nodeid(local_cache, i) && (i < n); i++)
			peers[i] = nodeid(local_cache,i);
			if (msize)
				memcpy(metadata+i*msize,mdata+i*msize,msize);
	return i;
}
