

#include "api.h"

#include "../handle_client/load_resource.h"
#include "../handle_client/send_http_response.h"
#include "../libs/cJSON.h"
#include "../util/StringUtil.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * -------------------------------------------------------------------------------------
 * Tries to find the athlete with the given fiscode in the database 
 * If that athlete is found it will be sent back over socket in JSON format with an Http response
 * If not, an Http response will also be sent to indicate the error
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
int api_getAthlete_fiscode(int socket, char* fiscode)
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
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter\n\0");
        return -1;
    }
    
    // -----------------------------------------------------------------
    // Load the file that stores all athletes 
    // -----------------------------------------------------------------
    char file[] = DB_ATHLETES;
    char* buffer = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(socket, file, &buffer, &status_code) == -1)
    {
        send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
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
        // const char* error_ptr = cJSON_GetErrorPtr();
        fprintf(stderr, "[%ld] HTTP 500: Failed to parse JSON file\n", (long)getpid());
        send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to parse file to JSON\n\0");
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

    cJSON_ArrayForEach(athlete, athletes)
    {
        fiscode_json = cJSON_GetObjectItemCaseSensitive(athlete, "fiscode");
        if (cJSON_IsString(fiscode_json) && fiscode_json->valuestring != NULL && fiscode != 0) 
        {
            if (strcmp(fiscode_json->valuestring, fiscode) == 0) {
                result = cJSON_Print(athlete);
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
        send_http_response(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested athlete\n\0");
        return -1;
    }

    // ------------------------------------------------------------
    // Send back the athlete over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* result_modified = (char*) malloc((strlen(result) + 2) * sizeof(char));
    strcpy(result_modified, result);
    strcat(result_modified, "\n");  // Adds '\0' after
    if (result != 0) free(result);

    if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_JSON, result_modified) == -1)
    {
        if (result_modified != 0) free(result_modified);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the requested athlete!\n", (long)getpid());
    if (result_modified != 0) free(result_modified);

    return 0;
}


/**
 * -------------------------------------------------------------------------------------
 * Api call for searching the database for athletes that has matching firstnames
 * See "getAthletesByName" for more details
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_firstname(int socket, char* firstname)
{
    return getAthletesByName(socket, "firstname", firstname);
}


/**
 * -------------------------------------------------------------------------------------
 * Api call for searching the database for athletes that has matching lastnames
 * See "getAthletesByName" for more details
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_lastname(int socket, char* lastname)
{
    return getAthletesByName(socket, "lastname", lastname);
}


/**
 * -------------------------------------------------------------------------------------
 * Tries to find athletes in the database that has a firstname/lastnames that matches the given name in the parameter "name"
 * If any matching athletes are found they will be sent over socket as an array in JSON format with an Http response
 * If not, an Http response will also be sent to indicate the error
 * The function also prints out messages that describes the error before returning on failure
 *
 * socket: The file descriptor that represents the socket to send the data over
 * FIELD_NAME: The name of the field inside each athlete object, that the function compares "name" with
 * name: The search string used to find athletes with matching names for the field: FIELD_NAME
 *       Should be the parameter section in path that gets returned after calling parse_requestline.
 *       It also needs to be a null terminated string.
 * 
 * Returns 0 on success, to indicate that one or more athletes was found
 * Returns -1 on failure, to indicate that no athlete was not found, and that the error was sent over socket as an Http response 
 * -------------------------------------------------------------------------------------
 */
int getAthletesByName(int socket, const char* FIELD_NAME, char* name)
{
    // -----------------------------------------------------------------
    // Validate the parameter
    // -----------------------------------------------------------------
    if (name == 0)
    {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter\n\0");
        return -1;
    }

    // Convert the parameter to lower case
    int name_size = strlen(name);
    char name_lower[name_size];
    strcpy(name_lower, name);
    for (int i = 0; i < name_size; i++)
        name_lower[i] = tolower(name_lower[i]);

    // -----------------------------------------------------------------
    // Load the file that stores all athletes 
    // -----------------------------------------------------------------
    char file[] = DB_ATHLETES;
    char* buffer = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(socket, file, &buffer, &status_code) == -1)
    {
        send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer != 0) free(buffer);
        return -1;
    }

    // ------------------------------------------------------------
    // Parse the loaded file into an JSON object
    // ------------------------------------------------------------
    cJSON* json = cJSON_Parse(buffer);
    cJSON* result_athletes = cJSON_CreateObject();
    cJSON* result_array = cJSON_CreateArray();

    if (buffer != 0) 
        free(buffer);

    if (json == NULL || result_athletes == NULL || result_array == NULL)
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to parse JSON file\n", (long)getpid());
        send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to parse file to JSON\n\0");
        cJSON_Delete(json);
        return -1;
    }
    
    cJSON_AddItemToObject(result_athletes, "athletes", result_array);  

    // ------------------------------------------------------------
    // Try to find any athletes that has matching firstnames 
    // ------------------------------------------------------------
    char current[LINE_MAX_SIZE];
    int counter = 0;
    char* result = NULL;
    cJSON* athlete = NULL;
    cJSON* athletes = cJSON_GetObjectItemCaseSensitive(json, "athletes");
    cJSON* field_value = NULL;
    
    cJSON_ArrayForEach(athlete, athletes)
    {
        field_value = cJSON_GetObjectItemCaseSensitive(athlete, FIELD_NAME);
        if ((cJSON_IsString(field_value)) && (field_value->valuestring != NULL)) 
        {
            // Convert the field to lower case and see if it matches the given parameter
            strcpy(current, field_value->valuestring);  // Adds '\0' at the end
            for (int i = 0; i < strlen(current); i++) 
                current[i] = tolower(current[i]);
            
            if (strncmp(current, name_lower, name_size) == 0) {
                cJSON_AddItemReferenceToArray(result_array, athlete);
                counter++;
            }
        }
    }

    result = cJSON_Print(result_athletes);
    cJSON_Delete(json);
    cJSON_Delete(result_athletes);

    // ------------------------------------------------------------
    // Check if any athletes was found
    // ------------------------------------------------------------
    if (counter <= 0 || result == 0)
    {
        fprintf(stderr, "[%ld] HTTP 404: Could not find the requested athlete\n", (long)getpid());
        send_http_response(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested athlete\n\0");
        if (result != 0) free(result);
        return -1;
    }

    // ------------------------------------------------------------
    // Send back the athletes over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* result_modified = (char*) malloc((strlen(result) + 2) * sizeof(char));
    strcpy(result_modified, result);
    strcat(result_modified, "\n");  // Adds '\0' at the end
    if (result != 0) free(result);

    if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_JSON, result_modified) == -1) 
    {
        if (result_modified != 0) free(result_modified);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the results successfully!\n", (long)getpid());
    if (result_modified != 0) free(result_modified);
    
    return 0;
}


/**
 * -------------------------------------------------------------------------------------
 * Tries to find athletes in the database that has names that matches the given parameter
 * If any matching athletes are found they will be sent over socket as an array in JSON format with an Http response
 * If not, an Http response will also be sent to indicate the error
 * The function also prints out messages that describes the error before returning on failure
 *
 * socket: The file descriptor that represents the socket to send the data over
 * fullname: Should contain the firstname and lastname seperated by a '\' character
 *           The first- and lastname are used to find matching names from all athletes
 *           Should be the parameter section in path that gets returned after calling parse_requestline.
 *           It also needs to be a null terminated string.
 * 
 * Returns 0 on success, to indicate that one or more athletes was found
 * Returns -1 on failure, to indicate that no athlete was not found, and that the error was sent over socket as an Http response 
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_fullname(int socket, char* fullname)
{
    // -----------------------------------------------------------------
    // Check if fullname has to names seperated by a '/' 
    // -----------------------------------------------------------------
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

    // -----------------------------------------------------------------
    // Split the two names into a first- and lastname 
    // -----------------------------------------------------------------
    bool isValidNames = false;
    char* firstname;
    char* lastname;
    if (hasTwoNames) 
    {
        // Set the substring for the lastname
        lastname = p;
        
        // Replace the name delimiter '/' with '\0' so a substring for fistname can be created
        firstname = fullname;    
        p = fullname;
        if (*p != '/') {
            do {
                p++;
                if (*p == '/') {
                    *p = '\0';
                    isValidNames = true;
                    break;
                }
            } while (*p != '\0' && *p != '/');
        }

        // Check so the lastname is not empty
        if (lastname[0] == '\0')
            isValidNames = false;
    } 

    // -----------------------------------------------------------------
    // Check if the given parameter is valid names 
    // -----------------------------------------------------------------
    if (!isValidNames)
    {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter\n\0");
        return -1;
    }

    // Convert the firstname to lower case
    int firstname_size = strlen(firstname);
    char firstname_lower[firstname_size];
    strcpy(firstname_lower, firstname);
    for (int i = 0; i < firstname_size; i++)
        firstname_lower[i] = tolower(firstname_lower[i]);
    
    // Convert the lastname to lower case
    int lastname_size = strlen(lastname);
    char lastname_lower[lastname_size];
    strcpy(lastname_lower, lastname);
    for (int i = 0; i < lastname_size; i++)
        lastname_lower[i] = tolower(lastname_lower[i]);

    
    // -----------------------------------------------------------------
    // Load the file that stores all athletes 
    // -----------------------------------------------------------------
    char file[] = DB_ATHLETES;
    char* buffer = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(socket, file, &buffer, &status_code) == -1)
    {
        send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer != 0) free(buffer);
        return -1;
    }

    // ------------------------------------------------------------
    // Parse the loaded file into an JSON object
    // ------------------------------------------------------------
    cJSON* json = cJSON_Parse(buffer);
    cJSON* result_athletes = cJSON_CreateObject();
    cJSON* result_array = cJSON_CreateArray();

    if (buffer != 0) 
        free(buffer);

    if (json == NULL || result_athletes == NULL || result_array == NULL)
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to parse JSON file\n", (long)getpid());
        send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to parse file to JSON\n\0");
        cJSON_Delete(json);
        return -1;
    }
    
    cJSON_AddItemToObject(result_athletes, "athletes", result_array);  

    // ------------------------------------------------------------
    // Try to find any athletes that has matching firstnames 
    // ------------------------------------------------------------
    char current_firstname[LINE_MAX_SIZE];
    char current_lastname[LINE_MAX_SIZE];
    int counter = 0;
    char* result = NULL;
    cJSON* athlete = NULL;
    cJSON* athletes = cJSON_GetObjectItemCaseSensitive(json, "athletes");
    cJSON* firstname_field = NULL;
    cJSON* lastname_field = NULL;
    
    cJSON_ArrayForEach(athlete, athletes)
    {
        firstname_field = cJSON_GetObjectItemCaseSensitive(athlete, "firstname");
        lastname_field = cJSON_GetObjectItemCaseSensitive(athlete, "lastname");
        
        if ((cJSON_IsString(firstname_field)) && (firstname_field->valuestring != NULL) &&
            (cJSON_IsString(lastname_field)) && (lastname_field->valuestring != NULL)) 
        {
            // Convert both current name fields to lowercase
            strcpy(current_firstname, firstname_field->valuestring);  // Adds '\0' at the end
            for (int i = 0; i < strlen(current_firstname); i++) 
                current_firstname[i] = tolower(current_firstname[i]);
            
            strcpy(current_lastname, lastname_field->valuestring);  // Adds '\0' at the end
            for (int i = 0; i < strlen(current_lastname); i++) 
                current_lastname[i] = tolower(current_lastname[i]);
            
            // If both fields matches the given parameters, then add the athlete to the result_array
            if ((strncmp(current_firstname, firstname_lower, firstname_size) == 0) &&
                (strncmp(current_lastname, lastname_lower, lastname_size) == 0)) {
                cJSON_AddItemReferenceToArray(result_array, athlete);
                counter++;
            }
        }
    }
    
    result = cJSON_Print(result_athletes);
    cJSON_Delete(json);
    cJSON_Delete(result_athletes);

    // ------------------------------------------------------------
    // Check if any athletes was found
    // ------------------------------------------------------------
    if (counter <= 0 || result == 0)
    {
        fprintf(stderr, "[%ld] HTTP 404: Could not find the requested athlete\n", (long)getpid());
        send_http_response(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested athlete\n\0");
        if (result != 0) free(result);
        return -1;
    }

    // ------------------------------------------------------------
    // Send back the athletes over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* result_modified = (char*) malloc((strlen(result) + 2) * sizeof(char));
    strcpy(result_modified, result);
    strcat(result_modified, "\n");  // Adds '\0' at the end
    if (result != 0) free(result);

    if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_JSON, result_modified) == -1) 
    {
        if (result_modified != 0) free(result_modified);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the results successfully!\n", (long)getpid());
    if (result_modified != 0) free(result_modified);
    
    return 0;
}



