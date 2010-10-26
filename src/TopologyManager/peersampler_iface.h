#ifndef PEERSAMPLER_IFACE
#define PEERSAMPLER_IFACE

struct peersampler_iface {
  int (*init)(struct nodeID *myID, void *metadata, int metadata_size, const char *config);
  int (*change_metadata)(void *metadata, int metadata_size);
  int (*add_neighbour)(struct nodeID *neighbour, void *metadata, int metadata_size);
  int (*parse_data)(const uint8_t *buff, int len);
  const struct nodeID **(*get_neighbourhood)(int *n);
  const void *(*get_metadata)(int *metadata_size);
  int (*grow_neighbourhood)(int n);
  int (*shrink_neighbourhood)(int n);
  int (*remove_neighbour)(struct nodeID *neighbour);
};

#endif	/* PEERSAMPLER_IFACE */
