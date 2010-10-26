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
#include "cyclon_proto.h"
#include "grapes_msg_types.h"


int cyclon_reply(const struct peer_cache *c, struct peer_cache *local_cache)
{
  return topo_reply(c, local_cache, MSG_TYPE_TOPOLOGY, CYCLON_REPLY, 0, 0);
}

int cyclon_query(struct peer_cache *sent_cache, struct nodeID *dst)
{
  return topo_query_peer(sent_cache, dst, MSG_TYPE_TOPOLOGY, CYCLON_QUERY, 0);
}
