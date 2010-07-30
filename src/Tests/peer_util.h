/*
 * peer.c headers
 *
 *  Created on: June 24, 2010
 *      Author: Marco Biazzini
 */


#include "peer.h"

struct peer *peerCreate (struct nodeID *id, int *size);


void peerDelete (struct peer *p) ;

// serialize nodeID and cb_size only
int peerDump(const struct peer *p, uint8_t **buf);


struct peer* peerUndump(const uint8_t *buf, int *size);
