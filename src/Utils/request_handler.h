/*
 *  Copyright (c) 2011 Andrea Zito
 *
 *  This is free software; see lgpl-2.1.txt
 *
 *  This module provide an easy way to perform request and receive response in an
 *  asynchronous way using blocking functions. Request and response are kept in a
 *  dedicated FIFO queue.
 */

#ifndef CLOUD_HELPER_DELEGATE_UTILS_H
#define CLOUD_HELPER_DELEGATE_UTILS_H



struct req_handler_ctx;

/* Callback called to process the req specified by req_data. Should return 0
   on success, 1 to requeue the request (as last element), -1 to abort the
   request. On success the request handler should allocate and populate the
   rsp_data pointer with the context of the response. If no response is
   needed, rsp_data should be assigned NULL. */
typedef int (*process_request_callback_p)(void *req_data, void **rsp_data);

typedef void (*free_request_callback_p)(void *req_data);

/* Initialize the data structures and threads and return a pointer to the
   cloud_req_handler context */
struct req_handler_ctx* req_handler_init();

/* Release the resource acquired  by init */
void req_handler_destroy(struct req_handler_ctx *ctx);

/* Add a request to the queue. Such a request will be handled by the function
   pointed by req_callback and freed by the function pointed by free_req_data.
   Return 0 on success, 1 on failure */
int req_handler_add_request(struct req_handler_ctx *ctx,
                            process_request_callback_p req_callback,
                            void *req_data,
                            free_request_callback_p free_req_data);

/* Wait for a response for at most tout */
void* req_handler_wait4response(struct req_handler_ctx *ctx,
                                struct timeval *tout);

/* Return the first response in the queue or NULL if none is ready */
void* req_handler_get_response(struct req_handler_ctx *ctx);

/* Return and remove the first response in the queue or NULL if none is ready*/
void* req_handler_remove_response(struct req_handler_ctx *ctx);

#endif /* CLOUD_HELPER_DELEGATE_UTILS_H */
