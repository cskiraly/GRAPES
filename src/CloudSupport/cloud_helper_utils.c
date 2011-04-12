/*
 *  Copyright (c) 2010 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "cloud_helper.h"
#include "cloud_helper_utils.h"

struct wait4context {
  struct nodeID* node;
  struct cloud_helper_context *cloud;
  struct timeval *tout;
  int *user_fds;

  pthread_mutex_t wait_mutex;
  pthread_cond_t wait_cond;

  int status;
  int source;
};


static void* wait4data_wrapper(void *ctx)
{
  struct wait4context *wait4ctx;
  struct timeval tout;
  int status;

  wait4ctx = (struct wait4context *) ctx;
  tout = *wait4ctx->tout;
  status = wait4data(wait4ctx->node, &tout, wait4ctx->user_fds);
  if (status == 1){
    pthread_mutex_lock(&wait4ctx->wait_mutex);
    wait4ctx->source = DATA_SOURCE_NET;
    wait4ctx->status = status;
    pthread_cond_signal(&wait4ctx->wait_cond);
    pthread_mutex_unlock(&wait4ctx->wait_mutex);
  }
  pthread_exit(NULL);
}

static void* wait4cloud_wrapper(void *ctx)
{
  struct wait4context *wait4ctx;
  struct timeval tout;
  int status;

  wait4ctx = (struct wait4context *) ctx;
  tout = *wait4ctx->tout;
  status = wait4cloud(wait4ctx->cloud, &tout);
  if (status == 1 || status == -1) {
    pthread_mutex_lock(&wait4ctx->wait_mutex);
    wait4ctx->source = DATA_SOURCE_CLOUD;
    wait4ctx->status = status;
    pthread_cond_signal(&wait4ctx->wait_cond);
    pthread_mutex_unlock(&wait4ctx->wait_mutex);
  }
  pthread_exit(NULL);
}


int wait4any_threaded(struct nodeID *n, struct cloud_helper_context *cloud, struct timeval *tout, int *user_fds, int *data_source)
{
  pthread_attr_t attr;
  pthread_t wait4data_thread;
  pthread_t wait4cloud_thread;
  struct wait4context *wait4ctx;
  struct timespec timeout;
  struct timeval now;

  int err;
  int result;

  wait4ctx = malloc(sizeof(struct wait4context));
  if (wait4ctx == NULL) return -1;
  wait4ctx->node = n;
  wait4ctx->cloud = cloud;
  wait4ctx->tout = tout;
  wait4ctx->user_fds = user_fds;

  pthread_mutex_init(&wait4ctx->wait_mutex, NULL);
  pthread_cond_init (&wait4ctx->wait_cond, NULL);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  pthread_mutex_lock(&wait4ctx->wait_mutex);
  pthread_create(&wait4data_thread, &attr, wait4data_wrapper, (void *)wait4ctx);
  pthread_create(&wait4cloud_thread, &attr, wait4cloud_wrapper, (void *)wait4ctx);

  gettimeofday(&now, NULL);
  timeout.tv_sec = now.tv_sec + tout->tv_sec;
  timeout.tv_nsec = now.tv_usec * 1000 + tout->tv_usec * 1000;

  // Wait for one of the thread to signal available data
  err = pthread_cond_timedwait(&wait4ctx->wait_cond, &wait4ctx->wait_mutex, &timeout);
  if (err ==  0) {
    *data_source = wait4ctx->source;
    result = 1;
  } else if(err == ETIMEDOUT){
    *data_source = DATA_SOURCE_NONE;
    result = 0;
  }

  // Clean up and return
  pthread_cancel(wait4data_thread);
  pthread_cancel(wait4cloud_thread);
  pthread_cond_destroy(&wait4ctx->wait_cond);
  pthread_mutex_unlock(&wait4ctx->wait_mutex);
  pthread_mutex_destroy(&wait4ctx->wait_mutex);

  return result;
}


/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}


int wait4any_polling(struct nodeID *n, struct cloud_helper_context *cloud, struct timeval *tout, struct timeval *step_tout, int *user_fds, int *data_source)
{
  struct timeval timeout;
  struct timeval *step_time;
  uint8_t turn;
  int status;

  timeout = *tout;
  if (step_tout == NULL) {
    /* If step time not specified use a standard value of 100 milliseconds */
    step_time = malloc(sizeof(struct timeval));
    step_time->tv_sec = 0;
    step_time->tv_usec = 100000;
  } else {
    /* Otherwise use the specified value */
    step_time = step_tout;
  }

  turn = DATA_SOURCE_NET;
  while (turn != DATA_SOURCE_NONE) {
    // Try waiting for 100 milliseconds on one resource
    if (turn == DATA_SOURCE_NET)
      status =  wait4data(n, step_time, user_fds);
    else
      status = wait4cloud(cloud, step_time);

    if (status == 1){
      // If we got a positive response we're done
      *data_source = turn;
      free(step_time);
      return 1;
    } else {
      // Go on with another step
      turn = (turn == DATA_SOURCE_NET)? DATA_SOURCE_CLOUD:DATA_SOURCE_NET;
      if (step_tout == NULL) {
        step_time->tv_sec = 0;
        step_time->tv_usec = 100000;
      } else {
        *step_time = *step_tout;
      }

      // If we exeded the timeout it's time to stop
      if (timeval_subtract(&timeout, &timeout, step_time) < 0)
        turn = DATA_SOURCE_NONE;
    }
  }

  free(step_time);
  *data_source = DATA_SOURCE_NONE;
  return 0;
}
