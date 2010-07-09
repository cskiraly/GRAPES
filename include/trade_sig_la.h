/** @file trade_sig_la.h
 *
 * @brief Chunk Signaling  API - Lower Abstraction
 *
 * The Chunk Signaling LA provides a set of primitives which encode and decode the signaling messages for chunks' negotiation. The former encode the signaling message returning a bit stream which is essentially the packet that will be sent, while the latter decode the received packet returning the signaling message understandable by the protocol.

 */

#ifndef TRADE_SIG_LA_H 
#define TRADE_SIG_LA_H

 /**
  * @brief Decode the bit streamEncode a sequence of information, filling the buffer with the corresponding bit stream.
  * 
  * Encode a sequence of information given as parameters and fills a buffer (given as parameter) with the corresponding bit stream.
  * The main reason to encode and return the bit stream is the possibility to either send directly a packet with the encoded bit stream, or 
  * add this bit stream in piggybacking
  * 
  * @param[in] h set of ChunkIDs
  * @param[in] meta metadata associated to the ChunkID set
  * @param[in] meta_len length of the metadata
  * @param[in] buff Buffer that will be filled with the bit stream obtained as a coding of the above parameters
  * @param[in] buff_len length of the buffer that will contain the bit stream
  * @return 0 on success, <0 on error
  */
int encodeChunkSignaling(const struct chunkID_set *h, const void *meta, int meta_len, uint8_t *buff, int buff_len);

/**
  * @brief Decode the bit stream.
  *
  * Decode the bit stream contained int the buffer, filling the other parameters. This is the dual of the encode function.
  *  
  * @param[in] meta pointer to the metadata associated to the ChunkID set
  * @param[in] meta_len length of the metadata
  * @param[in] buff Buffer which contain the bit stream to decode, filling the above parameters
  * @param[in] buff_len length of the buffer that contain the bit stream
  * @return a pointer to the chunk ID set or NULL with meta-data when on success, NULL with empty values on error.
  */
struct chunkID_set *decodeChunkSignaling(void **meta, int *meta_len, const uint8_t *buff, int buff_len);


#endif /* TRADE_SIG_LA_H */
