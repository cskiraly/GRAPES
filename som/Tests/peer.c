/*
 * peer.c
 *
 *  Created on: Mar 29, 2010
 *      Author: Marco Biazzini
 */

#include <stdio.h>

#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "net_helper.h"
#include "chunkidset.h"
#include "peer.h"

struct peer *peerCreate (struct nodeID *id, int *size)
{
	struct peer *res;

	*size = sizeof(struct peer);
	res = (struct peer *)malloc(*size);
	res->id = nodeid_dup(id);
	gettimeofday(&(res->creation_timestamp),NULL);
	gettimeofday(&(res->bmap_timestamp),NULL);
	res->cb_size = 0;
	res->bmap = chunkID_set_init(0);
	return res;
}

void peerDelete (struct peer *p) {
	nodeid_free(p->id);
	chunkID_set_free(p->bmap);
	free(p);
}

// serialize nodeID and cb_size only
int peerDump(const struct peer *p, uint8_t **buf) {

	uint8_t id[256];
	int id_size = nodeid_dump(id, p->id, 256);
	int res = id_size+sizeof(int);
	*buf = realloc(*buf, res);
	memcpy(*buf,id,id_size);
	memcpy((*buf)+id_size,&(p->cb_size),sizeof(int));

	return res;

}


struct peer* peerUndump(const uint8_t *buf, int *size) {

	struct peer *p;
	p = malloc(sizeof(struct peer));
	p->id = nodeid_undump(buf,size);
	memcpy(&p->cb_size,buf+(*size),sizeof(int));
	*size = *size + sizeof(int);

	gettimeofday(&(p->creation_timestamp),NULL);
	gettimeofday(&(p->bmap_timestamp),NULL);
	p->bmap = chunkID_set_init(0);

	return p;

}

