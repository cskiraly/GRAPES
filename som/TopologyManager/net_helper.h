struct nodeID;

struct nodeID *nodeid_dup(const struct nodeID *s);
int nodeid_equal(const struct nodeID *s1, const struct nodeID *s2);

struct nodeID *create_socket(const char *IPaddr, int port);
int send_data(const struct nodeID *from, const struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size);
int recv_data(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size);

int getFD(const struct nodeID *n);

const char *node_addr(const struct nodeID *s);
struct nodeID *nodeid_undump(const uint8_t *b, int *len);
int nodeid_dump(uint8_t *b, const struct nodeID *s);
