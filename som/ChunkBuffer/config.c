int config_value_int(struct cfg_values *tag, const char *value)
{
  i = 0; done = 0;
  while (done) {
    if (!strcmp(tag[i].name, "size")) {
      return atoi(tag[i].value);
    }
    if (tag[i].name[0] == 0) {
      done = 1;
    }
  }
}
