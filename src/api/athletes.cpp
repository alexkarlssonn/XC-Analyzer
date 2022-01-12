
#include "athletes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../http-response.h"
#include "../libs/cJSON.h"


/**
 * -------------------------------------------------------------------------------------
 * Tries to find the athlete with the given fiscode in the database 
 * If found it will be sent in JSON format with an Http Response. If not, an Http Response will also be sent to indicate the error
 * The function also prints out messages that descibes the error before returning
 *
 * socket: The file descriptor that represents the socket to send the data over
 * fiscode: The fiscode for the requested athlete. It needs to be a null terminated string, and not be NULL when thsi function gets called
 * 
 * Returns 0 on success, to indicate that the athlete was found and sent over socket as an Http Response
 * Returns -1 on failure, to indicate that the athlete was not found, and that the error was sent over socket as an Http Response 
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete(int socket, char* fiscode)
{
    char file[] = DB_ATHLETES;
    char* buffer = 0;
    int status_code = 500;
    char* error_message = (char*) malloc(LINE_MAX_SIZE * sizeof(char));

    if (loadResource(socket, file, &buffer, &status_code, &error_message) == -1)
    {
        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        if (error_message != 0)
            strcpy(body, error_message);

        fprintf(stderr, "[%ld] Sending HTTP Response %d to client\n", (long)getpid(), status_code);
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, &body);

        if (body != 0) free(body);
        if (error_message != 0) free(error_message);
        if (buffer != 0) free(buffer);

        return -1;
    }

    cJSON* json = cJSON_Parse(buffer);
    if (buffer != 0) free(buffer);

    if (json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
            fprintf(stderr, "[%ld] Failed to parse JSON file. Error before: %s\n", (long)getpid(), error_ptr);
        else
            fprintf(stderr, "[%ld] Failed to parse JSON file\n", (long)getpid());

        status_code = 500;
        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "500 Internal Server Error: Failed to parse the file to JSON\n");

        fprintf(stderr, "[%ld] Sending HTTP Response %d to client\n", (long)getpid(), status_code);
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, &body);

        if (body != 0) free(body);
        cJSON_Delete(json);

        return -1;
    }

    char* result = NULL;
    const cJSON* athlete = NULL;
    const cJSON* athletes = NULL;
    
    athletes = cJSON_GetObjectItemCaseSensitive(json, "athletes");
    cJSON_ArrayForEach(athlete, athletes)
    {
        cJSON* fiscode_json = cJSON_GetObjectItemCaseSensitive(athlete, "fiscode");
        if (cJSON_IsString(fiscode_json)) {
            if (strcmp(fiscode_json->valuestring, fiscode) == 0) {
                result = cJSON_Print(athlete);
                break;
            }
        }
    }

    if (result == NULL)
    {
        fprintf(stderr, "[%ld] Could not find the athlete with the given fiscode\n", (long)getpid());

        status_code = 404;
        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "404 Not Found: Could not find the athlete with the given fiscode\n");

        fprintf(stderr, "[%ld] Sending HTTP Response %d to client\n", (long)getpid(), status_code);
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, &body);

        if (body != 0) free(body);
        cJSON_Delete(json);

        return -1;
    }

    if (sendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_JSON, &result) == -1)
    {
        if (result != 0) free(result);
        cJSON_Delete(json);
        
        return -1;
    }

    fprintf(stderr, "[%ld] Successfully sent HTTP Response to client\n", (long)getpid());
    
    if (result != 0) free(result);
    cJSON_Delete(json);

    return 0;
}




