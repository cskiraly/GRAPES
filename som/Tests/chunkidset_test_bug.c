/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "chunkidset.h"

#include "chunkid_set_h.h"

static void display_cset(struct chunkID_set *cset)
{
  uint32_t ret;

  printf("Chunk ID Set initialised: size is %d\n", chunkID_set_size(cset));
  printChunkID_set(cset);
  printf("Earliest chunk %"PRIu32"\n", (ret = chunkID_set_get_earliest(cset)));
  printf("%s chunk\n",(ret == CHUNKID_INVALID ? "Invalid" : "Valid"));
  check_chunk(cset, 0);
  printf("Latest chunk %"PRIu32"\n", (ret = chunkID_set_get_latest(cset)));
  printf("%s chunk\n",(ret == CHUNKID_INVALID ? "Invalid" : "Valid"));
  check_chunk(cset, 0);
}

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

  display_cset(cset);
  chunkID_set_add_chunk(cset, 7);
  chunkID_set_add_chunk(cset, 2);
  display_cset(cset);

  chunkID_set_free(cset);
}

int main(int argc, char *argv[])
{
  simple_test();

  return 0;
}
