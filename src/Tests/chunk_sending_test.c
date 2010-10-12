/*
 *  Copyright (c) 2009 Luca Abeni
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/select.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>

#include "net_helper.h"
#include "chunk.h"
#include "trade_msg_la.h"
#include "trade_msg_ha.h"
#include "net_helpers.h"
#include "grapes_msg_types.h"

static const char *my_addr = "127.0.0.1";
static int port = 6666;
static int dst_port;
static const char *dst_ip;

#define BUFFSIZE 1024

static void chunk_print(FILE *f, const struct chunk *c)
{
  const uint8_t *p;

  fprintf(f, "Chunk %d:\n", c->id);
  fprintf(f, "\tTS: %"PRIu64"\n", c->timestamp);
  fprintf(f, "\tPayload size: %d\n", c->size);
  fprintf(f, "\tAttributes size: %d\n", c->attributes_size);
  p = c->data;
  fprintf(f, "\tPayload:\n");
  fprintf(f, "\t\t%c %c %c %c ...:\n", p[0], p[1], p[2], p[3]);
  if (c->attributes_size > 0) {
    p = c->attributes;
    fprintf(f, "\tAttributes:\n");
    fprintf(f, "\t\t%c %c %c %c ...:\n", p[0], p[1], p[2], p[3]);
  }
}

static struct nodeID *init(void)
{
  struct nodeID *myID;

  myID = net_helper_init(my_addr, port, "");
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);

    return NULL;
  }
  chunkDeliveryInit(myID);

  return myID;
}

static void cmdline_parse(int argc, char *argv[])
{
  int o;

  while ((o = getopt(argc, argv, "p:i:P:I:")) != -1) {
    switch(o) {
      case 'p':
        dst_port = atoi(optarg);
        break;
      case 'i':
        dst_ip = strdup(optarg);
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

int main(int argc, char *argv[])
{
  struct nodeID *my_sock;
  struct chunk c;

  cmdline_parse(argc, argv);

  my_sock = init();
  if (dst_port != 0) {
    struct nodeID *dst;

    c.id = 666;
    c.timestamp = 1000000000ULL;
    c.size = strlen("ciao") + 1;
    c.data = strdup("ciao");
    c.attributes_size = 0;

    dst = create_node(dst_ip, dst_port);
    sendChunk(dst, &c);
    nodeid_free(dst);
    free(c.data);
  } else {
    /* Receive a chunk and print it! */
    int res;
    struct nodeID *remote;
    static uint8_t buff[BUFFSIZE];

    res = recv_from_peer(my_sock, &remote, buff, BUFFSIZE);
    /* TODO: Error check! */
    if (buff[0] != MSG_TYPE_CHUNK) {
      fprintf(stderr, "Wrong message type!!!\n");

      return -1;
    }
    res = decodeChunk(&c, buff + 1, res);
    fprintf(stdout, "Decoding: %d\n", res);
    chunk_print(stdout, &c);
    free(c.data);
    nodeid_free(remote);
  }
  nodeid_free(my_sock);

  return 0;
}

