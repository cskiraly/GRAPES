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

struct iw {
  int index;
  double weight;
};

static int cmp_iw_reverse(const void *a, const void *b)
{
  const struct iw *a1 = (const struct iw *) a;
  const struct iw *b1 = (const struct iw *) b;
  return a1->weight==b1->weight ? 0 : (a1->weight<b1->weight ? 1 : -1);
}

/**
  * Select best N of K based using a given evaluator function
  */
void selectBests(size_t size,unsigned char *base, size_t nmemb, double(*evaluate)(void *),unsigned char *bests,size_t *bests_len){
  struct iw iws[nmemb];
  int i;

  // calculate weights
  for (i=0; i<nmemb; i++){
     iws[i].index = i;
     iws[i].weight = evaluate(base + size*i);
  }

  // sort in descending order
  qsort(iws, nmemb, sizeof(struct iw), cmp_iw_reverse);

  // ensure uniform random tie
  for (i=0; i<nmemb; i++){
    int j, k;
    for (j=i; j<nmemb && iws[i].weight == iws[j].weight; j++);
    for (k=i; k<j; k++){
      struct iw iwt = iws[k];
      int r = i + (rand() % (j-i));
      iws[k] = iws[r];
      iws[r] = iwt;
    }
  }

  *bests_len = MIN(*bests_len, nmemb);

  // copy bests in their place
  for (i=0; i<*bests_len; i++){
     memcpy(bests + size*i, base + size*iws[i].index, size);
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
  int s_max = MIN (*selected_len, nmemb);
  int zeros = 0;

  // calculate weights
  for (i=0; i<nmemb; i++){
     weights[i] = weight(base + size*i);
     // weights should not be negative
     weights[i] = MAX (weights[i], 0);
     if (weights[i] == 0) zeros += 1;
     w_sum += weights[i];
  }

  // all weights shuold not be zero, but if if happens, do something
  if (w_sum == 0) {
    for (i=0; i<nmemb; i++){
      weights[i] = 1;
      w_sum += weights[i];
    }
  } else { //exclude 0 weight from selection
    s_max = MIN (s_max, nmemb - zeros);
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
void selectPeers(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, peerEvaluateFunction peerevaluate, schedPeerID *selected, size_t *selected_len ){
  selectWithOrdering(ordering, sizeof(peers[0]), (void*)peers, peers_len, (evaluateFunction)peerevaluate, (void*)selected, selected_len);
}

/**
  * Select best N of K chunks with the given ordering method
  */
void selectChunks(SchedOrdering ordering, schedChunkID *chunks, size_t chunks_len, chunkEvaluateFunction chunkevaluate, schedChunkID *selected, size_t *selected_len ){
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
void filterPeers2(schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len,
                     schedPeerID *filteredpeers, size_t *filtered_len,	//out, inout
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
void filterChunks2(schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len,
                     schedChunkID *filtered, size_t *filtered_len,	//out, inout
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
void selectPeersForChunks(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
                     schedPeerID *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction evaluate){

  size_t filtered_len=peers_len;
  schedPeerID filtered[filtered_len];

  filterPeers2(peers, peers_len, chunks,chunks_len, filtered, &filtered_len, filter);

  selectPeers(ordering, filtered, filtered_len, evaluate, selected, selected_len);
}

void selectChunksForPeers(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
                     schedChunkID *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     chunkEvaluateFunction evaluate){

  size_t filtered_len=chunks_len;
  schedChunkID filtered[filtered_len];

  filterChunks2(peers, peers_len, chunks,chunks_len, filtered, &filtered_len, filter);

  selectChunks(ordering, filtered, filtered_len, evaluate, selected, selected_len);
}


void toPairsPeerFirst(schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
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

void toPairsChunkFirst(schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
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

void toPairs(schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *pairs, size_t *pairs_len)	//out, inout
{
  toPairsChunkFirst(peers, peers_len, chunks, chunks_len, pairs, pairs_len);
}

/*----------------- scheduler_la implementations --------------*/
void schedSelectChunksForPeers(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
                     schedChunkID *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     chunkEvaluateFunction evaluate){
   selectChunksForPeers(ordering, peers, peers_len, chunks, chunks_len, selected, selected_len, filter, evaluate);
}

void schedSelectPeerFirst(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate){

  size_t p_len=1;
  schedPeerID p[p_len];
  size_t c_len=*selected_len;
  schedChunkID c[c_len];
  selectPeersForChunks(ordering, peers, peers_len, chunks, chunks_len, p, &p_len, filter, peerevaluate);
  selectChunksForPeers(ordering, p, p_len, chunks, chunks_len, c, &c_len, filter, chunkevaluate);

  toPairsPeerFirst(p,p_len,c,c_len,selected,selected_len);
}

void schedSelectChunkFirst(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate){

  size_t p_len=*selected_len;
  schedPeerID p[p_len];
  size_t c_len=1;
  schedChunkID c[c_len];
  selectChunksForPeers(ordering, peers, peers_len, chunks, chunks_len, c, &c_len, filter, chunkevaluate);
  selectPeersForChunks(ordering, peers, peers_len, c, c_len, p, &p_len, filter, peerevaluate);

  toPairsChunkFirst(p,p_len,c,c_len,selected,selected_len);
}

void schedSelectHybrid(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
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
void schedSelectComposed(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len, 	//in
                     struct PeerChunk *selected, size_t *selected_len,	//out, inout
                     filterFunction filter,
                     peerEvaluateFunction peerevaluate, chunkEvaluateFunction chunkevaluate, double2op weightcombine)
{
  peer_ev=peerevaluate;
  chunk_ev=chunkevaluate;
  peerchunk_wc=weightcombine;
  schedSelectHybrid(ordering,peers,peers_len,chunks,chunks_len,selected,selected_len,filter,combinedWeight);

}

void schedSelectPeersForChunks(SchedOrdering ordering, schedPeerID *peers, size_t peers_len, schedChunkID *chunks, size_t chunks_len,        //in
                     schedPeerID *selected, size_t *selected_len,       //out, inout
                     filterFunction filter,
                     peerEvaluateFunction evaluate){
   selectPeersForChunks(ordering, peers, peers_len, chunks, chunks_len,        //in
                     selected, selected_len,       //out, inout
                     filter,
                      evaluate);
}




