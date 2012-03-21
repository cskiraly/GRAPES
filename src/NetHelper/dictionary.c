/*
 *  Copyright (c) 2012 Arber Fama,
 *                     Giovanni Simoni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <dacav/dacav.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "dictionary.h"

/* -- Constants ------------------------------------------------------ */

static const char * CONF_KEY_NBUCKETS = "tcp_hashbuckets";

// Arbitrary and reasonable(?) prime number.
static const unsigned DEFAULT_N_BUCKETS = 23;

/* -- Internal functions --------------------------------------------- */

static int invalid_fd (int fd);

/* -- Interface exported symbols ------------------------------------- */

struct dict {
    dhash_t *ht;
    size_t count;
};

static
uintptr_t sockaddr_hash (const void *key)
{
    const struct sockaddr_in *K = key;
    return K->sin_addr.s_addr + K->sin_port;
}

static
int sockaddr_cmp (const void *v0, const void *v1)
{
    return memcmp(v0, v1, sizeof(struct sockaddr_in));
}

static
void * create_copy (const void *src, size_t size)
{
    void * dst = malloc(size);
    assert(dst != NULL);
    memcpy(dst, src, size);
    return dst;
}

static
void * sockaddr_copy (const void *s)
{
    return create_copy(s, sizeof(struct sockaddr_in));
}

static
void * peer_info_copy (const void *s)
{
    return create_copy(s, sizeof(peer_info_t));
}

static
void peer_info_free (void *s)
{
    close(((peer_info_t *)s)->fd);
    free(s);
}

dict_t dict_new (int af, int autoclose, struct tag *cfg_tags)
{
    int nbks;
    dict_t ret;
    dhash_cprm_t addr_cprm, peer_info_cprm;

    if (af != AF_INET) {
        fprintf(stderr, "dictionary: Only AF_INET is supported for the"
                        " moment\n");
        abort();
    }

    nbks = DEFAULT_N_BUCKETS;
    if (cfg_tags != NULL) {
        config_value_int_default(cfg_tags, CONF_KEY_NBUCKETS, &nbks,
                                 DEFAULT_N_BUCKETS);
    }

    ret = malloc(sizeof(struct dict));
    assert(ret != NULL);

    addr_cprm.cp = sockaddr_copy;
    addr_cprm.rm = free;
    peer_info_cprm.cp = peer_info_copy;
    peer_info_cprm.rm = autoclose ? peer_info_free : free;
    ret->ht = dhash_new(nbks, sockaddr_hash, sockaddr_cmp,
                        &addr_cprm, &peer_info_cprm);
    ret->count = 0;
    return ret;
}

void dict_delete (dict_t D)
{
    dhash_free(D->ht);
    free(D);
}

size_t dict_size (dict_t D)
{
    return D->count;
}

int dict_lookup (dict_t D, const struct sockaddr *addr,
                 peer_info_t **info)
{
    if (dhash_search(D->ht, (const void *)addr, (void **)info)
            == DHASH_FOUND) {
        return invalid_fd((*info)->fd) ? -1 : 0;
    } else {
        return -1;
    }
}

struct lookup_data {
    int (* make_socket) (void *ctx);
    void *ctx;
};

static
void * build_peer_info (void * param)
{
    struct lookup_data *ld = param;
    peer_info_t * info = malloc(sizeof(peer_info_t));

    assert(info != NULL);
    info->flags.used = 0;
    info->fd = ld->make_socket(ld->ctx);

    return (void *)info;
}

void dict_lookup_default (dict_t D, const struct sockaddr *addr,
                          peer_info_t *info,
                          int (* make_socket) (void *ctx), void *ctx)
{
    /* NOTE: currently this code is NOT used. If enabling this feature,
     * please double-check the code correctness:
     *
     * - is file descriptor valid?
     * - compare with dict_lookup
     */
    struct lookup_data ld = {
        .make_socket = make_socket,
        .ctx = ctx
    };
    dhash_search_default(D->ht, (const void *)addr, (void **)&info,
                         build_peer_info, (void *) &ld);
}

int dict_insert (dict_t D, const struct sockaddr * addr, int fd)
{
    peer_info_t info;
    info.fd = fd;
    info.flags.used = 0;

    if (dhash_insert(D->ht, (const void *)addr, (void *)&info)
            == DHASH_FOUND) {
        return 1;
    } else {
        D->count ++;
        return 0;
    }
}

int dict_remove (dict_t D, const struct sockaddr * addr)
{
    if (dhash_delete(D->ht, (const void *)addr, NULL)
            == DHASH_FOUND) {
        D->count --;
        return 0;
    } else {
        return -1;
    }
}

void dict_scan (dict_t D, dict_scancb_t cback, void *ctx)
{
    diter_t *it = dhash_iter_new(D->ht);
    int go = 1;
    while (go && diter_hasnext(it)) {
        dhash_pair_t *P;
        peer_info_t *info;

        P = diter_next(it);
        info = dhash_val(P);

        if (invalid_fd(info->fd)) {
            diter_remove(it, NULL);
            D->count --;
        } else {
            switch (cback(ctx, dhash_key(P), info)) {
                case DICT_SCAN_DEL_CONTINUE:
                    D->count --;
                    diter_remove(it, NULL);
                case DICT_SCAN_CONTINUE:
                    break;
                case DICT_SCAN_DEL_STOP:
                    D->count --;
                    diter_remove(it, NULL);
                case DICT_SCAN_STOP:
                    go = 0;
                    break;
            }
        }
    }
    dhash_iter_free(it);
}

/* -- Internal functions --------------------------------------------- */

static
int invalid_fd (int fd)
{
    return fcntl(fd, F_GETFL) == -1;
}
