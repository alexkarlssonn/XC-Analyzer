
#include "Request.h"

#include "Response.h"
#include "./pages/CreatePages.h"
#include "./api/api.h"
#include "./libs/Restart.h"
#include "./util/StringUtil.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


/**
 * ---------------------------------------------------------------------------------------
 * Checks if a string starts with another substring
 *
 * str1: The original string to check
 * str2: The substring to check if str1 start with
 *
 * Returns true if str1 starts with str2, and false if not
 * ---------------------------------------------------------------------------------------
 */
/*
bool does_str_begin_with(char* str1, char* str2)
{
    int str1_size = strlen(str1);
    int str2_size = strlen(str2);
    if (str1_size >= str2_size) {
        if (strncmp(str1, str2, str2_size) == 0) {
            return true;
        }
    }
    return false;
}
*/


/**
 * -------------------------------------------------------------------------------------------
 * Reads the message from the given socket and parses the expected http request
 *
 * socket: The file desscriptor that the client is connected over
 * request: The structure that will contain the relevant request data on success
 *
 * Returns 0 on success and sets the parameter "request" to contain the data that was parsed from the client message
 * Returns -1 on failure, prints an error message and sends an http response back to the client over the given "socket"
 * -------------------------------------------------------------------------------------------
 */
int read_and_parse_request(int socket, Request* request)
{
    char* buffer = 0;
    if ((buffer = (char*) malloc(sizeof(char) * REQUEST_MAX_SIZE)) == 0) 
    {
        send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Server Error: Failed read request\n\0");
        fprintf(stderr, "[%ld] Failed to allocate memory for client request\n", (long)getpid());
        return -1;
    }

    int size = 0;
    if ((size = r_read(socket, buffer, REQUEST_MAX_SIZE-1)) == -1) 
    {
        send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Server Error: Failed read request\n\0");
        fprintf(stderr, "[%ld] Failed to read client request to buffer\n", (long)getpid());
        if (buffer) {
            free(buffer);
        }
        return -1;
    }
    buffer[size] = '\0';

    char* command;
    char* server;
    char* path;
    char* protocol;
    char* port;
    if (parse_requestline(buffer, size, &command, &server, &path, &protocol, &port) == -1) 
    {
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Please provide a valid request line: [HTTP-METHOD] [PATH] [PROTOCOL]\n\0");
        fprintf(stderr, "[%ld] Failed to parse request line from client request\n", (long)getpid());
        if (buffer) {
            free(buffer);
        }
        return -1;
    }


    if (request->method == 0 && does_str_begin_with(command, (char*)"GET")) {
        request->method = HTTP_GET;
    }
    else if (request->method == 0 && does_str_begin_with(command, (char*)"POST")) {
        request->method = HTTP_POST;
    }
    else if (request->method == 0 && does_str_begin_with(command, (char*)"PUT")) {
        request->method = HTTP_PUT;
    }
    else if (request->method == 0 && does_str_begin_with(command, (char*)"DELETE")) {
        request->method = HTTP_DELETE;
    }
    else 
    {
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Please provide a valid HTTP-Method\n\0");
        fprintf(stderr, "[%ld] Invalid request. Invalid http method: %s\n", (long)getpid(), command);
        if (buffer) {
            free(buffer);
        }
        return -1;
    }


    if (does_str_begin_with(path, (char*)"/api/"))
    {
        if (request->type == 0 && does_str_begin_with(path, (char*)"/api/athlete/fiscode/")) {
            request->parameter_start = strlen(                     "/api/athlete/fiscode/");
            request->type = API_ATHLETE_FISCODE;
            strncpy(request->path, path, PATH_MAX_SIZE);
        }
        else if (request->type == 0 && does_str_begin_with(path, (char*)"/api/athletes/firstname/")) {
            request->parameter_start = strlen(                          "/api/athletes/firstname/");
            request->type = API_ATHLETES_FIRSTNAME;
            strncpy(request->path, path, PATH_MAX_SIZE);
        }
        else if (request->type == 0 && does_str_begin_with(path, (char*)"/api/athletes/lastname/")) {
            request->parameter_start = strlen(                          "/api/athletes/lastname/");
            request->type = API_ATHLETES_LASTNAME;
            strncpy(request->path, path, PATH_MAX_SIZE);
        }
        else if (request->type == 0 && does_str_begin_with(path, (char*)"/api/athletes/fullname/")) {
            request->parameter_start = strlen(                          "/api/athletes/fullname/");
            request->type = API_ATHLETES_FULLNAME;
            strncpy(request->path, path, PATH_MAX_SIZE);
        }
        else if (request->type == 0 && does_str_begin_with(path, (char*)"/api/raceids/fiscode/")) {
            request->parameter_start = strlen(                          "/api/raceids/fiscode/");
            request->type = API_RACEIDS_FISCODE;
            strncpy(request->path, path, PATH_MAX_SIZE);
        }
        else if (request->type == 0 && does_str_begin_with(path, (char*)"/api/raceinfo/raceid/")) {
            request->parameter_start = strlen(                          "/api/raceinfo/raceid/");
            request->type = API_RACEINFO_RACEID;
            strncpy(request->path, path, PATH_MAX_SIZE);
        }
        else if (request->type == 0 && does_str_begin_with(path, (char*)"/api/raceresults/raceid/")) {
            request->parameter_start = strlen(                          "/api/raceresults/raceid/");
            request->type = API_RACERESULTS_RACEID;
            strncpy(request->path, path, PATH_MAX_SIZE);
        }
        else if (request->type == 0 && does_str_begin_with(path, (char*)"/api/analyze/qual/fiscode/")) {
            request->parameter_start = strlen(                          "/api/analyze/qual/fiscode/");
            request->type = API_ANALYZE_QUAL_FISCODE;
            strncpy(request->path, path, PATH_MAX_SIZE);
        }
        else 
        {
            send_http_response(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: The requested api call does not exist\n\0");
            fprintf(stderr, "[%ld] Invalid request. The API request does not exist: %s\n", (long)getpid(), path);
            if (buffer) {
                free(buffer);
            }
            return -1;
        } 
    }
    else if (request->method == HTTP_GET)
    {
        char* filepath = 0;
        if ((filepath = (char*) malloc(sizeof(char) * PATH_MAX_SIZE)) == 0) 
        {
            send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Server Error: Failed to parse the request\n\0");
            fprintf(stderr, "[%ld] Failed to allocate memory for the requested filepath: %s\n", (long)getpid(), path);
            if (buffer) {
                free(buffer);
            }
            return -1;
        }

        
        fprintf(stderr, "[%ld] path: %s\n", (long)getpid(), path);

        // Map the requested path to a physical resource
        strcpy(filepath, RESOURCE_PATH);
       
        // Homepage 
        if (path != 0 && strcmp(path, "/") == 0) {
            strcat(filepath, "/homepage/index.html");    
            request->type = GET_RESOURCE;
        }
        else if (path != 0 && strcmp(path, "/main.js") == 0) {
            strcat(filepath, "/homepage/main.js");    
            request->type = GET_RESOURCE;
        }
        else if (path != 0 && strcmp(path, "/style.css") == 0) {
            strcat(filepath, "/homepage/style.css");    
            request->type = GET_RESOURCE;
        }
        // Athlete page
        else if (path != 0 && strcmp(path, "/athlete/style.css") == 0) {
            strcat(filepath, "/athlete/style.css");
            request->type = GET_RESOURCE;
        }
        else if (path != 0 && strcmp(path, "/athlete/main.js") == 0) {
            strcat(filepath, "/athlete/main.js");
            request->type = GET_RESOURCE;
        }
        else if (path != 0 && strncmp(path, "/athlete/", 9) == 0) {
            strcat(filepath, "/athlete/index.html");
            request->type = GET_RESOURCE;
        }
        // Race page
        else if (path != 0 && strncmp(path, "/race/", 6) == 0) {
            strcpy(filepath, &(path[5]));
            request->type = GET_TEMPLATE;
        }
        /*
        // Race page
        else if (path != 0 && strcmp(path, "/race/style.css") == 0) {
            strcat(filepath, "/race/style.css");
        } 
        else if (path != 0 && strcmp(path, "/race/main.js") == 0) {
            strcat(filepath, "/race/main.js");
        }
        else if (path != 0 && strncmp(path, "/race/", 6) == 0) {
            strcat(filepath, "/race/index.html");
        }
        */
        // Resource not found page
        else {
            strcat(filepath, NOT_FOUND);
        }

        strncpy(request->path, filepath, PATH_MAX_SIZE);
        if (filepath) {
            free(filepath);
        }
    }
    else
    {
        send_http_response(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: The requested resource does not exist\n\0");
        fprintf(stderr, "[%ld] Invalid request. No request exist for: %s %s\n", (long)getpid(), command, path);
        if (buffer) {
            free(buffer);
        }
        return -1;
    }


    if (buffer) {
        free(buffer);
    }
    return 0;
}


/**
 * ------------------------------------------------------------------------------------------------
 * Handles the given client request and sends back an http response
 *
 * socket: The file desscriptor that the client is connected over
 * request: The structure that contains the request data
 *
 * Returns 0 on success. The request was handled and responded to successfully
 * Returns -1 on failure, prints an error message that describes the error and sends back an error response over the socket
 * ------------------------------------------------------------------------------------------------
 */
int handle_request(int socket, Request* request)
{
    if (request->type == GET_RESOURCE && request->method == HTTP_GET)
    {
        char* buffer = 0;
        int buffer_size = 0;
        int status_code = 500;  // Default status code on failure
        if (load_resource(request->path, &buffer, &buffer_size, &status_code) == -1) 
        {
            if (status_code == 404) {
                fprintf(stderr, "[%ld] HTTP 404: The requested resource does not exist: %s\n", (long)getpid(), request->path);
                send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: The requested resource does not exist\n\0");
            } else {
                fprintf(stderr, "[%ld] HTTP 500: Failed to find the requested resource: %s\n", (long)getpid(), request->path);
                send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "500 Server Error: Failed to find the requested resource\n\0");    
            }
            if (buffer != 0) 
                free(buffer);
            return -1;
        }

        int return_value = -1;
        if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_HTML, buffer) == 0) {
            fprintf(stderr, "[%ld] HTTP 200: Found and sent the requested resource successfully: %s\n", (long)getpid(), request->path);
            return_value = 0;
        }

        if (buffer != 0) 
            free(buffer);
        return return_value;
    }
    
    // TEMPLATES
    else if (request->type == GET_TEMPLATE && request->method == HTTP_GET)
    {
        if (strlen(request->path) < 2) {
            return -1;
        }

        /*
        bool isNumber = false;
        char* p = &(request->path[1]);
        while (*p != '\0') {
            if (is_digit(*p)) {
                isNumber = true;
            } else {
                isNumber = false;
                break;
            }
        }

        if (!isNumber) {
            return -1;
        }
        */

        int raceid = atoi(&(request->path[1]));
        if (raceid == 0) {
            return 0;
        }


        char* PageBuffer = 0;
        int PageBuffer_size = 0;
        if (CreatePage_RaceResults(raceid, &PageBuffer, &PageBuffer_size) == -1) 
        {
            fprintf(stderr, "[%ld] HTTP 500: Failed to create the requested resource for race page: %s\n", (long)getpid(), request->path);
            send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Server Error: Failed to create the requested resource\n\0");    
            if (PageBuffer) { free(PageBuffer); }
            return -1;
        }
        
        int return_value = -1;
        if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_HTML, PageBuffer) == 0) {
            fprintf(stderr, "[%ld] HTTP 200: Found and sent the requested resource successfully: %s\n", (long)getpid(), request->path);
            return_value = 0;
        }

        if (PageBuffer) { free(PageBuffer); }
        return (return_value);
    }

    // API REQUESTES
    else if (request->type == API_ATHLETE_FISCODE && request->method == HTTP_GET)
    {
        char* parameter = &(request->path[request->parameter_start]);
        return api_getAthlete_fiscode(socket, parameter);        
    }
    else if (request->type == API_ATHLETES_FIRSTNAME && request->method == HTTP_GET)
    {
        char* parameter = &(request->path[request->parameter_start]);
        return api_getAthlete_firstname(socket, parameter);        
    }
    else if (request->type == API_ATHLETES_LASTNAME && request->method == HTTP_GET)
    {
        char* parameter = &(request->path[request->parameter_start]);
        return api_getAthlete_lastname(socket, parameter);        
    }
    else if (request->type == API_ATHLETES_FULLNAME && request->method == HTTP_GET)
    {
        char* parameter = &(request->path[request->parameter_start]);
        return api_getAthlete_fullname(socket, parameter);        
    }
    else if (request->type == API_RACEIDS_FISCODE && request->method == HTTP_GET)
    {
        char* parameter = &(request->path[request->parameter_start]);
        return api_getAthletesRaceids(socket, parameter);        
    }
    else if (request->type == API_RACEINFO_RACEID && request->method == HTTP_GET)
    {
        char* parameter = &(request->path[request->parameter_start]);
        return api_getRaceInfo(socket, parameter);        
    }
    else if (request->type == API_RACERESULTS_RACEID && request->method == HTTP_GET)
    {
        char* parameter = &(request->path[request->parameter_start]);
        return api_getRaceResult(socket, parameter);        
    }
    else if (request->type == API_ANALYZE_QUAL_FISCODE && request->method == HTTP_GET)
    {
        char* parameter = &(request->path[request->parameter_start]);
        return api_getAnalyzedResults_qual(socket, parameter);        
    }

    fprintf(stderr, "[%ld] Invalid request: The request does not exist\n", (long)getpid());
    send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid request\n\0");
    return -1;
}


/**
 * ----------------------------------------------------------------------------
 * Parses the initial HTTP request line
 *  Returns 0 on success which means atleast the following 3 tokens was parsed and set: command, path and protocol
 *  Returns -1 on failure
 *  
 *  The first two parameters whould be the line and it's length. The line should end with a newline character.
 *  The other parameters will represent the different tokens (substrings) inside line, once it has been parsed.
 *  These parameters are pointers that will point to the start of their respective tokens. 
 *  Each of these tokens will be set to end with a '\0' character once line has been parsed.
 *  
 *  The server and port tokens are optional, and will be set to 0 if they are not found.
 *  The command, path and protocol tokens will also be set to 0 if the functions fails.
 * ----------------------------------------------------------------------------
 */
int parse_requestline(char* line, int len, char** command, char** server, char** path, char** protocol, char** port)
{
    *command = 0;
    *server = 0;
    *path = 0;
    *protocol = 0;
    *port = 0;
    
    // =======================================================================
    // COMMAND, PATH and PROTOCOL TOKEN
    // =======================================================================
    int tokens = 0;
    bool newline = false;
    bool creatingToken = false;

    for (int i = 0; i < len; i++) {
        if (line[i] == '\n')
            newline = true;
        
        // Start creating a token if a new string was detected, and no token is currently being created
        if (!creatingToken && (line[i] != ' ') && (line[i] != '\n') && (line[i] != '\r') && (line[i] != '\t')) {
            creatingToken = true;
            if (tokens == 0) 
                *command = &(line[i]);
            else if (tokens == 1) 
                *path = &(line[i]);
            else if (tokens == 2) 
                *protocol = &(line[i]);
        }
        // Finish creating the current token if the end of a string is detected 
        else if (creatingToken && (line[i] == ' ' || line[i] == '\n' || line[i] == '\r' || line[i] == '\t')) {
            creatingToken = false;
            line[i] = '\0';
            tokens++;
        }
        
        if (newline) 
            break;
    } 

    // Parsing failed if not excatly 3 tokens was created successfully
    if (tokens != 3 || creatingToken) {
        *command = 0;
        *path = 0;
        *protocol = 0;
        return -1;
    }

    // =======================================================================
    // SERVER TOKEN (optional)
    // =======================================================================
    char* p = *path;
    int counter = 0;
    bool startsWithHttp = true;
    bool serverTokenCreated = false;
    bool creatingPortToken = false;

    while (*p != '\0' && startsWithHttp)
    {
        if (counter == 0 && *p != 'h') startsWithHttp = false;
        if (counter == 1 && *p != 't') startsWithHttp = false;
        if (counter == 2 && *p != 't') startsWithHttp = false;
        if (counter == 3 && *p != 'p') startsWithHttp = false;
        if (counter == 4 && *p != ':') startsWithHttp = false;
        if (counter == 5 && *p != '/') startsWithHttp = false;
        if (counter == 6 && *p != '/') startsWithHttp = false;
        
        // Move every character back one step if the path-strings starts with "http://"
        // Do this until the server token has been created, and also set the server pointer to start after "http://"
        if (counter > 6 && startsWithHttp) {
            if (*server == 0)
                *server = (p - sizeof(char));
            if (!serverTokenCreated) 
                *(p - sizeof(char)) = *p;
        }
       
        // If server token is being created and a '/' char was found, then set it to a null-character which marks the end of the server token
        // Also update the path token to start after the server token
        if (counter > 6 && startsWithHttp && *p == '/') {
            if (!serverTokenCreated) {
                *(p - sizeof(char)) = '\0'; 
                serverTokenCreated = true;
                *path = p;
            }
        }
        
        p++;
        counter++;
    }

    if (*server != 0 && !serverTokenCreated)
        *server = 0;

    // =======================================================================
    // PORT TOKEN (optional)
    // =======================================================================
    if (*server != 0) {
        p = *server;
        while (*p != '\0') {    
            // Clear the port token if a non-digit shows up during creation
            // Also set the port token to the digit after the first ':' character
            if (*port != 0 && !is_digit(*p))
                *port = 0;
            if (*p == ':' && *port == 0 && is_digit(*(p + sizeof(char)))) 
                *port = (p + sizeof(char));
            p++;
        }

        // If a port was given, then the server token should end before the port
        if (*port != 0) {
            p = *server;
            while (*p != '\0') {
                if (*p == ':')
                    *p = '\0';
                p++;
            }
        }
    }

    return 0;
}






