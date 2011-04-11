#ifndef PROTO
#define PROTO

struct topo_header {
  uint8_t protocol;
  uint8_t type;
} __attribute__((packed));

#define NCAST_QUERY 0x01
#define NCAST_REPLY 0x02
#define TMAN_QUERY 0x03
#define TMAN_REPLY 0x04
#define CYCLON_QUERY 0x05
#define CYCLON_REPLY 0x06
#define CLOUDCAST_QUERY 0x07
#define CLOUDCAST_REPLY 0x08
#define CLOUDCAST_CLOUD 0x09

#endif  /* PROTO */
