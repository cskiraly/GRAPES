#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net_helper.h"

struct nodeID {
  struct sockaddr_in addr;
  int fd;
};

struct nodeID *create_socket(const char *IPaddr, int port)
{
  struct nodeID *s;
  int res;

  s = malloc(sizeof(struct nodeID));
  memset(s, 0, sizeof(struct nodeID));
  s->addr.sin_family = AF_INET;
  s->addr.sin_port = htons(port);
  res = inet_aton(IPaddr, &s->addr.sin_addr);
  if (res == 0) {
    free(s);

    return NULL;
  }

  s->fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (s->fd < 0) {
    free(s);
    
    return NULL;
  }
fprintf(stderr, "My sock: %d\n", s->fd);
  res = bind(s->fd, (struct sockaddr *)&s->addr, sizeof(struct sockaddr_in));
  if (res < 0) {
    /* bind failed: not a local address... Just close the socket! */
    close(s->fd);
    s->fd = -1;
  }

  return s;
}

int send_data(const struct nodeID *from, const struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size)
{
  return sendto(from->fd, buffer_ptr, buffer_size, 0,
                (struct sockaddr *)&to->addr, sizeof(struct sockaddr_in));
}

int recv_data(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size)
{
  int res;
  struct sockaddr_in raddr;
  socklen_t raddr_size = sizeof(struct sockaddr_in);

  *remote = malloc(sizeof(struct nodeID));
  if (*remote == NULL) {
    return -1;
  }
  res = recvfrom(local->fd, buffer_ptr, buffer_size, 0, (struct sockaddr *)&raddr, &raddr_size);
  memcpy(&(*remote)->addr, &raddr, raddr_size);
  (*remote)->fd = -1;
fprintf(stderr, "Read %d from %s\n", res, inet_ntoa(raddr.sin_addr));
  return res;
}

const char *node_addr(const struct nodeID *s)
{
  static char addr[256];

  sprintf(addr, "%s:%d", inet_ntoa(s->addr.sin_addr), ntohs(s->addr.sin_port));

  return addr;
}

int getFD(const struct nodeID *n)
{
  return n->fd;
}

struct nodeID *nodeid_dup(const struct nodeID *s)
{
  struct nodeID *res;

  res = malloc(sizeof(struct nodeID));
  if (res != NULL) {
    memcpy(res, s, sizeof(struct nodeID));
  }

  return res;
}
int nodeid_equal(const struct nodeID *s1, const struct nodeID *s2)
{
  return (memcmp(&s1->addr, &s2->addr, sizeof(struct sockaddr_in)) == 0);
}

int nodeid_dump(uint8_t *b, const struct nodeID *s)
{
  memcpy(b, s, sizeof(struct nodeID));

  return sizeof(struct nodeID);
}

struct nodeID *nodeid_undump(const uint8_t *b, int *len)
{
  struct nodeID *res;
  res = malloc(sizeof(struct nodeID));
  if (res != NULL) {
    memcpy(res, b, sizeof(struct nodeID));
  }
  *len = sizeof(struct nodeID);

  return res;
}
