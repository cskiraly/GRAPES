#ifndef TOPMAN_H
#define TOPMAN_H
int topInit(struct socketID *myID);
int topAddNeighbour(struct socketID *neighbour);
int topParseData(const struct connectionID *conn, const uint8_t *buff, int len);
const struct socketID **topGetNeighbourhood(int *n);

int topGrowNeighbourhood(int n);
int topShrinkNeighbourhood(int n);
int topRemoveNeighbour(struct socketID *neighbour);
/*
//const struct peer* topGetNeighbour(PeerID?) ???
//void topRegAddListener(void (*NewNeighbourCB)(const struct peer ))
//void topRegDelListener(void (*DeadNeighbourCB)(const struct peer ))
*/

#endif /* TOPMAN_H */
