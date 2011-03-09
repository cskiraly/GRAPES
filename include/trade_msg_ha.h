/** @file trade_msg_ha.h
 *
 * @brief Chunk Delivery API - Higher Abstraction
 *
 * The Chunk Delivery HA provides the primitives for effective chunks exchange with other peers. <br>
 * This is a part of the Data Exchange Protocol which provides high level abstraction for sending chunks to a target peers.
 * See @link chunk_sending_test.c chunk_sending_test.c @endlink for an usage example
 *
 */

/** @example chunk_sending_test.c
 * 
 * A test program showing how to use the chunk delivery API.
 *
 */

#include "chunk.h"

/**
 * @brief Parse an incoming chunk message, providing the chunk structure and transaction ID.
 * @return 1 on success, <0 on error.
 *
 * @param[in] buff containing the incoming message.
 * @param[in] buff_len length of the buffer.
 * @param[out] c the chunk filled with data (an already allocated chunk structure must be passed!).
 * @param[out] transid the transaction ID.
 * @return 1 on success, <0 on error.
 */
int parseChunkMsg(const uint8_t *buff, int buff_len, struct chunk *c, uint16_t *transid);

/**
  * @brief Send a Chunk to a target Peer
  *
  * Send a single Chunk to a given Peer
  *
  * @param[in] to destination peer
  * @param[in] c Chunk to send
  * @param[in] transid the ID of transaction this send belongs to (if any)
  * @return 0 on success, <0 on error
  */
int sendChunk(struct nodeID *to, const struct chunk *c, uint16_t transid);

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
