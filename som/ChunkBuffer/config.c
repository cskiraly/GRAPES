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
  struct tag *res;
  int i = 0;
  const char *p = cfg;

  res = malloc(sizeof(struct tag) * MAX_TAGS);
  if (res == NULL) {
    return res;
  }
  while (p && *p != 0) {
    char *p1 = strchr(p, '=');
    if (p1) {
      if (i % MAX_TAGS == 0) {
        res = realloc(res, sizeof(struct tag) * (i + MAX_TAGS));
      }
      memcpy(res[i].name, p, p1 - p);
      res[i].name[p1 - p + 1] = 0;
      p = strchr(p1, ',');
      if (p == NULL) {
        strcpy(res[i++].value, p1 + 1);
      } else {
        memcpy(res[i].value, p1 + 1, p - p1 - 1);
        res[i++].value[p - p1] = 0;
        p++;
      }
    } else {
      p = NULL;
    }
  }
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
