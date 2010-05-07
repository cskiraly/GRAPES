#ifndef CHUNKID_SET_PRIVATE
#define CHUNKID_SET_PRIVATE

struct chunkID_set {
  uint32_t type;
  uint32_t size;
  uint32_t n_elements;
  uint32_t *elements;
};

#endif /* CHUNKID_SET_PRIVATE */
