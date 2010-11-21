#ifndef TMAN_H
#define TMAN_H

/** @file tman.h
 *
 * @brief Topology Manager interface.
 *
 * This is the Topology Manager interface.
 *
 */

/**
  @brief Compare neighbors features.

  The functions implementing this prototype may be used to compare neighbors features against a given
  target neighbor, in order to obtain a rank, to be used to build a given topology.

  @param target pointer to data that describe the target against which the ranking has to be made.
  @param p1 pointer to data that describe the first neighbor.
  @param p2 pointer to data that describe the second neighbor.
  @return 1 if p1 is to be ranked first; 2 if p2 is to be ranked first; 0 if there is a tie.
*/
typedef int (*tmanRankingFunction)(const void *target, const void *p1, const void *p2);

/**
  @brief Initialise the Topology Manager.

  This function initializes the Topology Manager protocol with all the mandatory parameters.

  @param myID the ID of this peer.
  @param metadata Pointer to data associated with the local peer.
  @param metadata_size Size (number of bytes) of the metadata associated with the local peer.
  @param rfun Ranking function that may be used to order the peers in tman cache.
  @param config Pointer to a configuration file that contains all relevant initialization data.
  @return 0 in case of success; -1 in case of error.
*/
int tmanInit(struct nodeID *myID, void *metadata, int metadata_size, tmanRankingFunction rfun, const char *config); 


/**
  @brief Insert a peer in the neighbourhood.

  This function can be used to add a specified neighbour to the
  neighbourhood.
  @param neighbour the id of the peer to be added to the neighbourhood.
  @param metadata Pointer to the array of metadata belonging to the peers to be added.
  @param metadata_size Number of bytes of each metadata.
  @return 0 in case of success; -1 in case of error.
*/
int tmanAddNeighbour(struct nodeID *neighbour, void *metadata, int metadata_size);

/**
  @brief Pass a received packet to Topology Manager.

  This function passes to Topology Manager a packet that has
  been received from the network. The Topology Manager will parse
  such packet and run the protocol, adding or removing peers to the
  neighbourhood, and sending overlay management messages to other peers.
  @param buff a memory buffer containing the received message.
  @param len the size of such a memory buffer.
  @param peers Array of nodeID pointers to be added in Topology Manager cache.
  @param size Number of elements in peers.
  @param metadata Pointer to the array of metadata belonging to the peers to be added.
  @param metadata_size Number of bytes of each metadata.
  @return 0 in case of success; -1 in case of error.
*/
int tmanParseData(const uint8_t *buff, int len, struct nodeID **peers, int size, const void *metadata, int metadata_size);


/**
  @brief Change the metadata in Topology Manager neighborhood.

  This function changes the metadata associated with the current node.

  @param metadata Pointer to the metadata belonging to the peer.
  @param metadata_size Number of bytes of the metadata.
  @return 1 if successful, -1 otherwise.
 */
int tmanChangeMetadata(void *metadata, int metadata_size);

/**
  @brief Get the metadata of the neighbors.
  @param metadata_size Address of the integer that will be set to the size of each metadata.
  @return a pointer to the array of metadata associated with the peers in the neighbourhood. NULL
          in case of error, or if the neighbourhood is empty.
*/
const void *tmanGetMetadata(int *metadata_size);


/**
  @brief Get the current neighborhood size.

  This function returns the current number of peers in the local Topology Manager neighborhood.

  @return The current size of the neighborhood.
*/
int tmanGetNeighbourhoodSize(void);


/**
 * @brief Get the best peers.
 *
 * This function allows the user to retrieve the best n peers from Topology Manager cache, along with
 * their metadata. The number of peers actually returned may be different from what is asked, depending
 * on the current size of the cache.
 *
 * @param n The number of peer the Topology Manager is asked for.
 * @param peers Array of nodeID pointers to be filled.
 * @param metadata Pointer to the array of metadata belonging to the peers to be given.
 * @return The number of elements in peers.
 */
int tmanGivePeers (int n, struct nodeID **peers, void *metadata);


/**
  @brief Increase the neighbourhood size.

  This function can be used to increase the number of neighbours (that is,
       the degree of the overlay graph) by a specified amount.
       The actual size change will be done the next time tmanParseData is called.
       As currently implemented, it doubles the current size at most.
       If the argument is negative or a resize has already been requested, but not performed yet,
       no change will occur and an error code is returned.
  @param n number of peers by which the neighbourhood size must be incremented.
  @return the new neighbourhood size in case of success; -1 in case of error.
*/
int tmanGrowNeighbourhood(int n);


/**
  @brief Decrease the neighbourhood size.

  This function can be used to reduce the number of neighbours (that is,
       the degree of the overlay graph) by a specified amount.
       The actual size change will be done the next time tmanParseData is called.
       If the argument is negative, or greater equal than the current cache size,
        or a resize has already been requested, but not performed yet,
       no change will occur and an error code is returned.
  @param n number of peers by which the neighbourhood size must be decreased.
  @return the new neighbourhood size in case of success; -1 in case of error.
*/
int tmanShrinkNeighbourhood(int n);


/**
 * @brief Remove a neighbour.

  Delete a neighbour from the Topology Manager neighborhood.

  @param neighbour Pointer to the nodeID of the neighbor to be removed.
  @return 1 if removal was successful, -1 if it was not.
 */
int tmanRemoveNeighbour(struct nodeID *neighbour);

#endif /* TMAN_H */

