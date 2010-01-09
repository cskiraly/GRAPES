#include <stdlib.h>
#include <string.h>
#include "config.h"

struct tag {
  char name[32];
  char value[16];
};

#define MAX_TAGS 16

#warning Fix config_parse
struct tag *config_parse(const char *cfg)
{
  /* Fake Implementation */
  struct tag *res;
  int i = 0;

  res = malloc(sizeof(struct tag) * MAX_TAGS);
  if (res == NULL) {
    return res;
  }
  strcpy(res[i].name, "size");
  strcpy(res[i++].value, "32");
  res[i++].name[0] = 0;

  res = realloc(res, sizeof(struct tag) * i);

  return res;
}

int config_value_int(const struct tag *cfg_values, const char *value, int *res)
{
  int i, done;

  i = 0; done = 0;
  while (!done) {
    if (!strcmp(cfg_values[i].name, value)) {
      *res = atoi(cfg_values[i].value);

      return 1;
    }
    if (cfg_values[i].name[0] == 0) {
      done = 1;
    } else {
      i++;
    }
  }

  return 0;
}
