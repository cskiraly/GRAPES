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
    struct nodeID *id;
    struct timeval creation_timestamp;
    struct chunkID_set *bmap;
    struct timeval bmap_timestamp;
    int cb_size;
};


#endif	/* _PEER_H */
