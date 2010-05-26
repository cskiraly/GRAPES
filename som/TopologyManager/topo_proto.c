/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "net_helper.h"
#include "topocache.h"
#include "proto.h"
#include "topo_proto.h"
#include "msg_types.h"

#define MAX_MSG_SIZE 1500

static struct peer_cache *myEntry;

static int ncast_payload_fill(uint8_t *payload, int size, struct peer_cache *c, struct nodeID *snot)
{
  int i;
  uint8_t *p = payload;

  p += cache_header_dump(p, c);
  p += entry_dump(p, myEntry, 0);
  for (i = 0; nodeid(c, i); i++) {
    if (!nodeid_equal(nodeid(c, i), snot)) {
      if (p - payload > size - 32 /* FIXME */) {
        fprintf(stderr, "too many entries!\n");
        return -1;
      }
      p += entry_dump(p, c, i);
    }
  }

  return p - payload;
}

static int topo_reply(const struct peer_cache *c, struct peer_cache *local_cache, int protocol, int type)
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
  dst = nodeid(c, 0);
  h->protocol = protocol;
  h->type = type;
  len = ncast_payload_fill(pkt + sizeof(struct topo_header), MAX_MSG_SIZE - sizeof(struct topo_header), local_cache, dst);

  res = len > 0 ? send_to_peer(nodeid(myEntry, 0), dst, pkt, sizeof(struct topo_header) + len) : len;

  return res;
}

static int topo_query_peer(struct peer_cache *local_cache, struct nodeID *dst, int protocol, int type)
{
  uint8_t pkt[MAX_MSG_SIZE];
  struct topo_header *h = (struct topo_header *)pkt;
  int len;

  h->protocol = protocol;
  h->type = type;
  len = ncast_payload_fill(pkt + sizeof(struct topo_header), MAX_MSG_SIZE - sizeof(struct topo_header), local_cache, dst);
  return len > 0  ? send_to_peer(nodeid(myEntry, 0), dst, pkt, sizeof(struct topo_header) + len) : len;
}

int ncast_reply(const struct peer_cache *c, struct peer_cache *local_cache)
{
  return topo_reply(c, local_cache, MSG_TYPE_TOPOLOGY, NCAST_REPLY);
}

int tman_reply(const struct peer_cache *c, struct peer_cache *local_cache)
{
  return topo_reply(c, local_cache, MSG_TYPE_TMAN, TMAN_REPLY);
}

int ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst)
{
  return topo_query_peer(local_cache, dst, MSG_TYPE_TOPOLOGY, NCAST_QUERY);
}

int tman_query_peer(struct peer_cache *local_cache, struct nodeID *dst)
{
  return topo_query_peer(local_cache, dst, MSG_TYPE_TMAN, TMAN_QUERY);
}

int ncast_query(struct peer_cache *local_cache)
{
  struct nodeID *dst;

  dst = rand_peer(local_cache, NULL);
  if (dst == NULL) {
    return 0;
  }
  return topo_query_peer(local_cache, dst, MSG_TYPE_TOPOLOGY, NCAST_QUERY);
}

int topo_proto_metadata_update(struct nodeID *peer, void *meta, int meta_size)
{
  if (cache_metadata_update(myEntry, peer, meta, meta_size) > 0) {
    return 1;
  }

  return -1;
}

int topo_proto_init(struct nodeID *s, void *meta, int meta_size)
{
  myEntry = cache_init(1, meta_size);
  cache_add(myEntry, s, meta, meta_size);

  return 0;
}
