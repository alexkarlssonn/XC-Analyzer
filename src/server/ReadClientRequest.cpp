
#include "Server.h"

#include "../libs/Restart.h"
#include "../util/StringUtil.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int ParseClientRequest(Request* request);


/**
 * ------------------------------------------------------------------------------------------------
 * Reads the message sent by the client over the socket, and then parses it into a Request object
 * If an error occurs, an error message will be printed, and all allocated memory will be freed
 *
 * socket: The file descirptor the client is connected over
 * request: A pointer to a Request object that will contain the parsed request once it has been read and validated
 *
 * Returns 0 on success, and -1 on failure
 * ------------------------------------------------------------------------------------------------
 */
int ReadClientRequest(int socket, Request* request)
{
    // Allocate memory for the request
    if ((request->buffer = (char*) malloc(REQUEST_MAX_SIZE * sizeof(char))) == 0)
    {
        SendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "Server Error: Failed to read request");
        fprintf(stderr, "[%ld] Failed to allocate memory for client request\n", (long)getpid());
        return -1;
    }


    // Read the message from the client
    if ((request->buffer_size = r_read(socket, request->buffer, REQUEST_MAX_SIZE - 1)) == -1)
    {
        SendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "Server Error: Failed to read request");
        if (request->buffer) { free(request->buffer); }
        fprintf(stderr, "[%ld] Failed to read client request to buffer\n", (long)getpid());
        return -1;
    }
    request->buffer[request->buffer_size] = '\0';


    // Parse the request
    if (ParseClientRequest(request) == -1)
    {
        SendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "Bad Request: Invalid request, failed to parse the request line");
        if (request->buffer) { free(request->buffer); }
        fprintf(stderr, "[%ld] Failed to parse the request line from the client\n", (long)getpid());
        return -1;
    }

    return 0;
}


/**
 * ------------------------------------------------------------------------------------------------
 * Parses the request from the client
 * This is done by creating substrings inside the buffer containing the message
 * The substrings is represented by pointers stored inside the Request object
 *
 * request: A pointer to the Request object containing the buffer, and the pointers to the substrings that will be created
 *
 * No error messages gets printed inside this function
 * Returns 0 on success, and -1 on failure. 
 * ------------------------------------------------------------------------------------------------
 */
static int ParseClientRequest(Request* request)
{
    // -------------------------------------------------------------------
    // Find and set the HEADERS and BODY
    // -------------------------------------------------------------------
    for (int i = 0; i < request->buffer_size; i++)
    {
        // Make sure the current and the next byte can be read
        if (i + 1 < request->buffer_size)
        {
            bool CRLF_found = (request->buffer[i] == '\r' && request->buffer[++i] == '\n');
            if (CRLF_found)
            {
                i++;
                // If neither the headers nor body has been set, then set the start of the headers after this CRLF
                if (request->headers == 0 && request->body == 0 && i < request->buffer_size)
                {
                    request->headers = &(request->buffer[i]);
                }
                // If the headers has been set but not the body
                else if (request->headers != 0 && request->body == 0 && (i + 1 < request->buffer_size)) 
                {
                    CRLF_found = (request->buffer[i] == '\r' && request->buffer[++i] == '\n');
                    if (CRLF_found)
                    {
                        // If another CRLF was found (two in a row), set a '\0' to mark the end of the headers section
                        // Also set the start of the body after the two CRLF
                        request->buffer[i] = '\0';
                        i++;
                        if (i < request->buffer_size) {
                            request->body = &(request->buffer[i]);
                        }
                    }
                }

            }
        }
    }


    // -------------------------------------------------------------------
    // Find the required tokens: METHOD, PATH and PROTOCOL
    // -------------------------------------------------------------------
    int tokens = 0;
    bool newline = false;
    bool creatingToken = false;

    for (int i = 0; i < request->buffer_size; i++) {
        if (request->buffer[i] == '\n')
            newline = true;

        // Start creating a token if a new string was detected, and no token is currently being created
        if (!creatingToken && (request->buffer[i] != ' ') && (request->buffer[i] != '\n') && (request->buffer[i] != '\r') && (request->buffer[i] != '\t')) 
        {
            creatingToken = true;
            if (tokens == 0) {
                request->method = &(request->buffer[i]);
            }
            else if (tokens == 1) {
                request->path = &(request->buffer[i]);
            }
            else if (tokens == 2) {
                request->protocol = &(request->buffer[i]);
            }
        }
        // Finish creating the current token if the end of a string is detected
        else if (creatingToken && (request->buffer[i] == ' ' || request->buffer[i] == '\n' || request->buffer[i] == '\r' || request->buffer[i] == '\t')) {
            creatingToken = false;
            request->buffer[i] = '\0';
            tokens++;
        }

        if (newline) {
            break;
        }
    }

    // Parsing failed if not excatly 3 tokens was created successfully
    if (tokens != 3 || creatingToken) {
        request->method = 0;
        request->path = 0;
        request->protocol = 0;
        return -1;
    }


    // -------------------------------------------------------------------
    // Find optional token: SERVER 
    // -------------------------------------------------------------------
    char* p = request->path;
    int counter = 0;
    bool startsWithHttp = true;
    bool serverTokenCreated = false;
    bool creatingPortToken = false;

    while (*p != '\0' && startsWithHttp)
    {
        // HTTP method has to be used
        if (counter == 0 && *p != 'h') startsWithHttp = false;
        if (counter == 1 && *p != 't') startsWithHttp = false;
        if (counter == 2 && *p != 't') startsWithHttp = false;
        if (counter == 3 && *p != 'p') startsWithHttp = false;
        if (counter == 4 && *p != ':') startsWithHttp = false;
        if (counter == 5 && *p != '/') startsWithHttp = false;
        if (counter == 6 && *p != '/') startsWithHttp = false;

        // Move every character back one step if the path-strings starts with "http://"
        // Do this until the server token has been created, and also set the server pointer to start after "http://"
        if (counter > 6 && startsWithHttp) 
        {
            if (request->server == 0) {
                request->server = (p - sizeof(char));
            }
            if (!serverTokenCreated) {
                *(p - sizeof(char)) = *p;
            }
        }

        // If server token is being created and a '/' char was found, then set it to a null-character which marks the end of the server token
        // Also update the path token to start after the server token
        if (counter > 6 && startsWithHttp && *p == '/') {
            if (!serverTokenCreated) {
                *(p - sizeof(char)) = '\0';
                serverTokenCreated = true;
                request->path = p;
            }
        }

        p++;
        counter++;
    }

    if (request->server != 0 && !serverTokenCreated) {
        request->server = 0;
    }


    // -------------------------------------------------------------------
    // Find optional token: PORT
    // -------------------------------------------------------------------
    if (request->server != 0) 
    {
        p = request->server;
        while (*p != '\0') 
        {
            // Clear the port token if a non-digit shows up during creation
            // Also set the port token to the digit after the first ':' character
            if (request->port != 0 && !is_digit(*p)) {
                request->port = 0;
            }
            if (*p == ':' && request->port == 0 && is_digit(*(p + sizeof(char)))) {
                request->port = (p + sizeof(char));
            }
            p++;
        }

        // If a port was given, then the server token should end before the port
        if (request->port != 0) 
        {
            p = request->server;
            while (*p != '\0') {
                if (*p == ':')
                    *p = '\0';
                p++;
            }
        }
    }

    // -------------------------------------------------------------------
    // PATH
    // -------------------------------------------------------------------
    if (request->path != 0) 
    {
        p = request->path; 
        while (*p != '\0')
        {
            // If '?' is found, that menas the start of the query has been found
            // Replace it with '\0' and set the start of the query to right after
            if (request->query == 0 && *p == '?') {
                *p = '\0';
                p++;
                if (*p != '\0') {
                    request->query = p;
                    break;
                }
            }
            p++;
        } 
    }


    return 0;

}







