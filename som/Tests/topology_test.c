/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see GPL.txt
 */
#include <sys/select.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "msglayer.h"
#include "topmanager.h"
#include "net_helpers.h"

static const char *my_addr = "127.0.0.1";
static int port = 6666;
static int srv_port;
static const char *srv_ip;
static struct timeval tout = {1, 0};

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "p:i:P:I:")) != -1) {
    switch(o) {
      case 'p':
        srv_port = atoi(optarg);
        break;
      case 'i':
        srv_ip = strdup(optarg);
        break;
      case 'P':
        port =  atoi(optarg);
        break;
      case 'I':
        my_addr = iface_addr(optarg);
        break;
      default:
        fprintf(stderr, "Error: unknown option %c\n", o);

        exit(-1);
    }
  }
}

static int wait4fd(void)
{
  struct conn_fd *c;
  fd_set fds;
  int n, i, res;
  int max = -1;
  struct timeval tv;

  c = getConnections(&n);
  FD_ZERO(&fds);
  for (i = 0; i < n; i++) {
    if (c[i].fd > max) {
      max = c[i].fd;
    }
    FD_SET(c[i].fd, &fds);
  }
  tv = tout;
  res = select(max + 1, &fds, NULL, NULL, &tv);
  for (i = 0; i < n; i++) {
    if (FD_ISSET(c[i].fd, &fds)) {
      //fprintf(stderr, "Connection %p %d\n", c[i].conn, c[i].fd);
      return c[i].fd;
    }
  }

  return -1;
}

static struct connectionID *fd2conn(int fd)
{
  struct conn_fd *c;
  int n, i;

  c = getConnections(&n);
  for (i = 0; i < n; i++) {
    if (c[i].fd == fd) {
      return c[i].conn;
    }
  }

  return NULL;
}

static void init(void)
{
  struct socketID *myID;

  myID = create_socket(my_addr, port);
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);
  }
  topInit(myID);
}

static void loop(void)
{
  int done = 0;
#define BUFFSIZE 1024
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;
  
  topParseData(NULL, NULL, 0);
  while (!done) {
    int len;
    int fd;

    fd = wait4fd();
    if (fd > 0) {
      struct connectionID *conn;

      conn = fd2conn(fd);
      len = recv_data(conn, buff, BUFFSIZE);
      topParseData(conn, buff, len);
    } else {
      topParseData(NULL, NULL, 0);
      if (cnt++ % 10 == 0) {
        const struct socketID** neighbourhoods;
        int n, i;

        neighbourhoods = topGetNeighbourhood(&n);
        printf("I have %d neighbourhoods:\n", n);
        for (i = 0; i < n; i++) {
          printf("\t%d: %s\n", i, socket_addr(neighbourhoods[i]));
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  cmdline_parse(argc, argv);

  init();
  if (srv_port != 0) {
    struct socketID *srv;

    srv = create_socket(srv_ip, srv_port);
    topAddNeighbour(srv);
  }

  loop();

  return 0;
}
