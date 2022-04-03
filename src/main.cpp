

#include "api/api.h"
#include "http-response.h"
#include "libs/Restart.h"
#include "libs/uici.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


#define DEFAULT_PORT 80
#define BUFFER_SIZE 32768

int handle_client(int socket, char* bytes, int size);


/* ----------------------------------------------------------------------------
 * MAIN
 * This program is an HTTP Server that listens on a given port.
 * All client requests are handled in separate child processes, 
 * while the parent process is used to listen for new connections.
 * --------------------------------------------------------------------------- */
int main(int argc, char** argv)
{
    int fd_listen, fd_active;
    char client[MAX_CANON];
    pid_t childpid;
    int bytes;

    // --------------------------------------------------------
    // Open a file descirptor for listening for connections
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
        if ((fd_active = u_accept(fd_listen, client, MAX_CANON)) == -1) {
            perror("Failed to accept connection");
            continue;
        }
        if ((childpid = fork()) == -1) {
            perror("Failed to fork child process");
            continue;
        }

        // -----------------------------------------------------------------------------
        // Child: Close the file descriptor that is used to listen for new connections
        // Handle the connected client and then return/exit the child process
        // -----------------------------------------------------------------------------
        if (childpid == 0)
        {
            fprintf(stderr, "[%ld] Client connected: %s\n", (long)getpid(), client);
            
            if (r_close(fd_listen) == -1) {
                fprintf(stderr, "[%ld] Failed to close fd_listen: %s\n", (long)getpid(), strerror(errno));
                return 1;
            }
  
            char readbuffer[BUFFER_SIZE];
            if ((bytes = r_read(fd_active, readbuffer, BUFFER_SIZE - 1)) == -1) {
                fprintf(stderr, "[%ld] Failed to read message from server: Closing connection...\n", (long)getpid());
                fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
                return 1;
            }
            readbuffer[bytes] = '\0';
            
            //handleClient(fd_active, readbuffer, bytes + 1);
            handle_client(fd_active, readbuffer, bytes + 1);

            fprintf(stderr, "[%ld] %s disconnected\n", (long)getpid(), client);
            return 0;
        }
        
        // -----------------------------------------------------------------------------
        // Parent: Close the file descriptor that the client is connected on
        // Wait for zombie processes and restart the loop to wait for new connections
        // -----------------------------------------------------------------------------
        if (r_close(fd_active) == -1) {
            fprintf(stderr, "[PARENT] Failed to close fd_active: %s\n", strerror(errno));
        }
        while (r_waitpid(-1, NULL, WNOHANG) > 0);  // Clean up zombies (non-blocking)
    }

    return 0;
}


/**
 * -----------------------------------------------------------------------------------------
 * Handles the request that was received from the client
 *
 * socket: Represents the file descriptor the client is connected over
 * bytes: The data that was received from the client, should be an HTTP Request
 * size: The buffer size of "bytes"
 *
 * Returns 0 on success, which means that the clients request was valid, handled and a response was sent back
 * Returns -1 on failure, which means the steps above didn't happen
 * -----------------------------------------------------------------------------------------
 */
int handle_client(int socket, char* bytes, int size)
{
    char* command;
    char* server;
    char* path;
    char* protocol;
    char* port;

    // -------------------------------------------------------------------------------------------------
    // Assumes that the received data (bytes) is an HTTP Request line, and tries to parse it
    // On success, each part of the request line gets divided into substrings (ending with a null-terminating character)
    // But this doesn't mean that the request line was valid, just that it was divided into substrings 
    // -------------------------------------------------------------------------------------------------
    if ((parse_requestline(bytes, size, &command, &server, &path, &protocol, &port)) == -1) 
    {
        fprintf(stderr, "[%ld] HTTP 400: Invalid request line\n", (long)getpid());
        sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Please provide a valid request line: [HTTP-Method] [PATH] [PROTOCOL]\n\0");
        return -1;
    }
    fprintf(stderr, "[%ld] Received HTTP Request from client: %s %s %s\n", (long)getpid(), command, path, protocol);
    
    
    //
    // TODO: Validate request line (list of valid commands, protocol is HTTP, etc.)
    //

    

    // List of API calls
    int API_SIZE = LINE_MAX_SIZE;
    char GET_ATHLETE_FISCODE[] =       "/api/athlete/fiscode/";
    char GET_ATHLETES_FIRSTNAME[] =    "/api/athletes/firstname/";
    char GET_ATHLETES_LASTNAME[] =     "/api/athletes/lastname/";
    char GET_ATHLETES_FULLNAME[] =     "/api/athletes/fullname/";
    char GET_RACES_FISCODE[] =         "/api/raceids/fiscode/";
    char GET_RACEINFO_RACEID[] =       "/api/raceinfo/raceid/";
    char GET_RACE_RESULTS_RACEID[] =   "/api/raceresults/raceid/";
    char GET_ANALYZED_QUAL_FISCODE[] = "/api/analyze/qual/fiscode/";


    // ------------------------------------------------------------------ 
    // API: Get athletes
    // ------------------------------------------------------------------ 
    API_SIZE = strlen(GET_ATHLETE_FISCODE);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_ATHLETE_FISCODE, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_fiscode(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_FIRSTNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_ATHLETES_FIRSTNAME, API_SIZE) == 0)) 
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_firstname(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_LASTNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_ATHLETES_LASTNAME, API_SIZE) == 0)) 
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_lastname(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_FULLNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_ATHLETES_FULLNAME, API_SIZE) == 0)) 
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_fullname(socket, parameter);
    }

    // ------------------------------------------------------------------ 
    // API: Get list of races for a given athlete
    // ------------------------------------------------------------------ 
    API_SIZE = strlen(GET_RACES_FISCODE);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_RACES_FISCODE, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthletesRaceids(socket, parameter);
    }
    
    // ------------------------------------------------------------------ 
    // API: Get race info and results
    // ------------------------------------------------------------------ 
    API_SIZE = strlen(GET_RACEINFO_RACEID);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_RACEINFO_RACEID, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getRaceInfo(socket, parameter);
    }

    API_SIZE = strlen(GET_RACE_RESULTS_RACEID);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_RACE_RESULTS_RACEID, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getRaceResult(socket, parameter);
    }

    // --------------------------------------------------------------------- 
    // API: Get analyzed sprint qualification results for a given athlete
    // --------------------------------------------------------------------- 
    API_SIZE = strlen(GET_ANALYZED_QUAL_FISCODE);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_ANALYZED_QUAL_FISCODE, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAnalyzedResults_qual(socket, parameter);
    }


    // ==============================================================
    // Attempt to find and return the requested resource 
    // ==============================================================
    if (strcmp(command, "GET") == 0)
    {
        char filepath[strlen(path) + (32 * sizeof(char))];
        strcpy(filepath, RESOURCE_PATH);
        if (path == 0 || (strlen(path) == 1 && path[0] == '/'))
            strcat(filepath, DEFAULT_FILE);
        else
            strcat(filepath, path);
            
        char* buffer = 0;
        int status_code = 500;  // Default status code on failure
        if (loadResource(socket, filepath, &buffer, &status_code) == -1) 
        {
            sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Error: Could not find what you were looking for\n\0");
            if (buffer != 0) 
                free(buffer);
            return -1;
        }

        if (sendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_HTML, buffer) == -1) {
            if (buffer != 0) 
                free(buffer);
            return -1;  
        }
            
        fprintf(stderr, "[%ld] HTTP 200: Found and sent the requested resource successfully!\n", (long)getpid());
        if (buffer != 0) 
            free(buffer);
        
        return 0;
    }


    // =======================================================================================
    // Unknown HTTP Method 
    // =======================================================================================
    fprintf(stderr, "[%ld] HTTP 400: Unknown HTTP method: %s\n", (long)getpid(), command);
    sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Unknown HTTP method\n\0");
    
    return -1;

}






