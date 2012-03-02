struct chunkiser_ctx;

struct chunkiser_iface {
  struct chunkiser_ctx *(*open)(const char *fname, int *period, const char *config);
  void (*close)(struct chunkiser_ctx *s);
  uint8_t *(*chunkise)(struct chunkiser_ctx *s, int id, int *size, uint64_t *ts, void **attr, int *attr_size);
  const int *(*get_fds)(const struct chunkiser_ctx *s);
};
