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
#include "ncast_proto.h"
#include "grapes_msg_types.h"

int ncast_reply(const struct peer_cache *c, struct peer_cache *local_cache)
{
  return topo_reply(c, local_cache, MSG_TYPE_TOPOLOGY, NCAST_REPLY, 0, 1);
}

int ncast_query_peer(struct peer_cache *local_cache, struct nodeID *dst)
{
  return topo_query_peer(local_cache, dst, MSG_TYPE_TOPOLOGY, NCAST_QUERY, 0);
}

int ncast_query(struct peer_cache *local_cache)
{
  struct nodeID *dst;

  dst = rand_peer(local_cache, NULL, 0);
  if (dst == NULL) {
    return 0;
  }
  return topo_query_peer(local_cache, dst, MSG_TYPE_TOPOLOGY, NCAST_QUERY, 0);
}
