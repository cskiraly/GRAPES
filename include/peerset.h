/** @file peerset.h
 *
 * Peer Set
 *
 * The Peer Set is an abstract data structure that can contain a set of
 * peer structures. It handles peers by their nodeIDs. Peer structures
 * are created and accessed based on their nodeID (i.e. unique address).
 *
 */
 
#ifndef PEERSET_H
#define PEERSET_H

typedef struct peerset PeerSet;

 /**
  * Allocate a  peer set.
  * 
  * Create an empty peer set, and return a pointer to it.
  * 
  * @parameter size the expected number of peers that will be stored
  *                 in the set; 0 if such a number is not known.
  * @return the pointer to the new set on success, NULL on error
  */
struct peerset *peerset_init(int size);

 /**
  * Add a peer to the set.
  * 
  * Insert a peer to the set, creating the peer structure. If the peer
  * is already in the set, nothing happens.
  *
  * @parameter h a pointer to the set where the peer has to be added
  * @parameter id the ID of the peer to be inserted in the set
  * @return > 0 if the peer is correctly inserted in the set, 0 if a peer with
  *         the same nodeID is already in the set, < 0 on error
  */
int peerset_add_peer(struct peerset *h, const struct nodeID *id);

 /**
  * Add peers to the set.
  * 
  * Comodity function to add several peers at the same time to the set.
  * See peerset_add_peer
  *
  * @parameter h a pointer to the set where the peer has to be added
  * @parameter ids the IDs of the peers to be inserted in the set
  * @parameter n length of the its array
  */
void peerset_add_peers(struct peerset *h, const struct nodeID **ids, int n);

 /**
  * Remove a peer from the set.
  * 
  * Remove a peer from the set, distroying all associated data.
  * If peer exists, pointers of peerset_get_peers move backwards.
  *
  * @parameter h a pointer to the set where the peer has to be added
  * @parameter id the ID of the peer to be removed from the set
  * @return > 0 if the peer is correctly removed from the set,
  *         < 0 on error
  */
int peerset_remove_peer(struct peerset *h, const struct nodeID *id);

 /**
  * Get a peer if it is in the set
  * 
  * @parameter h a pointer to the set
  * @parameter id the nodeID we are searching for
  * @return a pointer to the peer if it is present in the set,
  *         NULL if the peer is not in the set
  */
struct peer *peerset_get_peer(const struct peerset *h, const struct nodeID *id);

 /**
  * Get the set size
  * 
  * Return the number of peers present in a set.
  *
  * @parameter h a pointer to the set
  * @return the number of peers in the set, or < 0 on error
  */
int peerset_size(const struct peerset *h);

 /**
  * Get a peer from a set
  * 
  * Return the peers of the set. The peer's priority is
  * assumed to depend on i.
  *
  * @parameter h a pointer to the set
  * @return the list of peer structures
  */
struct peer *peerset_get_peers(const struct peerset *h);

 /**
  * Check if a peer is in a set
  * 
  * @parameter h a pointer to the set
  * @parameter id the nodeID we are searching for
  * @return the position of the peer if it is present in the set,
  *         < 0 on error or if the peer is not in the set
  */
int peerset_check(const struct peerset *h, const struct nodeID *id);


 /**
  * Clear a set
  * 
  * Remove all the peers from a set.
  *
  * @parameter h a pointer to the set
  * @parameter size the expected number of peers that will be stored
  *                 in the set; 0 if such a number is not known.
  */
void peerset_clear(struct peerset *h, int size);

#endif	/* PEERSET_H */
