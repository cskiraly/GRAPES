#ifndef PEERSAMPLER_IFACE
#define PEERSAMPLER_IFACE

struct peersampler_context;

struct peersampler_iface {
  struct peersampler_context* (*init)(struct nodeID *myID, const void *metadata, int metadata_size, const char *config);
  int (*change_metadata)(struct peersampler_context *context, const void *metadata, int metadata_size);
  int (*add_neighbour)(struct peersampler_context *context, struct nodeID *neighbour, const void *metadata, int metadata_size);
  int (*parse_data)(struct peersampler_context *context, const uint8_t *buff, int len);
  const struct nodeID *const*(*get_neighbourhood)(struct peersampler_context *context, int *n);
  const void *(*get_metadata)(struct peersampler_context *context, int *metadata_size);
  int (*grow_neighbourhood)(struct peersampler_context *context, int n);
  int (*shrink_neighbourhood)(struct peersampler_context *context, int n);
  int (*remove_neighbour)(struct peersampler_context *context, const struct nodeID *neighbour);
};

#endif	/* PEERSAMPLER_IFACE */
