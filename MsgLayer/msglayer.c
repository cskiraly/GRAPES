#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "msglayer.h"

struct socketID {
  struct sockaddr_in addr;
};
struct connectionID {
  struct socketID localid;
  struct socketID remoteid;
  int s;
};

static struct connectionID my_end;

struct socketID *create_socket(const char *IPaddr, int port)
{
  struct socketID *s;
  int res;

  s = malloc(sizeof(struct socketID));
  memset(s, 0, sizeof(struct socketID));
  s->addr.sin_family = AF_INET;
  s->addr.sin_port = htons(port);
  res = inet_aton(IPaddr, &s->addr.sin_addr);
  if (res == 0) {
    free(s);

    s = NULL;
  }

  if (my_end.s <= 0) {
    my_end.s = socket(AF_INET, SOCK_DGRAM, 0);
    if (my_end.s < 0) {
      free(s);
    
      return NULL;
    }
fprintf(stderr, "My sock: %d\n", my_end.s);
    res = bind(my_end.s, (struct sockaddr *)&s->addr, sizeof(struct sockaddr_in));
    if (res < 0) {
      perror("Bind");
      close(my_end.s);
      free(s);
    
      return NULL;
    }
  }

  return s;
}

struct connectionID *open_connection(struct socketID *myid,struct socketID *otherid, int parameters){
  struct connectionID *c;

  if (my_end.localid.addr.sin_port != 0) {
    if (memcmp(&my_end.localid, myid, sizeof(struct socketID)) != 0) {
      fprintf(stderr, "Error: Only one local address is currently supported!\n");
      return NULL;
    }
  } else {
    my_end.localid = *myid;
  }

  c = malloc(sizeof(struct connectionID));
  memset(c, 0, sizeof(struct connectionID));
  c->localid = *myid;
  c->remoteid = *otherid;
  c->s = my_end.s;

  return c;
}

int send_data(const struct connectionID *conn, const uint8_t *buffer_ptr, int buffer_size){
  return sendto(conn->s, buffer_ptr, buffer_size, 0,
                (struct sockaddr *)&conn->remoteid.addr, sizeof(struct sockaddr_in));
}
int recv_data(struct connectionID *conn, uint8_t *buffer_ptr, int buffer_size)
{
  int res;
  struct sockaddr_in raddr;
  socklen_t raddr_size = sizeof(struct sockaddr_in);

  res = recvfrom(conn->s, buffer_ptr, buffer_size, 0, (struct sockaddr *)&raddr, &raddr_size);
  memcpy(&conn->remoteid.addr, &raddr, raddr_size);
fprintf(stderr, "Read %d from %s\n", res, inet_ntoa(raddr.sin_addr));
  return res;
}

int close_connection(const struct connectionID *conn)
{
  //close(conn->s);		FIXME!
  free((void *)conn);

  return 0;
}

const char *socket_addr(const struct socketID *s)
{
  static char addr[256];

  sprintf(addr, "%s:%d", inet_ntoa(s->addr.sin_addr), ntohs(s->addr.sin_port));

  return addr;
}

struct conn_fd *getConnections(int *n)
{
  static struct conn_fd fds;

  fds.conn = &my_end;
  fds.fd = my_end.s;
  *n = 1;

  return &fds;
}

struct socketID *sockid_dup(const struct socketID *s)
{
  struct socketID *res;

  res = malloc(sizeof(struct socketID));
  if (res != NULL) {
    memcpy(res, s, sizeof(struct socketID));
  }

  return res;
}
int sockid_equal(const struct socketID *s1, const struct socketID *s2)
{
  return (memcmp(s1, s2, sizeof(struct socketID)) == 0);
}
/*
void setRecvTimeout(struct connectionID *conn, int timeout);
void setMeasurementParameters(meas_params): sets monitoring parameters for the messaging layer
*/

int sockid_dump(uint8_t *b, const struct socketID *s)
{
  memcpy(b, s, sizeof(struct socketID));

  return sizeof(struct socketID);
}

struct socketID *sockid_undump(const uint8_t *b, int *len)
{
  struct socketID *res;
  res = malloc(sizeof(struct socketID));
  if (res != NULL) {
    memcpy(res, b, sizeof(struct socketID));
  }
  *len = sizeof(struct socketID);

  return res;
}

