
#include "ConvertDB.h"

#include "../src/libs/Restart.h"
#include "../src/libs/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/**
 * -------------------------------------------------------------------------------------------------
 * Converts the races results from the database that are stored as JSON objects, into my own custom file format
 * This file format will store the data as binary data which will make accessing and searching the db much faster!
 * The converted data are stored in a new file that can be found under the "output" directory
 * -------------------------------------------------------------------------------------------------
 */
int convert_racesResults_from_json_to_custom_format(bool print_buffer)
{
    char file_1[] = "../db/races-results/races-results-0-22999.json";
    char file_2[] = "../db/races-results/races-results-23000-25999.json";
    char file_3[] = "../db/races-results/races-results-26000-28999.json";
    char file_4[] = "../db/races-results/races-results-29000-31999.json";
    char file_5[] = "../db/races-results/races-results-32000-34999.json";
    char file_6[] = "../db/races-results/races-results-35000-37999.json";
    char file_7[] = "../db/races-results/races-results-38000-40999.json";


    //
    // TODO: Allocate a "total-buffer" that can write all the converted data into a single file as well
    //

    for (int i = 0; i < 7; i++)
    {
        char* file = 0;
        if (i == 0) file = file_1;
        if (i == 1) file = file_2;
        if (i == 2) file = file_3;
        if (i == 3) file = file_4;
        if (i == 4) file = file_5;
        if (i == 5) file = file_6;
        if (i == 6) file = file_7;
        fprintf(stderr, "Converting file: %s\n", file);

        // ------------------------------------------------------------------------------------
        // Load and parse the current file, and allocate a buffer to store the converted data
        // ------------------------------------------------------------------------------------
        int loadedFileSize = 0;
        cJSON* json = loadAndParse(file, &loadedFileSize);
        if (json == 0)
            return -1;

        int size = 0;  // Current buffer size
        unsigned char* buffer = (unsigned char*) malloc(loadedFileSize * sizeof(unsigned char));
        if (buffer == 0) {
            perror("Failed to allocate buffer for storing new formated data");
            return -1;
        }
     
        
        // ------------------------------------------------------------------------------------
        // Loop through all races in the cJSON object 
        // ------------------------------------------------------------------------------------
        cJSON* races = cJSON_GetObjectItemCaseSensitive(json, "races");
        cJSON* race;
        cJSON_ArrayForEach(race, races)
        {
            
        }

        cJSON_Delete(json);
        if (buffer != 0)
            free(buffer);

    }
   
/*
    char inputFile[] = "../db/races-info.json";
    int loadedFileSize = 0;
    cJSON* json = loadAndParse(inputFile, &loadedFileSize);
    if (json == 0)
        return -1;

    int size = 0;  // Current buffer size
    unsigned char* buffer = (unsigned char*) malloc(loadedFileSize * sizeof(unsigned char));
    if (buffer == 0) {
        perror("Failed to allocate buffer for storing new formated data");
        return -1;
    }
    for (int i = 0; i < loadedFileSize; i++)
        buffer[i] = 0;  // Make sure the buffer is filled with 0's
*/

    


    return 0;
}

