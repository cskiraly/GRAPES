/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <winsock2.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net_helper.h"

struct nodeID {
  struct sockaddr_in addr;
  int fd;
  uint8_t *buff;
};

static int inet_aton(const char *cp, struct in_addr *addr)
{
    if( cp==NULL || addr==NULL )
    {
        return(0);
    }

    addr->s_addr = inet_addr(cp);
    return (addr->s_addr == INADDR_NONE) ? 0 : 1;
}

int wait4data(const struct nodeID *s, struct timeval *tout, int *user_fds)
{
  fd_set fds;
  int i, res, max_fd;

  FD_ZERO(&fds);
  max_fd = s->fd;
  if (user_fds) {
    for (i = 0; user_fds[i] != -1; i++) {
      FD_SET(user_fds[i], &fds);
      if (user_fds[i] > max_fd) {
        max_fd = user_fds[i];
      }
    }
  }
  FD_SET(s->fd, &fds);
  res = select(max_fd + 1, &fds, NULL, NULL, tout);
  if (res <= 0) {
    return res;
  }
  if (FD_ISSET(s->fd, &fds)) {
    return 1;
  }

  /* If execution arrives here, user_fds cannot be 0
     (an FD is ready, and it's not s->fd) */
  for (i = 0; user_fds[i] != -1; i++) {
    if (!FD_ISSET(user_fds[i], &fds)) {
      user_fds[i] = -2;
    }
  }

  return 2;
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
  s->buff = malloc(60 * 1024 + 1);

  s->fd = -1;

  return s;
}

struct nodeID *net_helper_init(const char *my_addr, int port, const char *config)
{
  int res;
  struct nodeID *myself;

  myself = create_node(my_addr, port);
  if (myself == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);
  }
  myself->fd =  socket(AF_INET, SOCK_DGRAM, 0);
  if (myself->fd < 0) {
    free(myself);
    
    return NULL;
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

void bind_msg_type (uint8_t msgtype)
{
}

int send_to_peer(const struct nodeID *from, struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size)
{
  int res, len;

  do {
    if (buffer_size > 1024 * 60) {
      len = 1024 * 60;
      from->buff[0] = 0;
    } else {
      len = buffer_size;
      from->buff[0] = 1;
    }
    memcpy(from->buff + 1, buffer_ptr, len);
    buffer_size -= len;
    buffer_ptr += len;
    res = sendto(from->fd, from->buff, len + 1, 0, &to->addr, sizeof(struct sockaddr_in));
  } while (buffer_size > 0);

  return res;
}

int recv_from_peer(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size)
{
  int res, recv, len, addrlen;
  struct sockaddr_in raddr;

  *remote = malloc(sizeof(struct nodeID));
  if (*remote == NULL) {
    return -1;
  }

  recv = 0;
  do {
    if (buffer_size > 1024 * 60) {
      len = 1024 * 60;
    } else {
      len = buffer_size;
    }
    addrlen = sizeof(struct sockaddr_in);
    res = recvfrom(local->fd, local->buff, len, 0, &raddr, &addrlen);
    memcpy(buffer_ptr, local->buff + 1, res - 1);
    buffer_size -= (res - 1);
    buffer_ptr += (res - 1);
    recv += (res - 1);
  } while ((local->buff[0] == 0) && (buffer_size > 0));
  memcpy(&(*remote)->addr, &raddr, addrlen);
  (*remote)->fd = -1;

  return recv;
}

int node_addr(const struct nodeID *s, char *addr, int len)
{
  int n;

  n = snprintf(addr, len, "%s:%d", !inet_ntoa(s->addr.sin_addr), ntohs(s->addr.sin_port));

  return n;
}

struct nodeID *nodeid_dup(struct nodeID *s)
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

int nodeid_cmp(const struct nodeID *s1, const struct nodeID *s2)
{
  return memcmp(&s1->addr, &s2->addr, sizeof(struct sockaddr_in));
}

int nodeid_dump(uint8_t *b, const struct nodeID *s, size_t max_write_size)
{
  if (max_write_size < sizeof(struct sockaddr_in)) return -1;

  memcpy(b, &s->addr, sizeof(struct sockaddr_in));

  return sizeof(struct sockaddr_in);
}

struct nodeID *nodeid_undump(const uint8_t *b, int *len)
{
  struct nodeID *res;
  res = malloc(sizeof(struct nodeID));
  if (res != NULL) {
    memcpy(&res->addr, b, sizeof(struct sockaddr_in));
    res->fd = -1;
  }
  *len = sizeof(struct sockaddr_in);

  return res;
}

void nodeid_free(struct nodeID *s)
{
  free(s);
}

int node_ip(const struct nodeID *s, char *ip, int len)
{
  sprintf(ip, "%s", inet_ntoa(s->addr.sin_addr));

  return 1;
}

