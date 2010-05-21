#ifndef NET_HELPER_H
#define NET_HELPER_H

/**
* @file net_helper.h
*
* @brief Communication facility interface for SOM.
*
* A clean interface is provided, through which all the communication procedures needed by SOM functions
* are handled. This way the different SOM functionalities are not dependent on any particular
* library with respect of the way they may call or be called by other applicative components.
*/

/**
* Implementation dependent internal representation of a node ID.
*/
struct nodeID;

/**
* Duplicate a nodeID.
* @param[in] s A pointer to the nodeID to be duplicated.
* @return A pointer to the duplicate of the argument nodeID.
*/
struct nodeID *nodeid_dup(struct nodeID *s);

/**
* Test if two nodeIDs can be considered identical (where the definition of identity is implementation dependent).
* @param[in] s1 The first nodeID to be compared.
* @param[in] s2 The second nodeID to be compared.
* @return 1 if the two nodeID are identical or 0 if they are not.
*/
int nodeid_equal(const struct nodeID *s1, const struct nodeID *s2);

/**
* Create a new nodeID from a given IP address and port number.
* @param[in] IPaddr The IP address in string form to be associated to the new nodeID.
* @param[in] port The port to be associated to the new nodeID.
* @return A pointer to the new nodeID.
*/
struct nodeID *create_node(const char *IPaddr, int port);

/**
* Delete a nodeID.
* @param[in] s A pointer to the nodeID to be deleted.
*/
void nodeid_free(struct nodeID *s);

/**
* Initialize the parameters for the networking facilities and create a nodeID representing the caller.
* @param[in] IPaddr The IP in string form to be associated to the caller.
* @param[in] port The port to be associated to the caller.
* @return A pointer to a nodeID representing the caller, initialized with all the necessary data.
*/
struct nodeID *net_helper_init(const char *IPaddr, int port);

/**
* To be used if ML support is needed. Register the common net_hepler callback for a msg_type.
* @param[in] mesType The MSG_TYPE of the message the caller is interested in.
*/
void bind_msg_type (uint8_t msgtype);

/**
* Send data to a remote peer.
* @param[in] from A pointer to the nodeID representing the caller.
* @param[in] to A pointer to the nodeID representing the remote peer.
* @param[in] buffer_ptr A pointer to the buffer containing the data to be sent.
* @param[in] buffer_size The length of the data buffer.
* @return The number of bytes sent or -1 if some error occurred.
*/
int send_to_peer(const struct nodeID *from, struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size);

/**
* Receive data from a remote peer.
* @param[in] local A pointer to the nodeID representing the caller.
* @param[out] remote The address to a pointer that has to be set to a new nodeID representing the sender peer.
* @param[out] buffer_ptr A pointer to the buffer containing the received data.
* @param[out] buffer_size The size of the data buffer.
* @return The number of received bytes or -1 if some error occurred.
*/
int recv_from_peer(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size);


/**
* Check if some data arrived for a given nodeID. It sets a timeout to return at most after a given time.
* @param[in] n A pointer to the nodeID representing the caller.
* @param[in] tout A pointer to a timer to be used to set the waiting timeout.
* @param[in] user_fds A pointer to a set of FDs to be monitored.
* @return 1 if some data has arrived, 0 otherwise.
*/
int wait4data(const struct nodeID *n, struct timeval *tout, fd_set *user_fds);

/**
* Give a string representation of a nodeID.
* @param[in] s A pointer to the nodeID to be printed.
* @return A string representing the nodeID.
*/
const char *node_addr(const struct nodeID *s);

/**
* Read from a properly filled byte array (@see #nodeid_dump) and build a new nodeID from its serialized representation in the buffer.
* @param[in] b A pointer to the byte array containing the data to be used.
* @param[in] len The number of bytes to be read from the buffer to build the new nodeID.
* @return A pointer to the new nodeID.
*/
struct nodeID *nodeid_undump(const uint8_t *b, int *len);

/**
* Serialize a nodeID in a byte array.
* @param[in] b A pointer to the byte array that will contain the nodeID serialization.
* @param[in] s A pointer to the nodeID to be serialized.
* @return The number of bytes written in the buffer.
*/
int nodeid_dump(uint8_t *b, const struct nodeID *s);

#endif /* NET_HELPER_H */
