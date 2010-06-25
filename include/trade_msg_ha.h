/** @file trade_msg_ha.h
 *
 * @brief Chunk Delivery API - Higher Abstraction
 *
 * The Chunk Delivery HA provides the primitives for effective chunks exchange with other peers. <br>
 * This is a part of the Data Exchange Protocol which provides high level abstraction for sending chunks to a target peers.

 */
#include "chunk.h"
/**
  * @brief Send a Chunk to a target Peer
  *
  * Send a single Chunk to a given Peer
  *
  * @param[in] to destination peer
  * @param[in] c Chunk to send
  * @return 0 on success, <0 on error
  */
int sendChunk(struct nodeID *to, const struct chunk *c);

/**
  * @brief Init the Chunk trading internals.
  *
  * Initialization facility.
  * @param myID address of this peer
  * @return >= 0 on success, <0 on error
  */
int chunkDeliveryInit(struct nodeID *myID);


#if 0
/** 
  * @brief Notification function for a Chunk arrival
  */
typedef int (*ChunkNotification)(struct peer *from, struct chunk  *c);

/**
  * @brief Register a notification for Chunk arrival
  * 
  * Register a notification function that should be called at every Chunk arrival
  *
  * @param[in] som Handle to the enclosing SOM instance
  * @param[in] p notify a send of chunk from Peer p, or from any peer if NULL
  * @param[in] fn pointer to the notification function.
  * @return a handle to the notification or NULL on error @see unregisterSendChunk
  */
void registerSendChunkNotifier(struct peer *p, ChunkNotification *fn);
#endif
