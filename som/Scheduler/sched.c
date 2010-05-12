/*
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <string.h>
#include <stdlib.h>
#include "scheduler_la.h"

#define MAX(A,B)    ((A)>(B) ? (A) : (B))
#define MIN(A,B)    ((A)<(B) ? (A) : (B))

/**
  * casted evaluator for generic use in generic selector functions
  */
typedef double (*evaluateFunction)(void*);


/**
  * qsort like sorting function to select the index of the N best, based on a given evaluator function
  * @TODO: randomize if tie
  */
void selectBestIndexes(double *weights,size_t weights_len,int *bests,size_t *bests_len){
  int e;
  int s=0;
  int s_max=*bests_len;

  for (e=0; e<weights_len; e++){
    //find insert position
    int i;
    int p_min = 0;
    int p;
    for (i=0; i < s; i++) {
      if (weights[e] < weights[bests[i]]) p_min = i+1;
      if (weights[e] > weights[bests[i]]) break;
    }

    //randomize position between equals
    p = p_min + (i-p_min) ? (rand() % (i-p_min)) : 0 ;

    if (p<s_max) {
      memmove(&bests[p+1], &bests[p], sizeof(int)*(s_max-p-1)); //shift later ones
      s = MIN(s+1,s_max);
      //put new one in the list
      bests[p]=e;
    }
  }

  *bests_len = s;
}

/**
  * Select best N of K based using a given evaluator function
  */
void selectBests(size_t size,unsigned char *base, size_t nmemb, double(*evaluate)(void *),unsigned char *bests,size_t *bests_len){
  double weights[nmemb];
  int selected_index[*bests_len];
  int i;
  // calculate weights
  for (i=0; i<nmemb; i++){
     weights[i] = evaluate(base + size*i);
  }
  selectBestIndexes(weights,nmemb,selected_index,bests_len);
  
  // copy bests in their place
  for (i=0; i<*bests_len; i++){
     memcpy(bests + size*i, base + size*selected_index[i], size);
  }
  
}

/**
  * Select N of K with weigthed random choice, without replacement (multiple selection), based on a given evaluator function
  */
void selectWeighted(size_t size,unsigned char *base, size_t nmemb, double(*weight)(void *),unsigned char *selected,size_t *selected_len){
  int i,j,k;
  double weights[nmemb];
  double w_sum=0;
  double selected_index[nmemb];
  int s=0;
  int s_max = MIN (*selected_len, nmemb);;

  // calculate weights
  for (i=0; i<nmemb; i++){
     weights[i] = weight(base + size*i);
     // weights should not be negative
     weights[i] = MAX (weights[i], 0);
     w_sum += weights[i];
  }

  // all weights shuold not be zero, but if if happens, do something
  if (w_sum == 0) {
    for (i=0; i<nmemb; i++){
      weights[i] = 1;
      w_sum += weights[i];
    }
  }

  while (s < s_max) {
    // select one randomly
    double t = w_sum * (rand() / (RAND_MAX + 1.0));
    //search for it in the CDF
    double cdf = 0;
    for (j=0; j<nmemb; j++){
      cdf += weights[j];
      //printf("(%f <? %f\n",t,cdf);
      if (t < cdf) {
        int already_selected=0;
        for (k=0; k<s; k++) {
          if (selected_index[k] == j)  already_selected=1;
        }
        if (!already_selected){ 
          //printf("selected %d(%f) for %d\n",j,weights[j],k);
          memcpy(selected + size*s, base + size*j, size);
          selected_index[s++]=j;
        }
        break;
      }
    }
  }
  *selected_len=s;
}

/**
  * Select best N of K with the given ordering method
  */
void selectWithOrdering(SchedOrdering ordering, size_t size, unsigned char *base, size_t nmemb, double(*evaluate)(void *), unsigned char *selected,size_t *selected_len){
  if (ordering == SCHED_WEIGHTED) selectWeighted(size, base, nmemb, evaluate, selected, selected_len);
  else selectBests(size, base, nmemb, evaluate, selected, selected_len);
}

/**
  * Select best N of K peers with the given ordering method
  */
void selectPeers(SchedOrdering ordering, struct peer **peers, size_t peers_len, peerEvaluateFunction peerevaluate, struct peer **selected, size_t *selected_len ){
  selectWithOrdering(ordering, sizeof(peers[0]), (void*)peers, peers_len, (evaluateFunction)peerevaluate, (void*)selected, selected_len);
}

/**
  * Select best N of K chunks with the given ordering method
  */
void selectChunks(SchedOrdering ordering, struct chunk **chunks, size_t chunks_len, chunkEvaluateFunction chunkevaluate, struct chunk **selected, size_t *selected_len ){
  selectWithOrdering(ordering, sizeof(chunks[0]), (void*)chunks,chunks_len, (evaluateFunction)chunkevaluate, (void*)selected, selected_len);
}

/**
  * Select best N of K peer-chunk pairs with the given ordering method
  */
void selectPairs(SchedOrdering ordering, struct PeerChunk *pairs, size_t pairs_len, pairEvaluateFunction evaluate, struct PeerChunk *selected, size_t *selected_len ){
  selectWithOrdering(ordering, sizeof(pairs[0]), (void*)pairs, pairs_len, (evaluateFunction)evaluate, (void*)selected, selected_len);
}


/**
  * Filter a list of peers. Include a peer if the filter function is true with at least one of the given chunks
  */
void filterPeers2(struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len,
                     struct peer **filteredpeers, size_t *filtered_len,	//out, inout
                     filterFunction filter){
  int p,c;
  int f=0;
  for (p=0; p<peers_len; p++){
    for (c=0; c<chunks_len; c++){
      if (!filter || filter(peers[p],chunks[c])) {
        filteredpeers[f++]=peers[p];
        break;
      }
    }
    if (*filtered_len==f) break;
  }
  *filtered_len=f;
}

/**
  * Filter a list of chunks. Include a chunk if the filter function is true with at least one of the given peers
  */
void filterChunks2(struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len,
                     struct chunk **filtered, size_t *filtered_len,	//out, inout
                     filterFunction filter){
  int p,c;
  int f=0;
  for (c=0; c<chunks_len; c++){
    for (p=0; p<peers_len; p++){
      if (!filter || filter(peers[p],chunks[c])) {
        filtered[f++]=chunks[c];
        break;
      }
    }
    if (*filtered_len==f) break;
  }
  *filtered_len=f;
}

/**
  * Filter a list of peer-chunk pairs (in place)
  */
void filterPairs(struct PeerChunk *pairs, size_t *pairs_len,
                     filterFunction filter){
  int pc;
  int f=0;
  for (pc=0; pc<(*pairs_len); pc++){
    if (!filter || filter(pairs[pc].peer,pairs[pc].chunk)) {
      pairs[f++]=pairs[pc];
    }
  }
  *pairs_len=f;
}

/**
  * Select at most N of K peers, among those where filter is true with at least one of the chunks.
  */
void selectPeersForChunks(SchedOrdering ordering, struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct peer **selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction evaluate){

  size_t filtered_len=peers_len;
  struct peer *filtered[filtered_len];

  filterPeers2(peers, peers_len, chunks,chunks_len, filtered, &filtered_len, filter);

  selectPeers(ordering, filtered, filtered_len, evaluate, selected, selected_len);
}

void selectChunksForPeers(SchedOrdering ordering, struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct chunk **selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     chunkEvaluateFunction evaluate){

  size_t filtered_len=chunks_len;
  struct chunk *filtered[filtered_len];

  filterChunks2(peers, peers_len, chunks,chunks_len, filtered, &filtered_len, filter);

  selectChunks(ordering, filtered, filtered_len, evaluate, selected, selected_len);
}


void toPairsPeerFirst(struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct PeerChunk *pairs, size_t *pairs_len) {	//out, inout
  size_t p,c;
  size_t i=0;
  for (p=0; p<peers_len; p++){
    for (c=0; c<chunks_len; c++){
      pairs[i  ].peer  = peers[p];
      pairs[i++].chunk = chunks[c];
    }
    if (*pairs_len==i) break;
  }
  *pairs_len=i;
}

void toPairsChunkFirst(struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct PeerChunk *pairs, size_t *pairs_len) {	//out, inout
  size_t p,c;
  size_t i=0;
  for (c=0; c<chunks_len; c++){
    for (p=0; p<peers_len; p++){
      pairs[i  ].peer  = peers[p];
      pairs[i++].chunk = chunks[c];
    }
    if (*pairs_len==i) break;
  }
  *pairs_len=i;
}

void toPairs(struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct PeerChunk *pairs, size_t *pairs_len)	//out, inout
{
  toPairsChunkFirst(peers, peers_len, chunks, chunks_len, pairs, pairs_len);
}

/*----------------- scheduler_la implementations --------------*/

void schedSelectPeerFirst(SchedOrdering ordering, struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate){

  size_t p_len=1;
  struct peer *p[p_len];
  size_t c_len=*selected_len;
  struct chunk *c[c_len];
  selectPeersForChunks(ordering, peers, peers_len, chunks, chunks_len, p, &p_len, filter, peerevaluate);
  selectChunksForPeers(ordering, p, p_len, chunks, chunks_len, c, &c_len, filter, chunkevaluate);

  toPairsPeerFirst(p,p_len,c,c_len,selected,selected_len);
}

void schedSelectChunkFirst(SchedOrdering ordering, struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate){

  size_t p_len=*selected_len;
  struct peer *p[p_len];
  size_t c_len=1;
  struct chunk *c[c_len];
  selectChunksForPeers(ordering, peers, peers_len, chunks, chunks_len, c, &c_len, filter, chunkevaluate);
  selectPeersForChunks(ordering, peers, peers_len, c, c_len, p, &p_len, filter, peerevaluate);

  toPairsChunkFirst(p,p_len,c,c_len,selected,selected_len);
}

void schedSelectHybrid(SchedOrdering ordering, struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     pairEvaluateFunction pairevaluate)
{
  size_t pairs_len=peers_len*chunks_len;
  struct PeerChunk pairs[pairs_len];
  toPairs(peers,peers_len,chunks,chunks_len,pairs,&pairs_len);
  filterPairs(pairs,&pairs_len,filter);
  selectPairs(ordering,pairs,pairs_len,pairevaluate,selected,selected_len);
}


peerEvaluateFunction peer_ev;
chunkEvaluateFunction chunk_ev;
double2op peerchunk_wc;

double combinedWeight(struct PeerChunk* pc){
  return peerchunk_wc(peer_ev(&pc->peer),chunk_ev(&pc->chunk));
}

/**
  * Convenience function for combining peer and chunk weights
  * not thread safe!
  */
void schedSelectComposed(SchedOrdering ordering, struct peer **peers, size_t peers_len, struct chunk **chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate, double2op weightcombine)
{
  peer_ev=peerevaluate;
  chunk_ev=chunkevaluate;
  peerchunk_wc=weightcombine;
  schedSelectHybrid(ordering,peers,peers_len,chunks,chunks_len,selected,selected_len,filter,combinedWeight);

}
