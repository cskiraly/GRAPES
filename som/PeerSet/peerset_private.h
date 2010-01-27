#ifndef CHUNKID_SET_PRIVATE
#define CHUNKID_SET_PRIVATE

struct peerset {
  int size;  //  
  int n_elements; // Number of ids in this array of chunks ids
  struct peer *elements;  // id number
};

#endif /* CHUNKID_SET_PRIVATE */
