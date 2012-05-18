/*
 *  Copyright (c) 2010 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cloud_helper.h"
#include "cloud_helper_iface.h"

#include "../Utils/fifo_queue.h"

#include "config.h"

#define CLOUD_HELPER_INITAIL_INSTANCES 2

#ifdef DELEGATE
extern struct cloud_helper_iface delegate;
#endif

struct cloud_helper_context {
  struct cloud_helper_iface *ch;
  struct cloud_helper_impl_context *ch_context;
};

struct ctx_map_entry {
  const struct nodeID *node;
  struct cloud_helper_context *cloud_ctx;
};

static struct fifo_queue *ctx_map = NULL;

static int add_context(const struct nodeID *local,
                       struct cloud_helper_context *ctx)
{
  int i;
  struct ctx_map_entry *entry;

  /* Checks whether the queue is already initialized */
  if (ctx_map == NULL) {
    ctx_map = fifo_queue_create(CLOUD_HELPER_INITAIL_INSTANCES);
    if (!ctx_map) {
      return 1;
    }
  }

  /* Check if the node is already present in the ctx_map */
  for (i=0; i<fifo_queue_size(ctx_map); i++) {
    entry = fifo_queue_get(ctx_map, i);
    if (nodeid_equal(entry->node, local)) {
      return 1;
    }
  }

  /* Add the new entry to the ctx_map */
  entry = malloc(sizeof(struct ctx_map_entry));
  if (!entry) {
    return 1;
  }

  entry->node = local;
  entry->cloud_ctx = ctx;

  if (fifo_queue_add(ctx_map, entry) != 0) {
    free (entry);
    return 1;
  }

  return 0;
}

struct cloud_helper_context* cloud_helper_init(struct nodeID *local,
                                               const char *config)
{
  struct cloud_helper_context *ctx;
  struct tag *cfg_tags;
  const char *provider;

  cfg_tags = config_parse(config);
  provider = config_value_str(cfg_tags, "provider");

  if (!provider) return NULL;

  ctx = malloc(sizeof(struct cloud_helper_context));
  if (!ctx) return NULL;
  memset(ctx, 0, sizeof(struct cloud_helper_context));
#ifdef DELEGATE
  if (strcmp(provider, "delegate") == 0){
    ctx->ch = &delegate;
  }
#endif
  if (!ctx->ch) {
    free(ctx);

    return NULL;
  }

 ctx->ch_context = ctx->ch->cloud_helper_init(local, config);
 if(!ctx->ch_context){
   free(ctx);
   return NULL;
 }

 if (add_context(local, ctx) != 0){
   //TODO: a better deallocation process is needed
   free(ctx->ch_context);
   free(ctx);
   return NULL;
 }

 return ctx;
}

struct cloud_helper_context* get_cloud_helper_for(const struct nodeID *local){
  struct cloud_helper_context *ctx;
  struct ctx_map_entry *entry;
  int i;

  ctx = NULL;
  for (i=0; i<fifo_queue_size(ctx_map); i++) {
    entry = fifo_queue_get(ctx_map, i);
    if (nodeid_equal(entry->node, local)) ctx = entry->cloud_ctx;
  }
  return ctx;
}

int get_from_cloud(struct cloud_helper_context *context, const char *key,
                   uint8_t *header_ptr, int header_size, int free_header)
{
  return context->ch->get_from_cloud(context->ch_context, key, header_ptr,
                                     header_size, free_header);
}

int get_from_cloud_default(struct cloud_helper_context *context, const char *key,
                           uint8_t *header_ptr, int header_size, int free_header,
                           uint8_t *defval_ptr, int defval_size, int free_defval)
{
  return context->ch->get_from_cloud_default(context->ch_context, key, header_ptr,
                                             header_size, free_header, defval_ptr,
                                             defval_size, free_defval);
}

int put_on_cloud(struct cloud_helper_context *context, const char *key,
                 uint8_t *buffer_ptr, int buffer_size, int free_buffer)
{
  return context->ch->put_on_cloud(context->ch_context, key, buffer_ptr,
                                   buffer_size, free_buffer);
}

struct nodeID* get_cloud_node(struct cloud_helper_context *context,
                              uint8_t variant)
{
  return context->ch->get_cloud_node(context->ch_context, variant);
}

time_t timestamp_cloud(struct cloud_helper_context *context)
{
  return context->ch->timestamp_cloud(context->ch_context);
}

int is_cloud_node(struct cloud_helper_context *context, struct nodeID* node)
{
  return context->ch->is_cloud_node(context->ch_context, node);
}

int wait4cloud(struct cloud_helper_context *context, struct timeval *tout)
{
  return context->ch->wait4cloud(context->ch_context, tout);
}

int recv_from_cloud(struct cloud_helper_context *context, uint8_t *buffer_ptr,
                    int buffer_size)
{
  return context->ch->recv_from_cloud(context->ch_context, buffer_ptr,
                                      buffer_size);
}
