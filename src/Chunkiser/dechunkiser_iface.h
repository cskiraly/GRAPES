struct dechunkiser_iface {
  struct output_stream *(*open)(const char *config);
  void (*write)(struct output_stream *o, int id, const uint8_t *data, int size);
};

