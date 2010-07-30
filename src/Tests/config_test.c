/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */

#include <stdlib.h>
#include <stdio.h>
#include "../config.h"

int main(int argc, char *argv[])
{
  struct tag *cfg_tags;
  int size=-1, len=-1, dummy=-1, res;
  
  cfg_tags = config_parse("size=10");
  res = config_value_int(cfg_tags, "size", &size);
  printf("%d: Is %d = %d?\n", res, size, 10);
  free(cfg_tags);

  cfg_tags = config_parse("len=5,size=10");
  res = config_value_int(cfg_tags, "size", &size);
  res = config_value_int(cfg_tags, "len", &len);
  printf("%d: Is %d = %d?\n", res, size, 10);
  printf("%d: Is %d = %d?\n", res, len, 5);
  res = config_value_int(cfg_tags, "blah", &dummy);
  printf("%d: Is %d = ...?\n", res, dummy);
  free(cfg_tags);

  return 0;
}
