struct chunkiser_iface {
  struct input_stream *(*open)(const char *fname, int *period, const char *config);
  void (*close)(struct input_stream *s);
  uint8_t *(*chunkise)(struct input_stream *s, int id, int *size, uint64_t *ts);
};
