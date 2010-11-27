#ifndef CLOUD_HELPER_IFACE
#define CLOUD_HELPER_IFACE

#include "net_helper.h"

struct cloud_helper_impl_context;

struct cloud_helper_iface {
  struct cloud_helper_impl_context* (*cloud_helper_init)(struct nodeID *local, const char *config);
  int (*get_from_cloud)(struct cloud_helper_impl_context *context, char *key, uint8_t *header_ptr, int header_size);
  int (*put_on_cloud)(struct cloud_helper_impl_context *context, char *key, uint8_t *buffer_ptr, int buffer_size);
  struct nodeID* (*get_cloud_node)(struct cloud_helper_impl_context *context);
  int (*wait4cloud)(struct cloud_helper_impl_context *context, struct timeval *tout);
  int (*recv_from_cloud)(struct cloud_helper_impl_context *context, uint8_t *buffer_ptr, int buffer_size);
};

#endif
