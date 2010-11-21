typedef int (*rankingFunction)(const void *target, const void *p1, const void *p2);	// FIXME!

struct topman_iface {
  int (*init)(struct nodeID *myID, void *metadata, int metadata_size, rankingFunction rfun, const char *config);
  int (*changeMetadata)(void *metadata, int metadata_size);
  int (*addNeighbour)(struct nodeID *neighbour, void *metadata, int metadata_size);
  int (*parseData)(const uint8_t *buff, int len, struct nodeID **peers, int size, const void *metadata, int metadata_size);
  int (*givePeers)(int n, struct nodeID **peers, void *metadata);
  const void *(*getMetadata)(int *metadata_size);
  int (*growNeighbourhood)(int n);
  int (*shrinkNeighbourhood)(int n);
  int (*removeNeighbour)(struct nodeID *neighbour);
  int (*getNeighbourhoodSize)(void);
};
