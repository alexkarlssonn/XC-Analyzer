
#include "api.h"

//#include "../handle_client/load_resource.h"
//#include "../handle_client/send_http_response.h"
#include "../Response.h"
#include "../libs/cJSON.h"
#include "../util/StringUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * -------------------------------------------------------------------------------------
 * Analyzes all Sprint Qualification results for the given athlete.
 * On success an Http response will be sent with the analyzed results in JSON format, where each race is an object in an array 
 * If the athlete, raceids, or races was not found, and Http respone will also be sent to indicate the error
 * The function also prints out messages that descibes the error before returning
 *
 * socket: The file descriptor that represents the socket to send the data over
 * fiscode: The fiscode for the requested athlete  
 *          Should be the parameter section in path that gets returned after calling parse_requestline
 *          It also needs to be a null terminated string
 * 
 * Returns 0 on success, to indicate that all requested resources was found, and the results was analyzed and sent over the socket
 * Returns -1 on failure, to indicate that not all resources was not found so the races couldn't be analyzed. The error was sent over socket as an Http response 
 * -------------------------------------------------------------------------------------
 */
int api_getAnalyzedResults_qual(int socket, char* fiscode_str)
{
    // -----------------------------------------------------------------
    // Validate the fiscode parameter
    // -----------------------------------------------------------------
    int fiscode_int = validate_and_convert_parameter(fiscode_str);
    if (fiscode_int <= -1) {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
        send_http_response(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter\n\0");
        return -1;
    }


    // -----------------------------------------------------------------
    // Load the file that contains the athletes raceids
    // -----------------------------------------------------------------
    char file_races[] = DB_ATHLETES_RACES;
    char* buffer = 0;
    int buffer_size = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(file_races, &buffer, &buffer_size, &status_code) == -1) {
        send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer != 0) free(buffer);
        return -1;
    }


    // -----------------------------------------------------------------
    // Find all raceids for the requested athlete
    // -----------------------------------------------------------------
    unsigned int* raceids = 0;
    unsigned int numberOfRaces = 0;
    int currentByte = 0;
    while (currentByte < buffer_size)
    {
        // Make sure the next 8 bytes can be read from the buffer
        if (currentByte + 8 >= buffer_size) break;

        // Read the current fiscode and how many races are stored for that athlete
        unsigned char bytes[4];
        bytes[0] = buffer[currentByte++]; 
        bytes[1] = buffer[currentByte++]; 
        bytes[2] = buffer[currentByte++];
        bytes[3] = buffer[currentByte++];
        unsigned int currentFiscode = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        bytes[0] = buffer[currentByte++]; 
        bytes[1] = buffer[currentByte++]; 
        bytes[2] = buffer[currentByte++];
        bytes[3] = buffer[currentByte++];
        numberOfRaces = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        // If the current fiscode is not the requested one, then skip past the list of raceids for this athlete
        if (currentFiscode != fiscode_int) {
            currentByte += (numberOfRaces * 4);
        }
        else 
        {
            // Allocate memory for all raceids
            if (numberOfRaces > 0) {
                raceids = (unsigned int*) malloc(numberOfRaces * sizeof(unsigned int));
                if (raceids == 0) {
                    break;  // Exit the loop if allocating memory failed
                }
            }

            // Loop through and read all the raceids
            for (int i = 0; i < numberOfRaces; i++)
            {
                // Make sure the next 4 bytes can be read from the buffer
                if (currentByte + 4 >= buffer_size) break;

                // Read the current raceid and add it to the list of raceids
                bytes[0] = buffer[currentByte++]; 
                bytes[1] = buffer[currentByte++]; 
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                unsigned int currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
                raceids[i] = currentRaceid;
            }
            break;
        }
    }

    if (buffer != 0) 
        free(buffer);


    // ------------------------------------------------------------
    // Check if any raceids was found
    // ------------------------------------------------------------
    if (raceids == 0 || numberOfRaces == 0) {
        fprintf(stderr, "[%ld] HTTP 404: Could not find any races for the requested athlete\n", (long)getpid());
        send_http_response(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find any races for the requested athlete\n\0");
        if (raceids != 0) free(raceids);
        return -1;
    }




    // -----------------------------------------------------------------
    // Load the two files that contains the info and results for the races
    // -----------------------------------------------------------------
    char file_info[] = DB_RACE_INFO;
    buffer = 0;
    buffer_size = 0;
    status_code = 500;  // Default status code on failure
    if (load_resource(file_info, &buffer, &buffer_size, &status_code) == -1) {
        send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer != 0) free(buffer);
        return -1;
    }

    char file_results[] = DB_RACE_RESULTS;
    char* buffer_results = 0;
    int buffer_results_size = 0;
    status_code = 500;  // Default status code on failure
    if (load_resource(file_results, &buffer_results, &buffer_results_size, &status_code) == -1) {
        send_http_response(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer_results != 0) free(buffer_results);
        return -1;
    }


    // -----------------------------------------------------------------
    // Create the JSON object that will be sent back to the client
    // -----------------------------------------------------------------
    cJSON* json_parent = cJSON_CreateObject();
    cJSON* json_array = cJSON_CreateArray();
    if (json_parent == NULL || json_array == NULL) 
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to create JSON object\n", (long)getpid());
        send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to create JSON object\n\0");
        if (json_parent != 0) cJSON_Delete(json_parent);
        if (json_array != 0) cJSON_Delete(json_array);
        return -1;
    }
    cJSON_AddItemToObject(json_parent, "races", json_array); 


    // --------------------------------------------------------------------------------------------------------------------
    // Extract all relevant data for the given races, and add them into the JSON object that gets sent back to the client
    // --------------------------------------------------------------------------------------------------------------------
    int races_counter = 0;
    for (int i = 0; i < numberOfRaces; i++)
    {
        // Loop though the buffer that contains the race info
        currentByte = 0;
        while (currentByte < buffer_size)
        {
            // If the next 4 bytes can be read, then read the current raceid
            if (currentByte + 4 >= buffer_size) break;
            unsigned char bytes[4];
            bytes[0] = buffer[currentByte++]; 
            bytes[1] = buffer[currentByte++]; 
            bytes[2] = buffer[currentByte++];
            bytes[3] = buffer[currentByte++];
            unsigned int currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

            // If this is not the requested race, then skip all the fields for the current race
            if (currentRaceid != raceids[i]) {
                currentByte += 4;  
                int string_counter = 0; 
                while (string_counter < 7 && currentByte < buffer_size) {
                    if (buffer[currentByte++] == '\0') {
                        string_counter++;  
                    }
                }
            }
            else
            {
                // The race that matches the current raceid was found, so read all relevant fields
                char date[256];
                char nation[256];
                char location[256];
                char category[256];
                char type[256];
                char gender[8];

                // Make sure the next 4 bytes can be read from the buffer
                if (currentByte + 4 >= buffer_size) break;
                currentByte += 4;

                // Read the date
                int str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    date[str_index++] = buffer[currentByte - 1]; 
                }
                date[str_index] = '\0';

                // Read the nation
                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    nation[str_index++] = buffer[currentByte - 1]; 
                }
                nation[str_index] = '\0';

                // Read the location
                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    location[str_index++] = buffer[currentByte - 1]; 
                }
                location[str_index] = '\0';

                // Read the category
                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    category[str_index++] = buffer[currentByte - 1]; 
                }
                category[str_index] = '\0';

                // Skip the field: discipline
                while (currentByte < buffer_size && buffer[currentByte++] != '\0');

                // Read the type
                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    type[str_index++] = buffer[currentByte - 1]; 
                }
                type[str_index] = '\0';

                // Read the gender
                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 8) {
                    gender[str_index++] = buffer[currentByte - 1]; 
                }
                gender[str_index] = '\0';

                // If the race is not of the type: "Sprint Qualifications", when skip it and break out of the loop
                if (strcmp(type, "SQ") != 0) break;


                // --------------------------------------------------------
                // Loop though the buffer that contains the race results
                // --------------------------------------------------------
                int currentByte_results = 0;
                while (currentByte_results < buffer_results_size)
                {
                    // If the next 6 bytes can be read, then read the current raceid and the number of ranks it has
                    if (currentByte_results + 6 >= buffer_results_size) break;
                    bytes[0] = buffer_results[currentByte_results++]; 
                    bytes[1] = buffer_results[currentByte_results++]; 
                    bytes[2] = buffer_results[currentByte_results++];
                    bytes[3] = buffer_results[currentByte_results++];
                    unsigned int currentResult_raceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                    bytes[0] = buffer_results[currentByte_results++];
                    bytes[1] = buffer_results[currentByte_results++];
                    unsigned int number_of_ranks = bytes[0] | (bytes[1] << 8); 

                    // If this is not the requested race, then skip all the fields for the current race
                    if (currentResult_raceid != raceids[i]) {
                        for (int j = 0; j < number_of_ranks; j++) {
                            currentByte_results += 18;  
                            int string_counter = 0; 
                            while (string_counter < 3 && currentByte_results < buffer_results_size) {
                                if (buffer_results[currentByte_results++] == '\0') {
                                    string_counter++;  
                                }
                            }
                        }
                    }
                    else 
                    {
                        // The requested race was found, loop though all its ranks and try to find the requested athlete
                        for (int j = 0; j < number_of_ranks; j++) 
                        {
                            // If the next 16 bytes can be read from the buffer, then start reading the relevant fields
                            if (currentByte_results + 18 >= buffer_results_size) break;
                            unsigned int rank;
                            unsigned int fiscode;
                            unsigned int time;
                            unsigned int diff;
                            char athlete[256];

                            // Read the rank
                            bytes[0] = buffer_results[currentByte_results++]; 
                            bytes[1] = buffer_results[currentByte_results++]; 
                            rank = bytes[0] | (bytes[1] << 8);

                            // Skip the bib
                            currentByte_results += 2;

                            // Read the fiscode
                            bytes[0] = buffer_results[currentByte_results++]; 
                            bytes[1] = buffer_results[currentByte_results++]; 
                            bytes[2] = buffer_results[currentByte_results++];
                            bytes[3] = buffer_results[currentByte_results++];
                            fiscode = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                            // Read the time
                            bytes[0] = buffer_results[currentByte_results++]; 
                            bytes[1] = buffer_results[currentByte_results++]; 
                            bytes[2] = buffer_results[currentByte_results++];
                            bytes[3] = buffer_results[currentByte_results++];
                            time = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                            // Read the diff
                            bytes[0] = buffer_results[currentByte_results++]; 
                            bytes[1] = buffer_results[currentByte_results++]; 
                            bytes[2] = buffer_results[currentByte_results++];
                            bytes[3] = buffer_results[currentByte_results++];
                            diff = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                            // Skip the year
                            currentByte_results += 2;

                            // Read the athlete
                            str_index = 0;
                            while (currentByte_results < buffer_results_size && buffer_results[currentByte_results++] != '\0' && str_index < 256) {
                                athlete[str_index++] = buffer_results[currentByte_results - 1]; 
                            }
                            athlete[str_index] = '\0';

                            // Skip the other string fields
                            while (currentByte_results < buffer_results_size && buffer_results[currentByte_results++] != '\0');
                            while (currentByte_results < buffer_results_size && buffer_results[currentByte_results++] != '\0');


                            // Check if the fiscode for the current rank matches the requested one
                            if (fiscode_int == fiscode) 
                            {
                                // ----------------------------------------------------------------
                                // Create a JSON object that contains all the data for this race 
                                // ----------------------------------------------------------------
                                cJSON* json_race = cJSON_CreateObject();
                                if (json_race != NULL) 
                                {
                                    // Create JSON objects for all fields
                                    cJSON* raceid_json = cJSON_CreateNumber(raceids[i]);
                                    cJSON* athlete_json = cJSON_CreateString(athlete);
                                    cJSON* fiscode_json = cJSON_CreateNumber(fiscode);
                                    cJSON* rank_json = cJSON_CreateNumber(rank);
                                    cJSON* date_json = cJSON_CreateString(date);
                                    cJSON* nation_json = cJSON_CreateString(nation);
                                    cJSON* location_json = cJSON_CreateString(location);
                                    cJSON* category_json = cJSON_CreateString(category);
                                    cJSON* type_json = cJSON_CreateString(type);
                                    cJSON* gender_json = cJSON_CreateString(gender);
                                    cJSON* time_json = cJSON_CreateNumber(time);
                                    cJSON* diff_json = cJSON_CreateNumber(diff);
                                    float diff_percentage = ((float)time / (time - diff));
                                    cJSON* diff_percentage_json = cJSON_CreateNumber(diff_percentage);

                                    if (raceid_json == NULL || athlete_json == NULL || fiscode_json == NULL || rank_json == NULL || 
                                        date_json == NULL || nation_json == NULL || location_json == NULL || category_json == NULL || 
                                        type_json == NULL || gender_json == NULL || time_json == NULL || diff_json == NULL || diff_percentage_json == NULL) {
                                        cJSON_Delete(json_race);
                                    }
                                    else 
                                    {
                                        // Add each field to the current rank, then add it to the array of all races
                                        cJSON_AddItemToObject(json_race, "raceid", raceid_json);
                                        cJSON_AddItemToObject(json_race, "name", athlete_json);
                                        cJSON_AddItemToObject(json_race, "fiscode", fiscode_json);
                                        cJSON_AddItemToObject(json_race, "rank", rank_json);
                                        cJSON_AddItemToObject(json_race, "date", date_json);
                                        cJSON_AddItemToObject(json_race, "nation", nation_json);
                                        cJSON_AddItemToObject(json_race, "location", location_json);
                                        cJSON_AddItemToObject(json_race, "category", category_json);
                                        cJSON_AddItemToObject(json_race, "type", type_json);
                                        cJSON_AddItemToObject(json_race, "gender", gender_json);
                                        cJSON_AddItemToObject(json_race, "time", time_json);
                                        cJSON_AddItemToObject(json_race, "diff", diff_json);
                                        cJSON_AddItemToObject(json_race, "diff percentage", diff_percentage_json);
                                        cJSON_AddItemReferenceToArray(json_array, json_race);
                                        races_counter++;
                                    }
                                }
                                
                                // Break out of the loop that goes through all the "ranks" for the current result
                                break;  
                            }
                        }

                        // Break out of the loop that goes through the "race results" buffer
                        break;
                    }
                }

                // Break out of the loop that goes through the "race info" buffer
                break; 
            }
        }
    }


    // Convert the JSON object to a string and free up allocated memory
    char* races_str = cJSON_Print(json_parent);
    cJSON_Delete(json_parent);
    if (raceids != 0) 
        free(raceids);
    if (buffer != 0) 
        free(buffer);
    if (buffer_results != 0) 
        free(buffer_results);


    // ------------------------------------------------------------
    // Check if the requested race was found
    // ------------------------------------------------------------
    if (races_counter == 0) {
        fprintf(stderr, "[%ld] HTTP 500: No races were analyzed\n", (long)getpid());
        send_http_response(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: No races were analyzed\n\0");
        if (races_str != 0) free(races_str);
        return -1;
    }


    // ------------------------------------------------------------------
    // Send back the race results over the socket as an HTTP Response 
    // ------------------------------------------------------------------
    char* response = (char*) malloc((strlen(races_str) + 2) * sizeof(char));
    strcpy(response, races_str);
    strcat(response, "\n");  // strcar also adds '\0' at the end
    if (races_str != 0) free(races_str);

    if (send_http_response(socket, 200, CONNECTION_CLOSE, TYPE_JSON, response) == -1) {
        if (response != 0) free(response);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Successfully sent back the analyzed results for the requested athlete!\n", (long)getpid());
    if (response != 0) free(response);
    
    return 0;
}





