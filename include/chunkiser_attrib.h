/** @file chunkiser_attrib.h
 *
 */
 
#ifndef CHUNKISER_ATTRIB_H
#define CHUNKISER_ATTRIB_H

#include<stdint.h>

struct chunk_attributes_chunker {
  uint8_t magic;
  uint8_t priority;
} __attribute__((packed));

inline void chunk_attributes_chunker_init(struct chunk_attributes_chunker *ca);

inline int chunk_attributes_chunker_verify(void *attr, int attr_size);

#endif	/* CHUNKISER_ATTRIB_H */
