#include <stdlib.h>
#include <stdio.h>

#include "fifo_queue.h"

typedef struct fifo_queue {
  void **store;

  int max_size;
  int current_size;
  int head_ptr;
  int tail_ptr;
} fifo_queue_t;

fifo_queue_p fifo_queue_create(int size)
{
  fifo_queue_p queue;

  if (size <= 0) return NULL;

  queue = malloc(sizeof(fifo_queue_t));
  if (!queue) return NULL;

  queue->store = calloc(size, sizeof(void *));
  if (!queue->store) {
    free(queue);
    return NULL;
  }

  queue->max_size = size;
  queue->current_size = 0;
  queue->head_ptr = 0;
  queue->tail_ptr = 0;
  return queue;
}

void fifo_queue_destroy(fifo_queue_p queue, void (*free_ptr)(void *ptr))
{
  int i, pos;

  if (free_ptr == NULL)
    free_ptr = &free;

  /* deallocating elements */
  if (queue->current_size > 0) {
    for (i=0; i<queue->current_size; i++) {
      pos = (queue->head_ptr + i > queue->max_size - 1) ?
        (queue->head_ptr + i) - (queue->max_size) : queue->head_ptr + i;

      free_ptr(queue->store[pos]);
    }
  }

  free(queue->store);
  free(queue);
}

int fifo_queue_size(fifo_queue_p queue)
{
  return queue->current_size;
}

int fifo_queue_add(fifo_queue_p queue, void *el)
{
  if (!queue) return 1;
  if (!el) return 1;

  /* expand the array if needed */
  if (queue->current_size + 1 > queue->max_size) {
    void **new_store;
    int new_size;
    int i, j;

    new_size = queue->max_size * 2;

    new_store =  calloc(new_size, sizeof(void *));

    if (!new_store) return 2;

    for (i=0; i<queue->current_size; i++) {
      j = (queue->head_ptr + i > queue->max_size - 1) ?
        (queue->head_ptr + i) - (queue->max_size) : queue->head_ptr + i;

      new_store[i] = queue->store[j];
    }

    free(queue->store);
    queue->store = new_store;

    queue->max_size = new_size;
    queue->head_ptr = 0;
    queue->tail_ptr = queue->current_size;
  }

  /* Add the element to the queue */
  if (queue->tail_ptr == queue->max_size)
    queue->tail_ptr = 0;

  queue->store[queue->tail_ptr] = el;
  queue->tail_ptr++;
  queue->current_size++;

  /*
    fprintf(stderr,
          "fifo_queue: Added element at pos %d " \
          "(head=%d, tail=%d, curr_size=%d, max_size=%d)\n",
          queue->tail_ptr - 1,
          queue->head_ptr, queue->tail_ptr,
          queue->current_size, queue->max_size);
  */

  return 0;
}

void* fifo_queue_get_head(fifo_queue_p queue)
{
  if (!queue) return NULL;
  if (queue->current_size == 0) return NULL;
  return queue->store[queue->head_ptr];
}

void* fifo_queue_get(fifo_queue_p queue, int nr)
{
  int pos;
  if (!queue) return NULL;
  if (queue->current_size < nr) return NULL;

  pos = (queue->head_ptr + nr > queue->max_size - 1) ?
    (queue->head_ptr + nr) - (queue->max_size) : queue->head_ptr + nr;


  return queue->store[pos];
}

void* fifo_queue_remove_head(fifo_queue_p queue)
{
  void *el;
  if (!queue) return NULL;
  if (queue->current_size == 0) return NULL;

  el = queue->store[queue->head_ptr];

  queue->current_size--;
  queue->head_ptr++;

  if (queue->head_ptr == queue->max_size){
    queue->head_ptr = 0;
  }

  if (queue->current_size == 0) {
    queue->head_ptr = 0;
    queue->tail_ptr = 0;
  }

  /*
  fprintf(stderr,
          "fifo_queue: Removed element at pos %d " \
          "(head=%d, tail=%d, curr_size=%d, max_size=%d)\n",
          queue->head_ptr - 1,
          queue->head_ptr, queue->tail_ptr,
          queue->current_size, queue->max_size);
  */
  return el;
}
