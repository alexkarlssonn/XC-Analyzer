
#include "handle_client.h"

#include "../api/api.h"
#include "load_resource.h"
#include "parse_requestline.h"
#include "send_http_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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
    // Assumes that the received data (bytes) is an HTTP request line, and tries to parse it
    // On success, each part of the request line gets divided into substrings (ending with a null-terminating character)
    // But this doesn't mean that the request line was valid, just that it was divided into substrings 
    // -------------------------------------------------------------------------------------------------
    if ((parse_requestline(bytes, size, &command, &server, &path, &protocol, &port)) == -1) 
    {
        fprintf(stderr, "[%ld] HTTP 400: Invalid request line\n", (long)getpid());
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Please provide a valid request line: [HTTP-Method] [PATH] [PROTOCOL]\n\0");
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
        // Get athlete by FISCODE
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_fiscode(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_FIRSTNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_ATHLETES_FIRSTNAME, API_SIZE) == 0)) 
    {
        // Get athlete by FIRSTNAME
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_firstname(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_LASTNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_ATHLETES_LASTNAME, API_SIZE) == 0)) 
    {
        // Get athlete by LASTNAME
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_lastname(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_FULLNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && (strncmp(path, GET_ATHLETES_FULLNAME, API_SIZE) == 0)) 
    {
        // Get athlete by FULLNAME
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


    // --------------------------------------------------------------------- 
    // The client request was not an API call
    // Attempt to find the requested resource if it's a GET request
    // --------------------------------------------------------------------- 
    if (strcmp(command, "GET") == 0)
    {
        // Create the file path the will be used to search for the requested resource
        char filepath[strlen(path) + (32 * sizeof(char))];
        strcpy(filepath, RESOURCE_PATH);
        if (path == 0 || (strlen(path) == 1 && path[0] == '/'))
            strcat(filepath, DEFAULT_FILE);
        else
            strcat(filepath, path);
        
        // Attempt to load and send back the requested resource    
        char* buffer = 0;
        int buffer_size = 0;
        int status_code = 500;  // Default status code on failure
        if (load_resource(filepath, &buffer, &buffer_size, &status_code) == -1) {
            send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Error: Could not find what you were looking for\n\0");
            if (buffer != 0) 
                free(buffer);
            return -1;
        }

        if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_HTML, buffer) == -1) {
            if (buffer != 0) 
                free(buffer);
            return -1;  
        }
            
        fprintf(stderr, "[%ld] HTTP 200: Found and sent the requested resource successfully!\n", (long)getpid());
        if (buffer != 0) 
            free(buffer);
        
        return 0;
    }


    // --------------------------------------------------------------------- 
    // Unknown HTTP Method
    // Send back a bad request response
    // --------------------------------------------------------------------- 
    fprintf(stderr, "[%ld] HTTP 400: Unknown HTTP method: %s\n", (long)getpid(), command);
    send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Unknown HTTP method\n\0");
    return -1;
}


