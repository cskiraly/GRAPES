#ifndef TOPMAN_H
#define TOPMAN_H

/** @file topmanager.h 
 *
 * This is the Topology Manager interface. See @link topology_test.c
 * topology_test.c @endlink for an usage example
 *
 */

/** @example topology_test.c
 * 
 * A test program showing how to use the Topology Manager API.
 *
 */


/**
  @brief Get the peer's neighbourhood.

  This function returns the current neighbourhood (i.e., the set of
  known peers) of a peer, and its size. Note that the current neighbourhood
  size can be different from the requested one, because of how the overlay
  management protocols work. 

  @param n pointer to an integer where the neighbourhood size is returned.
           Is set to -1 in case of error, or 0 if the neighbourhood is empty.
  @return a pointer to an array of nodeID describing the neighbourhood. NULL
          in case of error, or if the neighbourhood is empty.
*/
const struct nodeID **topGetNeighbourhood(int *n);

/**
  @brief Increase the neighbourhood size.

  This function can be used to increase the number of neighbours (that is,
       the degree of the overlay graph) by a specified amount.
  @param n number of peers by which the neighbourhood size must be incremented.
  @return the new neighbourhood size in case of success; -1 in case of error.
*/
int topGrowNeighbourhood(int n);

/**
  @brief Decrease the neighbourhood size.

  This function can be used to reduce the number of neighbours (that is,
       the degree of the overlay graph) by a specified amount.
  @param n number of peers by which the neighbourhood size must be decreased.
  @return the new neighbourhood size in case of success; -1 in case of error.
*/
int topShrinkNeighbourhood(int n);

/**
  @brief Remove a peer from the neighbourhood.

  This function can be used to remove a specified neighbour from the
  neighbourhood. Note that the requested neighbourhood size is not
  modified, so the peer will be soon replaced by a different one.
  @param neighbour the id of the peer to be removed from the
         neighbourhood.
  @return 0 in case of success; -1 in case of error.
*/
int topRemoveNeighbour(struct nodeID *neighbour);

/**
  @brief Initialise the Topology Manager.

  @param myID the ID of this peer.
  @return 0 in case of success; -1 in case of error.
*/
int topInit(struct nodeID *myID);


/**
  @brief Insert a peer in the neighbourhood.

  This function can be used to add a specified neighbour to the
  neighbourhood. It is useful in the bootstrap phase.
  @param neighbour the id of the peer to be added to the
         neighbourhood.
  @return 0 in case of success; -1 in case of error.
*/
int topAddNeighbour(struct nodeID *neighbour);

/**
  @brief Pass a received packet to the Topology Manager.

  This function passes to the Topology Manager a packet that has
  been received from the network. The Topology Manager will parse
  such packet and run the protocol, adding or removing peers to the
  neighbourhood, and sending overlay management messages to other peers.
  @param buff a memory buffer containing the received message.
  @param len the size of such a memory buffer.
  @return 0 in case of success; -1 in case of error.
*/
int topParseData(const uint8_t *buff, int len);

/*
//const struct peer* topGetNeighbour(PeerID?) ???
//void topRegAddListener(void (*NewNeighbourCB)(const struct peer ))
//void topRegDelListener(void (*DeadNeighbourCB)(const struct peer ))
*/

#endif /* TOPMAN_H */
