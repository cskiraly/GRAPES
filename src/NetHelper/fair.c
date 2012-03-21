/*
 *  Copyright (c) 2012 Arber Fama,
 *                     Giovanni Simoni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include "fair.h"

#include <errno.h>

struct fill_fd_set_data {
    fd_set *readfds;
    int maxfd;
    size_t count_unused;
};

static
dict_scanact_t scan_fill_fd_set (void *ctx, const struct sockaddr *addr,
                                 peer_info_t *info)
{
    /* Used initially to scan all the file descriptors and put them
     * into the file descriptors set. Also collecting the maximum of them
     * (used by select(2)) and count how much file descriptors have never
     * been used */
    struct fill_fd_set_data *ffsd = ctx;

    FD_SET(info->fd, ffsd->readfds);
    if (info->fd > ffsd->maxfd) {
        ffsd->maxfd = info->fd;
    }
    if (!info->flags.used) {
        ffsd->count_unused ++;
    }
    return DICT_SCAN_CONTINUE;
}

struct pick_fair_data {
    fd_set *readfds;
    connection_t *result;
    struct {
        unsigned flip : 1;      // Search logic flipped (i.e. all used once)
        unsigned backup : 1;    // Some already used node can be read
    } flags;
};

static
dict_scanact_t scan_pick_fair (void *ctx, const struct sockaddr *addr,
                               peer_info_t *info)
{
    struct pick_fair_data *pfd = ctx;
    connection_t *result = pfd->result;

    if (pfd->flags.flip) {

        /* In flipped logic we select the very first file descriptor (any
         * would do fine), then we continue the iteration setting
         * file descriptors to unused.
         *
         * Note: since here we flipped (i.e. "all used" become "all
         * unused", by construction there must be a node willing to
         * transmit (otherwise we would be still locked on select(2)).
         */
        if (result->fd == -1 && FD_ISSET(info->fd, pfd->readfds)) {
            /* First one. We'll use this, so we won't set it unused. */
            result->fd = info->fd;
            result->addr = addr;
        } else {
            info->flags.used = 0;
        }
        return DICT_SCAN_CONTINUE;

    } else {

        /* In non-flipped logic we are searching for a non-used node. If
         * we find it we stop and we are ok. However we may not find it,
         * so we also search in the used ones as backup plan */

        if (info->flags.used) {

            if (!pfd->flags.backup && FD_ISSET(info->fd, pfd->readfds)) {
                /* We have our backup plan */
                pfd->flags.backup = 1;
                result->fd = info->fd;
                result->addr = addr;
            }

        } else {

            if (FD_ISSET(info->fd, pfd->readfds)) {
                /* We found our never-used peer */
                result->fd = info->fd;
                result->addr = addr;
                return DICT_SCAN_STOP;
            }

        }

        return DICT_SCAN_CONTINUE;

    }
}

int fair_select(dict_t neighbours, struct timeval *timeout,
                fd_set *fdset, int nfds, connection_t *conn, int *e)
{
    int ret;
    struct pick_fair_data pfd;
    struct fill_fd_set_data ffsd;

    if (dict_size(neighbours) == 0) {
        /* If we have no neighbors, just behave as a normal select.
         *
         * Note: the dictionary may contain invalid file descriptors,
         * which are transparently removed during the scanning phase.
         *
         * If however the size is 0 we are sure that there's nothing in,
         * so this optimization is perfectly safe.
         */
        return select(nfds, fdset, NULL, NULL, timeout);
    }

    ffsd.readfds = fdset;
    ffsd.maxfd = nfds - 1;
    ffsd.count_unused = 0;

    dict_scan(neighbours, scan_fill_fd_set, (void *) &ffsd);
    ret = select(ffsd.maxfd + 1, fdset, NULL, NULL, timeout);
    if (ret <= 0) {
        /* On timeout and error */
        if (e) *e = errno;
        return ret;
    }

    pfd.readfds = fdset,
    pfd.flags.flip = !ffsd.count_unused;
    pfd.flags.backup = 0;
    pfd.result = conn;
    conn->fd = -1;

    dict_scan(neighbours, scan_pick_fair, (void *) &pfd);

    return 1;
}
