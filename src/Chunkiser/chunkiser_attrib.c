/** @file chunkiser_attrib.c
 *
 */
 
#include "chunkiser_attrib.h"

inline void chunk_attributes_chunker_init(struct chunk_attributes_chunker *ca)
{
  ca->magic = 0x11;
}

inline int chunk_attributes_chunker_verify(void *attr, int attr_size)
{
  struct chunk_attributes_chunker *ca = attr;

  if (attr_size != sizeof(*ca)) {
    return 0;
  }
  if (ca->magic != 0x11) {
    return 0;
  }

  return 1;
}
