/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "net_helper.h"
#include "blist_cache.h"
#include "proto.h"
#include "blist_proto.h"
#include "grapes_msg_types.h"

#define MAX_MSG_SIZE 1500

static struct peer_cache *myEntry;

static int blist_payload_fill(uint8_t *payload, int size, struct peer_cache *c, struct nodeID *snot, int max_peers)
{
  int i;
  uint8_t *p = payload;

  if (!max_peers) max_peers = MAX_MSG_SIZE; // just to be sure to dump the whole cache...
  p += blist_cache_header_dump(p, c);
  p += blist_entry_dump(p, myEntry, 0, size - (p - payload));
  for (i = 0; blist_nodeid(c, i) && max_peers; i++) {
    if (!nodeid_equal(blist_nodeid(c, i), snot)) {
      int res;
      res = blist_entry_dump(p, c, i, size - (p - payload));
      if (res < 0) {
        fprintf(stderr, "too many entries!\n");
        return -1;
      }
      p += res;
      --max_peers;
    }
  }

  return p - payload;
}

static int blist_topo_reply(const struct peer_cache *c, struct peer_cache *local_cache, int protocol, int type, int max_peers)
{
  uint8_t pkt[MAX_MSG_SIZE];
  struct topo_header *h = (struct topo_header *)pkt;
  int len, res;
  struct nodeID *dst;

#if 0
  n = psize / sizeof(struct cache_entry);
  if (n * sizeof(struct cache_entry) != psize) {
    fprintf(stderr, "Wrong number of elems %d (%d / %d)!!!\n", n, psize, sizeof(struct cache_entry));
    return -1;
  }
#endif
  dst = blist_nodeid(c, 0);
  h->protocol = protocol;
  h->type = type;
  len = blist_payload_fill(pkt + sizeof(struct topo_header), MAX_MSG_SIZE - sizeof(struct topo_header), local_cache, dst, max_peers);

  res = len > 0 ? send_to_peer(blist_nodeid(myEntry, 0), dst, pkt, sizeof(struct topo_header) + len) : len;

  return res;
}

static int blist_topo_query_peer(struct peer_cache *local_cache, struct nodeID *dst, int protocol, int type, int max_peers)
{
  uint8_t pkt[MAX_MSG_SIZE];
  struct topo_header *h = (struct topo_header *)pkt;
  int len;

  h->protocol = protocol;
  h->type = type;
  len = blist_payload_fill(pkt + sizeof(struct topo_header), MAX_MSG_SIZE - sizeof(struct topo_header), local_cache, dst, max_peers);
  return len > 0  ? send_to_peer(blist_nodeid(myEntry, 0), dst, pkt, sizeof(struct topo_header) + len) : len;
}

int blist_ncast_reply(const struct peer_cache *c, struct peer_cache *local_cache)
{
  return blist_topo_reply(c, local_cache, MSG_TYPE_TOPOLOGY, NCAST_REPLY, 0);
}

int blist_tman_reply(const struct peer_cache *c, struct peer_cache *local_cache, int max_peers)
{
  return blist_topo_reply(c, local_cache, MSG_TYPE_TMAN, TMAN_REPLY, max_peers);
}

int blist_ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst)
{
  return blist_topo_query_peer(local_cache, dst, MSG_TYPE_TOPOLOGY, NCAST_QUERY, 0);
}

int blist_tman_query_peer(struct peer_cache *local_cache, struct nodeID *dst, int max_peers)
{
  return blist_topo_query_peer(local_cache, dst, MSG_TYPE_TMAN, TMAN_QUERY, max_peers);
}

int blist_ncast_query(struct peer_cache *local_cache)
{
  struct nodeID *dst;

  dst = blist_rand_peer(local_cache, NULL, 0);
  if (dst == NULL) {
    return 0;
  }
  return blist_topo_query_peer(local_cache, dst, MSG_TYPE_TOPOLOGY, NCAST_QUERY, 0);
}

int blist_proto_metadata_update(void *meta, int meta_size)
{
  if (blist_cache_metadata_update(myEntry, blist_nodeid(myEntry, 0), meta, meta_size) > 0) {
    return 1;
  }

  return -1;
}

int blist_proto_init(struct nodeID *s, void *meta, int meta_size)
{
  if (!myEntry) {
	myEntry = blist_cache_init(1, meta_size, 0);
	blist_cache_add(myEntry, s, meta, meta_size);
  }
  return 0;
}
