
#include "Routes.h"

#include "../Server.h"
#include "../../pages/CreatePages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * ------------------------------------------------------------------------
 * The route for serving the race page
 *
 * socket: The file descriptor the client is connected over
 * request: The object containing the client request
 *
 * Returns 0 if the request was responded to correctly
 * That means that even if the request was invalid but responded to correctly, the function will still return 0
 * Returns -1 on failure. The request could not be handled and responded to correctly
 * ------------------------------------------------------------------------
 */
int Route_RacePage(int socket, Request* request)
{
    // ----------------------------------------------
    // Validate the method used in the request
    // ----------------------------------------------
    if (strcmp(request->method, "GET") != 0)
    {
        SendHttpResponse(socket, 405, CONNECTION_CLOSE, TYPE_HTML, "Method Not Allowed: The resource was found but the method is not allowed");
        fprintf(stderr, "[%ld] Method Not Allowed: The request path was valid: %s, but the method is not allowed: %s\n", (long)getpid(), request->path, request->method);
        return 0;
    }

    
    // ----------------------------------------------
    // Get the race id from the request query
    // ----------------------------------------------
    int raceid = 0;
    if (request->query) {
        raceid = atoi(&(request->query[0]));
    }
    if (raceid <= 0)
    {
        SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "Not Found: Invalid race id");
        fprintf(stderr, "[%ld] Not Found: Failed to create race page. Invalid race id was given in the query parameter: %s\n", (long)getpid(), request->query);
        return 0;
    }

    
    // ----------------------------------------------
    // Create the race page for the given race id
    // ----------------------------------------------
    char* PageBuffer = 0;
    int PageBuffer_size = 0;
    int res = 0;
    if ((res = CreatePage_RaceResults(raceid, &PageBuffer, &PageBuffer_size)) < 0)
    {
        if (res == -2) {
            SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "Not Found: The requested race id could not be found");
            fprintf(stderr, "[%ld] Not Found: Failed to create the race page since the requested raceid could not be found: %s\n", (long)getpid(), request->query);
        }
        else {
            SendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "Server Error: Failed to create the requested resource");
            fprintf(stderr, "[%ld] Server Error: Failed to create the race page for the requested raceid: %s\n", (long)getpid(), request->query);
        }
        if (PageBuffer) { free(PageBuffer); }
        return 0;
    }


    SendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_HTML, PageBuffer);
    fprintf(stderr, "[%ld] OK: The requested race page for (%s) was created and sent back to the client\n", (long)getpid(), request->query);
    if (PageBuffer) { free(PageBuffer); }
    
    return 0;
}







