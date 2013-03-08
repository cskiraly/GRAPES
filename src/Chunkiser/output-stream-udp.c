/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "int_coding.h"
#include "payload.h"
#include "config.h"
#include "dechunkiser_iface.h"

#define UDP_PORTS_NUM_MAX 10

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
#endif

struct dechunkiser_ctx {
  int outfd;
  char ip[16];
  int port[UDP_PORTS_NUM_MAX];
  int ports;
};

static int dst_parse(const char *config, int *ports, char *ip)
{
  int i = 0;
  struct tag *cfg_tags;

  sprintf(ip, "127.0.0.1");
  cfg_tags = config_parse(config);
  if (cfg_tags) {
    int j;
    const char *addr;

    addr = config_value_str(cfg_tags, "addr");
    if (addr) {
      sprintf(ip, "%s", addr);
    }
    for (j = 0; j < UDP_PORTS_NUM_MAX; j++) {
      char tag[8];

      sprintf(tag, "port%d", j);
      if (config_value_int(cfg_tags, tag, &ports[i])) {
        i++;
      }
    }
  }
  free(cfg_tags);

  return i;
}

static struct dechunkiser_ctx *udp_open_out(const char *fname, const char *config)
{
  struct dechunkiser_ctx *res;

  if (!config) {
    fprintf(stderr, "udp output not configured, please specify the output ports\n");
    return NULL;
  }

  res = malloc(sizeof(struct dechunkiser_ctx));
  if (res == NULL) {
    return NULL;
  }
  res->outfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (res->outfd < 0) {
    fprintf(stderr, "cannot open the output socket.\n");
    free(res);

    return NULL;
  }

  res->ports = dst_parse(config, res->port, res->ip);
  if (res->ports ==  0) {
    fprintf(stderr, "cannot parse the output ports.\n");
    close(res->outfd);
    free(res);

    return NULL;
  }

  return res;
}

static void packet_write(int fd, const char *ip, int port, uint8_t *data, int size)
{
  struct sockaddr_in si_other;

  memset(&si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(port);
  if (inet_aton(ip, &si_other.sin_addr) == 0) {
     fprintf(stderr, " output socket: inet_aton() failed\n");

     return;
  }

  sendto(fd, data, size, 0, (struct sockaddr *)&si_other, sizeof(si_other));
}

static void udp_write(struct dechunkiser_ctx *o, int id, uint8_t *data, int size)
{
  int i = 0;

  while (i < size) {
    int stream, psize;

    udp_payload_header_parse(data + i, &psize, &stream);
    if (stream > o->ports) {
      fprintf(stderr, "Bad stream %d > %d\n", stream, o->ports);

      return;
    }

    packet_write(o->outfd, o->ip, o->port[stream], data + i + UDP_PAYLOAD_HEADER_SIZE, psize);
    i += UDP_PAYLOAD_HEADER_SIZE + psize;
  }
}

static void udp_close(struct dechunkiser_ctx *s)
{
  close(s->outfd);
  free(s);
}

struct dechunkiser_iface out_udp = {
  .open = udp_open_out,
  .write = udp_write,
  .close = udp_close,
};
