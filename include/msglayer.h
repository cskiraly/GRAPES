struct socketID;
struct connectionID;

struct socketID *sockid_dup(const struct socketID *s);
int sockid_equal(const struct socketID *s1, const struct socketID *s2);
struct socketID *create_socket(const char *IPaddr, int port);
struct connectionID *open_connection(struct socketID *myid,struct socketID *otherID, int parameters);
int send_data(const struct connectionID *conn, const uint8_t *buffer_ptr, int buffer_size);
int recv_data(struct connectionID *conn, uint8_t *buffer_ptr, int buffer_size);
int close_connection(const struct connectionID *conn);
void setRecvTimeout(struct connectionID *conn, int timeout);
/* void setMeasurementParameters(meas_params): sets monitoring parameters for the messaging layer */

struct conn_fd {
  struct connectionID *conn;
  int fd;
};

struct conn_fd *getConnections(int *n);
const char *socket_addr(const struct socketID *s);
struct socketID *sockid_undump(const uint8_t *b, int *len);
int sockid_dump(uint8_t *b, const struct socketID *s);
