#ifndef TMAN_H
#define TMAN_H

/** @file tman.h
 *
 * @brief Tman interface.
 *
 * This is the Tman interface. Tman builds a given topology according to a user provided ranking
 * algorithm, by means of epidemic-style message exchanges with neighbor peers.
 *
 */

/**
  @brief Compare neighbors features.

  The functions implementing this prototype are used to compare neighbors features against a given
  target neighbor, in order to obtain a rank, to be used to build a given topology.

  @param target pointer to data that describe the target against which the ranking has to be made.
  @param p1 pointer to data that describe the first neighbor.
  @param p2 pointer to data that describe the second neighbor.
  @return 1 if p1 is to be ranked first; 2 if p2 is to be ranked first; 0 if there is a tie.
*/
typedef int (*tmanRankingFunction)(const void *target, const void *p1, const void *p2);

/*
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
//const struct nodeID **tmanGetNeighbourhood(int *n);

/*
  @brief Remove a peer from the neighbourhood.

  This function can be used to remove a specified neighbour from the
  neighbourhood. Note that the requested neighbourhood size is not
  modified, so the peer will be soon replaced by a different one.
  @param neighbour the id of the peer to be removed from the neighbourhood.
  @return 0 in case of success; -1 in case of error.
*/
//int tmanRemoveNeighbour(struct nodeID *neighbour);

/**
  @brief Initialise Tman.

  This function initializes tman protocol with all the mandatory parameters.

  @param myID the ID of this peer.
  @param metadata Pointer to data associated with the local peer.
  @param metadata_size Size (number of bytes) of the metadata associated with the local peer.
  @param rfun Ranking function to be used to order the peers in tman cache.
  @param gossip_peers Number of peers, among those in the cache, to be gossiped in a messaged exchange.
  @return 0 in case of success; -1 in case of error.
*/
int tmanInit(struct nodeID *myID, void *metadata, int metadata_size, tmanRankingFunction rfun, int gossip_peers); 
//, int max_peers, int max_idle, int periodicity

/*
  @brief Start Tman.

  This function starts Tman by feeding it with fresh peers from some known source (usually a
  peer sampling service, giving a random sample of the network).

  @param peers Array of nodeID pointers to be added in tman cache.
  @param size Number of elements in peers.
  @param metadata Pointer to the array of metadata belonging to the peers to be added.
  @param metadata_size Number of bytes of each metadata.
  @param rfun Ranking function to be used to order the peers in tman cache.
  @param best_peers
  @param gossip_peers
  @return 0 in case of success; -1 in case of error.
*/
//int tmanStart(const struct nodeID **peers, int size, const void *metadata, int metadata_size);


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
  @brief Pass a received packet to Tman.

  This function passes to Tman a packet that has
  been received from the network. The Topology Manager will parse
  such packet and run the protocol, adding or removing peers to the
  neighbourhood, and sending overlay management messages to other peers.
  @param buff a memory buffer containing the received message.
  @param len the size of such a memory buffer.
  @param peers Array of nodeID pointers to be added in tman cache.
  @param size Number of elements in peers.
  @param metadata Pointer to the array of metadata belonging to the peers to be added.
  @param metadata_size Number of bytes of each metadata.
  @return 0 in case of success; -1 in case of error.
*/
int tmanParseData(const uint8_t *buff, int len, const struct nodeID **peers, int size, const void *metadata, int metadata_size);


/**
  @brief Change the metadata in tman neighborhood.

  This function changes the metadata associated with the current node.

  @param metadata Pointer to the metadata belonging to the peer.
  @param metadata_size Number of bytes of the metadata.
  @return 1 if successful, -1 otherwise.
 */
int tmanChangeMetadata(struct nodeID *peer, void *metadata, int metadata_size);

/**
  @brief Get the metadata of the neighbors.
  @param metadata_size Address of the integer that will be set to the size of each metadata.
  @return a pointer to the array of metadata associated with the peers in the neighbourhood. NULL
          in case of error, or if the neighbourhood is empty.
*/
const void *tmanGetMetadata(int *metadata_size);


/**
  @brief Get the current neighborhood size.

  This function returns the current number of peers in the local tman neighborhood.

  @return The current size of the neighborhood.
*/
int tmanGetNeighbourhoodSize(void);

/*
	@brief Get the state of Tman.

	This function tells whether Tman is active or idle.

	@return 1 if Tman is active, 0 if it is idle.
*/
//const int tmanGetState();


//int tmanSetRankingFunction(tmanRankingFunction f);


//int tmanSetBestPeers(int p);


//int tmanSetPeriod(int p);

/**
 * @brief Get the highest ranked tman peers.
 *
 * This function allows the user to retrieve the highest ranked n peers from tman cache, along with
 * their metadata. The number of peers actually returned may be different from what is asked, depending
 * on the current size of the cache.
 * Notice that if the user asks for a number of peers that exceeds the current cache size, a join with
 * the known peers set provided via tmanParseData will be triggered at the next
 * time tmanParseData is called. This will change (and make unstable,
 * at least for few subsequent iterations) the current known topology. Thus, it is advisable to do so only if
 * necessary (i.e. when the user really needs more peers to communicate with).
 *
 * @param n The number of peer tman is asked for.
 * @param peers Array of nodeID pointers to be filled.
 * @param metadata Pointer to the array of metadata belonging to the peers to be given.
 * @return The number of elements in peers.
 */
int tmanGivePeers (int n, struct nodeID **peers, void *metadata);//, int *metadata_size);


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

#endif /* TMAN_H */
