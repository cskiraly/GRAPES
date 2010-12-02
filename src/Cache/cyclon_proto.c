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

struct cyclon_proto_context {
  struct topo_context *context;
};

struct cyclon_proto_context* cyclon_proto_init(struct nodeID *s, const void *meta, int meta_size)
{
  struct cyclon_proto_context *con;
  con = malloc(sizeof(struct cyclon_proto_context));

  if (!con) return NULL;

  con->context = topo_proto_init(s, meta, meta_size);
  if (!con->context){
    free(con);
    return NULL;
  }

  return con;
}

int cyclon_reply(struct cyclon_proto_context *context, const struct peer_cache *c, const struct peer_cache *local_cache)
{
  return topo_reply(context->context, c, local_cache, MSG_TYPE_TOPOLOGY, CYCLON_REPLY, 0, 0);
}

int cyclon_query(struct cyclon_proto_context *context, const struct peer_cache *sent_cache, struct nodeID *dst)
{
  return topo_query_peer(context->context, sent_cache, dst, MSG_TYPE_TOPOLOGY, CYCLON_QUERY, 0);
}

int cyclon_proto_change_metadata(struct cyclon_proto_context *context, const void *metadata, int metadata_size)
{
  if (topo_proto_metadata_update(context->context, metadata, metadata_size) <= 0) {
    return -1;
  }

  return 1;
}
