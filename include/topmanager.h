#ifndef TOPMAN_H
#define TOPMAN_H
int topInit(struct nodeID *myID);
int topAddNeighbour(struct nodeID *neighbour);
int topParseData(const uint8_t *buff, int len);
const struct nodeID **topGetNeighbourhood(int *n);

int topGrowNeighbourhood(int n);
int topShrinkNeighbourhood(int n);
int topRemoveNeighbour(struct nodeID *neighbour);
/*
//const struct peer* topGetNeighbour(PeerID?) ???
//void topRegAddListener(void (*NewNeighbourCB)(const struct peer ))
//void topRegDelListener(void (*DeadNeighbourCB)(const struct peer ))
*/

#endif /* TOPMAN_H */
