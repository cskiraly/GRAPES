/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "chunkidset.h"
#include "trade_sig_la.h"

static void check_chunk(struct chunkID_set *c, int n)
{
  printf("Is %d in the Chunk ID Set?\t", n);
  if (chunkID_set_check(c, n) >= 0) {
    printf("Yes\n");
  } else {
    printf("No\n");
  }
}

static void add_chunk(struct chunkID_set *c, int n)
{
  int res;

  res = chunkID_set_add_chunk(c, n);
  printf("Add %d ---> Result=%d\tSize=%d\n", n, res, chunkID_set_size(c));
}

static void print_set(struct chunkID_set *c)
{
  int i, size = chunkID_set_size(c);

  printf("Chunk ID Set size: %d\n", size);
  printf("Chunks (in priority order): ");
  for (i = 0; i < size; i++) {
    printf(" %d", chunkID_set_get_chunk(c, i));
  }

  printf("\n");
}

static void populate(struct chunkID_set *cset)
{
  int i,c;
  for(i = 1; i < 400; i+=3){
    add_chunk(cset, (i%140+ i*5) );
  }  
}

static void simple_test(void)
{
  struct chunkID_set *cset;
  char config[32];
  sprintf(config,"size=%d",10);
  cset = chunkID_set_init(config);
  printf("Chunk ID Set initialised: size is %d\n", chunkID_set_size(cset));
  populate(cset);
  print_set(cset);
  check_chunk(cset, 4);
  check_chunk(cset, 2);
  check_chunk(cset, 3);
  check_chunk(cset, 9);
  chunkID_set_clear(cset, 0);
  free(cset);
}

static void encoding_test(int mode)
{
  struct chunkID_set *cset, *cset1;
  static uint8_t buff[2048];
  int res, meta_len;
  void *meta;

  char config[32];
  sprintf(config,"type=%d", mode);
  cset = chunkID_set_init(config);
  populate(cset);
  res = encodeChunkSignaling(cset, NULL, 0, buff, sizeof(buff));
  printf("Encoding Result: %d\n", res);
  chunkID_set_clear(cset, 0);
  free(cset);
  
  cset1 = decodeChunkSignaling(&meta, &meta_len, buff, res);
  print_set(cset1);
  chunkID_set_clear(cset1, 0);
  free(cset1);
}

int main(int argc, char *argv[])
{
  simple_test();
  encoding_test(CIST_PRIORITY);
  encoding_test(CIST_BITMAP);

  return 0;
}
