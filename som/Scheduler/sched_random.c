#include "scheduler_ha.h"
#include <stdlib.h>
#include <string.h>


#define MAX(A,B)    ((A)>(B) ? (A) : (B))
#define MIN(A,B)    ((A)<(B) ? (A) : (B))

/*
 * A simple RBp/RBc scheduler
*/
void schedSelectRandom(struct peer **peers, int peers_len, struct chunk **chunks, int chunks_len, 	//in
                     struct PeerChunk *selected, int *selected_len)	//out, inout
{

  int s;

  //compose list of selected pairs
  *selected_len = MIN(*selected_len, peers_len * chunks_len);
  s=0;
  while (s != *selected_len)  {
    int i;
    int already_selected = 0;

    selected[s].peer = peers[(int)(peers_len * (rand() / (RAND_MAX + 1.0)))];
    selected[s].chunk = chunks[(int)(chunks_len * (rand() / (RAND_MAX + 1.0)))];

    //don't be so blind to include the same pair twice
    for (i=0; i<s; i++){
      if (selected[i].peer == selected[s].peer && selected[i].chunk == selected[s].chunk) {
        already_selected = 1;
        break;
      }
    }
    if (already_selected) continue;

    s++;
  }
}
