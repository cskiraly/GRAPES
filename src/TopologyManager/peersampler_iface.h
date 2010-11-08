#ifndef PEERSAMPLER_IFACE
#define PEERSAMPLER_IFACE

struct peersampler_iface {
  int (*init)(struct nodeID *myID, void *metadata, int metadata_size, const char *config, void **context);
  int (*change_metadata)(void *context, void *metadata, int metadata_size);
  int (*add_neighbour)(void *context, struct nodeID *neighbour, void *metadata, int metadata_size);
  int (*parse_data)(void *context, const uint8_t *buff, int len);
  const struct nodeID **(*get_neighbourhood)(void *context, int *n);
  const void *(*get_metadata)(void *context, int *metadata_size);
  int (*grow_neighbourhood)(void *context, int n);
  int (*shrink_neighbourhood)(void *context, int n);
  int (*remove_neighbour)(void *context, struct nodeID *neighbour);
};

#endif	/* PEERSAMPLER_IFACE */
