/** @file peerset.h
 *
 * @brief Structure containing a set of peers.
 *
 * The Peer Set is an abstract data structure that can contain a set of
 * peer structures. It handles peers by their nodeIDs. Peer structures
 * are created and accessed based on their nodeID (i.e. unique address).
 *
 */
 
#ifndef PEERSET_H
#define PEERSET_H


/**
* Opaque data type representing a Peer Set
*/
typedef struct peerset PeerSet;

 /**
  * @brief Allocate a  peer set.
  * 
  * Create an empty peer set, and return a pointer to it.
  * 
  * @param config a string containing tags which describe the peerset.
  *                   For example, the "size" tag indicates the expected
  *                   number of peers that will be stored in the set;
  *                   0 or not present if such a number is not known.
  * @return the pointer to the new set on success, NULL on error
  */
struct peerset *peerset_init(const char *config);

 /**
  * @brief Add a peer to the set.
  * 
  * Insert a peer to the set, creating the peer structure. If the peer
  * is already in the set, nothing happens.
  *
  * @param h a pointer to the set where the peer has to be added
  * @param id the ID of the peer to be inserted in the set
  * @return > 0 if the peer is correctly inserted in the set, 0 if a peer with
  *         the same nodeID is already in the set, < 0 on error
  */
int peerset_add_peer(struct peerset *h, struct nodeID *id);

 /**
  * @brief Add peers to the set.
  * 
  * Comodity function to add several peers at the same time to the set.
  * See peerset_add_peer
  *
  * @param h a pointer to the set where the peer has to be added
  * @param ids the IDs of the peers to be inserted in the set
  * @param n length of the its array
  */
void peerset_add_peers(struct peerset *h, struct nodeID **ids, int n);

 /**
  * @brief Remove a peer from the set.
  * 
  * Remove a peer from the set, distroying all associated data.
  * If peer exists, pointers of peerset_get_peers move backwards.
  *
  * @param h a pointer to the set where the peer has to be added
  * @param id the ID of the peer to be removed from the set
  * @return > 0 if the peer is correctly removed from the set,
  *         < 0 on error
  */
int peerset_remove_peer(struct peerset *h, const struct nodeID *id);

 /**
  * @brief Get a peer if it is in the set
  * 
  * @param h a pointer to the set
  * @param id the nodeID we are searching for
  * @return a pointer to the peer if it is present in the set,
  *         NULL if the peer is not in the set
  */
struct peer *peerset_get_peer(const struct peerset *h, const struct nodeID *id);

 /**
  * @brief Get the set size
  * 
  * Return the number of peers present in a set.
  *
  * @param h a pointer to the set
  * @return the number of peers in the set, or < 0 on error
  */
int peerset_size(const struct peerset *h);

 /**
  * @brief Get a peer from a set
  * 
  * Return the peers of the set. The peer's priority is
  * assumed to depend on i.
  *
  * @param h a pointer to the set
  * @return the list of peer structures
  */
struct peer **peerset_get_peers(const struct peerset *h);

 /**
  * @brief Check if a peer is in a set
  * 
  * @param h a pointer to the set
  * @param id the nodeID we are searching for
  * @return the position of the peer if it is present in the set,
  *         < 0 on error or if the peer is not in the set
  */
int peerset_check(const struct peerset *h, const struct nodeID *id);


 /**
  * @brief Clear a set
  * 
  * Remove all the peers from a set.
  *
  * @param h a pointer to the set
  * @param size the expected number of peers that will be stored
  *                 in the set; 0 if such a number is not known.
  */
void peerset_clear(struct peerset *h, int size);

#endif	/* PEERSET_H */
