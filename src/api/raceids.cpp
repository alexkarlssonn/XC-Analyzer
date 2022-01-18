
#include "raceids.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../http-response.h"
#include "../libs/cJSON.h"


/**
 * -------------------------------------------------------------------------------------
 * Tries to find the list of raceids for the athlete with the given fiscode in the database 
 * If that athlete is found, the list of raceids will be sent back over socket in JSON format with an Http response
 * If not, an Http response will also be sent to indicate the error.
 * Or an empty array is sent if the athlete was found but no raceids was found for that athlete
 * The function also prints out messages that descibes the error before returning
 *
 * socket: The file descriptor that represents the socket to send the data over
 * fiscode: The fiscode for the requested athlete 
 *          Should be the parameter section in path that gets returned after calling parse_requestline
 *          It also needs to be a null terminated string
 * 
 * Returns 0 on success, to indicate that the athlete was found
 * Returns -1 on failure, to indicate that the athlete was not found, and that the error was sent over socket as an Http response 
 * -------------------------------------------------------------------------------------
 */
int api_getAthletesRaceids(int socket, char* fiscode)
{
    // -----------------------------------------------------------------
    // Validate the fiscode parameter
    // -----------------------------------------------------------------
    bool isValidFiscode = false;
    char* p = fiscode;
    while (*p != '\0') {
        isValidFiscode = true;
        if (!is_digit(*p)) {
            isValidFiscode = false;
            break;
        }
        p++;
    }
    if (!isValidFiscode)
    {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
        sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter\n\0");
        return -1;
    }
    
    // -----------------------------------------------------------------
    // Load the file that stores all races for each athlete
    // -----------------------------------------------------------------
    char file[] = DB_ATHLETES_RACES;
    char* buffer = 0;
    int status_code = 500;  // Default status code on failure
    if (loadResource(socket, file, &buffer, &status_code) == -1)
    {
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer != 0) free(buffer);
        return -1;
    }

    // ------------------------------------------------------------
    // Parse the loaded file into an JSON object
    // ------------------------------------------------------------
    cJSON* json = cJSON_Parse(buffer);
    if (buffer != 0) free(buffer);

    if (json == NULL)
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to parse JSON file\n", (long)getpid());
        sendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to parse file to JSON\n\0");
        cJSON_Delete(json);
        return -1;
    }

    
    // ------------------------------------------------------------
    // Try to find the requested athlete 
    // ------------------------------------------------------------
    char* result = NULL;
    cJSON* athlete = NULL;
    cJSON* athletes = cJSON_GetObjectItemCaseSensitive(json, "athletes");
    cJSON* fiscode_json = NULL;
    cJSON* races = NULL;

    cJSON_ArrayForEach(athlete, athletes)
    {
        fiscode_json = cJSON_GetObjectItemCaseSensitive(athlete, "fiscode");
        if (cJSON_IsString(fiscode_json) && fiscode_json->valuestring != NULL && fiscode != 0) 
        {
            if (strcmp(fiscode_json->valuestring, fiscode) == 0) 
            {
                races = cJSON_GetObjectItemCaseSensitive(athlete, "races");
                if (races != NULL) 
                {
                    // 204 (No Content): Athlete was found, but has no races
                    (cJSON_GetArraySize(races) > 0) ? status_code = 200 : status_code = 204;
                    result = cJSON_Print(races);
                }
                break;
            }
        }
    }
    cJSON_Delete(json);

    // ------------------------------------------------------------
    // Check if the requested athlete was found
    // ------------------------------------------------------------
    if (result == NULL)
    {
        fprintf(stderr, "[%ld] HTTP 404: Could not find the requested athlete\n", (long)getpid());
        sendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested athlete\n\0");
        return -1;
    }

    // ------------------------------------------------------------
    // Send back the athlete over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* result_modified = (char*) malloc((strlen(result) + 2) * sizeof(char));
    strcpy(result_modified, result);
    strcat(result_modified, "\n");  // Adds '\0' after
    if (result != 0) free(result);

    // status_code is 204 (No content) if the athlete was found, but no races exist (result_modified is "[]")
    // status_code is 200 (OK) if the athlete was found, and a list of races exist as well
    if (sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_JSON, result_modified) == -1)
    {
        if (result_modified != 0) free(result_modified);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the list of races for the requested athlete!\n", (long)getpid());
    if (result_modified != 0) free(result_modified);

    return 0;
}



