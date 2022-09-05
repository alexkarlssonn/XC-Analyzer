

#include "./server/Server.h"
#include "./libs/Restart.h"
#include "./libs/uici.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT 80
#define BUFFER_SIZE 32768


/**
 * ----------------------------------------------------------------------------
 * MAIN
 *
 * This program is an HTTP Server that listens on a given port.
 * All client requests are handled in separate child processes, 
 * while the parent process is used to listen for new connections.
 * --------------------------------------------------------------------------- 
 */
int main(int argc, char** argv)
{
    int MAX_CANON = 255;
    int fd_listen, fd_active;
    char client[MAX_CANON];
    pid_t childpid;
    int bytes;

    // --------------------------------------------------------
    // Open a file descriptor for listening for connections
    // --------------------------------------------------------
    u_port_t portnumber = DEFAULT_PORT;
    if (argc == 2) {
        portnumber = (u_port_t) atoi(argv[1]); 
        if (portnumber < 0) portnumber = (u_port_t) DEFAULT_PORT;
    }
    
    if ((fd_listen = u_open(portnumber)) == -1) {
        perror("Failed to create listening endpoint");
        return 1;
    }
    fprintf(stderr, "[PARENT] Waiting for connection on port: %d\n", (int)portnumber);
   
    while (true)
    {
        // Accept a new connection and fork a new process used to handle the connected client
        if ((fd_active = u_accept(fd_listen, client, MAX_CANON)) == -1) {
            perror("Failed to accept connection");
            continue;
        }
        if ((childpid = fork()) == -1) {
            perror("Failed to fork child process");
            continue;
        }

        // -----------------------------------------------------------------------------
        // Child: 
        // Read the received messasge and handle the connected client
        // Close the file descriptor that is used to listen for new connections
        // -----------------------------------------------------------------------------
        if (childpid == 0)
        {
            fprintf(stderr, "[%ld] Client connected: %s\n", (long)getpid(), client);
            if (r_close(fd_listen) == -1) {
                fprintf(stderr, "[%ld] Failed to close fd_listen: %s. Closing connection...\n", (long)getpid(), strerror(errno));
                fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
                return 1;
            }
            
            
            // Read and parse the message received from the client over the socket
            Request request;
            if (ReadClientRequest(fd_active, &request) == -1) {
                fprintf(stderr, "[%ld] Failed to read and parse client request: Closing connection...\n", (long)getpid());
                fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
                return 1;
            }


            if (request.query) {
                fprintf(stderr, "[%ld] Client request line: %s %s?%s %s\n", (long)getpid(), request.method, request.path, request.query, request.protocol);
            } else {
                fprintf(stderr, "[%ld] Client request line: %s %s %s\n", (long)getpid(), request.method, request.path, request.protocol);
            }


            // Handle the client request and respond to it
            if (HandleClientRequest(fd_active, &request) == -1) {
                fprintf(stderr, "[%ld] Failed to handle the client request: Closing connection...\n", (long)getpid());
                fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
                if (request.buffer) { free(request.buffer); }
                return 1;
            }

        
            fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
            if (request.buffer) { free(request.buffer); }
            return 0;
        }
        
        // -----------------------------------------------------------------------------
        // Parent: 
        // Close the file descriptor that the client is connected on
        // Wait for zombie processes and restart the loop to wait for new connections
        // -----------------------------------------------------------------------------
        if (r_close(fd_active) == -1) {
            fprintf(stderr, "[PARENT] Failed to close fd_active: %s\n", strerror(errno));
        }
        while (r_waitpid(-1, NULL, WNOHANG) > 0);  // Clean up zombies (non-blocking)
    }

    return 0;
}







