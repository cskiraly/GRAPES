/*
 *  Copyright (c) 2010 Luca Abeni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "chunkids_private.h"
#include "chunkidset.h"
#include "trade_sig_la.h"

static inline void int_cpy(uint8_t *p, int v)
{
  int tmp;
  
  tmp = htonl(v);
  memcpy(p, &tmp, 4);
}

static inline int int_rcpy(const uint8_t *p)
{
  int tmp;
  
  memcpy(&tmp, p, 4);
  tmp = ntohl(tmp);

  return tmp;
}

int encodeChunkSignaling(const struct chunkID_set *h, const void *meta, int meta_len, uint8_t *buff, int buff_len)
{
    int i;
    uint32_t type_length;
    int_cpy(buff + 4, 0);
    int_cpy(buff + 8, meta_len);
    switch (h->type) {
        case CIST_BITMAP:
        {
            int elements;
            uint32_t c_min, c_max;
            c_min = c_max = h->elements[i++];
            for (; i < h->n_elements; i++) {
                if (h->elements[i] < c_min)
                    c_min = h->elements[i];
                else if (h->elements[i] > c_max)
                    c_max = h->elements[i];
            }
            elements = c_max - c_min + 1;
            type_length = elements | (h->type << ((sizeof (h->n_elements) - 1)*8));
            int_cpy(buff, type_length);
            elements = elements / 8 + (elements % 8 ? 1 : 0);
            if (buff_len < (elements * 4) + 16 + meta_len) {
                return -1;
            }
            int_cpy(buff + 12, c_min); //first value in the bitmap, i.e., base value
            for (i = 0; i < h->n_elements; i++) {
                buff[16 + (h->elements[i] - c_min) / 8] |= 1 << ((h->elements[i] - c_min) % 8);
            }
            if (meta_len) {
                memcpy(buff + 16 + elements * 4, meta, meta_len);
            }
            return 16 + elements * 4 + meta_len;
        }
        case CIST_PRIORITY:
        default:
        {
            type_length = h->n_elements | (h->type << ((sizeof (h->n_elements) - 1)*8));
            int_cpy(buff, type_length);
            if (buff_len < h->n_elements * 4 + 12 + meta_len) {
                return -1;
            }
            for (i = 0; i < h->n_elements; i++) {
                int_cpy(buff + 12 + i * 4, h->elements[i]);
            }
            if (meta_len) {
                memcpy(buff + h->n_elements * 4 + 12, meta, meta_len);
            }
            return h->n_elements * 4 + 12 + meta_len;
        }
    }
}

struct chunkID_set *decodeChunkSignaling(void **meta, int *meta_len, const uint8_t *buff, int buff_len)
{
    int i, val;
    uint32_t size;
    uint8_t type;
    struct chunkID_set *h;
    char cfg[32];
    size = int_rcpy(buff);
    type = size >> (sizeof (size) - 1)*8;
    size = (size << 8) >> 8;
    val = int_rcpy(buff + 4);
    *meta_len = int_rcpy(buff + 8);
    sprintf(cfg, "size=%d", size);
    sprintf(cfg, "%s,type=%d", cfg, type);
    h = chunkID_set_init(cfg);

    if (h == NULL) {
        fprintf(stderr, "Error in decoding chunkid set - not enough memory to create a chunkID set.\n");
        return NULL;
    }
    if (val) {
        return h; /* Not supported yet! */
    }
    switch (h->type) {
        case CIST_BITMAP:
        {
            // uint8_t bitmap;
            int base;
            int byte_cnt;
            byte_cnt = size / 8 + (size % 8 ? 1 : 0);
            if (buff_len < 16 + byte_cnt + *meta_len) {
                return NULL;
            }
            base = int_rcpy(buff + 12);
            for (i = 0; i < size; i++) {
                if (buff[16 + (i / 8)] & 1 << (i % 8))
                    h->elements[h->n_elements++] = base + i;
            }
            if (!realloc(h->elements, h->n_elements * sizeof (int))) {
                fprintf(stderr, "Error in decoding chunkid set while realloc.\n");
                return NULL;
            }
            h->size = h->n_elements;
            if (*meta_len) {
                *meta = malloc(*meta_len);
                if (*meta != NULL) {
                    memcpy(*meta, buff + 16 + byte_cnt, *meta_len);
                } else {
                    *meta_len = 0;
                }
            } else {
                *meta = NULL;
            }
            break;
        }
        case CIST_PRIORITY:
        default:
        {
            if (buff_len != size * 4 + 12 + *meta_len) {
                return NULL;
            }
            for (i = 0; i < size; i++) {
                h->elements[i] = int_rcpy(buff + 12 + i * 4);
            }
            h->n_elements = size;
            if (*meta_len) {
                *meta = malloc(*meta_len);
                if (*meta != NULL) {
                    memcpy(*meta, buff + 12 + size * 4, *meta_len);
                } else {
                    *meta_len = 0;
                }
            } else {
                *meta = NULL;
            }
            break;
        }
    }
    return h;
}
