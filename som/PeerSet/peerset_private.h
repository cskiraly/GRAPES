#ifndef CHUNKID_SET_PRIVATE
#define CHUNKID_SET_PRIVATE

struct chunkID_set {
  int size;  //  
  int n_elements; // Number of ids in this array of chunks ids
  int *elements;  // id number
};

#endif /* CHUNKID_SET_PRIVATE */
