
#pragma once

#include <netinet/in.h>

#define REENTRANT_NONE 0
#define REENTRANT_R 1
#define REENTRANT_MUTEX 2
#define REENTRANT_POSIX 3
#define UPORT
typedef unsigned short u_port_t;

int u_open(u_port_t port);
int u_accept(int fd, char *hostn, int hostnsize);
int u_connect(u_port_t port, char *hostn);

// Handles conversion between address name and byte address
int name2addr(char *name, in_addr_t *addrp);
void addr2name(struct in_addr addr, char *name, int namelen);
