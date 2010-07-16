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

  sprintf(config,"size=%d",10);
  cset = chunkID_set_init(config);
  if(!cset){
    fprintf(stderr,"Unable to allocate memory for rcset\n");

    return;
  }

  printf("Chunk ID Set initialised: size is %d\n", chunkID_set_size(cset));
  fillChunkID_set(cset, 0);
  printChunkID_set(cset);
  check_chunk(cset, 4);
  check_chunk(cset, 2);
  check_chunk(cset, 3);
  check_chunk(cset, 9);
  printf("Earliest chunk %d\nLatest chunk %d.\n",
          chunkID_set_get_earliest(cset), chunkID_set_get_latest(cset));
  chunkID_set_clear(cset, 0);
  free(cset);
}

static void encoding_test(const char *mode)
{
  struct chunkID_set *cset, *cset1;
  static uint8_t buff[2048];
  int res, meta_len;
  void *meta;
  char config[32];

  sprintf(config, "type=%s", mode);
  cset = chunkID_set_init(config);
  if(!cset){
    fprintf(stderr,"Unable to allocate memory for rcset\n");

    return;
  }

  fillChunkID_set(cset, 0);
  res = encodeChunkSignaling(cset, NULL, 0, buff, sizeof(buff));
  printf("Encoding Result: %d\n", res);
  chunkID_set_clear(cset, 0);
  free(cset);
  
  cset1 = decodeChunkSignaling(&meta, &meta_len, buff, res);
  printChunkID_set(cset1);
  chunkID_set_clear(cset1, 0);
  free(cset1);
}

static void metadata_test(void)
{
  struct chunkID_set *cset;
  static uint8_t buff[2048];
  int res, meta_len;
  void *meta;

  meta = strdup("This is a test");
  res = encodeChunkSignaling(NULL, meta, strlen(meta) + 1, buff, sizeof(buff));
  printf("Encoding Result: %d\n", res);
  free(meta);
  
  cset = decodeChunkSignaling(&meta, &meta_len, buff, res);
  printf("Decoded MetaData: %s (%d)\n", (char *)meta, meta_len);

  if (cset) {
    chunkID_set_clear(cset, 0);
    free(cset);
  }

  free(meta);
}

int main(int argc, char *argv[])
{
  simple_test();
  encoding_test("priority");
  encoding_test("bitmap");
  metadata_test();

  return 0;
}
