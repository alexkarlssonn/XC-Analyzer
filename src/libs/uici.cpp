
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include "uici.h"

#ifndef MAXBACKLOG
#define MAXBACKLOG 50
#endif

/*
 *                           u_igniore_sigpipe
 * Ignore SIGPIPE if the default action is in effect.
 *
 * returns: 0 if successful
 *          -1 on error and sets errno
 */
static int u_ignore_sigpipe() {
   struct sigaction act;

   if (sigaction(SIGPIPE, (struct sigaction *)NULL, &act) == -1)
      return -1;
   if (act.sa_handler == SIG_DFL) {
      act.sa_handler = SIG_IGN;
      if (sigaction(SIGPIPE, &act, (struct sigaction *)NULL) == -1)
         return -1;
   }   
   return 0;
}

/*
 *                           u_open
 * Return a file descriptor, which is bound to the given port.
 *
 * parameter:
 *        s = number of port to bind to
 * returns:  file descriptor if successful
 *           -1 on error and sets errno
 */
int u_open(u_port_t port) {
   int error;  
   struct sockaddr_in server;
   int sock;
   int _true = 1;

   if ((u_ignore_sigpipe() == -1) || ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1))
      return -1; 

   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&_true, sizeof(_true)) == -1) {
      error = errno;
      while ((close(sock) == -1) && (errno == EINTR)); 
      errno = error;
      return -1;
   }
 
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = htonl(INADDR_ANY);
   server.sin_port = htons((short)port);
   if ((bind(sock, (struct sockaddr *)&server, sizeof(server)) == -1) ||
        (listen(sock, MAXBACKLOG) == -1)) {
      error = errno;
      while ((close(sock) == -1) && (errno == EINTR)); 
      errno = error;
      return -1;
   }
   return sock;
}

/*
 *                           u_accept
 * Wait for a connection request from a host on a specified port.
 *
 * parameters:
 *      fd = file descriptor previously bound to listening port
 *      hostn = a buffer that will hold the name of the remote host
 *      hostnsize = size of hostn buffer
 * returns:  a communication file descriptor on success
 *              hostn is filled with the name of the remote host.
 *           -1 on error with errno set
 *
 * comments: This function is used by the server to wait for a
 * communication.  It blocks until a remote request is received
 * from the port bound to the given file descriptor.
 * hostn is filled with an ASCII string containing the remote
 * host name.  It must point to a buffer of size at least hostnsize.
 * If the name does not fit, as much of the name as is possible is put
 * into the buffer.
 * If hostn is NULL or hostnsize <= 0, no hostname is copied.
 */
int u_accept(int fd, char *hostn, int hostnsize) {
   int len = sizeof(struct sockaddr);
   struct sockaddr_in netclient;
   int retval;

   while (((retval =
           accept(fd, (struct sockaddr *)(&netclient), (socklen_t *)(&len))) == -1) && (errno == EINTR)) ;  
   if ((retval == -1) || (hostn == NULL) || (hostnsize <= 0))
      return retval;
   addr2name(netclient.sin_addr, hostn, hostnsize);
   return retval;
}

/*
 *                           u_connect
 * Initiate communication with a remote server.
 *
 * parameters:
 *     port  = well-known port on remote server
 *     hostn = character string giving the Internet name of remote host
 * returns:  a communication file descriptor if successful
 *           -1 on error with errno set
 */
int u_connect(u_port_t port, char *hostn) {
   int error;
   int retval;
   struct sockaddr_in server;
   int sock;
   fd_set sockset;

   if (name2addr(hostn,&(server.sin_addr.s_addr)) == -1) {
      errno = EINVAL;
      return -1;
   }
   server.sin_port = htons((short)port);
   server.sin_family = AF_INET;

   if ((u_ignore_sigpipe() == -1) ||
        ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1))
      return -1;

   if (((retval =
       connect(sock, (struct sockaddr *)&server, sizeof(server))) == -1) &&
       ((errno == EINTR) || (errno == EALREADY))) {
       FD_ZERO(&sockset);
       FD_SET(sock, &sockset);
       while ( ((retval = select(sock+1, NULL, &sockset, NULL, NULL)) == -1) &&
               (errno == EINTR) ) {
          FD_ZERO(&sockset);
          FD_SET(sock, &sockset);
       } 
   }
   if (retval == -1) {
        error = errno;
        while ((close(sock) == -1) && (errno == EINTR)); 
        errno = error;
        return -1;
   }
   return sock;
}


// =====================================================================================================

#ifndef REENTRANCY
#define REENTRANCY REENTRANT_NONE
#endif

#if REENTRANCY==REENTRANT_MUTEX
#include <pthread.h>
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#if REENTRANCY==REENTRANT_NONE
/* Convert struct in_addr to a host name */
void addr2name(struct in_addr addr, char *name, int namelen) {
   struct hostent *hostptr;
   hostptr = gethostbyaddr((char *)&addr, 4, AF_INET);
   if (hostptr == NULL)
      strncpy(name, inet_ntoa(addr), namelen-1);
   else
      strncpy(name, hostptr->h_name, namelen-1);
   name[namelen-1] = 0;
}

/* Return -1 on error, 0 on success */
int name2addr(char *name, in_addr_t *addrp) {
   struct hostent *hp;

   if (isdigit((int)(*name)))
      *addrp = inet_addr(name);
   else {
      hp = gethostbyname(name);
      if (hp == NULL)
         return -1;
      memcpy((char *)addrp, hp->h_addr_list[0], hp->h_length);
   }
   return 0;
}
#elif REENTRANCY==REENTRANT_R
#define GETHOST_BUFSIZE 8192
void addr2name(struct in_addr addr, char *name, int namelen) {
   char buf[GETHOST_BUFSIZE];
   int h_error;
   struct hostent *hp;
   struct hostent result;

   hp = gethostbyaddr_r((char *)&addr, 4, AF_INET, &result, buf,
                         GETHOST_BUFSIZE, &h_error);
   if (hp == NULL)
      strncpy(name, inet_ntoa(addr), namelen-1);
   else
      strncpy(name, hp->h_name, namelen-1);
   name[namelen-1] = 0;
}
 
/* Return -1 on error, 0 on success */
int name2addr(char *name, in_addr_t *addrp) {
   char buf[GETHOST_BUFSIZE];
   int h_error;
   struct hostent *hp;
   struct hostent result;
 
   if (isdigit((int)(*name)))
      *addrp = inet_addr(name);
   else {
      hp = gethostbyname_r(name, &result, buf, GETHOST_BUFSIZE, &h_error);
      if (hp == NULL)
         return -1;
      memcpy((char *)addrp, hp->h_addr_list[0], hp->h_length);
   }
   return 0;
}   
#elif REENTRANCY==REENTRANT_MUTEX
/* Convert struct in_addr to a host name */
void addr2name(struct in_addr addr, char *name, int namelen) {
   struct hostent *hostptr;

   if (pthread_mutex_lock(&mutex) == -1) {
      strncpy(name, inet_ntoa(addr), namelen-1); 
      name[namelen-1] = 0;
      return;
   }
   hostptr = gethostbyaddr((char *)&addr, 4, AF_INET);
   if (hostptr == NULL)
      strncpy(name, inet_ntoa(addr), namelen-1); 
   else  
      strncpy(name, hostptr->h_name, namelen-1);
   pthread_mutex_unlock(&mutex);
   name[namelen-1] = 0;
}
 
/* Return -1 on error, 0 on success */
int name2addr(char *name, in_addr_t *addrp) {
   struct hostent *hp;
 
   if (isdigit((int)(*name)))
      *addrp = inet_addr(name);
   else {
      if (pthread_mutex_lock(&mutex) == -1)
         return -1;
      hp = gethostbyname(name);
      if (hp == NULL) {
         pthread_mutex_unlock(&mutex);
         return -1;
      }
      memcpy((char *)addrp, hp->h_addr_list[0], hp->h_length);
      pthread_mutex_unlock(&mutex);
   }
   return 0;
}
#elif REENTRANCY==REENTRANT_POSIX
/* Convert struct in_addr to a host name */
void addr2name(struct in_addr addr, char *name, int namelen) {
   struct sockaddr_in saddr;
   saddr.sin_family = AF_INET;
   saddr.sin_port = 0;
   saddr.sin_addr = addr;
   if (getnameinfo((struct sockaddr *)&saddr, sizeof(saddr), name, namelen,
         NULL, 0, 0) != 0) {
      strncpy(name, inet_ntoa(addr), namelen-1);
      name[namelen-1] = 0;
   }
}

/* Return -1 on error, 0 on success */
int name2addr(char *name, in_addr_t *addrp) {
   struct addrinfo hints;
   struct addrinfo *res;
   struct sockaddr_in *saddrp;

   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = PF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = 0;
   hints.ai_addrlen = 0;
   hints.ai_canonname = NULL;
   hints.ai_addr = NULL;
   hints.ai_next = NULL;
 
   if (getaddrinfo(name, NULL, &hints, &res) != 0)
      return -1;
   saddrp = (struct sockaddr_in *)(res->ai_addr);
   memcpy(addrp, &saddrp->sin_addr.s_addr, 4);
   freeaddrinfo(res);
   return 0;
}

#endif



