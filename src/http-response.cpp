
#include "http-response.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "api/athletes.h"
#include "api/raceids.h"
#include "api/races.h"
#include "libs/Restart.h"


/**
 * ----------------------------------------------------------------------------
 * Handle the received request from a connected client and sends back an Http Response
 * socket: The socket file descriptor connected to the client
 * response: The entire message that was received from the client
 * size: The size of the received response
 *
 * Returns 0 on success, which means that the clients request was valid, and handled as intended
 * Returns -1 on failure, which means that the clients request was not handled as intended for some reason 
 * ----------------------------------------------------------------------------
 */
int handleClient(int socket, char* response, int size)
{
    char* command;
    char* server;
    char* path;
    char* protocol;
    char* port;

    if ((parse_requestline(response, size, &command, &server, &path, &protocol, &port)) == -1) 
    {
        fprintf(stderr, "[%ld] HTTP 400: Invalid request line\n", (long)getpid());
        sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Please provide a valid request line: [HTTP-Method] [PATH] [PROTOCOL]\n\0");
        return -1;
    }
    
    // Assume the protocol is HTTP/1.1
    fprintf(stderr, "[%ld] Received HTTP Request from client: %s %s %s\n", (long)getpid(), command, path, protocol);


    // List of API calls
    int API_SIZE = LINE_MAX_SIZE;
    char GET_ATHLETE_FISCODE[] = "/api/athlete/fiscode/";
    char GET_ATHLETES_FIRSTNAME[] = "/api/athletes/firstname/";
    char GET_ATHLETES_LASTNAME[] = "/api/athletes/lastname/";
    char GET_ATHLETES_FULLNAME[] = "/api/athletes/fullname/";
    char GET_RACES_FISCODE[] = "/api/raceids/fiscode/";
    char GET_RACEINFO_RACEID[] = "/api/raceinfo/raceid/";
    char GET_RACE_RESULTS_RACEID[] = "/api/raceresults/raceid/";


    // ------------------------------------------------------------------ 
    // API: Get athletes
    // ------------------------------------------------------------------ 
    API_SIZE = strlen(GET_ATHLETE_FISCODE);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) &&
        (strncmp(path, GET_ATHLETE_FISCODE, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_fiscode(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_FIRSTNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) &&
        (strncmp(path, GET_ATHLETES_FIRSTNAME, API_SIZE) == 0)) 
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_firstname(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_LASTNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) &&
        (strncmp(path, GET_ATHLETES_LASTNAME, API_SIZE) == 0)) 
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_lastname(socket, parameter);
    }
    
    API_SIZE = strlen(GET_ATHLETES_FULLNAME);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) &&
        (strncmp(path, GET_ATHLETES_FULLNAME, API_SIZE) == 0)) 
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthlete_fullname(socket, parameter);
    }
    
    // ------------------------------------------------------------------ 
    // API: Get list of races for a given athlete
    // ------------------------------------------------------------------ 
    API_SIZE = strlen(GET_RACES_FISCODE);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && 
        (strncmp(path, GET_RACES_FISCODE, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getAthletesRaceids(socket, parameter);
    }
    
    // ------------------------------------------------------------------ 
    // API: Get race info and results
    // ------------------------------------------------------------------ 
    API_SIZE = strlen(GET_RACEINFO_RACEID);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && 
        (strncmp(path, GET_RACEINFO_RACEID, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getRaceInfo(socket, parameter);
    }

    API_SIZE = strlen(GET_RACE_RESULTS_RACEID);
    if ((strcmp(command, "GET") == 0) && (strlen(path) > API_SIZE) && 
        (strncmp(path, GET_RACE_RESULTS_RACEID, API_SIZE) == 0))
    {
        char* parameter = &(path[API_SIZE]);
        return api_getRaceResult(socket, parameter);
    }


    // ==============================================================
    // GET - Return the content of the requested resource
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
            sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
            if (buffer != 0) free(buffer);
            return -1;
        }

        if (sendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_HTML, buffer) == -1) 
        {
            if (buffer != 0) free(buffer);
            return -1;  
        }
            
        fprintf(stderr, "[%ld] HTTP 200: Found and sent the requested resource successfully!\n", (long)getpid());
        if (buffer != 0) free(buffer);
        
        return 0;
    }

    // ==============================================================
    // Unknown HTTP Method 
    // ==============================================================
    fprintf(stderr, "[%ld] HTTP 400: Unknown HTTP method: %s\n", (long)getpid(), command);
    sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Unknown HTTP method\n\0");
    
    return -1;
}


/**
 * ----------------------------------------------------------------------------
 * Constructs and sends an http response over the given socket
 * On failure, an error message will be printed and -1 will be returned.
 * Only the socket and status code are required by the function, the rest can be NULL / 0.
 * All parameters for setting the header lines needs to be null terminated to prevent unexpected errors
 *
 * socket: The file descriptor that represents the socket to send the http response over
 * connection: The connection type in the header. Should be a valid connection type.
 * type: The content type in the header. Should be a valid content type.
 * body: The actual body of the HTTP response.
 *
 * Returns 0 on success
 * Return -1 on failure 
 * ----------------------------------------------------------------------------
 */
int sendHttpResponse(int socket, int statuscode, const char* connection, const char* type, const char* body)
{
    if (statuscode < 100 || statuscode >= 600) {
        fprintf(stderr, "[%ld] Failed to send HTTP Response: Invalid status code\n", (long)getpid());
        return -1;
    }

    // Convert the status code into a string
    int digit3 = (statuscode / 100);
    int digit2 = ((statuscode - (digit3 * 100)) / 10);
    int digit1 = (statuscode - (digit3 * 100) - (digit2 * 10));
    char statuscode_str[5];
    statuscode_str[0] = (char) (((int)'0') + digit3);
    statuscode_str[1] = (char) (((int)'0') + digit2);
    statuscode_str[2] = (char) (((int)'0') + digit1);
    statuscode_str[3] = ' ';
    statuscode_str[4] = '\0';

    // Create the individual header lines
    char status_line[LINE_MAX_SIZE];
    strcpy(status_line, "HTTP/1.1 ");
    strcat(status_line, statuscode_str);
    if (statuscode == 200)
        strcat(status_line, "OK");
    else if (statuscode == 201)
        strcat(status_line, "Created");
    else if (statuscode == 202)
        strcat(status_line, "Accepted");
    else if (statuscode == 204)
        strcat(status_line, "No Content");
    else if (statuscode == 206)
        strcat(status_line, "Partial Content");
    else if (statuscode == 400)
        strcat(status_line, "Bad Request");
    else if (statuscode == 401)
        strcat(status_line, "Unauthorized");
    else if (statuscode == 403)
        strcat(status_line, "Forbidden");
    else if (statuscode == 404)
        strcat(status_line, "Not Found");
    else if (statuscode == 500)
        strcat(status_line, "Internal Server Error");
    else
        strcat(status_line, " No-Response-Phrase");
    strcat(status_line, "\r\n");

    char date[32];
    time_t time_now;
    struct tm ts;
    time_now = time(0);
    ts = *gmtime(&time_now);
    strftime(date, LINE_MAX_SIZE, "%a, %d %b %Y %H:%M:%S %Z", &ts);
    
    char date_line[LINE_MAX_SIZE];
    strcpy(date_line, "Date: ");
    strcat(date_line, date);
    strcat(date_line, "\r\n");

    char connection_line[LINE_MAX_SIZE];
    if (connection != 0) {
        strcpy(connection_line, "Connection: ");
        strcat(connection_line, connection);
        strcat(connection_line, "\r\n");
    }

    char type_line[LINE_MAX_SIZE];
    if (type != 0) {
        strcpy(type_line, "Content-Type: "); 
        strcat(type_line, type);
        strcat(type_line, "\r\n");
    }

    int status_line_size = strlen(status_line); 
    int date_line_size = strlen(date_line);
    int connection_line_size = 0; 
    int type_line_size = 0;
    if (connection != 0) 
        connection_line_size = strlen(connection_line);
    if (type != 0) 
        type_line_size = strlen(type_line);


    // Combine all the individual lines into a full HTTP Header
    int HEADER_SIZE = status_line_size + date_line_size + connection_line_size + type_line_size + (sizeof(char) * 3);    
    char header[HEADER_SIZE];
    strcpy(header, status_line);
    strcat(header, date_line);
    if (connection != 0) 
        strcat(header, connection_line);
    if (type != 0) 
        strcat(header, type_line);
    if (body != 0) 
        strcat(header, "\r\n");
    

    // Combine the header and body into a full HTTP Response
    int BODY_SIZE = 0;
    if (body != 0) 
        BODY_SIZE = strlen(body);
    int RESPONSE_SIZE = (HEADER_SIZE + BODY_SIZE);
    char http_response[RESPONSE_SIZE]; 
    strcpy(http_response, header);
    if (body != 0) 
        strcat(http_response, body); 

    // Send the full HTTP Response over the socket
    // Using (RESPONSE_SIZE - 1) for the size, since '\0' is not needed and actually prevents javascript files from working
    if (r_write(socket, http_response, RESPONSE_SIZE - 1) == -1) { 
        fprintf(stderr, "[%ld] Failed to send HTTP Response: r_write failed: %s\n", (long)getpid(), strerror(errno));
        return -1;
    }

    return 0;
}


/**
 * ----------------------------------------------------------------------------
 * Load the resource that is requested in the paremter path.
 * The content of the file will be allocated dynamically and stored in the buffer parameter. This needs to be manually freed later.
 * If the function fails, it will print an error message and return -1.
 *
 * socket: The file descriptor for the socket used to communicate with the connected client. 
 * path: The requested resource. Should be a null terminated string and be a full path relative to the executable.
 * buffer: A pointer to the buffer that will store the content of the loaded file. 
 *         Should be null when calling this function, and needs to be manually freed later.
 * status_code: The status code to send back with the Http Response if this function fails.
 *
 *  Returns 0 on success, and sets buffer to the content of the file
 *  Returns -1 on failure and prints an error message
 * ----------------------------------------------------------------------------
 */
int loadResource(int socket, char* path, char** buffer, int* status_code)
{
    int fd; 
    if ((fd = r_open2(path, O_RDONLY)) == -1) 
    {
        // Try to append .html if opening the file failed
        char newpath[strlen(path) + 8];
        strcpy(newpath, path);
        strcat(newpath, ".html");
        if ((fd = r_open2(newpath, O_RDONLY)) == -1) 
        {
            fprintf(stderr, "[%ld] Failed to open %s: %s\n", (long)getpid(), path, strerror(errno));
            *status_code = 404;
            return -1;
        }
    } 
    
    int filesize = (int) lseek(fd, 0, SEEK_END); 
    if ((int)lseek(fd, 0, SEEK_SET) == -1) 
    { 
        fprintf(stderr, "[%ld] Failed to move file offset back to beginning after getting the file size: %s\n", (long)getpid(), strerror(errno));
        if (r_close(fd) == -1) 
            fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));
        
        *status_code = 500;
        return -1;
    }
    
    char* bytes;
    if ((bytes = (char*) malloc( (filesize+1) * sizeof(char))) == 0) 
    {
        fprintf(stderr, "[%ld] Failed to allocate memory to read the file content\n", (long)getpid());
        if (r_close(fd) == -1) 
            fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));
        
        *status_code = 500;
        return -1;
    }
    
    int bytesread;
    if ((bytesread = readblock(fd, bytes, filesize)) <= 0) 
    {
        fprintf(stderr, "[%ld] Failed to read the requested file: %s: %s\n", (long)getpid(), path, strerror(errno));
        if (r_close(fd) == -1) 
            fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));

        *status_code = 500;
        return -1;
    }
    
    if (r_close(fd) == -1)
        fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));

    bytes[bytesread] = '\0';
    *buffer = bytes;  // The buffer needs to be manually freed later

    return 0;
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


/**
 * ----------------------------------------------------------------------------
 * Checks if a given character is a digit or not
 *  Returns 1 if true,
 *  Returns 0 if false
 * ----------------------------------------------------------------------------
 */
int is_digit(char ch)
{
    if ((ch == '0') || (ch == '1') || 
        (ch == '2') || (ch == '3') || 
        (ch == '4') || (ch == '5') ||  
        (ch == '6') || (ch == '7') || 
        (ch == '8') || (ch == '9')) 
        return 1;
    return 0;
}


