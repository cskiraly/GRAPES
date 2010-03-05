/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "net_helper.h"
#include "nccache.h"
#include "proto.h"
#include "ncast_proto.h"
#include "msg_types.h"

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

int ncast_reply(const uint8_t *payload, int psize, struct peer_cache *local_cache)
{
  uint8_t pkt[1500];
  struct ncast_header *h = (struct ncast_header *)pkt;
  struct peer_cache *c = entries_undump(payload, psize);
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
  h->protocol = MSG_TYPE_TOPOLOGY;
  h->type = NCAST_REPLY;
  len = ncast_payload_fill(pkt + sizeof(struct ncast_header), 1500 - sizeof(struct ncast_header), local_cache, dst);

  res = send_to_peer(nodeid(myEntry, 0), dst, pkt, sizeof(struct ncast_header) + len);
  cache_free(c);
  return res;
}

int ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst)
{
  uint8_t pkt[1500];
  struct ncast_header *h = (struct ncast_header *)pkt;
  int len;

  h->protocol = MSG_TYPE_TOPOLOGY;
  h->type = NCAST_QUERY;
  len = ncast_payload_fill(pkt + sizeof(struct ncast_header), 1500 - sizeof(struct ncast_header), local_cache, dst);
  return send_to_peer(nodeid(myEntry, 0), dst, pkt, sizeof(struct ncast_header) + len);
}

int ncast_query(struct peer_cache *local_cache)
{
  struct nodeID *dst;

  dst = rand_peer(local_cache);
  if (dst == NULL) {
    return 0;
  }
  return ncast_query_peer(local_cache, dst);
}

int ncast_proto_init(struct nodeID *s, void *meta, int meta_size)
{
  myEntry = cache_init(1, meta_size);
  cache_add(myEntry, s, meta, meta_size);

  return 0;
}
