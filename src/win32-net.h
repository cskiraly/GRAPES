#include <winsock2.h>
#include <ws2tcpip.h>
#include <assert.h>

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
static int inet_aton(const char *cp, struct in_addr *addr)
{
    if( cp==NULL || addr==NULL )
    {
        return(0);
    }

    addr->s_addr = inet_addr(cp);
    return (addr->s_addr == INADDR_NONE) ? 0 : 1;
}

static ssize_t recvmsg (int sd, struct msghdr *msg, int flags)
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

static ssize_t sendmsg (int sd, struct msghdr * msg, int flags)
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

static const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
  const struct in_addr *addr = src;
  char *str = inet_ntoa(*addr);

  if (strlen(str) >= size) {
    return NULL;
  }
  strcpy(dst, str);

  return dst;
}

