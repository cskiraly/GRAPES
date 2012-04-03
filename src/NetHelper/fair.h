/*
 *  Copyright (c) 2012 Arber Fama,
 *                     Giovanni Simoni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#ifndef FAIR_H
#define FAIR_H

#include "dictionary.h"
#include "nh-types.h"

#include <sys/time.h>
#include <sys/select.h>

/** Fair select of the receiving peer.
 *
 * This function is a wrapper on the traditional select(2) system call.
 * Internally the `neighbours` dictionary will be scanned and its file
 * descriptors will be added to the `fdset` file-descriptors set. Then a
 * select(2) will be achieved and the resulting set will be scanned.
 *
 * The scan is performed in a fair order with respect to the neighbours,
 * and eventually the `conn` parameter will be setted if some neighbour
 * is willing to send some data.
 *
 * @param[in] neighbours The neighbours dictionary;
 * @param[in] timeout The timeout (or NULL for indefinte waiting);
 * @param[in] fdset A pre-initialized (and possibly non-empty) set of file
 *            descriptors to be analyzed;
 * @param[in] nfds The maximum file descriptor in the set + 1 (or 0 if
 *                 fdset is empty);
 * @param[out] conn The found connection (may be invalid: conn->fd == -1);
 * @param[out] e Pointer where errno will be stored in case of select(2)
 *               failure (you may pass NULL).
 *
 * @retval 1 if either there's a node ready for reading (the `conn`
 *           parameter will contain references to the neighbour) or an
 *           external file descriptor is ready;
 * @retval 0 On timeout (the `conn` output parameter must be considered
 *           invalid);
 * @retval -1 On select error (the `conn` output parameter must be
 *            considered invalid).
 *
 */
int fair_select(dict_t neighbours, struct timeval *timeout,
                fd_set *fdset, int nfds, connection_t *conn, int *e);

#endif // FAIR_H

