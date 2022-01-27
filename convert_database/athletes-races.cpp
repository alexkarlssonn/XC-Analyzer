
#include "ConvertDB.h"

#include "../src/libs/Restart.h"
#include "../src/libs/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * -------------------------------------------------------------------------------------------------
 * Converts the athletes races from the database that are stored as JSON objects, into my own custom file format
 * This file format will store the data as binary data which will make accessing and searching the db much faster!
 * The converted athletes are stored in a new file that can be found under the "output" directory
 * -------------------------------------------------------------------------------------------------
 */
int convert_athletesRaces_from_json_to_custom_format(bool print_buffer)
{
    // ------------------------------------------------------------------------------
    // Load the file and allocate a buffer that will store the converted file data 
    // ------------------------------------------------------------------------------
    char inputFile[] = "../db/athletes-races.json";
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

    
    // ------------------------------------------------------------------------------
    // Loop through all athletes in the cJSON object 
    // ------------------------------------------------------------------------------
    int counter = -1;
    cJSON* athletes = cJSON_GetObjectItemCaseSensitive(json, "athletes");
    cJSON* athlete = NULL;
    cJSON* fiscode = NULL;
    cJSON* raceids = NULL;
    cJSON* raceid = NULL;
    cJSON_ArrayForEach(athlete, athletes)
    {
        if ((raceids = cJSON_GetObjectItemCaseSensitive(athlete, "races")) == 0 || 
            (fiscode = cJSON_GetObjectItemCaseSensitive(athlete, "fiscode")) == 0)
            continue;
        counter++;
       
        // Write the fiscode (u32) to the buffer in individual bytes (little endian)
        unsigned int fiscode_u32 = atoi(fiscode->valuestring);
        buffer[size] = (fiscode_u32 & 0xFF);
        buffer[++size] = (fiscode_u32 >> 8) & 0xFF;
        buffer[++size] = (fiscode_u32 >> 16) & 0xFF;
        buffer[++size] = (fiscode_u32 >> 24) & 0xFF;

        // Write the number of raceids (u32) for this athlete to the buffer in individual bytes (little endian)
        unsigned int raceids_size = cJSON_GetArraySize(raceids);
        buffer[++size] = (raceids_size & 0xFF);
        buffer[++size] = (raceids_size >> 8) & 0xFF;
        buffer[++size] = (raceids_size >> 16) & 0xFF;
        buffer[++size] = (raceids_size >> 24) & 0xFF;

        if (raceids_size > 0) {
            cJSON_ArrayForEach(raceid, raceids)
            {
                // Go through all raceids and write each one (u32) to the buffer in individual bytes (little endian)
                unsigned int raceid_u32 = atoi(raceid->valuestring);
                buffer[++size] = (raceid_u32 & 0xFF);
                buffer[++size] = (raceid_u32 >> 8) & 0xFF;
                buffer[++size] = (raceid_u32 >> 16) & 0xFF;
                buffer[++size] = (raceid_u32 >> 24) & 0xFF;
            }
        }
        size++;
    } 

    cJSON_Delete(json);


    // Print each byte in the buffer in hex
    if (print_buffer)
        printBuffer(&buffer, size);
    fprintf(stderr, "Converted %d athletes!\n\n", counter+1);


    // ------------------------------------------------------------------------
    // Save the buffer to the given file
    // ------------------------------------------------------------------------
    char file[] = "./output/athletes-races.txt";
    int fd;
    if ((fd = r_open2(file, O_RDWR)) == -1) {
        perror("Failed to open output file");
        if (buffer != 0)
            free(buffer);
        return -1;
    }
    int byteswritten = 0;
    if ((byteswritten = r_write(fd, buffer, size)) < size) {
        perror("Failed to write entire buffer to output file");
    } else {
        fprintf(stderr, "All athletes races was converted and written to %s successfully!\n", file);
    }    
    if (buffer != 0)
        free(buffer);

    return 0;
}

/**
 * --------------------------------------------------------------------------------
 * Loads all atheltes races that are stored in the new custom file format
 * Once the file has been loaded they will be printed 
 * 
 * This function is a "proof of concept" to show and test that the data can be read and parsed
 * successfully from this new custom file format
 * --------------------------------------------------------------------------------
 */
int load_and_print_athletesRaces(bool printAll)
{
    // Open and read all bytes from the file
    char file[] = "./output/athletes-races.txt";
    int fd; 
    if ((fd = r_open2(file, O_RDONLY)) == -1) {
        perror("Failed to open file");
        return -1;
    } 
    int filesize = (int) lseek(fd, 0, SEEK_END); 
    if ((int)lseek(fd, 0, SEEK_SET) == -1) { 
        perror("Failed to move file offset back to the beginning");
        return -1;
    }
    unsigned char* buffer = 0;
    if ((buffer = (unsigned char*) malloc( (filesize+1) * sizeof(unsigned char))) == 0) {
        perror("Failed to allocate buffer for loading file");
        return -1;
    }
    int bytesread = 0;
    if ((bytesread = readblock(fd, buffer, filesize)) <= 0) {
        perror("Failed to read content of file into buffer");
        if (buffer != 0)
            free(buffer);
        return -1;
    }
    if (r_close(fd) == -1) {
        fprintf(stderr, "Failed to close the file\n");
    }


    // Parse the file and print out all athletes races
    for (int i = 0; i < bytesread;)
    {
        // Read the fiscode
        unsigned char bytes[4];
        bytes[0] = buffer[i];    
        bytes[1] = buffer[++i];    
        bytes[2] = buffer[++i];    
        bytes[3] = buffer[++i];
        unsigned int fiscode = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        // Read how many raceids exist for this athlete    
        bytes[0] = buffer[++i];    
        bytes[1] = buffer[++i];    
        bytes[2] = buffer[++i];    
        bytes[3] = buffer[++i];
        unsigned int number_of_raceids = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
       
        if (printAll) {
            fprintf(stderr, "\nFiscode: %d. Raceids: ", fiscode);
        }

        for (int j = 0; j < number_of_raceids; j++) 
        {
            // Read the current raceid    
            bytes[0] = buffer[++i];    
            bytes[1] = buffer[++i];    
            bytes[2] = buffer[++i];    
            bytes[3] = buffer[++i];
            unsigned int raceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            
            if (printAll) {
                fprintf(stderr, "%d, ", raceid);
            }
        }
        i++;
    }
    
    fprintf(stderr, "\nDONE! %d bytes was read and processed!\n", bytesread);
    if (buffer != 0)
        free(buffer);
    return 0;
}



