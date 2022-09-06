
#include "Server.h"

#include "./routes/Routes.h"
#include "../util/StringUtil.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>


/**
 * --------------------------------------------------------------------------------------------
 * Handles the request received from the client and response to the client  over the socket
 *
 * socket: The file descriptor the client is connected over
 * request: The object containing the client request
 *
 * Returns 0 on success
 * Returns -1 when failing to handle and/or responding to the request correctly
 * --------------------------------------------------------------------------------------------
 */
int HandleClientRequest(int socket, Request* request)
{
    // Validate the protocol before handling the request
    if (strcmp(request->protocol, "HTTP/1.1") != 0 )
    {
        SendHttpResponse(socket, 505, CONNECTION_CLOSE, TYPE_HTML, "HTTP Version Not Supported: Only HTTP/1.1 is supported by the server");
        fprintf(stderr, "[%ld] Invalid Request: HTTP Version Not Supported: %s. Only HTTP/1.1 is supported by the server\n", (long)getpid(), request->protocol);
        return 0;  // Return 0 since the client request was technically handled
    }


    // ----------------------------------
    // Routes
    // ----------------------------------
    // Homepage
    if (strcmp(request->path, "/") == 0 || strcmp(request->path, "/main.js") == 0)
    {
        return Route_HomePage(socket, request);
    }
    if (strcmp(request->path, "/athlete") == 0)  // || strcmp(request->path, "/athlete/style.css") == 0 || strcmp(request->path, "/athlete/main.js") == 0)
    {
        return Route_AthletePage(socket, request);
    }
    if (strcmp(request->path, "/race") == 0)
    {
        return Route_RacePage(socket, request);
    }
    if (does_str_begin_with(request->path, (char*)"/api/"))
    {
        return Route_Api(socket, request);
    }


    // Not Found: Invalid path
    SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "Not Found: The requested resource was not found");
    fprintf(stderr, "[%ld] Not Found: The path given by the client was not found: %s\n", (long)getpid(), request->path);
    return 0;  // Return 0 since the client request was technically handled
}





