#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "msglayer.h"
#include "nccache.h"
#include "proto.h"
#include "ncast_proto.h"

static struct cache_entry *myEntry;

int ncast_payload_fill(uint8_t *payload, int size, struct cache_entry *c, struct nodeID *snot)
{
  int i;
  uint8_t *p = payload;

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

int ncast_reply(const uint8_t *payload, int psize, struct cache_entry *local_cache)
{
  uint8_t pkt[1500];
  struct ncast_header *h = (struct ncast_header *)pkt;
  const struct cache_entry *c = entries_undump(payload, psize);
  int len;
  struct nodeID *dst;

#if 0
  n = psize / sizeof(struct cache_entry);
  if (n * sizeof(struct cache_entry) != psize) {
    fprintf(stderr, "Wrong number of elems %d (%d / %d)!!!\n", n, psize, sizeof(struct cache_entry));
    return -1;
  }
#endif
  dst = nodeid(c, 0);
  h->protocol = PROTO_NCAST;
  h->type = NCAST_REPLY;
  len = ncast_payload_fill(pkt + sizeof(struct ncast_header), 1500 - sizeof(struct ncast_header), local_cache, dst);
  free((void *)c);

  return send_data(nodeid(myEntry, 0), dst, pkt, sizeof(struct ncast_header) + len);
}

int ncast_query_peer(struct cache_entry *local_cache, struct nodeID *dst)
{
  uint8_t pkt[1500];
  struct ncast_header *h = (struct ncast_header *)pkt;
  int len;

  h->protocol = PROTO_NCAST;
  h->type = NCAST_QUERY;
  len = ncast_payload_fill(pkt + sizeof(struct ncast_header), 1500 - sizeof(struct ncast_header), local_cache, dst);
  return send_data(nodeid(myEntry, 0), dst, pkt, sizeof(struct ncast_header) + len);
}

int ncast_query(struct cache_entry *local_cache)
{
  struct nodeID *dst;

  dst = rand_peer(local_cache);
  if (dst == NULL) {
    return 0;
  }
  return ncast_query_peer(local_cache, dst);
}

int ncast_proto_init(struct nodeID *s)
{
  myEntry = cache_init(2);
  cache_add(myEntry, s);

  return 0;
}
