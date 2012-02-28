/*
 *  Copyright (c) 2012 Arber Fama,
 *                     Giovanni Simoni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#ifndef NET_HELPER_ALL_H
#define NET_HELPER_ALL_H

#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* The idea:
 *
 * TCP and UDP may share some basic algorithms (like comparison of
 * internet addresses, which are equal for both the implementations).
 *
 * As I'm only working on the TCP implementation, I'm not touching the
 * (already present) UDP-based system, but subsequenf modification may
 * adapt it.
 *
 */

typedef struct {
    struct sockaddr_in addr;
    int fd;
} sock_data_t;

static inline
int sock_data_cmp (const sock_data_t *s1, const sock_data_t *s2)
{
    return memcmp(&s1->addr, &s2->addr, sizeof(struct sockaddr_in));
}

static inline 
int sock_data_equal (const sock_data_t *s1, const sock_data_t *s2)
{
    return sock_data_cmp(s1, s2) == 0;
}

static inline
const char * sock_data_ipstring (const sock_data_t *s, char * strrep)
{
    return inet_ntop(AF_INET, (const void *) &s->addr, strrep,
                     INET_ADDRSTRLEN);
}

static inline
int sock_data_init (sock_data_t *s, const char *IPaddr, int port)
{
    s->addr.sin_family = AF_INET;
    s->addr.sin_port = htons(port);
    s->fd = -1;
    if (IPaddr == NULL) {
        s->addr.sin_addr.s_addr = INADDR_ANY;
        return 1;
    } else {
        /* NOTE: returns 0 on error */
        return inet_aton(IPaddr, &s->addr.sin_addr);
    }
}

#endif // NET_HELPER_ALL_H

