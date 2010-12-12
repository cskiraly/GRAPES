struct dechunkiser_iface {
  struct output_stream *(*open)(const char *fname, const char *config);
  void (*write)(struct output_stream *o, int id, uint8_t *data, int size);
};

