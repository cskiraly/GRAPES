#include "net_helper.h"

#include <assert.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static const uint16_t START_PORT = 9500;
static const size_t BUFSIZE = 30;

static void node0 ()
{
    struct nodeID *self, *other;
    struct nodeID *remote;
    char buffer[BUFSIZE];
    ssize_t N;

    self = net_helper_init("127.0.0.1", START_PORT, "");
    other = create_node("127.0.0.1", START_PORT + 1);

    if (self == NULL || other == NULL) {
        return;
    }

    printf("node0 - Waiting 1 second\n");
    sleep(3);

    printf("node0 - sending chip\n");
    N = send_to_peer(self, other, "hello", sizeof("hello"));
    printf("node0 - sent %i bytes. Are they coming back?\n", (int)N);

    N = recv_from_peer(self, &remote, (void *)buffer, BUFSIZE);
    printf("node0 - recv %i bytes: \"%s\"\n", (int)N, buffer);
    N = recv_from_peer(self, &remote, (void *)buffer, BUFSIZE);
    printf("node0 - recv %i bytes: \"%s\"\n", (int)N, buffer);

    nodeid_free(self);
    nodeid_free(other);
    nodeid_free(remote);
}

static void node1 ()
{
    struct nodeID *self, *other, *remote;
    char buffer[BUFSIZE];
    ssize_t N;
    int w4d_ret;

    self = net_helper_init("127.0.0.1", START_PORT + 1, "");
    other = create_node("127.0.0.1", START_PORT);

    if (self == NULL || other == NULL) {
        return;
    }

    printf("node1 - Wait4data\n");

    do {
        struct timeval tout = { 0, 500000 };
        w4d_ret = wait4data(self, &tout, NULL);
        printf("node1 - ...waiting for data... (got %i)\n", w4d_ret);
    } while (w4d_ret == 0);

    printf("node1 - Going receive\n");
    N = recv_from_peer(self, &remote, (void *)buffer, BUFSIZE);
    assert(buffer[N-1] == 0);
    printf("node1 - Received: \"%s\", now sending twice\n", buffer);

    send_to_peer(self, remote, (void *)buffer, N);
    send_to_peer(self, other, (void *)buffer, N);

    nodeid_free(self);
    nodeid_free(other);
    nodeid_free(remote);
}

int main (int argc, char **argv)
{
    pid_t P0, P1;

    P0 = fork();
    if (P0 == 0) {
        node0();
        exit(EXIT_SUCCESS);
    }
    P1 = fork();
    if (P1 == 0) {
        node1();
        exit(EXIT_SUCCESS);
    }

    waitpid(P0, NULL, 0);
    waitpid(P1, NULL, 0);

    exit(EXIT_SUCCESS);
}

