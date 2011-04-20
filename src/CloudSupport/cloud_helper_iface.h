#ifndef CLOUD_HELPER_IFACE
#define CLOUD_HELPER_IFACE

#include <time.h>
#include "net_helper.h"

struct cloud_helper_impl_context;

struct cloud_helper_iface {
  struct cloud_helper_impl_context*
  (*cloud_helper_init)(struct nodeID *local, const char *config);

  int (*get_from_cloud)(struct cloud_helper_impl_context *context,
                        const char *key, uint8_t *header_ptr, int header_size,
                        int free_header);

  int (*get_from_cloud_default)(struct cloud_helper_impl_context *context, const char *key,
                                uint8_t *header_ptr, int header_size, int free_header,
                                uint8_t *defval_ptr, int defval_size, int free_defval);

  int (*put_on_cloud)(struct cloud_helper_impl_context *context,
                      const char *key, uint8_t *buffer_ptr, int buffer_size,
                      int free_buffer);

  struct nodeID* (*get_cloud_node)(struct cloud_helper_impl_context *context,
                                   uint8_t variant);

  time_t (*timestamp_cloud)(struct cloud_helper_impl_context *context);

  int (*is_cloud_node)(struct cloud_helper_impl_context *context,
                       struct nodeID* node);

  int (*wait4cloud)(struct cloud_helper_impl_context *context,
                    struct timeval *tout);

  int (*recv_from_cloud)(struct cloud_helper_impl_context *context,
                         uint8_t *buffer_ptr, int buffer_size);
};

#endif
