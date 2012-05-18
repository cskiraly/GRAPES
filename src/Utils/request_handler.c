/*
 *  Copyright (c) 2011 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#include "request_handler.h"
#include "fifo_queue.h"


void* request_handler(void *data);


struct req_handler_ctx {
  /* request/response management */
  pthread_mutex_t req_queue_lock;
  fifo_queue_p req_queue;

  pthread_mutex_t rsp_queue_sync_mutex;
  pthread_cond_t rsp_queue_sync_cond;
  pthread_mutex_t rsp_queue_lock;
  fifo_queue_p rsp_queue;

  /* thread management */
  pthread_attr_t req_handler_thread_attr;
  pthread_cond_t req_handler_sync_cond;
  pthread_mutex_t req_handler_sync_mutex;
  pthread_t req_handler_thread;
};


typedef struct request {
  process_request_callback_p req_callback;
  free_request_callback_p free_callback;
  void *req_data;
} request_t;

/***********************************************************************
 * Initialization/destruction
 ***********************************************************************/
void req_handler_destroy(struct req_handler_ctx *ctx)
{
  if (!ctx) return;

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

/* Allocate and initialize needed structures */
struct req_handler_ctx* req_handler_init()
{
  struct req_handler_ctx *ctx;
  int err;

  ctx = malloc(sizeof(struct req_handler_ctx));
  memset(ctx, 0, sizeof(struct req_handler_ctx));

  ctx->req_queue = fifo_queue_create(10);
  if (!ctx->req_queue) {
    req_handler_destroy(ctx);
    return 0;
  }

  ctx->rsp_queue = fifo_queue_create(10);
  if (!ctx->rsp_queue) {
    req_handler_destroy(ctx);
    return 0;
  }


  err = pthread_mutex_init(&ctx->req_queue_lock, NULL);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  err = pthread_mutex_init(&ctx->rsp_queue_lock, NULL);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  err = pthread_mutex_init(&ctx->rsp_queue_sync_mutex, NULL);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  err = pthread_cond_init (&ctx->rsp_queue_sync_cond, NULL);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  err = pthread_mutex_init(&ctx->req_handler_sync_mutex, NULL);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  err = pthread_cond_init (&ctx->req_handler_sync_cond, NULL);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  err = pthread_attr_init(&ctx->req_handler_thread_attr);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  err = pthread_attr_setdetachstate(&ctx->req_handler_thread_attr,
                                    PTHREAD_CREATE_JOINABLE);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  err = pthread_create(&ctx->req_handler_thread,
                       &ctx->req_handler_thread_attr,
                       &request_handler,
                       (void *)ctx);
  if (err) {
    req_handler_destroy(ctx);
    return 0;
  }

  return ctx;
}

/***********************************************************************
 * Request management
 ***********************************************************************/
static void free_request(request_t *req)
{
  if (req->free_callback) req->free_callback(req->req_data);
  free(req);
}

int req_handler_add_request(struct req_handler_ctx *ctx,
                            process_request_callback_p req_callback,
                            void *req_data,
                            free_request_callback_p free_callback)
{
  request_t *request;
  int err;

  request = malloc(sizeof(request_t));
  if (!request) return 1;

  request->req_callback = req_callback;
  request->req_data = req_data;
  request->free_callback = free_callback;

  /* add the request to the pool */
  pthread_mutex_lock(&ctx->req_queue_lock);
  err = fifo_queue_add(ctx->req_queue, request);
  pthread_mutex_unlock(&ctx->req_queue_lock);

  if (err) {
    free (request);
    return 1;
  }

  /* notify request handler thread */
  pthread_mutex_lock(&ctx->req_handler_sync_mutex);
  pthread_cond_signal(&ctx->req_handler_sync_cond);
  pthread_mutex_unlock(&ctx->req_handler_sync_mutex);

  return 0;
}

/***********************************************************************
 * Response management
 ***********************************************************************/
void* req_handler_wait4response(struct req_handler_ctx *ctx,
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

  return req_handler_get_response(ctx);
}

static int add_response(struct req_handler_ctx *ctx, void *rsp)
{
  int err;
  /* add the response to the pool */
  pthread_mutex_lock(&ctx->rsp_queue_lock);
  err = fifo_queue_add(ctx->rsp_queue, rsp);
  pthread_mutex_unlock(&ctx->rsp_queue_lock);

  if (err) return 1;

  /* notify wait4response there's a response in the queue */
  pthread_mutex_lock(&ctx->rsp_queue_sync_mutex);
  pthread_cond_signal(&ctx->rsp_queue_sync_cond);
  pthread_mutex_unlock(&ctx->rsp_queue_sync_mutex);

  return 0;
}

void* req_handler_get_response(struct req_handler_ctx *ctx)
{
  if (fifo_queue_size(ctx->rsp_queue) > 0) {
    void *rsp;

    pthread_mutex_lock(&ctx->rsp_queue_lock);
    rsp = fifo_queue_get_head(ctx->rsp_queue);
    pthread_mutex_unlock(&ctx->rsp_queue_lock);
    return rsp;
  }

  return NULL;
}

void* req_handler_remove_response(struct req_handler_ctx *ctx)
{
  void *rsp;

  pthread_mutex_lock(&ctx->rsp_queue_lock);
  rsp = fifo_queue_remove_head(ctx->rsp_queue);
  pthread_mutex_unlock(&ctx->rsp_queue_lock);

  return rsp;
}


/***********************************************************************
 * Request handler implementation
 ***********************************************************************/
static int request_handler_process_requests(struct req_handler_ctx *ctx)
{
  request_t *req;
  int req_number;

  req_number = 0;
  do {
    pthread_mutex_lock(&ctx->req_queue_lock);
    req = fifo_queue_remove_head(ctx->req_queue);
    pthread_mutex_unlock(&ctx->req_queue_lock);

    if (req) {
      int status;
      void *rsp_data;

      req_number++;

      status = req->req_callback(req->req_data, &rsp_data);

      switch(status){
      case 0:
        /* request successful */
        if (rsp_data) {
          /* We have a response */
          status = add_response(ctx, rsp_data);
          if (status != 0) {
            fprintf(stderr, "req_handler: error adding response to queue\n");
          }
        }

        free_request(req);
        break;
      case 1:
        /* request to requeue */
        fifo_queue_add(ctx->req_queue, req);
        break;

      case -1:
        /* request aborted */
        free_request(req);
        break;

      default:
        fprintf(stderr,"req_handler: invalid return status from callback\n");
        free_request(req);
      }
    }
  } while(req != NULL);

  return req_number;
}

void* request_handler(void *data)
{
  struct req_handler_ctx *ctx;

  ctx = (struct req_handler_ctx *) data;
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
