/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see lgpl-2.1.txt
 */

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

int wait4data(const struct nodeID *s, struct timeval tout)
{
  fd_set fds;
  int res;

  FD_ZERO(&fds);
  FD_SET(s->fd, &fds);
  res = select(s->fd + 1, &fds, NULL, NULL, &tout);
  if (FD_ISSET(s->fd, &fds)) {
    return 1;
  }

  return 0;
}

struct nodeID *create_node(const char *IPaddr, int port)
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

  return s;
}

struct nodeID *net_helper_init(const char *my_addr, int port)
{
  int res;
  struct nodeID *myself;

  myself = create_node(my_addr, port);
  if (myself == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);
  }
  fprintf(stderr, "My sock: %d\n", myself->fd);

  res = bind(myself->fd, (struct sockaddr *)&myself->addr, sizeof(struct sockaddr_in));
  if (res < 0) {
    /* bind failed: not a local address... Just close the socket! */
    close(myself->fd);
    free(myself);

    return NULL;
  }

  return myself;
}

int send_to_peer(const struct nodeID *from, const struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size)
{
  return sendto(from->fd, buffer_ptr, buffer_size, 0,
                (const struct sockaddr *)&to->addr, sizeof(struct sockaddr_in));
}

int recv_from_peer(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size)
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
  //fprintf(stderr, "Read %d from %s\n", res, inet_ntoa(raddr.sin_addr));

  return res;
}

const char *node_addr(const struct nodeID *s)
{
  static char addr[256];

  sprintf(addr, "%s:%d", inet_ntoa(s->addr.sin_addr), ntohs(s->addr.sin_port));

  return addr;
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
  memcpy(b, &s->addr, sizeof(struct sockaddr_in));

  return sizeof(struct sockaddr_in);
}

struct nodeID *nodeid_undump(const uint8_t *b, int *len)
{
  struct nodeID *res;
  res = malloc(sizeof(struct nodeID));
  if (res != NULL) {
    memcpy(&res->addr, b, sizeof(struct sockaddr_in));
  }
  *len = sizeof(struct sockaddr_in);

  return res;
}

void nodeid_free(struct nodeID *s)
{
  free(s);
}

