#ifndef CHUNKID_SET_PRIVATE
#define CHUNKID_SET_PRIVATE

#define CIST_BITMAP 1
#define CIST_PRIORITY 2

struct chunkID_set {
  uint32_t type;
  uint32_t size;
  uint32_t n_elements;
  uint32_t *elements;
};

#endif /* CHUNKID_SET_PRIVATE */
