struct dechunkiser_ctx;

struct dechunkiser_iface {
  struct dechunkiser_ctx *(*open)(const char *fname, const char *config);
  void (*close)(struct dechunkiser_ctx *s);
  void (*write)(struct dechunkiser_ctx *o, int id, uint8_t *data, int size);
};

