
#include "Routes.h"

#include "../../db/Database.h"
#include <stdlib.h>

#include "../Server.h"
#include "../../util/StringUtil.h"
#include "../../api/api.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>


/**
 * ------------------------------------------------------------------------
 * Used for the API calls when an invalid HTTP method is used
 * Sends an "405 Method Not Allowed" HTTP Response over the given socket and prints an error message
 *
 * socket: The file descriptor the client is connected over
 * Returns 0 once the response has been sent and the message printed
 * ------------------------------------------------------------------------
 */
static int SendAndPrint_MethodNotAllowed(int socket, Request* request)
{
    SendHttpResponse(socket, 405, CONNECTION_CLOSE, TYPE_HTML, "Method Not Allowed: The API call exist but the method is not allowed");
    fprintf(stderr, "[%ld] Method Not Allowed: The request path was valid: %s, but the method is not allowed: %s\n", (long)getpid(), request->path, request->method);
    return 0;
}


/**
 * ------------------------------------------------------------------------
 * The route for handling an api call
 *
 * socket: The file descriptor the client is connected over
 * request: The object containing the client request
 *
 * Returns 0 if the request was responded to correctly
 * That means that even if the request was invalid but responded to correctly, the function will still return 0
 * Returns -1 on failure. The request could not be handled and responded to correctly
 * ------------------------------------------------------------------------
 */
int Route_Api(int socket, Request* request)
{
    // -------------------------------------------------------------------
    // Athlete by fiscode
    // -------------------------------------------------------------------
    char ATHLETE_FISCODE[] = "/api/athlete/fiscode/"; 
    if (does_str_begin_with(request->path, &(ATHLETE_FISCODE[0]))) 
    {
        if (strcmp(request->method, "GET") == 0)
        {
            char* fiscode = &(request->path[strlen(ATHLETE_FISCODE)]);
            return api_getAthlete_fiscode(socket, fiscode);
        }
        else 
        {
            return SendAndPrint_MethodNotAllowed(socket, request);
        }
    }

    // -------------------------------------------------------------------
    // Athletes by lastname
    // -------------------------------------------------------------------
    char ATHLETE_FIRSTNAME[] = "/api/athletes/firstname/"; 
    if (does_str_begin_with(request->path, &(ATHLETE_FIRSTNAME[0]))) 
    {
        if (strcmp(request->method, "GET") == 0)
        {
            char* firstname = &(request->path[strlen(ATHLETE_FIRSTNAME)]);
            return api_getAthlete_firstname(socket, firstname);
        }
        else 
        {
            return SendAndPrint_MethodNotAllowed(socket, request);
        }
    }

    // -------------------------------------------------------------------
    // Athletes by lastname
    // -------------------------------------------------------------------
    char ATHLETE_LASTNAME[] = "/api/athletes/lastname/"; 
    if (does_str_begin_with(request->path, &(ATHLETE_LASTNAME[0]))) 
    {
        if (strcmp(request->method, "GET") == 0)
        {
            char* lastname = &(request->path[strlen(ATHLETE_LASTNAME)]);
            return api_getAthlete_lastname(socket, lastname);
        }
        else 
        {
            return SendAndPrint_MethodNotAllowed(socket, request);
        }
    }
    
    // -------------------------------------------------------------------
    // Athletes by fullname
    // -------------------------------------------------------------------
    char ATHLETE_FULLNAME[] = "/api/athletes/fullname/"; 
    if (does_str_begin_with(request->path, &(ATHLETE_FULLNAME[0]))) 
    {
        if (strcmp(request->method, "GET") == 0)
        {
            char* fullname = &(request->path[strlen(ATHLETE_FULLNAME)]);
            return api_getAthlete_fullname(socket, fullname);
        }
        else 
        {
            return SendAndPrint_MethodNotAllowed(socket, request);
        }
    }

    // -------------------------------------------------------------------
    // Raceids for an athlete by fiscode
    // -------------------------------------------------------------------
    char RACEIDS_FISCODE[] = "/api/raceids/fiscode/"; 
    if (does_str_begin_with(request->path, &(RACEIDS_FISCODE[0]))) 
    {
        if (strcmp(request->method, "GET") == 0)
        {
            char* fiscode = &(request->path[strlen(RACEIDS_FISCODE)]);
            return api_getAthletesRaceids(socket, fiscode);
        }
        else 
        {
            return SendAndPrint_MethodNotAllowed(socket, request);
        }
    }
    
    // -------------------------------------------------------------------
    // Race info by raceid
    // -------------------------------------------------------------------
    char RACEINFO_RACEID[] = "/api/raceinfo/raceid/"; 
    if (does_str_begin_with(request->path, &(RACEINFO_RACEID[0]))) 
    {
        if (strcmp(request->method, "GET") == 0)
        {
            char* raceid = &(request->path[strlen(RACEINFO_RACEID)]);
            return api_getRaceInfo(socket, raceid);
        }
        else 
        {
            return SendAndPrint_MethodNotAllowed(socket, request);
        }
    }

    // -------------------------------------------------------------------
    // Race results by raceid
    // -------------------------------------------------------------------
    char RACE_RESULTS_RACEID[] = "/api/raceresults/raceid/"; 
    if (does_str_begin_with(request->path, &(RACE_RESULTS_RACEID[0]))) 
    {
        if (strcmp(request->method, "GET") == 0)
        {
            char* raceid = &(request->path[strlen(RACE_RESULTS_RACEID)]);
            return api_getRaceResult(socket, raceid);
        }
        else 
        {
            return SendAndPrint_MethodNotAllowed(socket, request);
        }
    }

    // -------------------------------------------------------------------
    // Analyzed sprint qualifications by fiscode
    // -------------------------------------------------------------------
    char ANALYZE_QUAL_FISCODE[] = "/api/analyze/qual/fiscode/"; 
    if (does_str_begin_with(request->path, &(ANALYZE_QUAL_FISCODE[0]))) 
    {
        if (strcmp(request->method, "GET") == 0)
        {
            char* fiscode = &(request->path[strlen(ANALYZE_QUAL_FISCODE)]);
            return api_getAnalyzedResults_qual(socket, fiscode);
        }
        else 
        {
            return SendAndPrint_MethodNotAllowed(socket, request);
        }
    }
    

    // -------------------------------------------------------------------
    // Not Found: Invalid API call
    // -------------------------------------------------------------------
    SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "Not Found: The requested API call was not found");
    fprintf(stderr, "[%ld] Not Found: The api call given by the client was not found: %s\n", (long)getpid(), request->path);
    return 0;
}




