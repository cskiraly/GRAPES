/*
 *  Copyright (c) 2009 Andrea Zito
 *
 *  This is free software; see gpl-3.0.txt
 *
 *  This is a small test program for the cloud interface
 *  To try the simple test: run it with
 *    ./cloud_test -c "provider=<cloud_provider>,<provider_opts>" [-g key | -d key=value | -p key=value | -n variant | -e ip:port]
 *
 *    -g  GET key from cloud
 *    -d  GET key from cloud with default
 *    -p  PUT key=value on cloud
 *    -n  print the cloud node for the specified variant
 *    -e  check if ip:port references the cloud
 *    -c  set the configuration of the cloud provider
 *
 *    For example, run
 *      ./cloud_test -c "provider=delegate,delegate_lib=libfilecloud.so" -p hello=world
 *      ./cloud_test -c "provider=delegate,delegate_lib=libfilecloud.so" -g hello
 *
 *    to test the delegate cloud provider with delegate implementation provided by libfilecloud.so.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#include "cloud_helper.h"

#define GET 0
#define PUT 1
#define GET_CLOUD_NODE 2
#define EQ_CLOUD_NODE 3
#define GETDEF 4

static const char *config;
static int operation;
static int variant;
static char *key;
static char *value;

static const uint8_t *HEADER = (const uint8_t *) "<-header->";

static void cmdline_parse(int argc, char *argv[])
{
  int o;
  char *temp;

  while ((o = getopt(argc, argv, "c:g:d:p:n:e:")) != -1) {
    switch(o) {
    case 'c':
      config = strdup(optarg);
      break;
    case 'p':
      temp = strdup(optarg);
      operation = PUT;

      key = strsep(&optarg, "=");
      value = optarg;

      if (!value || !key){
        printf("Expected key=value for option -p");
        free(temp);
        exit(-1);
      }
      break;
    case 'g':
        key =  strdup(optarg);
        break;
    case 'd':
      temp = strdup(optarg);
      operation = GETDEF;

      key = strsep(&optarg, "=");
      value = optarg;

      if (!value || !key){
        printf("Expected key=value for option -d");
        free(temp);
        exit(-1);
      }
      break;
    case 'n':
      operation = GET_CLOUD_NODE;
      variant = atoi(optarg);
      break;
    case 'e':
      operation = EQ_CLOUD_NODE;
      temp = strdup(optarg);

      key = strsep(&optarg, ":");
      value = optarg;


      if (!value || !key){
        printf("Expected ip:port for option -e");
        free(temp);
        exit(-1);
      }
      break;
    default:
        fprintf(stderr, "Error: unknown option %c\n", o);

        exit(-1);
    }
  }
}

static struct cloud_helper_context *init(const char *config)
{
  struct nodeID *myID;
  struct cloud_helper_context *cloud_ctx;

  myID = net_helper_init("127.0.0.1", 1234, "");
  if (myID == NULL) {
    fprintf(stderr, "Error creating my socket\n");

    return NULL;
  }
  cloud_ctx = cloud_helper_init(myID, config);
  if (cloud_ctx == NULL) {
    fprintf(stderr, "Error initiating cloud_helper\n");
    return NULL;
  }

  return cloud_ctx;
}



int main(int argc, char *argv[])
{
  struct cloud_helper_context *cloud;
  char buffer[100];
  char addr[256];
  int err;
  struct nodeID *t;
  struct timeval tout = {10, 0};

  cmdline_parse(argc, argv);

  cloud = init(config);

  if (!cloud) return -1;

  switch(operation) {
  case PUT:
    printf("Putting on cloud value \"%s\" for key \"%s\"\n", value, key);
    err = put_on_cloud(cloud, key, value, strlen(value), 0);
    if (err) {
      printf("Error performing the operation");
      return 1;
    }
    break;
  case GETDEF:
  case GET:
    printf("Getting from cloud value for key \"%s\": ", key);
    memcpy(buffer, HEADER, strlen(HEADER));

    if (operation == GETDEF) {
      printf("(Using default value: \"%s\")", value);
      err = get_from_cloud_default(cloud, key, buffer, strlen(HEADER), 0,
                                   value, strlen(value), 0);
    } else {
      err = get_from_cloud(cloud, key, buffer, strlen(HEADER), 0);
    }

    if (err) {
      printf("Error performing the operation");
      return 1;
    }

    err = wait4cloud(cloud, &tout);
    if (err > 0) {
      err = recv_from_cloud(cloud, buffer, sizeof(buffer)-1);
      if (err < 0) {
        printf("Erorr receiving cloud response\n");
        return 1;
      } else {
        time_t timestamp;
        int i;
        buffer[err] = '\0';
        printf("len=%d\n", err);
        for (i=0; i<err; i++)
          printf("%x(%c) ", buffer[i], buffer[i]);
        printf("\n");
        timestamp = timestamp_cloud(cloud);
        printf("Timestamp: %s\n", ctime(&timestamp));
      }
    } else if (err == 0){
      printf("No response from cloud\n");
      return 1;
    } else {
      memset(buffer, 0, sizeof(buffer));
      err = recv_from_cloud(cloud, buffer, sizeof(buffer)-1);
      buffer[sizeof(buffer) - 1] = '\0';
      printf("No value for the specified key. Received: %s\n", buffer);
      return 1;
    }
    break;
  case GET_CLOUD_NODE:
    node_addr(get_cloud_node(cloud, variant), addr, 256);
    printf("Cloud node: %s\n", addr);
    break;
  case EQ_CLOUD_NODE:
    t = create_node(key, atoi(value));
    node_addr(get_cloud_node(cloud, variant), addr, 256);
    printf("Node %s references cloud? %d\n", addr,
           is_cloud_node(cloud, t));
    break;
  }


  return 0;
}
