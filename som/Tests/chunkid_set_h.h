/* 
 * File:   chunkid_set_h.h
 * Author: ax
 *
 * Created on July 9, 2010, 9:59 PM
 */

#ifndef _CHUNKID_SET_H_H
#define	_CHUNKID_SET_H_H

int fillChunkID_set(ChunkIDSet *cset, int random);
int printChunkID_set(ChunkIDSet *cset);
void check_chunk(ChunkIDSet *cset, int n);
#endif	/* _CHUNKID_SET_H_H */

