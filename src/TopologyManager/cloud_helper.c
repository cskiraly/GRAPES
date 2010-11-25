#include <stdlib.h>
#include <string.h>

#include "cloud_helper.h"
#include "cloud_helper_iface.h"
#include "config.h"

extern struct cloud_helper_iface delegate;

struct cloud_helper_context {
  struct cloud_helper_iface *ch;
  struct cloud_helper_impl_context *ch_context;
};

static int ctx_counter = 0;
static struct nodeID* node_ids[CLOUD_HELPER_MAX_INSTANCES];
static struct cloud_helper_context* cloud_ctxs[CLOUD_HELPER_MAX_INSTANCES];

static int add_context(struct nodeID *local, struct cloud_helper_context *ctx)
{
  int i;
  if (ctx_counter >= CLOUD_HELPER_MAX_INSTANCES) return 1;
  
  for (i=0; i<ctx_counter; i++)
    if (nodeid_equal(node_ids[i], local)) return 0;

  node_ids[ctx_counter] = local;
  cloud_ctxs[ctx_counter] = ctx;
  ctx_counter++;
  
  return 1;
}

struct cloud_helper_context* cloud_helper_init(struct nodeID *local, const char *config)
{
  struct cloud_helper_context *ctx;
  struct tag *cfg_tags;
  const char *provider;

  cfg_tags = config_parse(config);
  provider = config_value_str(cfg_tags, "provider");

  if (!provider) return NULL;

  ctx = malloc(sizeof(struct cloud_helper_context));
  if (!ctx) return NULL;
  if (strcmp(provider, "delegate") == 0){
    ctx->ch = &delegate;
  }
   
 ctx->ch_context = ctx->ch->cloud_helper_init(local, config);
 if(!ctx->ch_context){
   free(ctx);
   return NULL;
 }

 if (!add_context(local, ctx)){
   //TODO: a better deallocation process is needed
   free(ctx->ch_context);
   free(ctx);
   return NULL;
 }

 return ctx;
}

struct cloud_helper_context* get_cloud_helper_for(struct nodeID *local){
  int i;
  for (i=0; i<ctx_counter; i++)
    if (node_ids[i] == local) return cloud_ctxs[i];
  
  return NULL;
}

int get_from_cloud(struct cloud_helper_context *context, char *key)
{
  return context->ch->get_from_cloud(context->ch_context, key);
}

int put_on_cloud(struct cloud_helper_context *context, char *key, uint8_t *buffer_ptr, int buffer_size)
{
  return context->ch->put_on_cloud(context->ch_context, key, buffer_ptr, buffer_size);
}

struct nodeID* get_cloud_node(struct cloud_helper_context *context)
{
  return context->ch->get_cloud_node(context->ch_context);
}

int wait4cloud(struct cloud_helper_context *context, struct timeval *tout)
{
  return context->ch->wait4cloud(context->ch_context, tout);
}

int recv_from_cloud(struct cloud_helper_context *context, uint8_t *buffer_ptr, int buffer_size)
{
  return context->ch->recv_from_cloud(context->ch_context, buffer_ptr, buffer_size);
}

