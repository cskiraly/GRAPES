#ifndef CONFIG_H
#define CONFIG_H
struct tag;

struct tag *config_parse(const char *cfg);
int config_value_int(const struct tag *cfg_values, const char *value, int *res);
const char *config_value_str(const struct tag *cfg_values, const char *value);

#endif /* CONFIG_H */
