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
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sys/time.h>

#include <libs3.h>

#include "net_helper.h"
#include "cloud_helper_iface.h"
#include "fifo_queue.h"
#include "config.h"

#define CLOUD_NODE_ADDR "0.0.0.0"

/***********************************************************************
 * Interface prototype for cloud_helper_delegate
 ***********************************************************************/
struct delegate_iface {
  void* (*cloud_helper_init)(struct nodeID *local, const char *config);
  int (*get_from_cloud)(void *context, const char *key, uint8_t *header_ptr,
                        int header_size, int free_header);
  int (*put_on_cloud)(void *context, const char *key, uint8_t *buffer_ptr,
                      int buffer_size, int free_buffer);
  struct nodeID* (*get_cloud_node)(void *context, uint8_t variant);
  time_t (*timestamp_cloud)(void *context);
  int (*is_cloud_node)(void *context, struct nodeID* node);
  int (*wait4cloud)(void *context, struct timeval *tout);
  int (*recv_from_cloud)(void *context, uint8_t *buffer_ptr, int buffer_size);
};

/***********************************************************************
 * Requests/Response pool data structures
 ***********************************************************************/
enum operation_t {PUT=0, GET=1};

typedef struct libs3_request {
  enum operation_t op;
  char *key;

  /* For GET operations this point to the header.
     For PUT this is the pointer to the actual data */
  uint8_t *data;
  int data_length;
  int free_data;
} libs3_request_t;

typedef struct libs3_get_response {
  S3Status status;
  uint8_t *data;
  uint8_t *current_byte;

  int data_length;
  int read_bytes;
  time_t last_timestamp;
} libs3_get_response_t;


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
  /* libs3 information */
  S3BucketContext s3_bucket_context;
  int blocking_put_request;

  /* request/response management */
  pthread_mutex_t req_queue_lock;
  fifo_queue_p req_queue;

  pthread_mutex_t rsp_queue_sync_mutex;
  pthread_cond_t rsp_queue_sync_cond;
  pthread_mutex_t rsp_queue_lock;
  fifo_queue_p rsp_queue;

  time_t last_rsp_timestamp;

  /* thread management */
  pthread_attr_t req_handler_thread_attr;
  pthread_cond_t req_handler_sync_cond;
  pthread_mutex_t req_handler_sync_mutex;
  pthread_t req_handler_thread;
};

struct libs3_request_context {
  struct libs3_cloud_context *cloud_ctx;
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

static void free_request(libs3_request_t *req)
{
  if (!req) return;

  free(req->key);
  if (req->free_data > 0) free(req->data);

  free(req);
}

static void add_request(struct libs3_cloud_context *ctx, libs3_request_t *req)
{
  /* add the request to the pool */
  pthread_mutex_lock(&ctx->req_queue_lock);
  fifo_queue_add(ctx->req_queue, req);
  pthread_mutex_unlock(&ctx->req_queue_lock);

  /* notify request handler thread */
  pthread_mutex_lock(&ctx->req_handler_sync_mutex);
  pthread_cond_signal(&ctx->req_handler_sync_cond);
  pthread_mutex_unlock(&ctx->req_handler_sync_mutex);
}


static libs3_get_response_t* get_response(struct libs3_cloud_context *ctx)
{
  if (fifo_queue_size(ctx->rsp_queue) > 0) {
    libs3_get_response_t *rsp;

    pthread_mutex_lock(&ctx->rsp_queue_lock);
    rsp = fifo_queue_get_head(ctx->rsp_queue);
    pthread_mutex_unlock(&ctx->rsp_queue_lock);
    return rsp;
  }

  return NULL;
}

static libs3_get_response_t* wait4response(struct libs3_cloud_context *ctx,
                                           struct timeval *tout)
{
  if (fifo_queue_size(ctx->rsp_queue) == 0) {
    /* if there's no data ready to process, let's wait */
    struct timespec timeout;
    struct timeval abs_tout;
    int err;

    gettimeofday(&abs_tout, NULL);
    abs_tout.tv_sec += tout->tv_sec;
    abs_tout.tv_usec += tout->tv_usec;

    timeout.tv_sec = abs_tout.tv_sec + (abs_tout.tv_usec / 1000000);
    timeout.tv_nsec = abs_tout.tv_usec % 1000000;


    pthread_mutex_lock(&ctx->rsp_queue_sync_mutex);
    /* make sure that no data came in the meanwhile */
    if (fifo_queue_size(ctx->rsp_queue) == 0) {
      err = pthread_cond_timedwait(&ctx->rsp_queue_sync_cond,
                                   &ctx->rsp_queue_sync_mutex,
                                   &timeout);
    }
    pthread_mutex_unlock(&ctx->rsp_queue_sync_mutex);
  }

  return get_response(ctx);
}

 static void add_response(struct libs3_cloud_context *ctx,
                          libs3_get_response_t *rsp)
{
  /* add the response to the pool */
  pthread_mutex_lock(&ctx->rsp_queue_lock);
  fifo_queue_add(ctx->rsp_queue, rsp);
  pthread_mutex_unlock(&ctx->rsp_queue_lock);

  /* notify wait4response there's a response in the queue */
  pthread_mutex_lock(&ctx->rsp_queue_sync_mutex);
  pthread_cond_signal(&ctx->rsp_queue_sync_cond);

  fflush(stderr);
  pthread_mutex_unlock(&ctx->rsp_queue_sync_mutex);
}

static void pop_response(struct libs3_cloud_context *ctx)
{
  libs3_get_response_t *rsp;

  pthread_mutex_lock(&ctx->rsp_queue_lock);
  rsp = fifo_queue_remove_head(ctx->rsp_queue);
  pthread_mutex_unlock(&ctx->rsp_queue_lock);

  if (rsp) {
    if (rsp->data) free(rsp->data);
    free(rsp);
  }
}

/************************************************************************
 * libs3 callback implementation
 ************************************************************************/
static S3Status
libs3_response_properties_callback(const S3ResponseProperties *properties,
                                   void *context)
{
  struct libs3_request_context *req_ctx;
  req_ctx = (struct libs3_request_context *) context;

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
  struct libs3_request_context *req_ctx;
  req_ctx = (struct libs3_request_context *) context;

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
  struct libs3_request_context *req_ctx;
  int towrite;
  req_ctx = (struct libs3_request_context *) context;

  towrite = req_ctx->current_req->data_length - req_ctx->bytes;

  if (towrite == 0) return 0;

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
  struct libs3_request_context *req_ctx;
  req_ctx = (struct libs3_request_context *) context;

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


static int
request_handler_process_put_request(struct libs3_cloud_context *ctx,
                                    libs3_request_t *req)
{
  struct libs3_request_context *req_ctx;
  int retries_left;
  should_retry(&retries_left, 3);

  req_ctx = malloc(sizeof(struct libs3_request_context));
  if (!req_ctx) return 1;

  req_ctx->current_req = req;
  req_ctx->cloud_ctx = ctx;
  req_ctx->status = S3StatusInternalError;
  req_ctx->start_ptr = req->data;
  req_ctx->bytes = 0;
  req_ctx->buffer = NULL;
  req_ctx->buffer_size = 0;

  do {
    S3_put_object(&ctx->s3_bucket_context, /* bucket info */
                  req->key,                /* key to insert */
                  req->data_length,        /* length of data  */
                  NULL,                     /* use standard properties */
                  NULL,                    /* do a blocking call... */
                  &libs3_put_object_handler,/* ...using these callback...*/
                  req_ctx);                /* ...with this data for context */

    /* if we get an error related to a temporary network state retry */
  } while(S3_status_is_retryable(req_ctx->status) &&
          should_retry(&retries_left, 0));

  return (req_ctx->status == S3StatusOK) ? 0 : 1;
}

static int
request_handler_process_get_request(struct libs3_cloud_context *ctx,
                                    libs3_request_t *req)
{
  struct libs3_request_context *req_ctx;
  struct libs3_get_response *rsp;
  int retries_left;
  should_retry(&retries_left, 3);

  req_ctx = malloc(sizeof(struct libs3_request_context));
  if (!req_ctx) return 1;

  req_ctx->current_req = req;
  req_ctx->cloud_ctx = ctx;
  req_ctx->status = S3StatusInternalError;
  req_ctx->start_ptr = NULL;
  req_ctx->bytes = 0;
  req_ctx->buffer = NULL;
  req_ctx->buffer_size = 0;

  do {
    S3_get_object(&ctx->s3_bucket_context, /* bucket info */
                  req->key,                /* key to retrieve */
                  NULL,                    /* do not use conditions */
                  0,                       /* start from byte 0... */
                  0,                       /* ...and read all bytes */
                  NULL,                    /* do a blocking call... */
                  &libs3_get_object_handler,/* ...using these callback...*/
                  req_ctx);                /* ...with this data for context */

    /* if we get an error related to a temporary network state retry */
  } while(S3_status_is_retryable(req_ctx->status) &&
          should_retry(&retries_left, 0));


  rsp = malloc(sizeof(struct libs3_get_response));
  if (!rsp) {
    if (req_ctx->buffer) free(req_ctx->buffer);
    free(req_ctx);
    return 1;
  }

  rsp->status = req_ctx->status;
  if (req_ctx->status == S3StatusOK) {
    rsp->data = req_ctx->buffer;
    rsp->current_byte = rsp->data;
    rsp->data_length = req_ctx->bytes + req->data_length;
    rsp->read_bytes = 0;
    rsp->last_timestamp = req_ctx->last_timestamp;
  }
  add_response(ctx, rsp);

  return (req_ctx->status == S3StatusOK) ? 0 : 1;
}

static int request_handler_process_requests(struct libs3_cloud_context *ctx)
{
  libs3_request_t *req;
  int req_number;

  req_number = 0;
  do {
    pthread_mutex_lock(&ctx->req_queue_lock);
    req = fifo_queue_remove_head(ctx->req_queue);
    pthread_mutex_unlock(&ctx->req_queue_lock);

    if (req) {
      int status;

      req_number++;

      switch (req->op) {
      case PUT:
        status = request_handler_process_put_request(ctx, req);
        break;
      case GET:
        status = request_handler_process_get_request(ctx, req);
        break;
      default:
        /* WTF: should not be here! */
        fprintf(stderr,
                "libs3_delegate_helper: operation type not supported!\n");

        status = 0;
      }

      if (status == 1) {
        fprintf(stderr,
                "libs3_delegate_helper: failed to perform operation\n");
      }

      free_request(req);
    }
  } while(req != NULL);

  return req_number;
}

void* request_handler(void *data)
{
  struct libs3_cloud_context *ctx;

  ctx = (struct libs3_cloud_context *) data;
  do{
    /* wait for main thread to signal there's some work to do */
    pthread_mutex_lock(&ctx->req_handler_sync_mutex);

    /* make sure not to block if there's already something to handle */
    if (fifo_queue_size(ctx->req_queue) == 0) {
      pthread_cond_wait(&ctx->req_handler_sync_cond,
                        &ctx->req_handler_sync_mutex);
    }
    request_handler_process_requests(ctx);

    pthread_mutex_unlock(&ctx->req_handler_sync_mutex);
  } while(1); /* grapes TopologyManager don't support termination */
}


/************************************************************************
 * cloud helper implementation
 ************************************************************************/
static void deallocate_context(struct libs3_cloud_context *ctx)
{
  if (!ctx) return;
  if (ctx->s3_bucket_context.accessKeyId)
    free(ctx->s3_bucket_context.accessKeyId);
  if (ctx->s3_bucket_context.secretAccessKey)
    free(ctx->s3_bucket_context.secretAccessKey);
  if (ctx->s3_bucket_context.bucketName)
    free(ctx->s3_bucket_context.bucketName);

  /* TODO: we should use a more specialized function instead of the
     standard free. But since the cloud_helper do not take into
     account deallocation the queue will always be empty when we
     enter this function */
  if (ctx->req_queue) fifo_queue_destroy(ctx->req_queue, NULL);
  if (ctx->rsp_queue) fifo_queue_destroy(ctx->rsp_queue, NULL);

  /* destroy mutexs, conds, threads, ...
     This should be safe as no mutex has already been locked and both
     mutex_destroy/cond_destroy check fail with EINVAL if the
     specified object is not initialized */
  pthread_mutex_destroy(&ctx->req_queue_lock);
  pthread_mutex_destroy(&ctx->rsp_queue_lock);
  pthread_mutex_destroy(&ctx->req_handler_sync_mutex);
  pthread_mutex_destroy(&ctx->rsp_queue_sync_mutex);

  pthread_cond_destroy(&ctx->rsp_queue_sync_cond);
  pthread_cond_destroy(&ctx->req_handler_sync_cond);

  pthread_attr_destroy(&ctx->req_handler_thread_attr);

  free(ctx);
  return;
}


void* cloud_helper_init(struct nodeID *local, const char *config)
{
  struct libs3_cloud_context *ctx;
  struct tag *cfg_tags;
  const char *arg;
  int err;

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

  ctx->s3_bucket_context.protocol = S3ProtocolHTTP;
  arg = config_value_str(cfg_tags, "s3_protocol");
  if (arg) {
    if (strcmp(arg, "https") == 0) {
      ctx->s3_bucket_context.protocol = S3ProtocolHTTPS;
    }
  }

  ctx->s3_bucket_context.uriStyle = S3UriStylePath;


  /* Parse optional parameters */
  ctx->blocking_put_request = 0;
  arg = config_value_str(cfg_tags, "s3_blocking_put");
  if (arg) {
    if (strcmp(arg, "1") == 0)
      ctx->blocking_put_request = 1;
  }

  /* Initialize data structures */
  if (S3_initialize("libs3_delegate_helper", S3_INIT_ALL) != S3StatusOK) {
    deallocate_context(ctx);
    return 0;
  }

  ctx->req_queue = fifo_queue_create(10);
  if (!ctx->req_queue) {
    deallocate_context(ctx);
    return 0;
  }

  ctx->rsp_queue = fifo_queue_create(10);
  if (!ctx->rsp_queue) {
    deallocate_context(ctx);
    return 0;
  }


  err = pthread_mutex_init(&ctx->req_queue_lock, NULL);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  err = pthread_mutex_init(&ctx->rsp_queue_lock, NULL);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  err = pthread_mutex_init(&ctx->rsp_queue_sync_mutex, NULL);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  err = pthread_cond_init (&ctx->rsp_queue_sync_cond, NULL);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  err = pthread_mutex_init(&ctx->req_handler_sync_mutex, NULL);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  err = pthread_cond_init (&ctx->req_handler_sync_cond, NULL);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  err = pthread_attr_init(&ctx->req_handler_thread_attr);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  err = pthread_attr_setdetachstate(&ctx->req_handler_thread_attr,
                                    PTHREAD_CREATE_JOINABLE);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  err = pthread_create(&ctx->req_handler_thread,
                       &ctx->req_handler_thread_attr,
                       &request_handler,
                       (void *)ctx);
  if (err) {
    deallocate_context(ctx);
    return 0;
  }

  return ctx;
}

int get_from_cloud(void *context, const char *key, uint8_t *header_ptr,
                   int header_size, int free_header)
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

  add_request(ctx, request);

  return 0;
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

  if (ctx->blocking_put_request) {
    int res;
    res = request_handler_process_put_request(ctx, request);
    free_request(request);
    return res;
  }
  else
    add_request(ctx, request);

  return 0;
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
  return strcmp(node_ip(node), CLOUD_NODE_ADDR) == 0;
}

int wait4cloud(void *context, struct timeval *tout)
{
  struct libs3_cloud_context *ctx;
  libs3_get_response_t *rsp;

  ctx = (struct libs3_cloud_context *) context;

  rsp = wait4response(ctx, tout);

  if (rsp) {
    if (rsp->status == S3StatusOK) {
      ctx->last_rsp_timestamp = rsp->last_timestamp;
      return 1;
    } else {
      /* there was some error with the request */
      pop_response(ctx);
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
  int toread;

  ctx = (struct libs3_cloud_context *) context;

  rsp = get_response(ctx);
  if (!rsp) return -1;

  if (rsp->read_bytes == rsp->data_length){
    pop_response(ctx);
    return 0;
  }

  toread = (rsp->data_length <= buffer_size)? rsp->data_length : buffer_size;

  memcpy(buffer_ptr, rsp->current_byte, toread);
  rsp->current_byte += toread;
  rsp->read_bytes += toread;

  return toread;
}

struct delegate_iface delegate_impl = {
  .cloud_helper_init = &cloud_helper_init,
  .get_from_cloud = &get_from_cloud,
  .put_on_cloud = &put_on_cloud,
  .get_cloud_node = &get_cloud_node,
  .timestamp_cloud = &timestamp_cloud,
  .is_cloud_node = &is_cloud_node,
  .wait4cloud = &wait4cloud,
  .recv_from_cloud = &recv_from_cloud
};
