
#include "Routes.h"

#include "../Server.h"
#include "../../LoadFile.h"  // Remove later
#include "../../pages/CreatePages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


int Route_AthletePage(int socket, Request* request)
{
    // ---------------------------------------------------
    // Validate the method used in the request
    // ---------------------------------------------------
    if (strcmp(request->method, "GET") != 0)
    {
        SendHttpResponse(socket, 405, CONNECTION_CLOSE, TYPE_HTML, "Method Not Allowed: The resource was found but the method is not allowed");
        fprintf(stderr, "[%ld] Method Not Allowed: The request path was valid: %s, but the method is not allowed: %s\n", (long)getpid(), request->path, request->method);
        return 0;
    }


    // ---------------------------------------------------
    // Get the fiscode from the request query
    // ---------------------------------------------------
    int fiscode = 0;
    if (request->query) {
        fiscode = atoi(request->query);
    }
    if (fiscode <= 0)
    {
        SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "Not Found: Invalid fiscode");
        fprintf(stderr, "[%ld] Not Found: Failed to create athlete page. Invalid fiscode was given in the query parameter: %s\n", (long)getpid(), request->query);
        return 0;
    }


    // ---------------------------------------------------
    // Create the athlete page for the given fiscode
    // ---------------------------------------------------
    char* PageBuffer = 0;
    int PageBuffer_size = 0;
    int res = 0;
    if ((res = CreatePage_Athlete(fiscode, &PageBuffer, &PageBuffer_size)) < 0)
    {
        if (res == -2) {
            SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "Not Found: The requested fiscode could not be found");
            fprintf(stderr, "[%ld] Not Found: Failed to create the athlete page since the requested fiscode could not be found: %s\n", (long)getpid(), request->query);
        }
        else {
            SendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "Server Error: Failed to create the requested resource");
            fprintf(stderr, "[%ld] Server Error: Failed to create the athlete page for the requested fiscode: %s\n", (long)getpid(), request->query);
        }
        if (PageBuffer) { free(PageBuffer); }
        return 0; 
    }

    SendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_HTML, PageBuffer);
    fprintf(stderr, "[%ld] OK: The requested athlete page for (%s) was created and sent back to the client\n", (long)getpid(), request->query);
    if (PageBuffer) { free(PageBuffer); }
    
    return 0;
}





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
int OLD_OLD_OLD_Route_AthletePage(int socket, Request* request)
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














