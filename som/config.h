#ifndef CONFIG_H

struct tag;

struct tag *config_parse(const char *cfg);
int config_value_int(const struct tag *cfg_values, const char *value, int *res);

#endif /* CONFIG_H */
