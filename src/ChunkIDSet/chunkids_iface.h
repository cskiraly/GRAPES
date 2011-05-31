#ifndef CHUNKIDS_IFACE
#define CHUNKIDS_IFACE

struct chunkID_set;

struct cids_ops_iface {
  int (*add_chunk)(struct chunkID_set *h, int chunk_id);
  int (*check)(const struct chunkID_set *h, int chunk_id);
};
struct cids_encoding_iface {
  uint8_t *(*encode)(const struct chunkID_set *h, uint8_t *buff, int buff_len, int meta_len);
  const uint8_t *(*decode)(struct chunkID_set *h, const uint8_t *buff, int buff_len, int *meta_len);
};

#endif	/* CHUNKIDS_IFACE */
