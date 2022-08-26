

//#include "./pages/CreatePages.h"

//#include "./handle_client/handle_client.h"
#include "Request.h"
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

/*    
    int raceid = 33732;  //36804;
    char* PageBuffer = 0;
    int PageBuffer_size = 0;
    if (CreatePage_RaceResults(raceid, &PageBuffer, &PageBuffer_size) == 0) {
        fprintf(stderr, "%s\n", PageBuffer);
    }
    if (PageBuffer) {
        free(PageBuffer);
    }
*/

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
                fprintf(stderr, "[%ld] Failed to close fd_listen: %s\n", (long)getpid(), strerror(errno));
                return 1;
            }
            
            
            /* 
            char client_message[BUFFER_SIZE];
            if ((bytes = r_read(fd_active, client_message, BUFFER_SIZE - 1)) == -1) {
                fprintf(stderr, "[%ld] Failed to read message from server: Closing connection...\n", (long)getpid());
                fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
                return 1;
            }
            client_message[bytes] = '\0';
            
            // Handle the message received from the connected client
            handle_client(fd_active, client_message, bytes + 1);

            fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
            return 0;
            */


            //
            // TODO: New path. Replace the old path above with this new one once it works.
            // TODO: Make sure all points of failure displays error messages and sends an http response in a good way
            //
            Request request;
            if (read_and_parse_request(fd_active, &request) == -1) {
                fprintf(stderr, "[%ld] Failed to read/parse the client request. Closing connection...\n", (long)getpid());
                fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
                return 1;
            }

            if (handle_request(fd_active, &request) == -1) {
                fprintf(stderr, "[%ld] Failed to handle the client request. Closing connection...\n", (long)getpid());
                fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
                return 1;
            }

            fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
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






















