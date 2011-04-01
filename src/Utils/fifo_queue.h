#ifndef FIFO_QUEUE
#define FIFO_QUEUE


typedef struct fifo_queue *fifo_queue_p;

/* Creates a fifo queue with the specified initial size */
fifo_queue_p fifo_queue_create(int size);

/* Destroy the fifo queue using free_ptr to deallocate elements. If
   free_ptr is NULL the standard free will be used */
void fifo_queue_destroy(fifo_queue_p queue, void (*free_ptr)(void *ptr));

/* Return the current actual size of the queue (nr of elements contained) */
int fifo_queue_size(fifo_queue_p queue);

/* Add the specified element to the tail of the queue. Return 0 on
   succes, 1 on error */
int fifo_queue_add(fifo_queue_p queue, void *element);

/* Return the head of the queue or NULL on error */
void* fifo_queue_get_head(fifo_queue_p queue);

/* Return the element at position nr or NULL on error */
void* fifo_queue_get(fifo_queue_p queue, int nr);

/* Remove and return the head of the queue of NULL on error */
void* fifo_queue_remove_head(fifo_queue_p queue);
#endif
