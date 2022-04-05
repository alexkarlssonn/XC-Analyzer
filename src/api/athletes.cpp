

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


enum name_t { FIRSTNAME, LASTNAME, FULLNAME };
static int getAthletes_name(int socket, name_t name_type, char* search_str);
static cJSON* convertAthleteToJSON(char* buffer, int buffer_size, int start);


/**
 * -------------------------------------------------------------------------------------
 * Api call for searching the database for athletes that has matching firstnames
 * See "getAthletes_name" for more details
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_firstname(int socket, char* firstname)
{
    return getAthletes_name(socket, FIRSTNAME, firstname);
}


/**
 * -------------------------------------------------------------------------------------
 * Api call for searching the database for athletes that has matching lastnames
 * See "getAthletes_name" for more details
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_lastname(int socket, char* lastname)
{
    return getAthletes_name(socket, LASTNAME, lastname);
}


/**
 * -------------------------------------------------------------------------------------
 * Api call for searching the database for athletes that has matching first- and lastnames
 * See "getAthletes_name" for more details
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_fullname(int socket, char* fullname)
{
    return getAthletes_name(socket, FULLNAME, fullname);
}


/**
 * -------------------------------------------------------------------------------------
 * Tries to find the athlete with the given fiscode in the database 
 * If that athlete is found it will be sent back over socket in JSON format with an http response
 * If not, an http response will also be sent to indicate the error
 * The function also prints out messages that descibes the error before returning
 *
 * socket: The file descriptor that represents the socket to send the data over
 * fiscode: The fiscode for the requested athlete 
 *          Should be the parameter section in path that gets returned after calling parse_requestline
 *          It also needs to be a null terminated string
 * 
 * Returns 0 on success, to indicate that the athlete was found
 * Returns -1 on failure, to indicate that the athlete was not found, and that the error was sent over socket as an http response 
 * -------------------------------------------------------------------------------------
 */
int api_getAthlete_fiscode(int socket, char* fiscode)
{
    // -----------------------------------------------------------------
    // Validate the fiscode parameter
    // -----------------------------------------------------------------
    int fiscode_int = validate_and_convert_parameter(fiscode);
    if (fiscode_int <= -1) {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter\n\0");
        return -1;
    }
    
    
    // -----------------------------------------------------------------
    // Load the file that stores all athletes 
    // -----------------------------------------------------------------
    char file[] = DB_ATHLETES;
    char* buffer = 0;
    int buffer_size = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(file, &buffer, &buffer_size, &status_code) == -1) {
        send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer != 0) free(buffer);
        return -1;
    }


    // -----------------------------------------------------------------
    // Try to find the athlete with the requsted fiscode
    // -----------------------------------------------------------------
    cJSON* athlete = NULL;
    int currentByte = 0;   
    int offset = 0; 
    while (currentByte < buffer_size)
    {
        // Extract and combines the bytes that stores the fiscode
        offset = currentByte;
        unsigned char bytes[4];
        bytes[0] = buffer[currentByte++]; 
        bytes[1] = buffer[currentByte++]; 
        bytes[2] = buffer[currentByte++];
        bytes[3] = buffer[currentByte++];
        unsigned int combined_bytes = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        if (fiscode_int == combined_bytes) {
            athlete = convertAthleteToJSON(buffer, buffer_size, offset);
            break;
        }
        else {
            // Move past all the data for this current athlete, since the current fiscode does not match the requested one
            currentByte += 4;  
            int string_counter = 0; 
            while (string_counter < 6 && currentByte < buffer_size) {
                if (buffer[currentByte++] == '\0') {
                    string_counter++;  
                }
            }
        }
    }

    if (buffer != 0) 
        free(buffer);


    // ------------------------------------------------------------
    // Check if the requested athlete was found
    // ------------------------------------------------------------
    if (athlete == NULL) {
        fprintf(stderr, "[%ld] HTTP 404: Could not find the requested athlete\n", (long)getpid());
        send_http_response(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested athlete\n\0");
        return -1;
    }

    // ------------------------------------------------------------
    // Send back the athlete over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* athlete_str = cJSON_Print(athlete);
    char* response = (char*) malloc((strlen(athlete_str) + 2) * sizeof(char));
    strcpy(response, athlete_str);
    strcat(response, "\n");  // strcar also adds '\0' at the end
    
    if (athlete_str != 0) free(athlete_str);
    cJSON_Delete(athlete);

    if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_JSON, response) == -1) {
        if (response != 0) free(response);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the requested athlete!\n", (long)getpid());
    if (response != 0) free(response);
    
    return 0;
}



/**
 * -------------------------------------------------------------------------------------
 * Tries to find athletes in the database that matches the given search string
 * If any athletes are found, they will be sent back over socket in JSON format with an http response
 * If not, an http response will also be sent to indicate the error
 * The function also prints out messages that descibes the error before returning
 *
 * socket: The file descriptor that represents the socket to send the data over
 * name_type: The field to search in with the given search string. Either FIRSTNAME, LASTNAME, or FULLNAME
 * search_str: The search string to use when searching for athletes
 *             Should be the parameter section in path that gets returned after calling parse_requestline
 *             It also needs to be a null terminated string
 * 
 * Returns 0 on success, to indicate that one or more athletes was found
 * Returns -1 on failure, to indicate that no athletes was found, and that the error was sent over socket as an http response 
 * -------------------------------------------------------------------------------------
 */
static int getAthletes_name(int socket, name_t name_type, char* search_str)
{
    // -----------------------------------------------------------------
    // Validate the search string and convert it to lower case
    // -----------------------------------------------------------------
    if (search_str == 0 || strlen(search_str) == 0) {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed. Invalid parameter, no search string was given\n", (long)getpid());
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter, no search string was given\n\0");
        return -1;
    }
    if (!isalpha(search_str[0])) {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed. Invalid search string\n", (long)getpid());
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid search string\n\0");
        return -1;
    }

    int search_str_size = strlen(search_str);
    for (int i = 0; i < strlen(search_str); i++) {
        search_str[i] = tolower(search_str[i]);
    }

    
    // If searching for FULLNAME, then split the first- and lastname into seperate strings
    char* search_str_firstname;
    char* search_str_lastname;
    int search_str_firstname_size = 0;
    int search_str_lastname_size = 0;
    if (name_type == FULLNAME)
    {
        // Check so the first- and lastname are seperated by: '/' 
        bool hasTwoNames = false;
        char* p = search_str;
        while (*p != '\0') {
            if (*p == '/') {
                hasTwoNames = true;
                p++;
                break;
            }
            p++;
        }

        // Split the two names into a first- and lastname 
        bool isValidNames = false;
        if (hasTwoNames) {
            // Set the substring for the lastname
            search_str_lastname = p;
            
            // Replace the name delimiter '/' with '\0' so a substring for fistname can be created
            search_str_firstname = search_str;    
            p = search_str;
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
            if (search_str_lastname[0] == '\0')
                isValidNames = false;
        } 

        // Make sure the substrings are valid names
        if (!isValidNames) {
            fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
            send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter\n\0");
            return -1;
        }

        search_str_firstname_size = strlen(search_str_firstname);
        search_str_lastname_size = strlen(search_str_lastname);
    }


    // -----------------------------------------------------------------
    // Load the file that stores all athletes 
    // -----------------------------------------------------------------
    char file[] = DB_ATHLETES;
    char* buffer = 0;
    int buffer_size = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(file, &buffer, &buffer_size, &status_code) == -1) {
        send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer != 0) free(buffer);
        return -1;
    }

    // -----------------------------------------------------------------
    // Create the JSON object that will be sent back to the client
    // -----------------------------------------------------------------
    cJSON* json_athletes = cJSON_CreateObject();
    cJSON* json_array = cJSON_CreateArray();
    if (json_athletes == NULL || json_array == NULL) 
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to create JSON object\n", (long)getpid());
        send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to create JSON object\n\0");
        if (json_athletes != 0) cJSON_Delete(json_athletes);
        if (json_array != 0) cJSON_Delete(json_array);
        return -1;
    }
    cJSON_AddItemToObject(json_athletes, "athletes", json_array);  


    // -----------------------------------------------------------------
    // Try to find athletes that matches the search
    // -----------------------------------------------------------------
    int found_counter = 0;
    int currentByte = 0;    
    int offset = 0;
    while (currentByte < buffer_size)
    {
        // Skip the fiscode and the compid
        offset = currentByte;
        currentByte += 8;
        if (currentByte >= buffer_size) break;


        if (name_type == FIRSTNAME)
        {
            // Read the firstname and convert it to lowercase
            char firstname[256];
            int str_index = 0;
            while (buffer[currentByte++] != '\0' && str_index < 256 && currentByte-1 < buffer_size) {
                firstname[str_index++] = tolower(buffer[currentByte - 1]); 
            }
            firstname[str_index] = '\0';

            // Add the athlete to the array if the firstname matches the search string
            if (strncmp(firstname, search_str, search_str_size) == 0) {
                cJSON* athlete = convertAthleteToJSON(buffer, buffer_size, offset);
                cJSON_AddItemReferenceToArray(json_array, athlete);
                found_counter++;
            }

            // Skip the rest of the data for this athlete (5 fields)
            int null_counter = 0; 
            while (null_counter < 5 && currentByte < buffer_size) {
                if (buffer[currentByte++] == '\0') {
                    null_counter++;  
                }
            }
        }


        if (name_type == LASTNAME)
        {
            // Skip the firstname
            while (currentByte < buffer_size && buffer[currentByte++] != '\0');

            // Read the lastname and convert it to lowercase
            char lastname[256];
            int str_index = 0;
            while (buffer[currentByte++] != '\0' && str_index < 256 && currentByte-1 < buffer_size) {
                lastname[str_index++] = tolower(buffer[currentByte - 1]); 
            }
            lastname[str_index] = '\0';

            // Add the athlete to the array if the lastname matches the search string
            if (strncmp(lastname, search_str, search_str_size) == 0) {
                cJSON* athlete = convertAthleteToJSON(buffer, buffer_size, offset);
                cJSON_AddItemReferenceToArray(json_array, athlete);
                found_counter++;
            }

            // Skip the rest of the data for this athlete (4 fields)
            int null_counter = 0; 
            while (null_counter < 4 && currentByte < buffer_size) {
                if (buffer[currentByte++] == '\0') {
                    null_counter++;  
                }
            }
        }


        if (name_type == FULLNAME)
        {
            // Read the first- and lastname and convert them to lowercase
            char firstname[256];
            int str_index = 0;
            while (buffer[currentByte++] != '\0' && str_index < 256 && currentByte-1 < buffer_size) {
                firstname[str_index++] = tolower(buffer[currentByte - 1]); 
            }
            firstname[str_index] = '\0';

            char lastname[256];
            str_index = 0;
            while (buffer[currentByte++] != '\0' && str_index < 256 && currentByte-1 < buffer_size) {
                lastname[str_index++] = tolower(buffer[currentByte - 1]); 
            }
            lastname[str_index] = '\0';

            // Add the athlete to the array if the first- and lastname matches the search string
            if ((strncmp(firstname, search_str_firstname, search_str_firstname_size) == 0) && 
                (strncmp(lastname, search_str_lastname, search_str_lastname_size) == 0)) {
                cJSON* athlete = convertAthleteToJSON(buffer, buffer_size, offset);
                cJSON_AddItemReferenceToArray(json_array, athlete);
                found_counter++;
            }

            // Skip the rest of the data for this athlete (4 fields)
            int null_counter = 0; 
            while (null_counter < 4 && currentByte < buffer_size) {
                if (buffer[currentByte++] == '\0') {
                    null_counter++;  
                }
            }
        }
    }


    // Convert the JSON object to a string and free up allocated memory
    char* athlete_str = cJSON_Print(json_athletes);
    cJSON_Delete(json_athletes);
    if (buffer != 0) 
        free(buffer);


    // ------------------------------------------------------------
    // Check if any athletes was found
    // ------------------------------------------------------------
    if (found_counter <= 0) {
        fprintf(stderr, "[%ld] HTTP 404: Could not find any athletes\n", (long)getpid());
        send_http_response(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find any athletes\n\0");
        if (athlete_str != 0) free(athlete_str);
        return -1;
    }


    // ------------------------------------------------------------
    // Send back the athlete over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* response = (char*) malloc((strlen(athlete_str) + 2) * sizeof(char));
    strcpy(response, athlete_str);
    strcat(response, "\n");  // strcar also adds '\0' at the end
    if (athlete_str != 0) free(athlete_str);

    if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_JSON, response) == -1) {
        if (response != 0) free(response);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the results successfully!\n", (long)getpid());
    if (response != 0) free(response);
    
    return 0;
}



/**
 * -------------------------------------------------------------------------------------
 * Reads one athlete from "buffer", starting at the offset given for "start".
 * It will be read and convert the athlete to a cJSON object. This object needs to be freed manually later.
 * The parameter "buffer" should contain the binary data that gets loaded from the database.
 * 
 * buffer: The binary data for all athletes in the database.
 * buffer_size: The size of the buffer.
 * start: The offset, i.e. the location in the buffer where the requested athlete is stored.
 * 
 * Returns a pointer to a cJSON object on success, and NULL on failure.
 * -------------------------------------------------------------------------------------
 */
static cJSON* convertAthleteToJSON(char* buffer, int buffer_size, int start)
{
    // Validate the buffer and make sure the first 8 bytes after the offset can be read
    if ((buffer == 0) || (start >= buffer_size) || (start + 8 >= buffer_size)) {
        return NULL;
    }

    unsigned int fiscode;
    unsigned int compid;
    char firstname[256];
    char lastname[256];
    char nation[256];
    char birthdate[32];
    char gender[8];
    char club[256];
    int currentByte = start;

    // Read the fiscode
    unsigned char bytes[4];
    bytes[0] = buffer[currentByte++]; 
    bytes[1] = buffer[currentByte++]; 
    bytes[2] = buffer[currentByte++];
    bytes[3] = buffer[currentByte++];
    fiscode = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

    // Read the compid
    bytes[0] = buffer[currentByte++]; 
    bytes[1] = buffer[currentByte++]; 
    bytes[2] = buffer[currentByte++];
    bytes[3] = buffer[currentByte++];
    compid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

    // Read the firstname
    int str_index = 0;
    while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
        firstname[str_index++] = buffer[currentByte - 1]; 
    }
    firstname[str_index] = '\0';

    // Read the lastname
    str_index = 0;
    while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
        lastname[str_index++] = buffer[currentByte - 1]; 
    }
    lastname[str_index] = '\0';

    // Read the nation
    str_index = 0;
    while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
        nation[str_index++] = buffer[currentByte - 1]; 
    }
    nation[str_index] = '\0';

    // Read the birthdate
    str_index = 0;
    while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 32) {
        birthdate[str_index++] = buffer[currentByte - 1]; 
    }
    birthdate[str_index] = '\0';

    // Read the gender
    str_index = 0;
    while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 8) {
        gender[str_index++] = buffer[currentByte - 1]; 
    }
    gender[str_index] = '\0';

    // Read the club
    str_index = 0;
    while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
        club[str_index++] = buffer[currentByte - 1]; 
    }
    club[str_index] = '\0';

    // Create a JSON object for the athlete
    cJSON* athlete = cJSON_CreateObject();
    if (athlete != NULL) {
        cJSON* fiscode_json = cJSON_CreateNumber(fiscode);
        cJSON* compid_json = cJSON_CreateNumber(compid);
        cJSON* firstname_json = cJSON_CreateString(firstname);
        cJSON* lastname_json = cJSON_CreateString(lastname);
        cJSON* nation_json = cJSON_CreateString(nation);
        cJSON* birthdate_json = cJSON_CreateString(birthdate);
        cJSON* gender_json = cJSON_CreateString(gender);
        cJSON* club_json = cJSON_CreateString(club);
        
        if (fiscode_json == NULL || compid_json == NULL || firstname_json == NULL || lastname_json == NULL || 
            nation_json == NULL || birthdate_json == NULL || gender_json == NULL || club_json == NULL) {
            cJSON_Delete(athlete);
        }
        else {
            cJSON_AddItemToObject(athlete, "fiscode", fiscode_json);
            cJSON_AddItemToObject(athlete, "competitionid", compid_json);
            cJSON_AddItemToObject(athlete, "firstname", firstname_json);
            cJSON_AddItemToObject(athlete, "lastname", lastname_json);
            cJSON_AddItemToObject(athlete, "nation", nation_json);
            cJSON_AddItemToObject(athlete, "birthdate", birthdate_json);
            cJSON_AddItemToObject(athlete, "gender", gender_json);
            cJSON_AddItemToObject(athlete, "club", club_json);
        }
    }

    return athlete;
}





