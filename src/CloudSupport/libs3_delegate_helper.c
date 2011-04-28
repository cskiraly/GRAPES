/*
 *  Copyright (c) 2011 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 *
 *
 *  Delegate cloud_handler for AmazonS3 based on the libs3 library.
 *  Supported parameters (*required):
 *
 *  - s3_access_key*:  the Amazon access key id
 *  - s3_secret_key*:  the Amazon secret access key
 *  - s3_bucket_name*: the bucket on which operate
 *  - s3_protocol:     http (default) or https
 *  - s3_blocking_put: a value of 1 enable blocking operation.
 *                     (default: disabled)
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#include <libs3.h>

#include "net_helper.h"
#include "cloud_helper_iface.h"
#include "request_handler.h"
#include "config.h"

#define CLOUD_NODE_ADDR "0.0.0.0"

/***********************************************************************
 * Interface prototype for cloud_helper_delegate
 ***********************************************************************/
struct delegate_iface {
  void* (*cloud_helper_init)(struct nodeID *local, const char *config);
  int (*get_from_cloud)(void *context, const char *key, uint8_t *header_ptr,
                        int header_size, int free_header);
  int (*get_from_cloud_default)(void *context, const char *key,
                                uint8_t *header_ptr, int header_size, int free_header,
                                uint8_t *defval_ptr, int defval_size, int free_defval);
  int (*put_on_cloud)(void *context, const char *key, uint8_t *buffer_ptr,
                      int buffer_size, int free_buffer);
  struct nodeID* (*get_cloud_node)(void *context, uint8_t variant);
  time_t (*timestamp_cloud)(void *context);
  int (*is_cloud_node)(void *context, struct nodeID* node);
  int (*wait4cloud)(void *context, struct timeval *tout);
  int (*recv_from_cloud)(void *context, uint8_t *buffer_ptr, int buffer_size);
};

/***********************************************************************
 * libs3 data structures
 ***********************************************************************/
/* response properties handler */
static S3Status
libs3_response_properties_callback
(const S3ResponseProperties *properties,
 void *callbackData);

/* request completion callback */
static void
libs3_response_complete_callback
(S3Status status,
 const S3ErrorDetails *error,
 void *callbackData);

/* put request callback */
static int
libs3_put_object_data_callback
(int bufferSize,
 char *buffer,
 void *callbackData);

/* get request callback */
static S3Status
libs3_get_object_data_callback
(int bufferSize,
 const char *buffer,
 void *callbackData);


static S3PutObjectHandler libs3_put_object_handler =
  {
    {
      &libs3_response_properties_callback,
      &libs3_response_complete_callback
    },
    &libs3_put_object_data_callback
  };

static S3GetObjectHandler libs3_get_object_handler =
  {
    {
      &libs3_response_properties_callback,
      &libs3_response_complete_callback
    },
    &libs3_get_object_data_callback
  };


/***********************************************************************
 * libs3_delegate_helper contexts
 ***********************************************************************/

/* libs3 cloud context definition */
struct libs3_cloud_context {
  struct req_handler_ctx *req_handler;
  S3BucketContext s3_bucket_context;
  int blocking_put_request;
  time_t last_rsp_timestamp;
};

/***********************************************************************
 * Requests/Response pool data structures
 ***********************************************************************/
enum operation_t {PUT=0, GET=1};
struct libs3_request {
  enum operation_t op;
  char *key;

  /* For GET operations this point to the header.
     For PUT this is the pointer to the actual data */
  uint8_t *data;
  int data_length;
  int free_data;

  uint8_t *default_value;
  int default_value_length;
  int free_default_value;

  struct libs3_cloud_context *ctx;
};
typedef struct libs3_request libs3_request_t;

struct libs3_get_response {
  S3Status status;
  uint8_t *data;
  uint8_t *current_byte;

  int data_length;
  int read_bytes;
  time_t last_timestamp;
};
typedef struct libs3_get_response libs3_get_response_t;

struct libs3_callback_context {
  libs3_request_t *current_req;

  /* point to the current byte to read/write */
  uint8_t *start_ptr;

  /* number of bytes read/written until now */
  int bytes;

  /* store for get request */
  uint8_t *buffer;
  size_t buffer_size;

  time_t last_timestamp;
  S3Status status;
};


static void free_request(void *req_ptr)
{
  libs3_request_t *req;
  if (!req_ptr) return;

  req = (libs3_request_t *) req_ptr;

  free(req->key);
  if (req->free_data > 0) free(req->data);
  if (req->free_default_value > 0)
    free(req->default_value);

  free(req);
}

static void free_response(libs3_get_response_t *rsp) {
  if (rsp->data) free(rsp->data);

  free(rsp);
}

/************************************************************************
 * libs3 callback implementation
 ************************************************************************/
static S3Status
libs3_response_properties_callback(const S3ResponseProperties *properties,
                                   void *context)
{
  struct libs3_callback_context *req_ctx;
  req_ctx = (struct libs3_callback_context *) context;

  if (properties->lastModified > 0) {
    req_ctx->last_timestamp = (time_t) properties->lastModified;
  } else {
    req_ctx->last_timestamp = 0;
  }

  if (properties->contentLength && req_ctx->current_req->op == GET) {
    uint64_t actual_length;
    size_t supported_length;

    actual_length = (properties->contentLength +
                     req_ctx->current_req->data_length);
    supported_length = (size_t) actual_length;

    /* This is probably useless as if actual_length is so big
       there is no way we can keep it all in memory */
    if (supported_length < actual_length)
      return S3StatusAbortedByCallback;

    req_ctx->buffer = malloc(actual_length);
    if (!req_ctx->buffer)
      return S3StatusAbortedByCallback;
    req_ctx->buffer_size = actual_length;

    if (req_ctx->current_req->data_length > 0) {
      memcpy(req_ctx->buffer,
             req_ctx->current_req->data,
             req_ctx->current_req->data_length);
    }
    req_ctx->start_ptr = req_ctx->buffer + req_ctx->current_req->data_length;
    req_ctx->bytes = 0;
  }

  return S3StatusOK;
}

/* request completion callback */
static void
libs3_response_complete_callback(S3Status status, const S3ErrorDetails *error,
                                 void *context)
{
  struct libs3_callback_context *req_ctx;
  req_ctx = (struct libs3_callback_context *) context;

  req_ctx->status = status;
  if (status != S3StatusOK) {
    if (error) {
      if (error->message) {
        fprintf(stderr, "libs3_delegate_helper: Error performing request" \
                "-> %s\n", error->message);
      } else {
        fprintf(stderr, "libs3_delegate_helper: Unknown error performing " \
                "request\n");
      }
    }
  }
}

/* put request callback */
static int
libs3_put_object_data_callback(int bufferSize, char *buffer,
                               void *context)
{
  struct libs3_callback_context *req_ctx;
  int towrite;
  req_ctx = (struct libs3_callback_context *) context;

  towrite = req_ctx->current_req->data_length - req_ctx->bytes;

  req_ctx->status = S3StatusOK;

  if (towrite == 0)
    return 0;

  towrite = (towrite > bufferSize)? bufferSize : towrite;

  memcpy(buffer, req_ctx->start_ptr, towrite);
  req_ctx->bytes += towrite;
  return towrite;
}

/* get request callback */
static S3Status
libs3_get_object_data_callback(int bufferSize, const char *buffer,
                               void *context)
{
  struct libs3_callback_context *req_ctx;
  req_ctx = (struct libs3_callback_context *) context;

  /* The buffer should have been prepared by the properties callback.
     If not, it means that s3 didn't report the content length */
  if (!req_ctx->buffer) {
    req_ctx->buffer = malloc(bufferSize + req_ctx->current_req->data_length);
    if (!req_ctx->buffer) return S3StatusAbortedByCallback;

    if (req_ctx->current_req->data_length > 0) {
      memcpy(req_ctx->buffer,
             req_ctx->current_req->data,
             req_ctx->current_req->data_length);
    }
    req_ctx->start_ptr = req_ctx->buffer + req_ctx->current_req->data_length;
    req_ctx->bytes = 0;
    req_ctx->buffer_size = bufferSize;
  }

  /* If s3 didn't report the content length we make room for it on
     the fly */
  if (req_ctx->bytes + bufferSize > req_ctx->buffer_size) {
    int new_size;
    uint8_t *old;

    new_size = req_ctx->buffer_size + (bufferSize * 2);

    old = req_ctx->buffer;
    req_ctx->buffer = realloc(req_ctx->buffer, new_size);
    if (!req_ctx->buffer) {
      free(old);
      return S3StatusAbortedByCallback;
    }

    req_ctx->start_ptr = (req_ctx->buffer +
                          req_ctx->bytes +
                          req_ctx->current_req->data_length);
  }

  memcpy(req_ctx->start_ptr, buffer, bufferSize);
  req_ctx->bytes += bufferSize;

  return S3StatusOK;
}

/************************************************************************
 * request_handler thread implementation
 ************************************************************************/

int should_retry(int *counter, int times)
{
  if (times > 0){
    *counter = times;
  } else {
    sleep(1); /* give some time to the network */
    (*counter)--;
  }
  return *counter > 0;
}


static int process_put_request(void *req_data, void **rsp_data)
{
  libs3_request_t *req;
  struct libs3_callback_context *cbk_ctx;
  int retries_left;
  int status;

  req = (libs3_request_t *) req_data;

  should_retry(&retries_left, 3);

  /* put operation never have response */
  *rsp_data = NULL;

  cbk_ctx = malloc(sizeof(struct libs3_callback_context));
  if (!cbk_ctx) return -1;

  cbk_ctx->current_req = req;
  cbk_ctx->status = S3StatusInternalError;
  cbk_ctx->start_ptr = req->data;
  cbk_ctx->bytes = 0;
  cbk_ctx->buffer = NULL;
  cbk_ctx->buffer_size = 0;

  do {
    S3_put_object(&req->ctx->s3_bucket_context, /* bucket info */
                  req->key,                /* key to insert */
                  req->data_length,        /* length of data  */
                  NULL,                     /* use standard properties */
                  NULL,                    /* do a blocking call... */
                  &libs3_put_object_handler,/* ...using these callback...*/
                  cbk_ctx);                /* ...with this data for context */

    /* if we get an error related to a temporary network state retry */
  } while(S3_status_is_retryable(cbk_ctx->status) &&
          should_retry(&retries_left, 0));

  status = (cbk_ctx->status == S3StatusOK) ? 0 : -1;

  free(cbk_ctx);

  return status;
}

static int process_get_request(void *req_data, void **rsp_data)
{
  libs3_request_t *req;
  struct libs3_callback_context *cbk_ctx;
  struct libs3_get_response *rsp;
  int retries_left;
  int status;

  req = (libs3_request_t *) req_data;

  should_retry(&retries_left, 3);

  /* Initilize s3 callback data */
  cbk_ctx = malloc(sizeof(struct libs3_callback_context));
  if (!cbk_ctx) return -1;
  cbk_ctx->current_req = req;
  cbk_ctx->status = S3StatusInternalError;
  cbk_ctx->start_ptr = NULL;
  cbk_ctx->bytes = 0;
  cbk_ctx->buffer = NULL;
  cbk_ctx->buffer_size = 0;


  do {
    S3_get_object(&req->ctx->s3_bucket_context, /* bucket info */
                  req->key,                /* key to retrieve */
                  NULL,                    /* do not use conditions */
                  0,                       /* start from byte 0... */
                  0,                       /* ...and read all bytes */
                  NULL,                    /* do a blocking call... */
                  &libs3_get_object_handler,/* ...using these callback...*/
                  cbk_ctx);                /* ...with this data for context */

    /* if we get an error related to a temporary network state retry */
  } while(S3_status_is_retryable(cbk_ctx->status) &&
          should_retry(&retries_left, 0));


  rsp = malloc(sizeof(struct libs3_get_response));
  if (!rsp) {
    if (cbk_ctx->buffer) free(cbk_ctx->buffer);
    free(cbk_ctx);
    return -1;
  }

  *rsp_data = rsp;
  rsp->status = cbk_ctx->status;
  if (cbk_ctx->status == S3StatusOK) {
    rsp->data = cbk_ctx->buffer;
    rsp->current_byte = rsp->data;
    rsp->data_length = cbk_ctx->bytes + req->data_length;
    rsp->read_bytes = 0;
    rsp->last_timestamp = cbk_ctx->last_timestamp;
  } else {
    /* Since there was no value for the specified key. If the caller specified a
       default value, use that */
    if (req->default_value) {
      rsp->data = malloc(req->data_length + req->default_value_length);
      if (!rsp->data) return 1;
      rsp->current_byte = rsp->data;
      if (req->data_length > 0)
        memcpy(rsp->data, req->data, req->data_length);

      memcpy(rsp->data, req->default_value, req->default_value_length);

      rsp->status = S3StatusOK;
    }
  }

  free(cbk_ctx);
  status = (rsp->status == S3StatusOK) ? 0 : -1;


  return status;
}


/************************************************************************
 * cloud helper implementation
 ************************************************************************/
static void deallocate_context(struct libs3_cloud_context *ctx)
{
  if (!ctx) return;

  free(ctx);
  return;
}


void* cloud_helper_init(struct nodeID *local, const char *config)
{
  struct libs3_cloud_context *ctx;
  struct tag *cfg_tags;
  const char *arg;

  ctx = malloc(sizeof(struct libs3_cloud_context));
  memset(ctx, 0, sizeof(struct libs3_cloud_context));
  cfg_tags = config_parse(config);

  /* Parse fundametal parameters */
  arg = config_value_str(cfg_tags, "s3_access_key");
  if (!arg) {
    deallocate_context(ctx);
    fprintf(stderr,
            "libs3_delegate_helper: missing required parameter " \
            "'s3_access_key'\n");
    return 0;
  }
  ctx->s3_bucket_context.accessKeyId = strdup(arg);

  arg = config_value_str(cfg_tags, "s3_secret_key");
  if (!arg) {
    deallocate_context(ctx);
    fprintf(stderr,
            "libs3_delegate_helper: missing required parameter " \
            "'s3_secret_key'\n");
    return 0;
  }
  ctx->s3_bucket_context.secretAccessKey = strdup(arg);

  arg = config_value_str(cfg_tags, "s3_bucket_name");
  if (!arg) {
    deallocate_context(ctx);
    fprintf(stderr,
            "libs3_delegate_helper: missing required parameter " \
            "'s3_bucket_name'\n");
    return 0;
  }
  ctx->s3_bucket_context.bucketName = strdup(arg);

  ctx->s3_bucket_context.protocol = S3ProtocolHTTPS;
  arg = config_value_str(cfg_tags, "s3_protocol");
  if (arg) {
    if (strcmp(arg, "https") == 0) {
      ctx->s3_bucket_context.protocol = S3ProtocolHTTPS;
    } else if (strcmp(arg, "http") == 0) {
      ctx->s3_bucket_context.protocol = S3ProtocolHTTP;
    }
  }

  ctx->s3_bucket_context.uriStyle = S3UriStylePath;


  /* Parse optional parameters */
  ctx->blocking_put_request = 1;
  arg = config_value_str(cfg_tags, "s3_blocking_put");
  if (arg) {
    if (strcmp(arg, "1") == 0)
      ctx->blocking_put_request = 1;
    else if (strcmp(arg, "0") == 0)
      ctx->blocking_put_request = 0;
  }

  /* Initialize data structures */
  if (S3_initialize("libs3_delegate_helper", S3_INIT_ALL) != S3StatusOK) {
    fprintf(stderr,
            "libs3_delegate_helper: error inizializing libs3\n");
    deallocate_context(ctx);
    return NULL;
  }

  ctx->req_handler = req_handler_init();
  if (!ctx->req_handler) {
    fprintf(stderr,
            "libs3_delegate_helper: error initializing request handler\n");
    deallocate_context(ctx);
    return NULL;
  }

  return ctx;
}

int get_from_cloud_default(void *context, const char *key, uint8_t *header_ptr,
                           int header_size, int free_header, uint8_t *defval_ptr,
                           int defval_size, int free_defval)
{
  struct libs3_cloud_context *ctx;
  libs3_request_t *request;

  ctx = (struct libs3_cloud_context *) context;
  request = malloc(sizeof(libs3_request_t));

  if (!request) return 1;

  request->op = GET;
  request->key = strdup(key);
  request->data = header_ptr;
  request->data_length = header_size;
  request->free_data = free_header;
  request->default_value = defval_ptr;
  request->default_value_length = defval_size;
  request->free_default_value = free_defval;
  request->ctx = ctx;

  req_handler_add_request(ctx->req_handler,
                          &process_get_request,
                          request,
                          &free_request);

  return 0;
}

int get_from_cloud(void *context, const char *key, uint8_t *header_ptr,
                   int header_size, int free_header)
{
  return get_from_cloud_default(context, key, header_ptr, header_size,
                                free_header, NULL, 0, 0);
}

int put_on_cloud(void *context, const char *key, uint8_t *buffer_ptr,
                 int buffer_size, int free_buffer)
{
  struct libs3_cloud_context *ctx;
  libs3_request_t *request;

  ctx = (struct libs3_cloud_context *) context;
  request = malloc(sizeof(libs3_request_t));

  if (!request) return 1;

  request->op = PUT;
  request->key = strdup(key);
  request->data = buffer_ptr;
  request->data_length = buffer_size;
  request->free_data = free_buffer;
  request->default_value = NULL;
  request->default_value_length = 0;
  request->free_default_value = 0;
  request->ctx = ctx;

  if (ctx->blocking_put_request) {
    int res;
    void *rsp;
    res = process_put_request(request, &rsp);
    free_request(request);
    return res;
  }
  else {
    return req_handler_add_request(ctx->req_handler, &process_put_request,
                                   request, &free_request);
  }
}

struct nodeID* get_cloud_node(void *context, uint8_t variant)
{
  return create_node(CLOUD_NODE_ADDR, variant);
}

time_t timestamp_cloud(void *context)
{
  struct libs3_cloud_context *ctx;
  ctx = (struct libs3_cloud_context *) context;

  return ctx->last_rsp_timestamp;
}

int is_cloud_node(void *context, struct nodeID* node)
{
  char buff[96];
  node_ip(node, buff, 96);
  return strcmp(buff, CLOUD_NODE_ADDR) == 0;
}

int wait4cloud(void *context, struct timeval *tout)
{
  struct libs3_cloud_context *ctx;
  libs3_get_response_t *rsp;

  ctx = (struct libs3_cloud_context *) context;

  rsp = (libs3_get_response_t*)req_handler_wait4response(ctx->req_handler, tout);

  if (rsp) {
    if (rsp->status == S3StatusOK) {
      ctx->last_rsp_timestamp = rsp->last_timestamp;
      return 1;
    } else {
      /* there was some error with the request */
      req_handler_remove_response(ctx->req_handler);
      free_response(rsp);
      return -1;
    }
  } else {
    return 0;
  }
}

int recv_from_cloud(void *context, uint8_t *buffer_ptr, int buffer_size)
{
  struct libs3_cloud_context *ctx;
  libs3_get_response_t *rsp;
  int remaining;
  int toread;

  ctx = (struct libs3_cloud_context *) context;

  rsp = (libs3_get_response_t *) req_handler_get_response(ctx->req_handler);
  if (!rsp) return -1;

  /* If do not have further data just remove the request */
  if (rsp->read_bytes == rsp->data_length) {
    req_handler_remove_response(ctx->req_handler);
    free_response(rsp);
    return 0;
  }

  remaining = rsp->data_length - rsp->read_bytes;
  toread = (remaining <= buffer_size)? remaining : buffer_size;

  memcpy(buffer_ptr, rsp->current_byte, toread);
  rsp->current_byte += toread;
  rsp->read_bytes += toread;

  /* remove the response only if the read bytes are less the the allocated
     buuffer otherwise the client can't know when a single response finished */
  if (rsp->read_bytes == rsp->data_length && rsp->read_bytes < buffer_size){
    req_handler_remove_response(ctx->req_handler);
    free_response(rsp);
  }

  return toread;
}

struct delegate_iface delegate_impl = {
  .cloud_helper_init = &cloud_helper_init,
  .get_from_cloud = &get_from_cloud,
  .get_from_cloud_default = &get_from_cloud_default,
  .put_on_cloud = &put_on_cloud,
  .get_cloud_node = &get_cloud_node,
  .timestamp_cloud = &timestamp_cloud,
  .is_cloud_node = &is_cloud_node,
  .wait4cloud = &wait4cloud,
  .recv_from_cloud = &recv_from_cloud
};
