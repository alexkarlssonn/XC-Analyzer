

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "http-response.h"
#include "libs/Restart.h"
#include "libs/uici.h"

#define DEFAULT_PORT 80
#define BUFFER_SIZE 32768

u_port_t setPortnumber(int argc, char** argv);


/* ----------------------------------------------------------------------------
 * MAIN
 * This program is an HTTP Server that listens on a given port.
 * All client requests are handled in separate child processes, 
 * while the parent process is used to listen for new connections.
 * --------------------------------------------------------------------------- */
int main(int argc, char** argv)
{
    u_port_t portnumber;
    int fd_listen, fd_active;
    char client[MAX_CANON];
    pid_t childpid;
    int bytes;

    portnumber = setPortnumber(argc, argv);
    if ((fd_listen = u_open(portnumber)) == -1) {
        perror("Failed to create listening endpoint");
        return 1;
    }
    fprintf(stderr, "[PARENT] Waiting for connection on port: %d\n", (int)portnumber);
   

    while (true)
    {
        if ((fd_active = u_accept(fd_listen, client, MAX_CANON)) == -1) {
            perror("Failed to accept connection");
            continue;
        }

        if ((childpid = fork()) == -1) {
            perror("Failed to fork child process");
            continue;
        }

        if (childpid == 0)
        {
            fprintf(stderr, "[%ld] Client connected: %s\n", (long)getpid(), client);
            
            if (r_close(fd_listen) == -1) {
                fprintf(stderr, "[%ld] Failed to close fd_listen: %s\n", (long)getpid(), strerror(errno));
                return 1;
            }
  
            char readbuffer[BUFFER_SIZE + 1];
            bytes = r_read(fd_active, readbuffer, BUFFER_SIZE);
            if (bytes == -1) {
                fprintf(stderr, "[%ld] Failed to read message from server - Closing connection\n", (long)getpid());
                return 1;
            }

            readbuffer[bytes] = '\0';
            handleClient(fd_active, readbuffer, bytes + 1);

            fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
            return 0;
        }
        
        if (r_close(fd_active) == -1) {
            fprintf(stderr, "[PARENT] Failed to close fd_active: %s\n", strerror(errno));
        }
        while (r_waitpid(-1, NULL, WNOHANG) > 0);  // Clean up zombies (non-blocking)
    }

    return 0;
}


/**
 * ----------------------------------------------------------------------------
 * Return the port number that is specified as the first program argument
 * If no argument is specified, or if it's invalid, then use the default value
 * ----------------------------------------------------------------------------
 */
u_port_t setPortnumber(int argc, char** argv)
{
    u_port_t portnumber = DEFAULT_PORT;
    if (argc == 2) {
        portnumber = (u_port_t) atoi(argv[1]); 
        if (portnumber < 0) portnumber = (u_port_t) DEFAULT_PORT;
    }
    return portnumber;
}


