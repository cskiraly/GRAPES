/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>     /* For struct ifreq */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net_helpers.h"

const char *iface_addr(const char *iface)
{
    int s, res;
    struct ifreq iface_request;
    struct sockaddr_in *sin;
    char buff[512];

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        return NULL;
    }

    memset(&iface_request, 0, sizeof(struct ifreq));
    sin = (struct sockaddr_in *)&iface_request.ifr_addr;
    strcpy(iface_request.ifr_name, iface);
    /* sin->sin_family = AF_INET); */
    res = ioctl(s, SIOCGIFADDR, &iface_request);
    if (res < 0) {
        perror("ioctl(SIOCGIFADDR)");
        close(s);

        return NULL;
    }
    close(s);

    inet_ntop(AF_INET, &sin->sin_addr, buff, sizeof(buff));

    return strdup(buff);
}



