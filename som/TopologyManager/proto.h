struct ncast_header {
  uint8_t protocol;
  uint8_t type;
} __attribute__((packed));

#define PROTO_NCAST 0x10
#define NCAST_QUERY 0x01
#define NCAST_REPLY 0x02
