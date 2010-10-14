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

#if 0

int ncast_init(struct nodeID *myID, void *metadata, int metadata_size, const char *config);
int ncast_change_metadata(void *metadata, int metadata_size);
int ncast_add_neighbourhood(struct nodeID *neighbour, void *metadata, int metadata_size);
int ncast_parse_data(const uint8_t *buff, int len);
const struct nodeID **ncast_get_neighbourhood(int *n);
const void *ncast_get_metadata(int *metadata_size);
int ncast_grow_neighbourhood(int n);
int ncast_shrink_neighbourhood(int n);
int ncast_remove_neighbour(struct nodeID *neighbour);
#endif
