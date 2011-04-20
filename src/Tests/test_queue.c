#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "fifo_queue.h"

int free_counter = 0;

void myfree(void *ptr) {
  free_counter++;
  free(ptr);
}

void check_queue(fifo_queue_p q, int n)
{
  int i;
  int *el;

  for (i=0; i<fifo_queue_size(q); i++) {
    el = fifo_queue_get(q, i);
    assert(*el == n);
    n++;
  }
}

int main(int argc, char* argv[])
{
  fifo_queue_p q;
  int i, count, tot;
  int *el;

  q = fifo_queue_create(1);

  assert(fifo_queue_remove_head(q) == NULL);

  for (i=0; i<20; i++) {
    int err;
    el = malloc(sizeof(int));
    *el = i;
    err = fifo_queue_add(q, el);
    assert(err==0);
    check_queue(q,0);
  }

  for (i=0; i<20; i++) {
    int *el;
    el = fifo_queue_remove_head(q);
    check_queue(q, i+1);
    assert(*el == i);
    free(el);
  }

  assert(fifo_queue_remove_head(q) == NULL);

  count = -1;
  tot = -1;
  for (i=0; i<20; i++) {
    int j, err;
    for (j=0; j<100*(i+1); j++) {
      el = malloc(sizeof (int));
      *el = ++tot;
      err = fifo_queue_add(q, el);
      check_queue(q, count+1);
      assert(err==0);
    }

    for (j=0; j<50*(i+1); j++) {
      el = fifo_queue_remove_head(q);
      check_queue(q, count+2);
      assert(*el == ++count);
      free(el);
    }
  }


  count = fifo_queue_size(q);
  fifo_queue_destroy(q, &myfree);

  assert(count == free_counter);
  printf("All test are ok\n");
  return 0;
}
