/*
 *  Copyright (c) 2011 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <stdio.h>

#include "cloud_helper_iface.h"
#include "config.h"

struct delegate_iface {
  void* (*cloud_helper_init)(struct nodeID *local, const char *config);

  int (*get_from_cloud)(void *context, const char *key, uint8_t *header_ptr,
                        int header_size, int free_header);

  int (*get_from_cloud_default)(void *context, const char *key,
                                uint8_t *header_ptr, int header_size, int free_header,
                                uint8_t *defval_ptr, int defval_size, int free_defval);

  int (*put_on_cloud)(void *context, const char *key, uint8_t *buffer_ptr,
                      int buffer_size, int free_buffer);

  struct nodeID* (*get_cloud_node)(void *context, uint8_t variant);

  time_t (*timestamp_cloud)(void *context);

  int (*is_cloud_node)(void *context, struct nodeID* node);

  int (*wait4cloud)(void *context, struct timeval *tout);

  int (*recv_from_cloud)(void *context, uint8_t *buffer_ptr, int buffer_size);
};

struct cloud_helper_impl_context {
  struct delegate_iface *delegate;
  void *delegate_context;
};

static struct cloud_helper_impl_context*
delegate_cloud_init(struct nodeID *local, const char *config)
{
  struct cloud_helper_impl_context *ctx;
  struct tag *cfg_tags;
  const char *dlib_name;
  struct delegate_iface *delegate_impl;
  void *dlib;

  cfg_tags = config_parse(config);
  dlib_name = config_value_str(cfg_tags, "delegate_lib");
  dlib = dlopen(dlib_name, RTLD_NOW);
  if (dlib == NULL) {
    printf("error: %s", dlerror());
    return NULL;
  }


  delegate_impl = (struct delegate_iface *) dlsym(dlib, "delegate_impl");
  if (!delegate_impl) return NULL;

  ctx = malloc(sizeof(struct cloud_helper_impl_context));
  ctx->delegate = delegate_impl;

  ctx->delegate_context = ctx->delegate->cloud_helper_init(local, config);
  if(!ctx->delegate_context) {
    free(ctx);
    return NULL;
  }

  return ctx;
}

static int
delegate_cloud_get_from_cloud(struct cloud_helper_impl_context *context,
                              const char *key, uint8_t *header_ptr,
                              int header_size, int free_header)
{
  return context->delegate->get_from_cloud(context->delegate_context, key,
                                           header_ptr, header_size,
                                           free_header);
}

static int
delegate_cloud_get_from_cloud_default(struct cloud_helper_impl_context *context,
                                      const char *key, uint8_t *header_ptr,
                                      int header_size, int free_header,
                                      uint8_t *defval_ptr, int defval_size,
                                      int free_defval)
{
  return context->delegate->get_from_cloud_default(context->delegate_context,
                                                   key, header_ptr, header_size,
                                                   free_header, defval_ptr,
                                                   defval_size, free_defval);
}

static int
delegate_cloud_put_on_cloud(struct cloud_helper_impl_context *context,
                            const char *key, uint8_t *buffer_ptr,
                            int buffer_size, int free_buffer)
{
  return context->delegate->put_on_cloud(context->delegate_context, key,
                                         buffer_ptr, buffer_size, free_buffer);
}
static struct nodeID*
delegate_cloud_get_cloud_node(struct cloud_helper_impl_context *context,
                              uint8_t variant)
{
  return context->delegate->get_cloud_node(context->delegate_context, variant);
}

static time_t
delegate_timestamp_cloud(struct cloud_helper_impl_context *context)
{
  return context->delegate->timestamp_cloud(context->delegate_context);
}

int delegate_is_cloud_node(struct cloud_helper_impl_context *context,
                           struct nodeID* node)
{
  return context->delegate->is_cloud_node(context->delegate_context, node);
}

static int delegate_cloud_wait4cloud(struct cloud_helper_impl_context *context,
                                     struct timeval *tout)
{
  return context->delegate->wait4cloud(context->delegate_context, tout);
}

static int
delegate_cloud_recv_from_cloud(struct cloud_helper_impl_context *context,
                               uint8_t *buffer_ptr, int buffer_size)
{
  return context->delegate->recv_from_cloud(context->delegate_context,
                                            buffer_ptr, buffer_size);
}

struct cloud_helper_iface delegate = {
  .cloud_helper_init = delegate_cloud_init,
  .get_from_cloud = delegate_cloud_get_from_cloud,
  .get_from_cloud_default = delegate_cloud_get_from_cloud_default,
  .put_on_cloud = delegate_cloud_put_on_cloud,
  .get_cloud_node = delegate_cloud_get_cloud_node,
  .timestamp_cloud = delegate_timestamp_cloud,
  .is_cloud_node = delegate_is_cloud_node,
  .wait4cloud = delegate_cloud_wait4cloud,
  .recv_from_cloud = delegate_cloud_recv_from_cloud,
};
