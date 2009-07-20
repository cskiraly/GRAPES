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

static int wait4data(int fd)
{
  fd_set fds;
  int res;
  struct timeval tv;

  FD_ZERO(&fds);
  FD_SET(fd, &fds);
  tv = tout;
  res = select(fd + 1, &fds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &fds)) {
    return fd;
  }

  return -1;
}

static struct nodeID *init(void)
{
  struct nodeID *myID;

  myID = create_socket(my_addr, port);
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);
  }
  topInit(myID);

  return myID;
}

static void loop(struct nodeID *s)
{
  int done = 0;
#define BUFFSIZE 1024
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;
  
  topParseData(NULL, 0);
  while (!done) {
    int len;
    int fd = getFD(s);

    fd = wait4data(fd);
    if (fd > 0) {
      struct nodeID *remote;

      len = recv_data(s, &remote, buff, BUFFSIZE);
      topParseData(buff, len);
      free(remote);
    } else {
      topParseData(NULL, 0);
      if (cnt++ % 10 == 0) {
        const struct nodeID **neighbourhoods;
        int n, i;

        neighbourhoods = topGetNeighbourhood(&n);
        printf("I have %d neighbourhoods:\n", n);
        for (i = 0; i < n; i++) {
          printf("\t%d: %s\n", i, node_addr(neighbourhoods[i]));
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  struct nodeID *my_sock;

  cmdline_parse(argc, argv);

  my_sock = init();
  if (srv_port != 0) {
    struct nodeID *srv;

    srv = create_socket(srv_ip, srv_port);
    topAddNeighbour(srv);
  }

  loop(my_sock);

  return 0;
}
