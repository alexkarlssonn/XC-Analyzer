

#include "api.h"

#include "../server/Server.h"
//#include "../Response.h"
#include "../libs/cJSON.h"
#include "../util/StringUtil.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * -------------------------------------------------------------------------------------
 * Tries to find the list of raceids for the athlete with the given fiscode in the database 
 * If that athlete is found, the list of raceids will be sent back over socket in JSON format with an http response
 * If not, an http response will also be sent to indicate the error.
 * Or an empty array is sent if the athlete was found but no raceids was found for that athlete
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
int api_getAthletesRaceids(int socket, char* fiscode)
{
    // -----------------------------------------------------------------
    // Validate the fiscode parameter
    // -----------------------------------------------------------------
    int fiscode_int = validate_and_convert_parameter(fiscode);
    if (fiscode_int <= -1) {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
        SendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter");
        return -1;
    }
    

    // -----------------------------------------------------------------
    // Load the file that stores all races for each athlete
    // -----------------------------------------------------------------
    char file[] = DB_ATHLETES_RACES;
    char* buffer = 0;
    int buffer_size = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(file, &buffer, &buffer_size, &status_code) == -1) {
        SendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource");
        if (buffer != 0) free(buffer);
        return -1;
    }


    // -----------------------------------------------------------------
    // Create the JSON object that will be sent back to the client
    // -----------------------------------------------------------------
    cJSON* json_races = cJSON_CreateObject();
    cJSON* json_array = cJSON_CreateArray();
    if (json_races == NULL || json_array == NULL) 
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to create JSON object\n", (long)getpid());
        SendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to create JSON object");
        if (json_races != 0) cJSON_Delete(json_races);
        if (json_array != 0) cJSON_Delete(json_array);
        return -1;
    }
    cJSON_AddItemToObject(json_races, "races", json_array); 


    // -----------------------------------------------------------------
    // Try to find the athlete with the requsted fiscode
    // -----------------------------------------------------------------
    bool foundAthlete = false;
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
        unsigned int numberOfRaces = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        // If the current fiscode is not the requested one, then skip past the list of raceids for this athlete
        if (currentFiscode != fiscode_int) {
            currentByte += (numberOfRaces * 4);
        }
        else {
            // Loop through and read all the raceids
            for (int i = 0; i < numberOfRaces; i++)
            {
                // Make sure the next 4 bytes can be read from the buffer
                if (currentByte + 4 >= buffer_size) break;

                // Read the current raceid and add it to the JSON object that will be sent back to the client
                bytes[0] = buffer[currentByte++]; 
                bytes[1] = buffer[currentByte++]; 
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                unsigned int currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                cJSON* json_raceid = cJSON_CreateNumber(currentRaceid);
                if (json_raceid != NULL) {
                    cJSON_AddItemReferenceToArray(json_array, json_raceid);
                }
            }
            foundAthlete = true;
            break;
        }
    }


    // Convert the JSON object to a string and free up allocated memory
    char* races_str = cJSON_Print(json_races);
    cJSON_Delete(json_races);
    if (buffer != 0) 
        free(buffer);


    // ------------------------------------------------------------
    // Check if the requested athlete was found
    // ------------------------------------------------------------
    if (!foundAthlete) {
        fprintf(stderr, "[%ld] HTTP 404: Could not find the requested athlete\n", (long)getpid());
        SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested athlete");
        if (races_str != 0) free(races_str);
        return -1;
    }


    // ------------------------------------------------------------
    // Send back the raceids over the socket as an HTTP Response 
    // ------------------------------------------------------------
    char* response = (char*) malloc((strlen(races_str) + 2) * sizeof(char));
    strcpy(response, races_str);
    //strcat(response, "\n");  // strcar also adds '\0' at the end
    if (races_str != 0) free(races_str);

    if (SendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_JSON, response) == -1) {
        if (response != 0) free(response);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the results successfully!\n", (long)getpid());
    if (response != 0) free(response);
    
    return 0;
}



/**
 * -------------------------------------------------------------------------------------
 * Tries to find the given race in the database
 * If that race is found, the info about it will be sent back over socket in JSON format with an http response
 * If not, an http response will also be sent to indicate the error.
 * The function also prints out messages that descibes the error before returning
 *
 * socket: The file descriptor that represents the socket to send the data over
 * raceid: The raceid for the requested race 
 *          Should be the parameter section in path that gets returned after calling parse_requestline
 *          It also needs to be a null terminated string
 * 
 * Returns 0 on success, to indicate that the race was found
 * Returns -1 on failure, to indicate that the race was not found, and that the error was sent over socket as an http response 
 * -------------------------------------------------------------------------------------
 */
int api_getRaceInfo(int socket, char* raceid)
{
    // -----------------------------------------------------------------
    // Validate the raceid parameter
    // -----------------------------------------------------------------
    int raceid_int = validate_and_convert_parameter(raceid);
    if (raceid_int <= -1) {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
        SendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter");
        return -1;
    }


    // -----------------------------------------------------------------
    // Load the file that stores the info for all races
    // -----------------------------------------------------------------
    char file[] = DB_RACE_INFO;
    char* buffer = 0;
    int buffer_size = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(file, &buffer, &buffer_size, &status_code) == -1) {
        SendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource");
        if (buffer != 0) free(buffer);
        return -1;
    }


    // -----------------------------------------------------------------
    // Try to find athletes that matches the search
    // -----------------------------------------------------------------
    cJSON* json_raceinfo = NULL;
    int currentByte = 0;
    while (currentByte < buffer_size)
    {
        // Make sure the next 4 bytes can be read from the buffer
        if (currentByte + 4 >= buffer_size) break;

        // Read the current raceid
        unsigned char bytes[4];
        bytes[0] = buffer[currentByte++]; 
        bytes[1] = buffer[currentByte++]; 
        bytes[2] = buffer[currentByte++];
        bytes[3] = buffer[currentByte++];
        unsigned int currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        if (currentRaceid != raceid_int)
        {
            // This is not the requested race, so skip all its fields
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
            // The requested race was found, so read all the fields
            unsigned int codex;
            char date[256];
            char nation[256];
            char location[256];
            char category[256];
            char discipline[256];
            char type[256];
            char gender[8];

            // Make sure the next 4 bytes can be read from the buffer
            if (currentByte + 4 >= buffer_size) break;

            // Read the codex
            bytes[0] = buffer[currentByte++]; 
            bytes[1] = buffer[currentByte++]; 
            bytes[2] = buffer[currentByte++];
            bytes[3] = buffer[currentByte++];
            codex = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

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

            // Read the discipline
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                discipline[str_index++] = buffer[currentByte - 1]; 
            }
            discipline[str_index] = '\0';

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

            // Create a JSON object that contains all the data for this race
            json_raceinfo = cJSON_CreateObject();
            if (json_raceinfo != NULL) {
                cJSON* raceid_json = cJSON_CreateNumber(currentRaceid);
                cJSON* codex_json = cJSON_CreateNumber(codex);
                cJSON* date_json = cJSON_CreateString(date);
                cJSON* nation_json = cJSON_CreateString(nation);
                cJSON* location_json = cJSON_CreateString(location);
                cJSON* category_json = cJSON_CreateString(category);
                cJSON* discipline_json = cJSON_CreateString(discipline);
                cJSON* type_json = cJSON_CreateString(type);
                cJSON* gender_json = cJSON_CreateString(gender);
                
                if (raceid_json == NULL || codex_json == NULL || date_json == NULL || nation_json == NULL || location_json == NULL || 
                    category_json == NULL || discipline_json == NULL || type_json == NULL || gender_json == NULL) {
                    cJSON_Delete(json_raceinfo);
                }
                else {
                    cJSON_AddItemToObject(json_raceinfo, "raceid", raceid_json);
                    cJSON_AddItemToObject(json_raceinfo, "codex", codex_json);
                    cJSON_AddItemToObject(json_raceinfo, "date", date_json);
                    cJSON_AddItemToObject(json_raceinfo, "nation", nation_json);
                    cJSON_AddItemToObject(json_raceinfo, "location", location_json);
                    cJSON_AddItemToObject(json_raceinfo, "category", category_json);
                    cJSON_AddItemToObject(json_raceinfo, "discipline", discipline_json);
                    cJSON_AddItemToObject(json_raceinfo, "type", type_json);
                    cJSON_AddItemToObject(json_raceinfo, "gender", gender_json);
                }
            }
            break;
        }
    }

   if (buffer != 0) 
       free(buffer);


   // ------------------------------------------------------------
   // Check if the requested race was found
   // ------------------------------------------------------------
   if (json_raceinfo == NULL) {
       fprintf(stderr, "[%ld] HTTP 404: Could not find the requested race\n", (long)getpid());
       SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested race");
       return -1;
   }


   // ------------------------------------------------------------
   // Send back the race info over the socket as an HTTP Response 
   // ------------------------------------------------------------
   char* raceinfo_str = cJSON_Print(json_raceinfo);
   char* response = (char*) malloc((strlen(raceinfo_str) + 2) * sizeof(char));
   strcpy(response, raceinfo_str);
   //strcat(response, "\n");  // strcar also adds '\0' at the end
   
   if (raceinfo_str != 0) free(raceinfo_str);
   cJSON_Delete(json_raceinfo);

   if (SendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_JSON, response) == -1) {
       if (response != 0) free(response);
       return -1;
   }

   fprintf(stderr, "[%ld] HTTP 200: Found and sent back the requested race info!\n", (long)getpid());
   if (response != 0) free(response);
   
   return 0;
}



/**
 * -------------------------------------------------------------------------------------
 * Tries to find the race results for the given race in the database
 * If that race is found, the result list will be sent back over socket in JSON format with an http response
 * If not, an http response will also be sent to indicate the error.
 * The function also prints out messages that descibes the error before returning
 *
 * socket: The file descriptor that represents the socket to send the data over
 * raceid: The raceid for the requested race 
 *          Should be the parameter section in path that gets returned after calling parse_requestline
 *          It also needs to be a null terminated string
 * 
 * Returns 0 on success, to indicate that the race results was found
 * Returns -1 on failure, to indicate that the race results was not found, and that the error was sent over socket as an http response 
 * -------------------------------------------------------------------------------------
 */
int api_getRaceResult(int socket, char* raceid)
{
    // -----------------------------------------------------------------
    // Validate the raceid parameter
    // -----------------------------------------------------------------
    int raceid_int = validate_and_convert_parameter(raceid);
    if (raceid_int <= -1) {
        fprintf(stderr, "[%ld] HTTP 400: Api call failed, invalid parameter\n", (long)getpid());
        SendHttpResponse(socket, 400, CONNECTION_CLOSE, TYPE_HTML, "400 Bad Request: Invalid parameter");
        return -1;
    }


    // -----------------------------------------------------------------
    // Load the file that stores all race results
    // -----------------------------------------------------------------
    char file[] = DB_RACE_RESULTS;
    char* buffer = 0;
    int buffer_size = 0;
    int status_code = 500;  // Default status code on failure
    if (load_resource(file, &buffer, &buffer_size, &status_code) == -1) {
        SendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource");
        if (buffer != 0) free(buffer);
        return -1;
    }


    // -----------------------------------------------------------------
    // Create the JSON object that will be sent back to the client
    // -----------------------------------------------------------------
    cJSON* json_race = cJSON_CreateObject();
    cJSON* json_resultsarray = cJSON_CreateArray();
    if (json_race == NULL || json_resultsarray == NULL) 
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to create JSON object\n", (long)getpid());
        SendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to create JSON object");
        if (json_race != 0) cJSON_Delete(json_race);
        if (json_resultsarray != 0) cJSON_Delete(json_resultsarray);
        return -1;
    }
    cJSON_AddItemToObject(json_race, "results", json_resultsarray); 


    // ------------------------------------------------------------
    // Try to find the requested race and its list of results
    // ------------------------------------------------------------
    bool foundRace = false;
    int currentByte = 0;
    while (currentByte < buffer_size)
    {
        // Make sure the next 6 bytes can be read from the buffer
        if (currentByte + 6 >= buffer_size) break;

        // Read the current raceid and the number of ranks it has
        unsigned char bytes[4];
        bytes[0] = buffer[currentByte++]; 
        bytes[1] = buffer[currentByte++]; 
        bytes[2] = buffer[currentByte++];
        bytes[3] = buffer[currentByte++];
        unsigned int currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        bytes[0] = buffer[currentByte++];
        bytes[1] = buffer[currentByte++];
        unsigned int number_of_ranks = bytes[0] | (bytes[1] << 8); 

        // Check if the current race is the requested one
        if (currentRaceid != raceid_int) 
        {
            for (int i = 0; i < number_of_ranks; i++) {
                // This is not the requested race, so skip all its fields for each rank    
                currentByte += 18;  
                int string_counter = 0; 
                while (string_counter < 3 && currentByte < buffer_size) {
                    if (buffer[currentByte++] == '\0') {
                        string_counter++;  
                    }
                }
            }
        }
        else {
            // This is the requested race, so loop through all its ranks
            for (int i = 0; i < number_of_ranks; i++) 
            {
                // Read all the fields for this rank
                unsigned int rank;
                unsigned int bib;
                unsigned int fiscode;
                unsigned int time;
                unsigned int diff;
                unsigned int year;
                char athlete[256];
                char nation[256];
                char fispoints[16];

                // Make sure the next 18 bytes can be read from the buffer
                if (currentByte + 18 >= buffer_size) break;

                // Read the rank
                bytes[0] = buffer[currentByte++]; 
                bytes[1] = buffer[currentByte++]; 
                rank = bytes[0] | (bytes[1] << 8);

                // Read the bib
                bytes[0] = buffer[currentByte++]; 
                bytes[1] = buffer[currentByte++]; 
                bib = bytes[0] | (bytes[1] << 8);

                // Read the fiscode
                bytes[0] = buffer[currentByte++]; 
                bytes[1] = buffer[currentByte++]; 
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                fiscode = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                // Read the time
                bytes[0] = buffer[currentByte++]; 
                bytes[1] = buffer[currentByte++]; 
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                time = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                // Read the diff
                bytes[0] = buffer[currentByte++]; 
                bytes[1] = buffer[currentByte++]; 
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                diff = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                // Read the year
                bytes[0] = buffer[currentByte++]; 
                bytes[1] = buffer[currentByte++]; 
                year = bytes[0] | (bytes[1] << 8);

                // Read the athlete
                int str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    athlete[str_index++] = buffer[currentByte - 1]; 
                }
                athlete[str_index] = '\0';

                // Read the nation
                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    nation[str_index++] = buffer[currentByte - 1]; 
                }
                nation[str_index] = '\0';

                // Read the fispoints
                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    fispoints[str_index++] = buffer[currentByte - 1]; 
                }
                fispoints[str_index] = '\0';

                // Create a JSON object that contains all the data for this rank
                cJSON* json_rank = cJSON_CreateObject();
                if (json_rank != NULL) 
                {
                    // Create JSON objects for all fields 
                    cJSON* rank_json = cJSON_CreateNumber(rank);
                    cJSON* bib_json = cJSON_CreateNumber(bib);
                    cJSON* fiscode_json = cJSON_CreateNumber(fiscode);
                    cJSON* time_json = cJSON_CreateNumber(time);
                    cJSON* diff_json = cJSON_CreateNumber(diff);
                    cJSON* year_json = cJSON_CreateNumber(year);
                    cJSON* athlete_json = cJSON_CreateString(athlete);
                    cJSON* nation_json = cJSON_CreateString(nation);
                    cJSON* fispoints_json = cJSON_CreateString(fispoints);
                    
                    if (rank_json == NULL || bib_json == NULL || fiscode_json == NULL || time_json == NULL || diff_json == NULL ||
                        year_json == NULL || athlete_json == NULL || nation_json == NULL || fispoints_json == NULL) {
                        cJSON_Delete(json_rank);
                    }
                    else 
                    {
                        // Add each field to the current rank, then add it to the array of all ranks
                        cJSON_AddItemToObject(json_rank, "rank", rank_json);
                        cJSON_AddItemToObject(json_rank, "bib", bib_json);
                        cJSON_AddItemToObject(json_rank, "fiscode", fiscode_json);
                        cJSON_AddItemToObject(json_rank, "time", time_json);
                        cJSON_AddItemToObject(json_rank, "diff", diff_json);
                        cJSON_AddItemToObject(json_rank, "year", year_json);
                        cJSON_AddItemToObject(json_rank, "athlete", athlete_json);
                        cJSON_AddItemToObject(json_rank, "nation", nation_json);
                        cJSON_AddItemToObject(json_rank, "fispoints", fispoints_json);
                        cJSON_AddItemReferenceToArray(json_resultsarray, json_rank);
                    }
                }
            }
            foundRace = true;
            break;
        }
    }

    // Convert the JSON object to a string and free up allocated memory
    char* race_str = cJSON_Print(json_race);
    cJSON_Delete(json_race);
    if (buffer != 0) 
        free(buffer);


    // ------------------------------------------------------------
    // Check if the requested race was found
    // ------------------------------------------------------------
    if (!foundRace) {
        fprintf(stderr, "[%ld] HTTP 404: Could not find the requested race\n", (long)getpid());
        SendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested race");
        if (race_str != 0) free(race_str);
        return -1;
    }


    // ------------------------------------------------------------------
    // Send back the race results over the socket as an HTTP Response 
    // ------------------------------------------------------------------
    char* response = (char*) malloc((strlen(race_str) + 2) * sizeof(char));
    strcpy(response, race_str);
    //strcat(response, "\n");  // strcar also adds '\0' at the end
    if (race_str != 0) free(race_str);

    if (SendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_JSON, response) == -1) {
        if (response != 0) free(response);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the results for the requested race!\n", (long)getpid());
    if (response != 0) free(response);
    
    return 0;
}







