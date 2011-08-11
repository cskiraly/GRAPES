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

struct topo_context{
 struct peer_cache *myEntry;
 uint8_t *pkt;
 int pkt_size;
};

static int topo_payload_fill(struct topo_context *context, uint8_t *payload, int size, const struct peer_cache *c, const struct nodeID *snot, int max_peers, int include_me)
{
  int i;
  uint8_t *p = payload;

  if (!max_peers) max_peers = 1000; // FIXME: just to be sure to dump the whole cache...
  p += cache_header_dump(p, c, include_me);
  if (include_me) {
    p += entry_dump(p, context->myEntry, 0, size - (p - payload));
    max_peers--;
  }
  for (i = 0; nodeid(c, i) && max_peers; i++) {
    if (!nodeid_equal(nodeid(c, i), snot)) {
      int res;
      res = entry_dump(p, c, i, size - (p - payload));
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

int topo_reply(struct topo_context *context, const struct peer_cache *c, const struct peer_cache *local_cache, int protocol, int type, int max_peers, int include_me)
{
  struct topo_header *h = (struct topo_header *)context->pkt;
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
  len = topo_payload_fill(context, context->pkt + sizeof(struct topo_header), context->pkt_size - sizeof(struct topo_header), local_cache, dst, max_peers, include_me);

  res = len > 0 ? send_to_peer(nodeid(context->myEntry, 0), dst, context->pkt, sizeof(struct topo_header) + len) : len;

  return res;
}

int topo_query_peer(struct topo_context *context, const struct peer_cache *local_cache, struct nodeID *dst, int protocol, int type, int max_peers)
{
  struct topo_header *h = (struct topo_header *)context->pkt;
  int len;

  h->protocol = protocol;
  h->type = type;
  len = topo_payload_fill(context, context->pkt + sizeof(struct topo_header), context->pkt_size - sizeof(struct topo_header), local_cache, dst, max_peers, 1);
  return len > 0  ? send_to_peer(nodeid(context->myEntry, 0), dst, context->pkt, sizeof(struct topo_header) + len) : len;
}

int topo_proto_myentry_update(struct topo_context *context, struct nodeID *s, int dts, const void *meta, int meta_size)
{
  int ret = 1;

  if (s && !nodeid_equal(nodeid(context->myEntry, 0), s)) {
    fprintf(stderr, "ERROR: myEntry change not implemented!\n");	//TODO
    exit(1);
  }

  if (dts) {
    cache_delay(context->myEntry, dts);
  }

  if (meta) {
    if (cache_metadata_update(context->myEntry, nodeid(context->myEntry, 0), meta, meta_size) <= 0) {
      ret = -1;
    }
  }

  return ret;
}

int topo_proto_metadata_update(struct topo_context *context, const void *meta, int meta_size)
{
  return topo_proto_myentry_update(context, nodeid(context->myEntry, 0), 0 , meta, meta_size);
}

struct topo_context* topo_proto_init(struct nodeID *s, const void *meta, int meta_size)
{
  struct topo_context* con;

  con = malloc(sizeof(struct topo_context));
  if (!con) return NULL;
  con->pkt_size = 60 * 1024;	// FIXME: Do something smarter, here!
  con->pkt = malloc(con->pkt_size);
  if (!con->pkt) {
    free(con);

    return NULL;
  }

  con->myEntry = cache_init(1, meta_size, 0);
  cache_add(con->myEntry, s, meta, meta_size);

  return con;
}
