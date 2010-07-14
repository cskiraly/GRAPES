/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chunkidset.h"
#include "trade_sig_la.h"
#include "chunkid_set_h.h"

static void simple_test(void)
{
  struct chunkID_set *cset;
  char config[32];

  sprintf(config, "size=%d", 10);
  cset = chunkID_set_init(config);
  if(!cset) {
    fprintf(stderr, "Unable to allocate memory for rcset\n");

    return;
  }

  printf("Chunk ID Set initialised: size is %d\n", chunkID_set_size(cset));
  printChunkID_set(cset);
  printf("Earliest chunk %u\n", chunkID_set_get_earliest(cset));
  check_chunk(cset, 0);
  printf("Latest chunk %u.\n", chunkID_set_get_latest(cset));
  check_chunk(cset, 0);
  chunkID_set_free(cset);
}

int main(int argc, char *argv[])
{
  simple_test();

  return 0;
}
