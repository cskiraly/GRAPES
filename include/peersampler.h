#ifndef PEERSAMPLER_H
#define PEERSAMPLER_H

/** @file peersampler.h 
 *
 * @brief Peer Sampler interface.
 *
 * This is the Peer Sampler interface. See @link topology_test.c
 * topology_test.c @endlink for a simple usage example, 
 * @link topology_test_th.c topology_test_th.c @endlink for an
 * example using multiple threads, and @link topology_test_attr.c
 * topology_test_attr.c @endlink for an example with metadata.
 *
 */

/** @example topology_test.c
 * 
 * A simple example showing how to use the Peer Sampler API.
 *
 */

/** @example topology_test_th.c
 * 
 * An example showing how to use the Peer Sampler API with multiple threads.
 *
 */

/** @example topology_test_attr.c
 * 
 * An example showing how to use the Peer Sampler API with peers' attributes
 * (metadata).
 *
 */


/**
   @brief Mantains the context of a topology manager instance
 */
struct psample_context;

/**
  @brief Get a sample of the active peers.

  This function returns the current peer sampler cache (i.e., the set of
  known peers), and its size. Note that the current cache 
  size can be different from the requested one, because of how the peer 
  sampling protocols work. 

  @param tc the pointer to the current topology manager instance context
  @param n pointer to an integer where the cache size is returned.
           Is set to -1 in case of error, or 0 if the cache is empty.
  @return a pointer to an array of nodeID describing a random view of the system. NULL
          in case of error, or if the cache is empty.
*/
const struct nodeID *const *psample_get_cache(struct psample_context *tc, int *n);

/**
  @brief Get the peer's metadata.

  Each known peer can have some opaque metadata attached to
  it, and such metadata is gossiped together with the nodeIDs.
  This function returns the metadata currently associated to the
  known peers, and the size of each object composing the metadata
  (metadata have the same structure for all the peers, so such size is
  constant). The number of known peers (cache size) can be known by calling
  psample_get_cache().

  @param tc the pointer to the current topology manager instance context
  @param metadata_size pointer to an integer where the size of the metadata
         associated to each peer is returned.
         Is set to -1 in case of error, or 0 if the cache is empty.
  @return a pointer to an array of metadata (ordered as the peers returned
          by psample_get_cache()).
          NULL in case of error, or if the cache is empty.
*/
const void *psample_get_metadata(struct psample_context *tc, int *metadata_size);

/**
  @brief Increase the cache size.

  This function can be used to increase the number of known peers (that is,
       the degree of the overlay graph) by a specified amount.
  @param tc the pointer to the current topology manager instance context
  @param n number of peers by which the cache size must be incremented.
  @return the new cache size in case of success; -1 in case of error.
*/
int psample_grow_cache(struct psample_context *tc, int n);

/**
  @brief Decrease the cache size.

  This function can be used to reduce the number of known peers (that is,
       the degree of the overlay graph) by a specified amount.
  @param tc the pointer to the current topology manager instance context
  @param n number of peers by which the cache size must be decreased.
  @return the new cache size in case of success; -1 in case of error.
*/
int psample_shrink_cache(struct psample_context *tc, int n);

/**
  @brief Remove a peer from the cache.

  This function can be used to remove a specified peer from the
  cache. Note that the requested cache size is not
  modified, so the peer will be soon replaced by a different one.
  @param tc the pointer to the current topology manager instance context
  @param neighbour the id of the peer to be removed from the
         cache.
  @return 0 in case of success; -1 in case of error.
*/
int psample_remove_peer(struct psample_context *tc, const struct nodeID *neighbour);

/**
  @brief Change the metadata.

  This function can be used to modify/update the metadata for a
  peer. Because of security concerns, only the metadata for the
  local peer can be modified.
  @param tc the pointer to the current topology manager instance context
  @param metadata pointer to the new metadata associated to the peer (will be
         gossiped).
  @param metadata_size size of the metadata associated to the peer (must
         be the same as for the other peers).
  @return 0 in case of success; -1 in case of error.
*/
int psample_change_metadata(struct psample_context *tc, const void *metadata, int metadata_size);

/**
  @brief Initialise the Peer Sampler.

  @param myID the ID of this peer.
  @param metadata pointer to the metadata associated to this peer (will be
         gossiped).
  @param metadata_size size of the metadata associated to this peer.
  @param config configuration parameter for the peer sampling module (specifying the
         peer sampling algorithm, the cache size, etc...)
  @return the topology manager context in case of success; NULL in case of error.
*/
struct psample_context *psample_init(struct nodeID *myID, const void *metadata, int metadata_size, const char *config);

/**
  @brief Insert a known peer in the cache.

  This function can be used to add a specified peer to the
  cache. It is useful in the bootstrap phase.
  @param tc the pointer to the current topology manager instance context
  @param neighbour the id of the peer to be added to the
         cache.
  @param metadata pointer to the metadata associated to the new peer (will be
         gossiped).
  @param metadata_size size of the metadata associated to the new peer (must
         be the same as for the other peers).
  @return 0 in case of success; -1 in case of error.
*/
int psample_add_peer(struct psample_context *tc, struct nodeID *neighbour, const void *metadata, int metadata_size);

/**
  @brief Pass a received packet to the Peer Sampler.

  This function passes to the Peer Sampler a packet that has
  been received from the network. The Peer Sampler will parse
  such packet and run the protocol, adding or removing peers to the
  cache, and sending peer sampling messages to other peers.
  @param tc the pointer to the current topology manager instance context
  @param buff a memory buffer containing the received message.
  @param len the size of such a memory buffer.
  @return 0 in case of success; -1 in case of error.
*/
int psample_parse_data(struct psample_context *tc, const uint8_t *buff, int len);

#endif /* PEERSAMPLER_H */
