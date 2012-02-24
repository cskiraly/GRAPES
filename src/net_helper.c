/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <sys/types.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <assert.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net_helper.h"

#define MAX_MSG_SIZE 1024 * 60

struct nodeID {
    /* TODO: use internally sock_data_t */
  struct sockaddr_in addr;
  int fd;
};

#ifdef _WIN32
static int inet_aton(const char *cp, struct in_addr *addr)
{
    if( cp==NULL || addr==NULL )
    {
        return(0);
    }

    addr->s_addr = inet_addr(cp);
    return (addr->s_addr == INADDR_NONE) ? 0 : 1;
}

struct iovec {                    /* Scatter/gather array items */
  void  *iov_base;              /* Starting address */
  size_t iov_len;               /* Number of bytes to transfer */
};

struct msghdr {
  void         *msg_name;       /* optional address */
  socklen_t     msg_namelen;    /* size of address */
  struct iovec *msg_iov;        /* scatter/gather array */
  size_t        msg_iovlen;     /* # elements in msg_iov */
  void         *msg_control;    /* ancillary data, see below */
  socklen_t     msg_controllen; /* ancillary data buffer len */
  int           msg_flags;      /* flags on received message */
};

#define MIN(A,B)    ((A)<(B) ? (A) : (B))
ssize_t recvmsg (int sd, struct msghdr *msg, int flags)
{
  ssize_t bytes_read;
  size_t expected_recv_size;
  ssize_t left2move;
  char *tmp_buf;
  char *tmp;
  int i;

  assert (msg->msg_iov);

  expected_recv_size = 0;
  for (i = 0; i < msg->msg_iovlen; i++)
    expected_recv_size += msg->msg_iov[i].iov_len;
  tmp_buf = malloc (expected_recv_size);
  if (!tmp_buf)
    return -1;

  left2move = bytes_read = recvfrom (sd,
                                     tmp_buf,
                                     expected_recv_size,
                                     flags,
                                     (struct sockaddr *) msg->msg_name,
                                     &msg->msg_namelen);

  for (tmp = tmp_buf, i = 0; i < msg->msg_iovlen; i++)
    {
      if (left2move <= 0)
        break;
      assert (msg->msg_iov[i].iov_base);
      memcpy (msg->msg_iov[i].iov_base,
              tmp, MIN (msg->msg_iov[i].iov_len, left2move));
      left2move -= msg->msg_iov[i].iov_len;
      tmp += msg->msg_iov[i].iov_len;
    }

  free (tmp_buf);

  return bytes_read;
}

ssize_t sendmsg (int sd, struct msghdr * msg, int flags)
{
  ssize_t bytes_send;
  size_t expected_send_size;
  size_t left2move;
  char *tmp_buf;
  char *tmp;
  int i;

  assert (msg->msg_iov);

  expected_send_size = 0;
  for (i = 0; i < msg->msg_iovlen; i++)
    expected_send_size += msg->msg_iov[i].iov_len;
  tmp_buf = malloc (expected_send_size);
  if (!tmp_buf)
    return -1;

  for (tmp = tmp_buf, left2move = expected_send_size, i = 0; i <
       msg->msg_iovlen; i++)
    {
      if (left2move <= 0)
        break;
      assert (msg->msg_iov[i].iov_base);
      memcpy (tmp,
              msg->msg_iov[i].iov_base,
              MIN (msg->msg_iov[i].iov_len, left2move));
      left2move -= msg->msg_iov[i].iov_len;
      tmp += msg->msg_iov[i].iov_len;
    }

  bytes_send = sendto (sd,
                       tmp_buf,
                       expected_send_size,
                       flags,
                       (struct sockaddr *) msg->msg_name, msg->msg_namelen);

  free (tmp_buf);

  return bytes_send;
}

#endif

int wait4data(const struct nodeID *s, struct timeval *tout, int *user_fds)
{
  fd_set fds;
  int i, res, max_fd;

  FD_ZERO(&fds);
  if (s) {
    max_fd = s->fd;
    FD_SET(s->fd, &fds);
  } else {
    max_fd = -1;
  }
  if (user_fds) {
    for (i = 0; user_fds[i] != -1; i++) {
      FD_SET(user_fds[i], &fds);
      if (user_fds[i] > max_fd) {
        max_fd = user_fds[i];
      }
    }
  }
  res = select(max_fd + 1, &fds, NULL, NULL, tout);
  if (res <= 0) {
    return res;
  }
  if (s && FD_ISSET(s->fd, &fds)) {
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

  /* TODO: this can be replaced with sock_data_init
   * res = sock_data_init(IPaddr, port)
   */
  s->addr.sin_family = AF_INET;
  s->addr.sin_port = htons(port);
  res = inet_aton(IPaddr, &s->addr.sin_addr);

  if (res == 0) {
    free(s);

    return NULL;
  }

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

    return NULL;
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

struct my_hdr_t {
  uint8_t m_seq;
  uint8_t frag_seq;
  uint8_t frags;
} __attribute__((packed));

int send_to_peer(const struct nodeID *from, struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size)
{
  static struct msghdr msg;
  static struct my_hdr_t my_hdr;
  struct iovec iov[2];
  int res;

  iov[0].iov_base = &my_hdr;
  iov[0].iov_len = sizeof(struct my_hdr_t);
  msg.msg_name = &to->addr;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_iovlen = 2;
  msg.msg_iov = iov;

  my_hdr.m_seq++;
  my_hdr.frags = (buffer_size / (MAX_MSG_SIZE)) + 1;
  my_hdr.frag_seq = 0;

  do {
    iov[1].iov_base = buffer_ptr;
    if (buffer_size > MAX_MSG_SIZE) {
      iov[1].iov_len = MAX_MSG_SIZE;
    } else {
      iov[1].iov_len = buffer_size;
    }
    my_hdr.frag_seq++;

    buffer_size -= iov[1].iov_len;
    buffer_ptr += iov[1].iov_len;
    res = sendmsg(from->fd, &msg, 0);

    if (res  < 0){
      int error = errno;
      fprintf(stderr,"net-helper: sendmsg failed errno %d: %s\n", error, strerror(error));
    }
  } while (buffer_size > 0);

  return res;
}

int recv_from_peer(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size)
{
  int res, recv, m_seq, frag_seq;
  struct sockaddr_in raddr;
  static struct msghdr msg;
  static struct my_hdr_t my_hdr;
  struct iovec iov[2];

  iov[0].iov_base = &my_hdr;
  iov[0].iov_len = sizeof(struct my_hdr_t);
  msg.msg_name = &raddr;
  msg.msg_namelen = sizeof(struct sockaddr_in);
  msg.msg_iovlen = 2;
  msg.msg_iov = iov;

  *remote = malloc(sizeof(struct nodeID));
  if (*remote == NULL) {
    return -1;
  }

  recv = 0;
  m_seq = -1;
  frag_seq = 0;
  do {
    iov[1].iov_base = buffer_ptr;
    if (buffer_size > MAX_MSG_SIZE) {
      iov[1].iov_len = MAX_MSG_SIZE;
    } else {
      iov[1].iov_len = buffer_size;
    }
    buffer_size -= iov[1].iov_len;
    buffer_ptr += iov[1].iov_len;
    res = recvmsg(local->fd, &msg, 0);
    recv += (res - sizeof(struct my_hdr_t));
    if (m_seq != -1 && my_hdr.m_seq != m_seq) {
      return -1;
    } else {
      m_seq = my_hdr.m_seq;
    }
    if (my_hdr.frag_seq != frag_seq + 1) {
      return -1;
    } else {
     frag_seq++;
    }
  } while ((my_hdr.frag_seq < my_hdr.frags) && (buffer_size > 0));
  memcpy(&(*remote)->addr, &raddr, msg.msg_namelen);
  (*remote)->fd = -1;

  return recv;
}

const char *node_addr(const struct nodeID *s)
{
  static char addr[256];

  sprintf(addr, "%s:%d", inet_ntoa(s->addr.sin_addr), ntohs(s->addr.sin_port));

  return addr;
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

/* TODO: use sock_data_equal */
int nodeid_equal(const struct nodeID *s1, const struct nodeID *s2)
{
  return (memcmp(&s1->addr, &s2->addr, sizeof(struct sockaddr_in)) == 0);
}

/* TODO: use sock_data_cmp */
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

/* TODO: use sock_data_str_ip
 *
 * Also I think that this kind of functions (there are a certain number
 * around in the implementation, this is just an instance of the problem)
 * are seriously killing one of the aims of this library
 *
 * (One of the design goals of the GRAPES library is not to force any
 * particular structure in the applications using it, and it should be
 * possible to use its APIs in either multi-threaded programs,
 * multi-process applications, single-threaded (event based)
 * architectures, etc...  Moreover, there should not be any dependency on
 * external libraries, and the code should be fairly portable.)
 */
const char *node_ip(const struct nodeID *s) {
  static char ip[64];

  sprintf(ip, "%s", inet_ntoa(s->addr.sin_addr));

  return ip;
}
