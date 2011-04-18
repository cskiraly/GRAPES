#ifndef PEERSET_PRIVATE
#define PEERSET_PRIVATE

struct peerset {
  int size;  //  
  int n_elements; // Number of ids in this array of chunks ids
  struct peer **elements;  // id number
};

#endif /* PEERSET_PRIVATE */
