

#include "api.h"

#include "../http-response.h"
#include "../libs/cJSON.h"
#include "../util/RaceTime.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <string.h>
#include <unistd.h>


typedef struct
{
    char* raceid = 0;
    char* date = 0;
    char* type = 0; 
    char* gender = 0;
    char* nation = 0;
    char* location = 0;
    char* category = 0;

    char* fiscode = 0;
    char* rank = 0;
    long time_ms = 0L;
    long winner_time_ms = 0L;
    long diff_ms = 0L;
    float diff_percentage = 0.0f;

} AnalyzedRace;

static cJSON* load_file_and_parse_to_json(int socket, char* file);
static int remove_enclosing_quotes(char* str);
static void destroy_analyzedRace(AnalyzedRace* race);


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
int api_getAnalyzedResults_qual(int socket, char* fiscode)
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
    // Load and parse the requested files 
    // -----------------------------------------------------------------
    char raceresult_0_22999_file[] = DB_RACE_RESULT_0_22999;
    char raceresult_23000_25999_file[] = DB_RACE_RESULT_23000_25999;
    char raceresult_26000_28999_file[] = DB_RACE_RESULT_26000_28999;
    char raceresult_29000_31999_file[] = DB_RACE_RESULT_29000_31999;
    char raceresult_32000_34999_file[] = DB_RACE_RESULT_32000_34999;
    char raceresult_35000_37999_file[] = DB_RACE_RESULT_35000_37999;
    char raceresult_38000_40999_file[] = DB_RACE_RESULT_38000_40999;
    char athletes_file[] = DB_ATHLETES_RACES;
    char raceinfo_file[] = DB_RACE_INFO;

    cJSON* JSON_athletes = NULL;
    if ((JSON_athletes = load_file_and_parse_to_json(socket, athletes_file)) == NULL)
        return -1;

    cJSON* JSON_racesinfo = NULL;
    if ((JSON_racesinfo = load_file_and_parse_to_json(socket, raceinfo_file)) == NULL)
        return -1;

    cJSON* JSON_race_result_0_22999 = NULL;
    cJSON* JSON_race_result_23000_25999 = NULL;
    cJSON* JSON_race_result_26000_28999 = NULL;
    cJSON* JSON_race_result_29000_31999 = NULL;
    cJSON* JSON_race_result_32000_34999 = NULL;
    cJSON* JSON_race_result_35000_37999 = NULL;
    cJSON* JSON_race_result_38000_40999 = NULL;

    // ------------------------------------------------------------
    // Try to find the requested athlete and his/hers races
    // ------------------------------------------------------------
    cJSON* athletes = cJSON_GetObjectItemCaseSensitive(JSON_athletes, "athletes");
    cJSON* athlete = NULL;
    cJSON* athlete_races = NULL;
    cJSON_ArrayForEach(athlete, athletes)
    {
        cJSON* field = cJSON_GetObjectItemCaseSensitive(athlete, "fiscode");
        if (cJSON_IsString(field) && field->valuestring != NULL && fiscode != 0) 
        {
            if (strcmp(field->valuestring, fiscode) == 0) {
                athlete_races = cJSON_GetObjectItemCaseSensitive(athlete, "races");
                break;
            }
        }
    }

    if (athlete_races == NULL)
    {
        fprintf(stderr, "[%ld] HTTP 404: Could not find the requested athlete\n", (long)getpid());
        sendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Could not find the requested athlete\n\0");
        cJSON_Delete(JSON_athletes);
        return -1;
    }
    
    int number_of_races = cJSON_GetArraySize(athlete_races);
    if (number_of_races <= 0)
    {
        fprintf(stderr, "[%ld] HTTP 404: Could not find any races for the given athlete\n", (long)getpid());
        sendHttpResponse(socket, 404, CONNECTION_CLOSE, TYPE_HTML, "404 Not Found: Couldn't find any races for given athlete\n\0");
        return -1;
    }

    // Initiate the array that will store the data for each analyzed race
    AnalyzedRace* analyzed_races[number_of_races];
    for (int i = 0; i < number_of_races; i++)
        analyzed_races[i] = 0;


    // ------------------------------------------------------------
    // Find race info for each raceid 
    // ------------------------------------------------------------
    cJSON* races = cJSON_GetObjectItemCaseSensitive(JSON_racesinfo, "races");
    cJSON* race = NULL;
    
    int index = -1;
    cJSON* athlete_race = NULL;
    cJSON_ArrayForEach(athlete_race, athlete_races) 
    {
        index++;
        if (athlete_race == NULL)
            continue;
            
        char* raceid_str = cJSON_Print(athlete_race);
        if (raceid_str == NULL)
            continue;
        
        
        // Loop trough all races from the "race info" file
        cJSON_ArrayForEach(race, races)
        {

            // Only continue of the raceids are matching
            cJSON* field = cJSON_GetObjectItemCaseSensitive(race, "raceid");
            if (!cJSON_IsString(field) || field->valuestring == NULL)
                continue;
            if (strncmp(field->valuestring, &(raceid_str[1]), (strlen(raceid_str) - 2)) != 0) 
                continue;
            
            // Only continue if the race type is Sprint Qualifications
            // Use break here since the raceid has already been found
            cJSON* field_type = cJSON_GetObjectItemCaseSensitive(race, "type");
            if (!cJSON_IsString(field_type) || field_type->valuestring == NULL)
                break;
            if (strcmp(field_type->valuestring, "SQ") != 0)
                break; 
                    
            

            // LOAD RESULTS LIST FOR CURRENT RACE

            cJSON* JSON_race_result = NULL;
            remove_enclosing_quotes(raceid_str);
            int raceid_int = atoi(raceid_str);

            // Get the JSON object that has the requested result 
            if (raceid_int > 0 && raceid_int < 23000) 
            {
                if (JSON_race_result_0_22999 == NULL) 
                    JSON_race_result_0_22999 = load_file_and_parse_to_json(socket, raceresult_0_22999_file);
                JSON_race_result = JSON_race_result_0_22999;
            }
            else if (raceid_int >= 23000 && raceid_int < 26000)
            {
                if (JSON_race_result_23000_25999 == NULL) 
                    JSON_race_result_23000_25999 = load_file_and_parse_to_json(socket, raceresult_23000_25999_file);
                JSON_race_result = JSON_race_result_23000_25999;
            }
            else if (raceid_int >= 26000 && raceid_int < 29000)
            {
                if (JSON_race_result_26000_28999 == NULL) 
                    JSON_race_result_26000_28999 = load_file_and_parse_to_json(socket, raceresult_26000_28999_file);
                JSON_race_result = JSON_race_result_26000_28999;
            }
            else if (raceid_int >= 29000 && raceid_int < 32000)
            {
                if (JSON_race_result_29000_31999 == NULL) 
                    JSON_race_result_29000_31999 = load_file_and_parse_to_json(socket, raceresult_29000_31999_file); 
                JSON_race_result = JSON_race_result_29000_31999;
            }
            else if (raceid_int >= 32000 && raceid_int < 35000)
            {
                if (JSON_race_result_32000_34999 == NULL) 
                    JSON_race_result_32000_34999 = load_file_and_parse_to_json(socket, raceresult_32000_34999_file);
                JSON_race_result = JSON_race_result_32000_34999;
            }
            else if (raceid_int >= 35000 && raceid_int < 38000)
            {
                if (JSON_race_result_35000_37999 == NULL) 
                    JSON_race_result_35000_37999 = load_file_and_parse_to_json(socket, raceresult_35000_37999_file);
                JSON_race_result = JSON_race_result_35000_37999;
            }
            else if (raceid_int >= 38000 && raceid_int < 41000)
            {
                if (JSON_race_result_38000_40999 == NULL) 
                    JSON_race_result_38000_40999 = load_file_and_parse_to_json(socket, raceresult_38000_40999_file);
                JSON_race_result = JSON_race_result_38000_40999;
            }
            
            // If no file could be loaded and parsed, free all allocated memory and return
            if (JSON_race_result == NULL)
            {
                if (JSON_athletes != NULL) { 
                    cJSON_Delete(JSON_athletes); 
                    JSON_athletes = NULL; 
                }
                if (JSON_racesinfo != NULL) { 
                    cJSON_Delete(JSON_racesinfo); 
                    JSON_racesinfo = NULL; 
                }
                if (JSON_race_result_0_22999 != NULL) { 
                    cJSON_Delete(JSON_race_result_0_22999); 
                    JSON_race_result_0_22999 = NULL; 
                }
                if (JSON_race_result_23000_25999 != NULL) { 
                    cJSON_Delete(JSON_race_result_23000_25999); 
                    JSON_race_result_23000_25999 = NULL; 
                }
                if (JSON_race_result_26000_28999 != NULL) { 
                    cJSON_Delete(JSON_race_result_26000_28999); 
                    JSON_race_result_26000_28999 = NULL; 
                }
                if (JSON_race_result_29000_31999 != NULL) { 
                    cJSON_Delete(JSON_race_result_29000_31999); 
                    JSON_race_result_29000_31999 = NULL; 
                }
                if (JSON_race_result_32000_34999 != NULL) { 
                    cJSON_Delete(JSON_race_result_32000_34999); 
                    JSON_race_result_32000_34999 = NULL; 
                }
                if (JSON_race_result_35000_37999 != NULL) { 
                    cJSON_Delete(JSON_race_result_35000_37999); 
                    JSON_race_result_35000_37999 = NULL; 
                }
                if (JSON_race_result_38000_40999 != NULL) { 
                    cJSON_Delete(JSON_race_result_38000_40999); 
                    JSON_race_result_38000_40999 = NULL; 
                }
                for (int i = 0; i < number_of_races; i++)
                    destroy_analyzedRace(analyzed_races[i]);

                return -1;
            }

            // Loop though all races from the "results file"
            char* rank = 0;
            long time = 0L;
            long winner_time = 0L;
            cJSON* races_results = cJSON_GetObjectItemCaseSensitive(JSON_race_result, "races");
            cJSON* current_race_results = NULL;
            
            cJSON_ArrayForEach(current_race_results, races_results)
            {
                // Check if the raceids are matching
                cJSON* raceid_field = cJSON_GetObjectItemCaseSensitive(current_race_results, "raceid");
                if (!cJSON_IsString(raceid_field) || raceid_field->valuestring == NULL)
                    continue;
                if (strcmp(raceid_field->valuestring, field->valuestring) != 0)
                    continue;

                cJSON* result_list = cJSON_GetObjectItemCaseSensitive(current_race_results, "result");
                cJSON* current_rank = NULL;
                cJSON_ArrayForEach(current_rank, result_list)
                {
                    cJSON* rank_field = cJSON_GetObjectItemCaseSensitive(current_rank, "rank");
                    cJSON* time_field = cJSON_GetObjectItemCaseSensitive(current_rank, "time");
                    cJSON* fiscode_field = cJSON_GetObjectItemCaseSensitive(current_rank, "fiscode");

                    if (winner_time == 0) 
                    {    
                        if (cJSON_IsString(rank_field) && rank_field->valuestring != NULL &&
                            cJSON_IsString(time_field) && time_field->valuestring != NULL) {
                            if (strcmp(rank_field->valuestring, "1") == 0) 
                            {
                                char* winner_time_str = cJSON_Print(time_field);
                                remove_enclosing_quotes(winner_time_str);
                                winner_time = RaceTime_string_to_ms(winner_time_str);
                                if (winner_time < 0)
                                    winner_time = 0;
                                if (winner_time_str != 0)
                                    free(winner_time_str);
                            }
                        } 
                    }
                    
                    if (cJSON_IsString(fiscode_field) && fiscode_field->valuestring != NULL) {
                        if (strcmp(fiscode_field->valuestring, fiscode) == 0) 
                        {
                            if (cJSON_IsString(rank_field) && rank_field->valuestring != NULL &&
                                cJSON_IsString(time_field) && time_field->valuestring != NULL) 
                            {
                                rank = cJSON_Print(rank_field);    
                                char* time_str = cJSON_Print(time_field);
                                remove_enclosing_quotes(time_str);
                                time = RaceTime_string_to_ms(time_str);
                                if (time < 0)
                                    time = 0;
                                if (time_str != 0)
                                    free(time_str);
                            }
                        }
                    }
                }
            }

            // ------------------------------------------------------------
            // Create an AnalyedRace object
            // ------------------------------------------------------------
            AnalyzedRace* current = (AnalyzedRace*) malloc(sizeof(AnalyzedRace));
            
            // RACEID and TYPE
            current->raceid = cJSON_Print(field);
            current->type = cJSON_Print(field_type); 

            // DATE
            field = cJSON_GetObjectItemCaseSensitive(race, "date");
            if (cJSON_IsString(field) && field->valuestring != NULL)
                current->date = cJSON_Print(field);  

            // GENDER
            field = cJSON_GetObjectItemCaseSensitive(race, "gender");
            if (cJSON_IsString(field) && field->valuestring != NULL)
                current->gender = cJSON_Print(field); 
            
            // NATION
            field = cJSON_GetObjectItemCaseSensitive(race, "nation");
            if (cJSON_IsString(field) && field->valuestring != NULL)
                current->nation = cJSON_Print(field); 

            // LOCATION
            field = cJSON_GetObjectItemCaseSensitive(race, "location");
            if (cJSON_IsString(field) && field->valuestring != NULL)
                current->location = cJSON_Print(field); 
           
            // CATEGORY
            field = cJSON_GetObjectItemCaseSensitive(race, "category");
            if (cJSON_IsString(field) && field->valuestring != NULL)
                current->category = cJSON_Print(field); 

            // FISCODE
            char* fiscode_str = (char*) malloc((strlen(fiscode) + 1) * sizeof(char));
            strcpy(fiscode_str, fiscode);  // Appends '\0' at the end
            current->fiscode = fiscode_str;
            
            // RANK, TIME and WINNER TIME
            current->rank = rank;
            current->time_ms = time;
            current->winner_time_ms = winner_time;

            // DIFF TIME
            current->diff_ms = (time - winner_time);
            if (current->diff_ms < 0)
                current->diff_ms = 0;
            
            // DIFF PERCENTAGE
            current->diff_percentage = ((float)time / winner_time);

            analyzed_races[index] = current;
            break;
        }

        // Free the memory for the athletes current raceid before getting the next one
        if (raceid_str != NULL) 
            free(raceid_str);   
    }
    
    
    //
    // TODO: Test if any more JSON objects needs to be deleted
    //
    
    // --------------------------------------------------------------
    // Delete all cJSON objects that has been allocated
    // --------------------------------------------------------------
    if (JSON_athletes != NULL) { 
        cJSON_Delete(JSON_athletes); 
        JSON_athletes = NULL; 
    }
    if (JSON_racesinfo != NULL) { 
        cJSON_Delete(JSON_racesinfo); 
        JSON_racesinfo = NULL; 
    }
    if (JSON_race_result_0_22999 != NULL) { 
        cJSON_Delete(JSON_race_result_0_22999); 
        JSON_race_result_0_22999 = NULL; 
    }
    if (JSON_race_result_23000_25999 != NULL) { 
        cJSON_Delete(JSON_race_result_23000_25999); 
        JSON_race_result_23000_25999 = NULL; 
    }
    if (JSON_race_result_26000_28999 != NULL) { 
        cJSON_Delete(JSON_race_result_26000_28999); 
        JSON_race_result_26000_28999 = NULL; 
    }
    if (JSON_race_result_29000_31999 != NULL) { 
        cJSON_Delete(JSON_race_result_29000_31999); 
        JSON_race_result_29000_31999 = NULL; 
    }
    if (JSON_race_result_32000_34999 != NULL) { 
        cJSON_Delete(JSON_race_result_32000_34999); 
        JSON_race_result_32000_34999 = NULL; 
    }
    if (JSON_race_result_35000_37999 != NULL) { 
        cJSON_Delete(JSON_race_result_35000_37999); 
        JSON_race_result_35000_37999 = NULL; 
    }
    if (JSON_race_result_38000_40999 != NULL) { 
        cJSON_Delete(JSON_race_result_38000_40999); 
        JSON_race_result_38000_40999 = NULL; 
    }


    // ------------------------------------------------------------
    // Construct a JSON object from the analyzed results
    // ------------------------------------------------------------
    cJSON* result_json = cJSON_CreateObject();
    cJSON* races_array = cJSON_CreateArray();
    
    if (result_json == NULL || races_array == NULL)
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to create JSON object for the analyzed races\n", (long)getpid());
        sendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to create JSON object\n\0");
        
        cJSON_Delete(result_json);
        cJSON_Delete(races_array);
        for (int i = 0; i < number_of_races; i++)
            destroy_analyzedRace(analyzed_races[i]);
        
        return -1;
    }
    cJSON_AddItemToObject(result_json, "races", races_array);


    // ------------------------------------------------------------
    // Convert all AnalyzedRace objects to JSON
    // ------------------------------------------------------------
    AnalyzedRace* analyzedRace = 0;
    cJSON* field_raceid = NULL;
    cJSON* field_date = NULL;
    cJSON* field_type = NULL;
    cJSON* field_gender = NULL;
    cJSON* field_nation = NULL;
    cJSON* field_location = NULL;
    cJSON* field_category = NULL;
    cJSON* field_fiscode = NULL;
    cJSON* field_rank = NULL;
    cJSON* field_time = NULL;
    cJSON* field_winner_time = NULL;
    cJSON* field_diff_ms = NULL;
    cJSON* field_diff_percentage = NULL;

    for (int i = 0; i < number_of_races; i++)
    {
        analyzedRace = analyzed_races[i];
        if (analyzedRace == 0)
            continue;

        cJSON* current_race = cJSON_CreateObject();
        if (current_race == NULL) {
            fprintf(stderr, "[%ld] Failed to create JSON object for one of the analyzed races, skipping...\n", (long)getpid());
            continue;
        }

        remove_enclosing_quotes(analyzedRace->raceid);
        remove_enclosing_quotes(analyzedRace->date);
        remove_enclosing_quotes(analyzedRace->type);
        remove_enclosing_quotes(analyzedRace->gender);
        remove_enclosing_quotes(analyzedRace->nation);
        remove_enclosing_quotes(analyzedRace->location);
        remove_enclosing_quotes(analyzedRace->category);
        remove_enclosing_quotes(analyzedRace->rank);
       
        // Convert the times in milliseconds to null terminated strings 
        char* time_str = RaceTime_ms_to_string(analyzedRace->time_ms);
        char* winner_time_str = RaceTime_ms_to_string(analyzedRace->winner_time_ms);
        char* diff_ms_str = RaceTime_ms_to_string(analyzedRace->diff_ms);
        char* diff_percentage_str = RaceTime_percentage_to_string(analyzedRace->diff_percentage);

        // Create JSON objects for each data field inside the current AnalyzedRace
        // If any of the creations fail, an error message will be printed and that field will be ignored
        if ((field_raceid = cJSON_CreateString(analyzedRace->raceid)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert raceid field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_date = cJSON_CreateString(analyzedRace->date)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert date field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_type = cJSON_CreateString(analyzedRace->type)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert type field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_gender = cJSON_CreateString(analyzedRace->gender)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert gender field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_nation = cJSON_CreateString(analyzedRace->nation)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert nation field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_location = cJSON_CreateString(analyzedRace->location)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert location field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_category = cJSON_CreateString(analyzedRace->category)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert category field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_fiscode = cJSON_CreateString(analyzedRace->fiscode)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert fiscode field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_rank = cJSON_CreateString(analyzedRace->rank)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert rank field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_time = cJSON_CreateString(time_str)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert time field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_winner_time = cJSON_CreateString(winner_time_str)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert winner time field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_diff_ms = cJSON_CreateString(diff_ms_str)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert diff_ms field to JSON for one of the analyzed races\n", (long)getpid());
        if ((field_diff_percentage = cJSON_CreateString(diff_percentage_str)) == NULL)
            fprintf(stderr, "[%ld] Failed to convert diff_percentage field to JSON for one of the analyzed races\n", (long)getpid());
        

        // Combine all fields into a single JSON object add it to races_array
        // If any fields are NULL, they will simply be ignored by the function call
        cJSON_AddItemToObject(current_race, "raceid", field_raceid);
        cJSON_AddItemToObject(current_race, "date", field_date);
        cJSON_AddItemToObject(current_race, "type", field_type);
        cJSON_AddItemToObject(current_race, "gender", field_gender);
        cJSON_AddItemToObject(current_race, "nation", field_nation);
        cJSON_AddItemToObject(current_race, "location", field_location);
        cJSON_AddItemToObject(current_race, "category", field_category);
        cJSON_AddItemToObject(current_race, "fiscode", field_fiscode);
        cJSON_AddItemToObject(current_race, "rank", field_rank);
        cJSON_AddItemToObject(current_race, "time", field_time);
        cJSON_AddItemToObject(current_race, "time_winner", field_winner_time);
        cJSON_AddItemToObject(current_race, "diff", field_diff_ms);
        cJSON_AddItemToObject(current_race, "percentage", field_diff_percentage);
        cJSON_AddItemToArray(races_array, current_race);

        if (time_str != 0)
            free(time_str);
        if (winner_time_str != 0)
            free(winner_time_str);
        if (diff_ms_str != 0)
            free(diff_ms_str);
        if (diff_percentage_str != 0)
            free(diff_percentage_str);
    }
    

    // Free all the allocated memory for all the analyzed races
    for (int i = 0; i < number_of_races; i++) {
        destroy_analyzedRace(analyzed_races[i]);
    }


    // --------------------------------------------------------------
    // Check if the complete json object was constructed as intended 
    // --------------------------------------------------------------
    char* result = NULL;
    result = cJSON_Print(result_json);
    cJSON_Delete(result_json);
    //
    // TODO: When debugging / testing, check if any more JSON objects needs to be deleted
    // This can be done by maybe printing the pointer and/or value at the end to see if any are still there
    //

    if (result == NULL)
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to construct the return data\n", (long)getpid());
        sendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to construct the return data\n\0");
        return -1;
    }

    // --------------------------------------------------------------
    // Send back the race info over the socket as an HTTP Response 
    // --------------------------------------------------------------
    char* result_modified = (char*) malloc((strlen(result) + 2) * sizeof(char));
    strcpy(result_modified, result);
    strcat(result_modified, "\n");
    if (result != 0) 
        free(result);

    if (sendHttpResponse(socket, 200, CONNECTION_CLOSE, TYPE_JSON, result_modified) == -1)
    {
        if (result_modified != 0) 
            free(result_modified);
        return -1;
    }

    fprintf(stderr, "[%ld] HTTP 200: Found and sent back the analyzed race data!\n", (long)getpid());
    if (result_modified != 0) 
        free(result_modified);

    return 0;
}



/*
 * -------------------------------------------------------------------------------------
 * Attempts to load the given file and parse the results to a cJSON object
 * On success, a pointer to this cJSON object will be resturned, but this needs to be manually destroyed later
 * On failure, an HTTP response will be sent over the socket to describe the error
 * An error message will also be printed, and NULL will be returned
 * -------------------------------------------------------------------------------------
 */
static cJSON* load_file_and_parse_to_json(int socket, char* file)
{
    char* buffer = 0;
    int status_code = 500;  // Default status code on failure
    
    if (loadResource(socket, file, &buffer, &status_code) == -1)
    {
        sendHttpResponse(socket, status_code, CONNECTION_CLOSE, TYPE_HTML, "Failed to load requested resource\n\0");
        if (buffer != 0) 
            free(buffer);
        
        return NULL;
    }

    cJSON* json = cJSON_Parse(buffer);
    if (buffer != 0) 
        free(buffer);
    
    if (json == NULL)
    {
        fprintf(stderr, "[%ld] HTTP 500: Failed to parse JSON file\n", (long)getpid());
        sendHttpResponse(socket, 500, CONNECTION_CLOSE, TYPE_HTML, "500 Internal Server Error: Failed to parse file to JSON\n\0");
        cJSON_Delete(json);

        return NULL;
    }
    
    return json;
}


/**
 * -------------------------------------------------------------------------------------
 * Removes the enclosing quotes from a null terminated string
 * Moves the start of string string forward one step and the null terminating character back one step
 * The string (str) HAS to be a null terminated string, or else undefined behaviour may occur
 * Returns 0 on success and -1 on failure
 * -------------------------------------------------------------------------------------
 */
static int remove_enclosing_quotes(char* str)
{
    if (str == 0)
        return -1;

    int size = strlen(str);
    if (size < 3)
        return -1;
    if (str[0] != '"' || str[size - 1] != '"')
        return -1;
    
    // Remove the enclosing quotes
    char* current = str;
    char* previous = str;
    for (int i = 0; i < size; i++)
    {
        if (i > 0)
            *previous = *current;
        previous = current;
        current++;
    }
    str[size - 2] = '\0';

    return 0;
}


/**
 * -------------------------------------------------------------------------------------
 * Frees the allocated memory for all fields inisde the given AnalyzedRace
 * Also frees the memory for the given AnalyzedRace 
 * -------------------------------------------------------------------------------------
 */
static void destroy_analyzedRace(AnalyzedRace* race)
{
    if (race == 0)
        return;
    if (race->raceid != 0) free(race->raceid);
    if (race->date != 0) free(race->date);
    if (race->type != 0) free(race->type);
    if (race->gender != 0) free(race->gender);
    if (race->nation != 0) free(race->nation);
    if (race->location != 0) free(race->location);
    if (race->category != 0) free(race->category);
    if (race->fiscode != 0) free(race->fiscode);
    if (race->rank != 0) free(race->rank);
    if (race != 0) free(race);
    return;
}

