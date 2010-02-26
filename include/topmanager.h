#ifndef TOPMAN_H
#define TOPMAN_H
const struct nodeID **topGetNeighbourhood(int *n);
const void **topGetMetadata(int *metadata_size);
int topGrowNeighbourhood(int n);
int topShrinkNeighbourhood(int n);
int topRemoveNeighbour(struct nodeID *neighbour);


int topInit(struct nodeID *myID, void *metadata, int metadata_size);
int topAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size);
int topParseData(const uint8_t *buff, int len);

/*
//const struct peer* topGetNeighbour(PeerID?) ???
//void topRegAddListener(void (*NewNeighbourCB)(const struct peer ))
//void topRegDelListener(void (*DeadNeighbourCB)(const struct peer ))
*/

#endif /* TOPMAN_H */
