/*
 *  Copyright (c) 2012 Arber Fama,
 *                     Giovanni Simoni
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "net_helper.h"
#include "config.h"
#include "NetHelper/fair.h"
#include "NetHelper/dictionary.h"

// FIXME : remove
#include <stdio.h>

/* -- Internal data types -------------------------------------------- */

typedef struct {
    int fd;
    dict_t neighbours;
    connection_t cached_peer;   // Cached during wait4data, used in
                                // recv_from_peer
    unsigned refcount;          // Reference counter (workaround for node
                                // copying).
} local_info_t;

typedef struct nodeID {
    struct sockaddr_in addr;
    struct {
        char ip[INET_ADDRSTRLEN];           // ip address
        char ip_port[INET_ADDRSTRLEN + 6];  // ip:port 2^16 -> 5 cyphers.
    } repr;                         // reentrant string representations
    local_info_t *local;            // non-NULL only for local node
} nodeid_t;

/* -- Internal functions --------------------------------------------- */

static int tcp_connect (struct sockaddr_in *to, int *out_fd);
static int tcp_serve (nodeid_t *sd, int backlog);
static int tcp_accept_queue (const nodeid_t *sd);
static void print_err (int e, const char *msg);
static int get_peer (const nodeid_t *self, struct sockaddr_in *addr);
static inline int would_block (int e);
static inline int dead_filedescriptor (int e);
static nodeid_t * addr_to_nodeid (const struct sockaddr_in *addr);
static int wait_incoming (const nodeid_t *self, struct timeval *tout,
                          fd_set *S, int nfds);
static int update_neighbours (dict_t neigh, struct sockaddr_in *addr,
                              int fd);
static int recv_serv_address (int fd, struct sockaddr_in *out_addr);
static int send_serv_address (int fd, const struct sockaddr_in *addr);

/* -- Constants ------------------------------------------------------ */

static const char *   CONF_KEY_BACKLOG = "tcp_backlog";
static const unsigned DEFAULT_BACKLOG = 50;
static const size_t   ERR_BUFLEN = 64;

/* -- Interface exported symbols ------------------------------------- */

struct nodeID *nodeid_dup (struct nodeID *s)
{
    nodeid_t *ret;

    if (s->local == NULL) {
        ret = malloc(sizeof(nodeid_t));
        if (ret == NULL) return NULL;
        memcpy(ret, s, sizeof(nodeid_t));
    } else {
        /* This reference counter trick will avoid copying around of the
         * nodeid for the local host */
        s->local->refcount ++;
        ret = s;
    }

    return ret;
}

/*
* @return -1, 0 or 1, depending on the relation between  s1 and s2.
*/
int nodeid_cmp (const nodeid_t *s1, const nodeid_t *s2)
{
    return memcmp(&s1->addr, &s2->addr, sizeof(struct sockaddr_in));
}

/* @return 1 if the two nodeID are identical or 0 if they are not. */
int nodeid_equal (const nodeid_t *s1, const nodeid_t *s2)
{
    return nodeid_cmp(s1, s2) == 0;
}

struct nodeID *create_node (const char *IPaddr, int port)
{
    nodeid_t *ret = malloc(sizeof(nodeid_t));
    if (ret == NULL) {
        return NULL;
    }

    /* Initialization of address */
    memset(&ret->addr, 0, sizeof(struct sockaddr_in));
    ret->addr.sin_family = AF_INET;
    ret->addr.sin_port = htons(port);

    /* Initialization of string representation */
    if (IPaddr == NULL) {
        /* In case of server, specifying NULL will allow anyone to
         * connect. */
        ret->addr.sin_addr.s_addr = INADDR_ANY;
        inet_ntop(AF_INET, (const void *)&ret->addr.sin_addr,
                  ret->repr.ip, INET_ADDRSTRLEN);
    } else {
        if (inet_pton(AF_INET, IPaddr,
                      (void *)&ret->addr.sin_addr) == 0) {
            fprintf(stderr, "Invalid ip address %s\n", IPaddr);
            free(ret);
            return NULL;
        }
        strcpy(ret->repr.ip, IPaddr);
    }
    sprintf(ret->repr.ip_port, "%s:%hu", ret->repr.ip, (uint16_t)port);

    /* The `local` pointer must be NULL for all instances except for an
     * instance initializated through `net_helper_init` */
    ret->local = NULL;

    return ret;
}

void nodeid_free (struct nodeID *s)
{
    local_info_t *local = s->local;
    if (local != NULL) {
        if (-- local->refcount == 0) {
            dict_delete(local->neighbours);
            close(local->fd);
            free(local);
        }
    }
    free(s);
}

struct nodeID * net_helper_init (const char *IPaddr, int port,
                                 const char *config)
{
    nodeid_t *self;
    struct tag *cfg_tags;
    int backlog;
    local_info_t *local;

    self = create_node(IPaddr, port);
    if (self == NULL) {
        return NULL;
    }

    self->local = local = malloc(sizeof(local_info_t));
    if (local == NULL) {
        nodeid_free(self);
        return NULL;
    }

    /* Default settings */
    backlog = DEFAULT_BACKLOG;

    /* Reading settings */
    cfg_tags = NULL;
    if (config) {
        cfg_tags = config_parse(config);
    }
    if (cfg_tags) {
        /* FIXME: this seems not to work! Testing needed */
        config_value_int_default(cfg_tags, CONF_KEY_BACKLOG, &backlog,
                                 DEFAULT_BACKLOG);
    }
    local->neighbours = dict_new(AF_INET, 1, cfg_tags);
    local->cached_peer.fd = -1;
    local->refcount = 1;

    free(cfg_tags);

    if (tcp_serve(self, backlog) < 0) {
        nodeid_free(self);
        return NULL;
    }

    return self;
}

void bind_msg_type(uint8_t msgtype) {}

/* TODO: Ask fix for the constantness of first parameter this? */
int send_to_peer(const struct nodeID *self, struct nodeID *to,
                 const uint8_t *buffer_ptr, int buffer_size)
{
    int retry;
    ssize_t sent;
    int peer_fd;
    local_info_t *local;

    if (buffer_size <= 0) {
        return 0;
    }

    local = self->local;
    assert(local != NULL);          // TODO: remove after testing
    assert(to->local == NULL);      // TODO: ditto

    tcp_accept_queue(self);
    peer_fd = get_peer(self, &to->addr);
    if (peer_fd == -1) {
        return -1;
    }

    sent = send(peer_fd, buffer_ptr, buffer_size, MSG_DONTWAIT);
    if (sent == -1 && !would_block(errno)) {
        print_err(errno, "sending");
    }

    return sent;
}

int recv_from_peer(const struct nodeID *self, struct nodeID **remote,
                   uint8_t *buffer_ptr, int buffer_size)
{
    local_info_t *local;
    connection_t *peer;
    int retval;

    assert(self->local != NULL);        // TODO: remove after testing
    local = self->local;
    peer = &local->cached_peer;
    if (peer->fd == -1) {
        fd_set fds;

        /* No cache from wait4data */
        FD_ZERO(&fds);
        if (wait_incoming(self, NULL, &fds, 0) == -1) {
            return -1;
        }
    }

    assert(peer->fd != -1);

    if ((*remote = addr_to_nodeid((const struct sockaddr_in *)peer->addr))
            == NULL) {
        return -1;
    }

    retval = recv(peer->fd, buffer_ptr, buffer_size, 0);
    switch (retval) {
        case -1:
            print_err(errno, "receiving");
        case 0:
            close(peer->fd);
            break;
        default:
            break;
    }
    peer->fd = -1;
    return retval;
}

int wait4data(const struct nodeID *self, struct timeval *tout,
              int *user_fds)
{
    fd_set fdset;
    int i;
    int maxfd;

    assert(self->local != NULL);        // TODO: remove after testing
    FD_ZERO(&fdset);
    maxfd = -1;

    if (user_fds) {
        for (i = 0; user_fds[i] != -1; i++) {
            FD_SET(user_fds[i], &fdset);
            if (user_fds[i] > maxfd) {
                maxfd = user_fds[i];
            }
        }
    }

    switch (wait_incoming(self, tout, &fdset, maxfd + 1)) {
        case -1:
            return -1;
        case 0:
            return 0;
        default:
            if (user_fds && self->local->cached_peer.fd == -1) {
                /* Update externally provided file descriptors as
                 * expected by outside algorithms */
                for (i = 0; user_fds[i] != -1; i++) {
                    if (!FD_ISSET(user_fds[i], &fdset)) {
                        user_fds[i] = -2;
                    }
                }
                return 2;
            }
            return 1;
    }
}

struct nodeID *nodeid_undump (const uint8_t *b, int *len)
{
    nodeid_t *ret = malloc(sizeof(nodeid_t));
    if (ret == NULL) {
        return NULL;
    }

    memcpy((void *)&ret->addr, (const void *)b,
            sizeof(struct sockaddr_in));
    inet_ntop(AF_INET, (const void *)b, ret->repr.ip,
              sizeof(struct sockaddr_in));
    sprintf(ret->repr.ip_port, "%s:%hu", ret->repr.ip,
            ntohs(ret->addr.sin_port));
    ret->local = NULL;

    return ret;
}

int nodeid_dump (uint8_t *b, const struct nodeID *s,
                 size_t max_write_size)
{
    assert(s->local == NULL);   // TODO: remove after testing

    if (max_write_size < sizeof(struct sockaddr_in)) {
        return -1;
    }
    memcpy((void *)b, (const void *)&s->addr, sizeof(struct sockaddr_in));
    return sizeof(struct sockaddr_in);
}

const char *node_ip(const struct nodeID *s)
{
    return s->repr.ip;
}

const char *node_addr (const struct nodeID *s)
{
    return s->repr.ip_port;
}

/* -- Internal functions --------------------------------------------- */

static
int tcp_connect (struct sockaddr_in *to, int *out_fd)
{
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        print_err(errno, "outgoing, creating socket");
        return -1;
    }

    if (connect(fd, (struct sockaddr *)to,
                sizeof(struct sockaddr_in)) == -1) {
        print_err(errno, "outgoing, connect");
        close(fd);
        return -2;
    }
    *out_fd = fd;

    return 0;
}

static
int tcp_serve (nodeid_t *sd, int backlog)
{
    int fd;
    assert(sd->local != NULL);  // TODO: remove when it works.

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        print_err(errno, "serving, creating socket");
        return -1;
    }

    if (bind(fd, (struct sockaddr *) &sd->addr,
             sizeof(struct sockaddr_in))) {
        print_err(errno, "serving, binding socket");
        close(fd);
        return -2;
    }

    if (listen(fd, backlog) == -1) {
        print_err(errno, "serving, listening");
        close(fd);
        return -3;
    }
    sd->local->fd = fd;

    return 0;
}

static
int tcp_accept_queue (const nodeid_t *sd)
{
    /* TODO: a possible optimization would be avoiding the first select if
     * we already know there's a connection waiting in the backlog queue.
     */
    int servfd;
    int need_test;

    assert(sd->local != NULL);      // TODO remvoe after testing

    servfd = sd->local->fd;
    need_test = 1;

    while (need_test) {
        fd_set S;
        socklen_t len;
        int clifd;
        struct sockaddr_in incoming;
        struct timeval nowait = {0, 0};

        FD_ZERO(&S);
        FD_SET(servfd, &S);
        switch (select(servfd + 1, &S, NULL, NULL, &nowait)) {

            case -1:
                /* Error. */
                print_err(errno, "accept queue, select");
                return -1;

            case 0:
                /* Timeout, no more connections. */
                need_test = 0;
                break;

            default:
                /* We have an incoming connection, let's accept it and
                 * store into the neighbours dictionary. */

                len = sizeof(struct sockaddr_in);
                clifd = accept(servfd, (struct sockaddr *)&incoming,
                               &len);
                if (clifd == -1) {
                    print_err(errno, "accept queue, accept");
                    return -1;
                }

                if (update_neighbours(sd->local->neighbours, &incoming,
                                      clifd) == -1) {
                    return -1;
                }
        }
    }

    return 0;
}

/* perror-like with parametrized error */
static
void print_err (int e, const char *msg)
{
    char buf[ERR_BUFLEN];
    strerror_r(e, buf, ERR_BUFLEN);
    if (msg) {
        fprintf(stderr, "net-helper-tcp: %s: %s\n", msg, buf);
    } else {
        fprintf(stderr, "net_helper-tcp: %s\n", buf);
    }
}

/* TODO: this can be optimized using new features for dictionaries */
static
int get_peer (const nodeid_t *self, struct sockaddr_in *addr)
{
    peer_info_t *peer;
    dict_t neighbours = self->local->neighbours;

    if (dict_lookup(neighbours, (const struct sockaddr *) addr,
                    &peer) == 0) {
        return peer->fd;
    } else {
        /* We don't have the address stored, thus we need to connect */
        int fd;

        if (tcp_connect(addr, &fd) < 0) {
            return -1;
        }

        dict_insert(neighbours, (struct sockaddr *) addr, fd);
        if (send_serv_address(fd, &self->addr) == -1) {
            return -1;
        }

        return fd;
    }
}

static inline
int would_block (int e)
{
    return e == EAGAIN||
           e == EWOULDBLOCK;
}

static inline
int dead_filedescriptor (int e)
{
    return e == ECONNRESET ||
           e == EBADF ||
           e == ENOTCONN;
}

static
nodeid_t * addr_to_nodeid (const struct sockaddr_in *addr)
{
    nodeid_t *ret;

    ret = malloc(sizeof(nodeid_t));
    if (ret == NULL) return NULL;

    ret->local = NULL;
    memcpy((void *)&ret->addr, (const void *)addr,
           sizeof(struct sockaddr_in));
    inet_ntop(AF_INET, (const void *)&ret->addr.sin_addr,
              ret->repr.ip, INET_ADDRSTRLEN);
    sprintf(ret->repr.ip_port, "%s:%hu", ret->repr.ip,
            ntohs(addr->sin_port));

    return ret;
}

static
int wait_incoming (const nodeid_t *self, struct timeval *tout, fd_set *S,
                   int nfds)
{
    int was_accept;
    local_info_t *local;

    local = self->local;
    nfds --;
    FD_SET(local->fd, S);
    if (local->fd > nfds) {
        nfds = local->fd;
    }

    do {
        int err;

        was_accept = 0;
        switch (fair_select(local->neighbours, tout, S, nfds + 1,
                            &local->cached_peer, &err)) {

            case -1:
                print_err(err, "fair selection");
                return -1;

            case 0:
                /* Timeout. Simply no data nor connections. */
                return 0;

        }

        if (FD_ISSET(local->fd, S)) {
            /* We have some new incoming connection. */
            if (tcp_accept_queue(self) == -1) {
                return -1;
            }
            was_accept = 1;
        }
    } while (was_accept);

    return 1;
}

static
int update_neighbours (dict_t neigh, struct sockaddr_in *addr, int fd)
{
    struct sockaddr_in neigh_serv;

    if (recv_serv_address(fd, &neigh_serv) == -1) {
        return -1;
    }

    dict_insert(neigh, (const struct sockaddr *)addr, fd);
    dict_insert(neigh, (const struct sockaddr *)&neigh_serv, fd);

    return 0;
}

static
int send_serv_address (int fd, const struct sockaddr_in *addr)
{
    const uint8_t *buffer;
    ssize_t size;

    buffer = (const uint8_t *)addr;
    size = sizeof(struct sockaddr_in);
    do {
        ssize_t n = write(fd, (const void *)buffer, size);
        if (n == -1) {
            print_err(errno, "sending local server address");
            return -1;
        }
        size -= n;
        buffer += n;
    } while (size > 0);

    return 0;
}

static
int recv_serv_address (int fd, struct sockaddr_in *out_addr)
{
    uint8_t *buffer;
    ssize_t size;

    buffer = (uint8_t *)out_addr;
    size = sizeof(struct sockaddr_in);
    do {
        ssize_t n = read(fd, (void *)buffer, size);
        if (n == -1) {
            print_err(errno, "receiving remote server address");
            return -1;
        }
        size -= n;
        buffer += n;
    } while (size > 0);

    return 0;
}

