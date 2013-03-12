/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/time.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#else
#include <winsock2.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <string.h>

#include "int_coding.h"
#include "payload.h"
#include "config.h"
#include "chunkiser_iface.h"

#define UDP_PORTS_NUM_MAX 10
#define UDP_BUF_SIZE 65536

struct chunkiser_ctx {
  int fds[UDP_PORTS_NUM_MAX + 1];
  int id;
  uint64_t start_time;
  uint8_t *buff;
  int size;
  int counter;
  int every;
};

static int input_get_udp(uint8_t *data, int fd)
{
  ssize_t msglen;

  msglen = recv(fd, data, UDP_BUF_SIZE, 0);
  if (msglen <= 0) {
    return 0;
  }

  return msglen;
}

static int listen_udp(int port)
{
  struct sockaddr_in servaddr;
  int r;
  int fd;

  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0) {
    return -1;
  }

#ifndef _WIN32
  fcntl(fd, F_SETFL, O_NONBLOCK);
#else
  {
    unsigned long nonblocking = 1;
    ioctlsocket(fd, FIONBIO, (unsigned long*) &nonblocking);
  }
#endif

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);
  r = bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  if (r < 0) {
    close(fd);

    return -1;
  }
  fprintf(stderr,"\topened fd:%d for port:%d\n", fd, port);

  return fd;
}

static const int *ports_parse(const char *config)
{
  static int res[UDP_PORTS_NUM_MAX + 1];
  int i = 0;
  struct tag *cfg_tags;

  cfg_tags = config_parse(config);
  if (cfg_tags) {
    int j;

    for (j = 0; j < UDP_PORTS_NUM_MAX; j++) {
      char tag[8];

      sprintf(tag, "port%d", j);
      if (config_value_int(cfg_tags, tag, &res[i])) {
        i++;
      }
    }
  }
  free(cfg_tags);
  res[i] = -1;

  return res;
}

static struct chunkiser_ctx *udp_open(const char *fname, int *period, const char *config)
{
  struct chunkiser_ctx *res;
  struct timeval tv;
  int i;
  const int *ports;

  res = malloc(sizeof(struct chunkiser_ctx));
  if (res == NULL) {
    return NULL;
  }

  ports = ports_parse(config);
  if (ports[0] == -1) {
    free(res);

    return NULL;
  }

  for (i = 0; ports[i] >= 0; i++) {
    res->fds[i] = listen_udp(ports[i]);
    if (res->fds[i] < 0) {
      fprintf(stderr, "Cannot open port %d\n", ports[i]);
      for (; i>=0 ; i--) {
        close(res->fds[i]);
      }
      free(res);

      return NULL;
    }
  }
  res->fds[i] = -1;

  gettimeofday(&tv, NULL);
  res->start_time = tv.tv_usec + tv.tv_sec * 1000000ULL;
  res->id = 1;

  res->buff = NULL;
  res->size = 0;
  res->counter = 0;
  res->every = 1;
  *period = 0;

  return res;
}

static void udp_close(struct chunkiser_ctx  *s)
{
  int i;

  for (i = 0; s->fds[i] >= 0; i++) {
    close(s->fds[i]);
  }
  free(s);
}

static uint8_t *udp_chunkise(struct chunkiser_ctx *s, int id, int *size, uint64_t *ts)
{
  int i;

  if (s->buff == NULL) {
    s->buff = malloc(UDP_BUF_SIZE + UDP_PAYLOAD_HEADER_SIZE);
    if (s->buff == NULL) {
      *size = -1;

      return NULL;
    }
  }
  for (i = 0; s->fds[i] >= 0; i++) {
    if ((*size = input_get_udp(s->buff + s->size + UDP_PAYLOAD_HEADER_SIZE, s->fds[i]))) {
      uint8_t *res = s->buff;
      struct timeval now;

      udp_payload_header_write(s->buff + s->size, *size, i);
      *size += UDP_PAYLOAD_HEADER_SIZE;
      s->size += *size;

      if (++s->counter % s->every) {
        *size = 0;

        return NULL;
      }

      s->size = 0;
      s->counter = 0;
      s->buff = NULL;
      gettimeofday(&now, NULL);
      *ts = now.tv_sec * 1000000ULL + now.tv_usec;

      return res;
    }
  }
  *size = 0;	// FIXME: Unneeded?

  return NULL;
}

const int *udp_get_fds(const struct chunkiser_ctx *s)
{
  return s->fds;
}

struct chunkiser_iface in_udp = {
  .open = udp_open,
  .close = udp_close,
  .chunkise = udp_chunkise,
  .get_fds = udp_get_fds,
};

