/*
 *  Copyright (c) 2012 Arber Fama,
 *                     Giovanni Simoni
 *
 *  This is free software; see lgpl-2.1.txt
 */

/*

   This module provides an independent interface for a generic dictionary.

   The dictionary maps (pointers to) objects of type `struct sockaddr`
   into integers (file descriptors).

   Behind the interface it may be implemented with either a local
   algorithm or an external library (the point here is avoiding
   dependencies).

 */

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stdint.h>
#include <stddef.h>
#include <arpa/inet.h>

#include "config.h"

typedef struct dict * dict_t;

typedef struct {
    int fd;
    struct {
        unsigned used : 1;
    } flags;
} peer_info_t;

/** Constructor for the dictionary.
 *
 * @param[in] af Either AF_INET or AF_INET6 (the latter is not supported,
 *               it's there just for future enhancements);
 * @param[in] autoclose If true automatically close internal file
 *                      descriptors on deletion.
 * @param[in] conf A string configuration which may provides
 *                 implementation-dependent parameter
 *
 * @warning The only supported value for the af parameter is AF_INET, for
 *          the moment.
 *
 * @warning The conf parameter depends on the implementation.
 *
 * @return The newly allocated dictionary.
 */
dict_t dict_new (int af, int autoclose, struct tag *cfg);

/** Destructor for the dictionary.
 *
 * @param[in] D The dictionary to be destroyed.
 */
void dict_delete (dict_t D);

/** Number of stored items
 *
 * @param[in] D The dictionary.
 *
 * @return the number of stored items.
 */
size_t dict_size (dict_t D);

/** Lookup function.
 *
 * @param[in] D The dictionary;
 * @param[in] addr The address to search;
 * @param[out] fd The file descriptor (valid only if 0 is returned);
 *
 * @retval 0 on success;
 * @retval -1 on failure.
 */
int dict_lookup (dict_t D, const struct sockaddr *addr,
                 peer_info_t **info);

/** Lookup function.
 *
 * @param[in] D The dictionary;
 * @param[in] addr The address to search;
 * @param[out] fd The file descriptor (valid only if 0 is returned);
 * @param[in] make_socket The callback to be internally called if no file
 *                        descriptor has been found;
 * @param[in] ctx The context for the callback.
 *
 */
void dict_lookup_default (dict_t D, const struct sockaddr *addr,
                          peer_info_t *info,
                          int (* make_socket) (void *ctx), void *ctx);

/** Insertion function.
 *
 * Items gets replaced when you overwrite them.
 *
 * @param[in] D The dictionary;
 * @param[in] addr The address to map;
 * @param[in] fd The file descriptor to be mapped on the address.
 *
 * @retval 0 on insertion;
 * @retval 1 on overwrite of an old value.
 */
int dict_insert (dict_t D, const struct sockaddr * addr, int fd);

/** Remotion function.
 *
 * Removes the required address from the dictionary, also closing the
 * corresponding connection.
 *
 * @param[in] D The dictionary;
 * @param[in] addr The address to be removed.
 *
 * @retval 0 on success;
 * @retval -1 on failure (no such item).
 */
int dict_remove (dict_t D, const struct sockaddr * addr);

typedef enum {
    DICT_SCAN_STOP,
    DICT_SCAN_CONTINUE,
    DICT_SCAN_DEL_STOP,
    DICT_SCAN_DEL_CONTINUE
} dict_scanact_t;

/** Callback for dictionary looping.
 *
 * Provide a function complying with this type in order to scan a
 * dictionary.
 *
 * @warning If that's not intuitively clear, try not to modify the
 *          dictionary while you are looping on it, please...
 *
 * @param[in] ctx The user context argument;
 * @param[in] addr The address;
 * @param[in] info A (modificable) internal instance of peer_info_t.
 *
 * @retval dict_scanact_t::DICT_SCAN_STOP to stop the iteration;
 * @retval dict_scanact_t::DICT_SCAN_CONTINUE to continue the iteration;
 * @retval dict_scanact_t::DICT_SCAN_DEL_STOP to delete the read value and
 *         stop the iteration;
 * @retval dict_scanact_t::DICT_SCAN_DEL_CONTINUE to delete the read and
 *         continue the iteration.
 *
 * @see dict_scan
 */
typedef dict_scanact_t (* dict_scancb_t) (void *ctx,
                                          const struct sockaddr *addr,
                                          peer_info_t *info);

/** Implementation-agnostic scanning procedure for the dictionary.
 *
 * This function can be used to scan on a dictionary. The dictionary can
 * so far be either scanned or modified pair by pair.
 *
 * @note The ctx parameter is provided in order to bring some user context
 *       inside the callback function. Please use it and avoid global
 *       variables, which are EVIL.
 *
 * @warning If that's not intuitively clear, try not to modify the
 *          dictionary while you are looping on it, please...
 *
 * @param[in,out] D The dictionary to loop on;
 * @param[in] cback The callback;
 * @param[in] ctx The user context;
 */
void dict_scan (dict_t D, dict_scancb_t cback, void *ctx);

#endif // DICTIONARY_H

