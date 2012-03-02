/**
 * @file:   peer.h
 * @brief Peer structure definition.
 * @author: Alessandro Russo <russo@disi.unitn.it>
 *
 * @date December 15, 2009, 2:09 PM
 */

#ifndef _PEER_H
#define	_PEER_H

#include <sys/time.h>

struct peer {
    struct nodeID *id; ///< NodeId associated to the peer
    struct timeval creation_timestamp; ///< creation timestamp
    struct chunkID_set *bmap; ///< buffermap of the peer
    struct timeval bmap_timestamp; ///< buffermap timestamp
    int cb_size; ///< chunk buffer size
    double capacity; ///< chunk buffer size
    int subnet;
};


#endif	/* _PEER_H */
