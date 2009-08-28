#include <stdint.h>
#include <stdio.h>

#include "net_helper.h"
#include "topmanager.h"

#define MAX_PEERS 5000
static struct socketID *table[MAX_PEERS];

int topInit(struct socketID *myID)
{
  FILE *f;
  int i = 0;

  f = fopen("peers.txt", "r");
  while(!feof(f)) {
    int res;
    char addr[32];
    int port;

    res = fscanf(f, "%s %d\n", addr, &port);
    if ((res == 2) && (i < MAX_PEERS - 1)) {
fprintf(stderr, "Creating table[%d]\n", i);
      table[i++] = create_socket(addr, port);
    }
  }
  table[i] = NULL;

  return i;
}

int topAddNeighbour(struct socketID *neighbour)
{
  int i;

  for (i = 0; table[i] != NULL; i++);
  table[i++] = neighbour;
  table[i] = NULL;

  return i;
}

int topParseData(const struct connectionID *conn, const uint8_t *buff, int len)
{
  /* FAKE! */
  return 0;
}

const struct socketID **topGetNeighbourhood(int *n)
{
  for (*n = 0; table[*n] != NULL; (*n)++) {
fprintf(stderr, "Checking table[%d]\n", *n);
  }
  return (const struct socketID **)table;
}
