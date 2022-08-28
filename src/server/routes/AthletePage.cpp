
#include "Routes.h"

#include "../Server.h"
#include "../../LoadFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * ------------------------------------------------------------------------
 * The route for serving the page for a specific athlete
 *
 * socket: The file descriptor the client is connected over
 * request: The object containing the client request
 *
 * Returns 0 if the request was responded to correctly
 * That means that even if the request was invalid but responded to correctly, the function will still return 0
 * Returns -1 on failure. The request could not be handled and responded to correctly
 * ------------------------------------------------------------------------
 */
int Route_AthletePage(int socket, Request* request)
{
    if (strcmp(request->method, "GET") != 0)
    {
        SendHttpResponse(socket, 405, CONNECTION_CLOSE, TYPE_HTML, "Method Not Allowed: The resource was found but the method is not allowed");
        fprintf(stderr, "[%ld] Method Not Allowed: The request path was valid: %s, but the method is not allowed: %s\n", (long)getpid(), request->path, request->method);
        return 0;
    }



    char* file = 0;
    char FILE_INDEX[] = "./resources/athlete/index.html";
    char FILE_STYLE[] = "./resources/athlete/style.css";
    char FILE_MAIN[] = "./resources/athlete/main.js";
    
    if (strcmp(request->path, "/athlete") == 0) {
        file = &(FILE_INDEX[0]);
    }
    else if (strcmp(request->path, "/athlete/style.css") == 0) {
        file = &(FILE_STYLE[0]);
    }
    else if (strcmp(request->path, "/athlete/main.js") == 0) {
        file = &(FILE_MAIN[0]);
    }
    else {
        SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "Not Found: The requested resource was not found");
        fprintf(stderr, "[%ld] Not Found: The path given by the client was not found: %s\n", (long)getpid(), request->path);
        return 0;  // Return 0 since the client request was technically handled
    }
    
    char* buffer = 0;
    int buffer_size = 0;
    if (LoadFile(file, &buffer, &buffer_size) < 0) 
    {
        if (buffer) { free(buffer); }
        SendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "Server Error: Failed to load the request resource");
        fprintf(stderr, "[%ld] Server Error: Failed to load the request resource: %s\n", (long)getpid(), request->path);
        return 0;  // Return 0 since the client request was technically handled
    }

    SendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_HTML, buffer);
    fprintf(stderr, "[%ld] OK: The requested resource (%s) was found and sent back to the client\n", (long)getpid(), request->path);
    return 0;
}














