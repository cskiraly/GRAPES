/*
 *  Copyright (c) 2009 Alessandro Russo
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "chunkidset.h"
#include "chunkid_set_h.h"
#define CSPACE 140
#define CSIZE CSPACE/10
unsigned int a_seed;

static void add_chunk(struct chunkID_set *c, int n)
{
  int res;

  res = chunkID_set_add_chunk(c, n);
  printf("Add %d ---> Result=%d\tSize=%d\n", n, res, chunkID_set_size(c));
}

int fillChunkID_set(ChunkIDSet *cset, int rnd)
{
    int i, res;

    if(rnd) {
      fprintf(stdout, "Random cset %d\n",rnd);
      a_seed = (unsigned int)time(NULL);
      srand(a_seed);
    }
    for(i = 1; i < 400; i+=3){
      add_chunk(cset, ((rnd ? rand() : i) % CSPACE) );
    }  
    res = chunkID_set_size(cset);

    return res;
}

int printChunkID_set(ChunkIDSet *c)
{
  int i = -1;

  if (c) {
    int size = chunkID_set_size(c);

    printf("Chunk ID Set size: %d\n", size);
    printf("Chunks (in priority order): ");
    for (i = 0; i < size; i++) {
      printf(" %d", chunkID_set_get_chunk(c, i));
    } 
  } else {
    printf("No chunks found\n");
  }
  
  printf("\n");
  
  return i;
}

void check_chunk(struct chunkID_set *c, int n)
{
  printf("Is %d in the Chunk ID Set?\t", n);
  if (chunkID_set_check(c, n) >= 0) {
    printf("Yes\n");
  } else {
    printf("No\n");
  }
}
