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

  res = malloc(sizeof(struct tag) * MAX_TAGS);
  if (res == NULL) {
    return res;
  }
  strcpy(res[0].name, "size");
  strcpy(res[0].value, "32");
  res[1].name[0] = 0;

  return res;
}

int config_value_int(const struct tag *cfg_values, const char *value, int *res)
{
  int i, done;

  i = 0; done = 0;
  while (done) {
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
