
#include "Database.h"

#include "../LoadFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * --------------------------------------------------------------------------------------------------
 * Loads the race infomation from the database for the given race
 *
 * raceid: The id of the race to look for in the database
 * race_info: The struct that will hold all race information
 *
 * Returns 0 on success
 * Returns -1 on failure. An error message will be printed to describe the error
 * Returns -2 if the given race could not be found
 * --------------------------------------------------------------------------------------------------
 */
int LoadFromDatabase_RaceInfo(int raceid, RaceInfo* race_info)
{
    if (race_info == 0) {
        fprintf(stderr, "[%ld] Failed to load Race Info from the database: The parameter result_info needs to be set\n", (long)getpid());
        return -1;
    }


    // ---------------------------------------------------------------------------
    // Load the file that stores all the race info
    // ---------------------------------------------------------------------------
    char file[] = DB_RACE_INFO;
    char* buffer = 0;
    int buffer_size = 0;
    if (LoadFile(file, &buffer, &buffer_size) < 0) {
        if (buffer) {
            free(buffer);
        }
        return -1;  // The LoadFile function will print the error message
    }


    // ---------------------------------------------------------------------------
    // Loop though the buffer and look for the race results for the given raceid
    // ---------------------------------------------------------------------------
    bool foundRace = false;
    int currentByte = 0;
    while (currentByte < buffer_size)  // Loop through all the bytes in the file
    {
        // Make sure it is possible to read the following 4 bytes
        if (currentByte + 4 >= buffer_size) {
            break;
        }

        // Read the current raceid 
        unsigned char bytes[4];
        bytes[0] = buffer[currentByte++];
        bytes[1] = buffer[currentByte++];
        bytes[2] = buffer[currentByte++];
        bytes[3] = buffer[currentByte++];
        unsigned int currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);


        if (currentRaceid != raceid)
        {
            // Skip past all the bytes for the current race
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
            // Make sure it is possible to read the following 4 bytes
            if (currentByte + 4 >= buffer_size) {
                break;
            }

            // Codex
            bytes[0] = buffer[currentByte++];
            bytes[1] = buffer[currentByte++];
            bytes[2] = buffer[currentByte++];
            bytes[3] = buffer[currentByte++];
            race_info->codex = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

            // Date
            int str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                race_info->date[str_index++] = buffer[currentByte - 1];
            }
            race_info->date[str_index] = '\0';

            // Nation
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                race_info->nation[str_index++] = buffer[currentByte - 1];
            }
            race_info->nation[str_index] = '\0';

            // Location
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                race_info->location[str_index++] = buffer[currentByte - 1];
            }
            race_info->location[str_index] = '\0';

            // Category
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                race_info->category[str_index++] = buffer[currentByte - 1];
            }
            race_info->category[str_index] = '\0';

            // Discipline
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                race_info->discipline[str_index++] = buffer[currentByte - 1];
            }
            race_info->discipline[str_index] = '\0';

            // Type
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                race_info->type[str_index++] = buffer[currentByte - 1];
            }
            race_info->type[str_index] = '\0';

            // Gender
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 8) {
                race_info->gender[str_index++] = buffer[currentByte - 1];
            }
            race_info->gender[str_index] = '\0';

            foundRace = true;
            break;
        }
    }

    if (buffer) {
        free(buffer);
    }

    if (!foundRace) {
        fprintf(stderr, "[%ld] Failed to load Race Info from the database: could not find race %d in the database\n", (long)getpid(), raceid);
        return -2;
    }

    return 0;
}







