
#include "athletes.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../http-response.h"
#include "../libs/cJSON.h"


/**
 * -------------------------------------------------------------------------------------
 * Tries to find the athlete with the given fiscode in the database 
 * If that athlete is found it will be sent back in JSON format with an Http Response
 * If not, an Http Response will also be sent to indicate the error
 * The function also prints out messages that descibes the error before returning
 *
 * socket: The file descriptor that represents the socket to send the data over
 * fiscode: The fiscode for the requested athlete 
 *          Should be the parameter section in path that gets returned after calling parse_requestline
 *          It also needs to be a null terminated string
 * 
 * Returns 0 on success, to indicate that the athlete was found
 * Returns -1 on failure, to indicate that the athlete was not found, and that the error was sent over socket as an Http Response 
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_fiscode(int socket, char* fiscode)
{
    // -----------------------------------------------------------------
    // Validate the fiscode parameter
    // -----------------------------------------------------------------
    bool isValidFiscode = false;
    char* p = fiscode;
    while (*p != '\0') {
        isValidFiscode = true;
        if (!isdigit(*p)) {
            isValidFiscode = false;
            break;
        }
        p++;
    }

    if (!isValidFiscode)
    {
        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "400 Bad Request: Invalid fiscode parameter\n");

        fprintf(stderr, "[%ld] Api call failed: Invalid fiscode parameter. Sending HTTP Response 400: Bad Request\n", (long)getpid());
        sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, &body);

        if (body != 0) 
            free(body);
        
        return -1;
    }
    
    // -----------------------------------------------------------------
    // Load the file that stores all athletes 
    // -----------------------------------------------------------------
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

    // ------------------------------------------------------------
    // Parse the loaded file into an JSON object
    // ------------------------------------------------------------
    cJSON* json = cJSON_Parse(buffer);
    if (error_message != 0) free(error_message);
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
        strcpy(body, "500 Internal Server Error: Failed to parse the requested data to JSON\n");

        fprintf(stderr, "[%ld] Sending HTTP Response %d to client\n", (long)getpid(), status_code);
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, &body);

        if (body != 0) free(body);
        cJSON_Delete(json);
        return -1;
    }

    // ------------------------------------------------------------
    // Try to find the requested athlete 
    // ------------------------------------------------------------
    char* result = NULL;
    const cJSON* athlete = NULL;
    const cJSON* athletes = NULL;
    
    athletes = cJSON_GetObjectItemCaseSensitive(json, "athletes");
    cJSON_ArrayForEach(athlete, athletes)
    {
        cJSON* fiscode_json = cJSON_GetObjectItemCaseSensitive(athlete, "fiscode");
        if (cJSON_IsString(fiscode_json) && fiscode != 0) {
            if (strcmp(fiscode_json->valuestring, fiscode) == 0) {
                result = cJSON_Print(athlete);
                break;
            }
        }
    }

    // ------------------------------------------------------------
    // Check if the requested athlete was found
    // ------------------------------------------------------------
    if (result == NULL)
    {
        fprintf(stderr, "[%ld] Could not find the athlete with the given fiscode\n", (long)getpid());

        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "404 Not Found: Could not find the athlete with the given fiscode\n");

        status_code = 404;
        fprintf(stderr, "[%ld] Sending HTTP Response %d to client\n", (long)getpid(), status_code);
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, &body);

        if (body != 0) free(body);
        cJSON_Delete(json);
        return -1;
    }

    // ------------------------------------------------------------
    // Send back the athlete over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* result_modified = (char*) malloc((strlen(result) + 2) * sizeof(char));
    strcpy(result_modified, result);
    strcat(result_modified, "\n");
    if (result != 0) free(result);

    if (sendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_JSON, &result_modified) == -1)
    {
        if (result_modified != 0) free(result_modified);
        cJSON_Delete(json);
        return -1;
    }

    fprintf(stderr, "[%ld] Successfully sent HTTP Response 200 to client\n", (long)getpid());
    
    if (result_modified != 0) free(result_modified);
    cJSON_Delete(json);

    return 0;
}

/**
 * -------------------------------------------------------------------------------------
 * Tries to find athletes in the database that has firstnames that matches the given parameter
 * If any matching athletes are found they will be sent as an array in JSON format with an Http Response
 * If not, an Http Response will also be sent to indicate the error
 * The function also prints out messages that describes the error before returning on failure
 *
 * socket: The file descriptor that represents the socket to send the data over
 * firstname: The search string used to find matching firstnames from all athletes
 *            Should be the parameter section in path that gets returned after calling parse_requestline.
 *            It also needs to be a null terminated string.
 * 
 * Returns 0 on success, to indicate that one or more athletes was found 
 * Returns -1 on failure, to indicate that no athlete was not found, and that the error was sent over socket as an Http Response 
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_firstname(int socket, char* firstname)
{
    // -----------------------------------------------------------------
    // Validate the firstname parameter
    // -----------------------------------------------------------------
    bool isValidName = false;
    char* p = firstname;
    int firstname_size = strlen(firstname);
    fprintf(stderr, "API: GET FIRSTNAME\nParameter: %s, Length: %d\n", firstname, firstname_size);
    
    while (*p != '\0') {
        isValidName = true;
        if (*p == '/') {
            isValidName = false;
            break;
        }
        p++;
    }

    if (!isValidName)
    {
        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "400 Bad Request: Invalid firstname parameter\n");
        
        fprintf(stderr, "[%ld] Api call failed: Invalid firstname parameter. Sending HTTP Response 400: Bad Request\n", (long)getpid());
        sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, &body);
        
        if (body != 0) 
            free(body);
        
        return -1;
    }

    // Convert the parameter to lower case
    char firstname_lower[strlen(firstname)];
    strcpy(firstname_lower, firstname);
    for (int i = 0; i < strlen(firstname); i++)
        firstname_lower[i] = tolower(firstname_lower[i]);


    fprintf(stderr, "Parameter: %s, LowerCase: %s\n", firstname, firstname_lower);


    
    // -----------------------------------------------------------------
    // Load the file that stores all athletes 
    // -----------------------------------------------------------------
    char file[] = DB_ATHLETES;
    int status_code = 500;
    char* error_message = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
    char* buffer = 0;

    if (loadResource(socket, file, &buffer, &status_code, &error_message) == -1)
    {
        char* body = 0; 
        if (error_message != 0) {
            body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
            strcpy(body, error_message);
        }

        fprintf(stderr, "[%ld] Sending HTTP Response %d to client\n", (long)getpid(), status_code);
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, &body);
        
        if (body != 0) 
            free(body);
        if (error_message != 0) 
            free(error_message);
        if (buffer != 0) 
            free(buffer);
        
        return -1;
    }

    // ------------------------------------------------------------
    // Parse the loaded file into an JSON object
    // ------------------------------------------------------------
    cJSON* json = cJSON_Parse(buffer);
    cJSON* result_athletes = cJSON_CreateObject();
    cJSON* result_array = cJSON_CreateArray();

    if (error_message != 0) 
        free(error_message);
    if (buffer != 0) 
        free(buffer);

    if (json == NULL || result_athletes == NULL || result_array == NULL)
    {
        status_code = 500;
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
            fprintf(stderr, "[%ld] Failed to parse JSON file. Error before: %s\n", (long)getpid(), error_ptr);
        else
            fprintf(stderr, "[%ld] Failed to parse JSON file\n", (long)getpid());

        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "500 Internal Server Error: Failed to parse the requested data to JSON\n");

        fprintf(stderr, "[%ld] Sending HTTP Response %d to client\n", (long)getpid(), status_code);
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, &body);

        if (body != 0) 
            free(body);
        cJSON_Delete(json);  // TODO: Delete more cJSON objects here (if that is necessary)
        //cJSON_Delete(result_athletes);
        
        return -1;
    }
    cJSON_AddItemToObject(result_athletes, "athletes", result_array);  


    // ------------------------------------------------------------
    // Try to find any athletes that has matching firstnames 
    // ------------------------------------------------------------
    char* result = NULL;
    char copy[LINE_MAX_SIZE];
    int counter = 0;
    cJSON* athletes = cJSON_GetObjectItemCaseSensitive(json, "athletes");
    cJSON* athlete = NULL;
    cJSON* field_firstname = NULL;


    //
    // TODO: Can't get this to work... I want to create a new array with all matching athletes
    // Try to maybe remove all non-matching athletes from the array?
    //


    cJSON_ArrayForEach(athlete, athletes)
    {
        field_firstname = cJSON_GetObjectItemCaseSensitive(athlete, "firstname");
        
        if ((cJSON_IsString(field_firstname)) && (field_firstname->valuestring != NULL)) 
        {
            // Convert the firstname field to lower case and see if it matches the given parameter
            strcpy(copy, field_firstname->valuestring);
            for (int i = 0; i < strlen(copy); i++) 
                copy[i] = tolower(copy[i]);

            if (strncmp(copy, firstname_lower, firstname_size) == 0) {
                cJSON_AddItemToArray(result_array, athlete);
                counter++;
            }
        }
    }

    result = cJSON_Print(result_athletes);
    cJSON_Delete(json);


    // ------------------------------------------------------------
    // Check if any athletes was found
    // ------------------------------------------------------------
    if (counter <= 0 || result == NULL)
    {
        status_code = 404;
        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "404 Not Found: Could not find any athletes for the given firstname\n");
        
        fprintf(stderr, "[%ld] Could not find any athletes for the given firstname\n", (long)getpid());
        fprintf(stderr, "[%ld] Sending HTTP Response %d to client\n", (long)getpid(), status_code);
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, &body);

        if (body != 0) 
            free(body);
        
        return -1;
    }

    // ------------------------------------------------------------
    // Send back the athletes over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* result_modified = (char*) malloc((strlen(result) + 2) * sizeof(char));
    strcpy(result_modified, result);
    strcat(result_modified, "\n");
    if (result != 0) 
        free(result);

    if (sendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_JSON, &result_modified) == -1) 
    {
        if (result_modified != 0) 
            free(result_modified);
        return -1;
    }

    fprintf(stderr, "[%ld] Successfully sent HTTP Response 200 to client\n", (long)getpid());
    
    if (result_modified != 0) 
        free(result_modified);
    return 0;
}


/**
 * -------------------------------------------------------------------------------------
 * Tries to find athletes in the database that has lastnames that matches the given parameter
 * If any matching athletes are found they will be sent as an array in JSON format with an Http Response
 * If not, an Http Response will also be sent to indicate the error
 * The function also prints out messages that describes the error before returning on failure
 *
 * socket: The file descriptor that represents the socket to send the data over
 * lastname: The search string used to find matching lastnames from all athletes
 *           Should be the parameter section in path that gets returned after calling parse_requestline.
 *           It also needs to be a null terminated string.
 * 
 * Returns 0 on success, to indicate that one or more athletes was found
 * Returns -1 on failure, to indicate that no athlete was not found, and that the error was sent over socket as an Http Response 
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_lastname(int socket, char* lastname)
{
    // -----------------------------------------------------------------
    // Validate the lastname parameter
    // -----------------------------------------------------------------
    bool isValidName = false;
    char* p = lastname;
    while (*p != '\0') {
        isValidName = true;
        if (*p == '/') {
            isValidName = false;
            break;
        }
        p++;
    }
    
    if (!isValidName)
    {
        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "400 Bad Request: Invalid lastname parameter\n");

        fprintf(stderr, "[%ld] Api call failed: Invalid lastname parameter. Sending HTTP Response 400: Bad Request\n", (long)getpid());
        sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, &body);
        
        if (body != 0) free(body);
        return -1;
    }

    return 0;
}


/**
 * -------------------------------------------------------------------------------------
 * Tries to find athletes in the database that has names that matches the given parameter
 * If any matching athletes are found they will be sent as an array in JSON format with an Http Response
 * If not, an Http Response will also be sent to indicate the error
 * The function also prints out messages that describes the error before returning on failure
 *
 * socket: The file descriptor that represents the socket to send the data over
 * fullname: Should contain the firstname and lastname seperated by a '\' character
 *           The first- and lastname are used to find matching names from all athletes
 *           Should be the parameter section in path that gets returned after calling parse_requestline.
 *           It also needs to be a null terminated string.
 * 
 * Returns 0 on success, to indicate that one or more athletes was found
 * Returns -1 on failure, to indicate that no athlete was not found, and that the error was sent over socket as an Http Response 
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_fullname(int socket, char* fullname)
{
    // -----------------------------------------------------------------
    // Validate the fullname parameter
    // -----------------------------------------------------------------
    
    // Check if the parameter has two names
    bool hasTwoNames = false;
    char* p = fullname;
    while (*p != '\0') {
        if (*p == '/') {
            hasTwoNames = true;
            p++;
            break;
        }
        p++;
    }

    // Split the names
    bool isValidNames = false;
    char* firstname;
    char* lastname;
    if (hasTwoNames) 
    {
        lastname = p;
        firstname = fullname;    
        p = fullname;

        while (*p != '\0' && *p != '/') {
            if (*p == '/') {
                *p = '\0';
                isValidNames = true;
                break;
            }
            p++;
        }
    } 
    
    //
    // TODO: REMOVE LATER - Valid api call: /api/athletes/fullname/FIRSTNAME/LASTNAME
    //
    if (isValidNames) {
        fprintf(stderr, "TWO NAMES FOUND ***** Firstname: %s, Lastname: %s\n", firstname, lastname);
    } else {
        fprintf(stderr, "FALSE **** isValidName is false, Invalid parameter: %s\n", fullname);
    }

    if (!isValidNames)
    {
        char* body = (char*) malloc(LINE_MAX_SIZE * sizeof(char));
        strcpy(body, "400 Bad Request: Invalid fullname parameter, use: '/api/athletes/FIRSTNAME/LASTNAME'\n");

        fprintf(stderr, "[%ld] Api call failed: Invalid fullname parameter. Sending HTTP Response 400: Bad Request\n", (long)getpid());
        sendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, &body);
        
        if (body != 0) free(body);
        return -1;
    }

    return 0;
}



