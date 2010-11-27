/*
 *  Copyright (c) 2009 Andrea Zito
 *
 *  This is free software; see gpl-3.0.txt
 *
 *  This is a small test program for the cloud interface
 *  To try the simple test: run it with
 *    ./cloud_test -c "provider=<cloud_provider>,<provider_opts>" [-g key | -p key=value]
 *
 *    the "-g" and "-p" parameters select the GET or PUT operation
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

#include "cloud_helper.h"

#define GET 0
#define PUT 1

static const char *config;
static int operation;
static char *key;
static char *value;

static void cmdline_parse(int argc, char *argv[])
{
  int o;
  char *temp;
      
  while ((o = getopt(argc, argv, "c:g:p:")) != -1) {
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
  int err;
  struct timeval tout = {10, 0};

  cmdline_parse(argc, argv);

  cloud = init(config);

  if (!cloud) return -1;

  switch(operation) {
  case PUT:
    printf("Putting on cloud value \"%s\" for key \"%s\"\n", value, key);
    err = put_on_cloud(cloud, key, value, strlen(value));
    if (err) {
      printf("Error performing the operation");
      return 1;
    }
    break;
  case GET:
    printf("Getting from cloud value for key \"%s\": ", key);
    err = get_from_cloud(cloud, key);
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
      } else if (err == 0) {
	printf("Key not present on the cloud\n");
      } else {
	buffer[sizeof(buffer) - 1] = '\0';
	printf("%s\n", buffer);
      }
    } else if (err == 0){
      printf("No response from cloud\n");
      return 1;
    } else {
      printf("No value for the specified key\n");
      return 1;      
    }
  }


  return 0;
}
