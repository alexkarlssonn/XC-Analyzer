
#include "Database.h"

#include "../LoadFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * --------------------------------------------------------------------------------------------------
 * Loads the athlete with the given fiscode from the database
 *
 * fiscode: The fiscode of the athlete to look for in the database
 * athlete: The structure containing the athlete data
 *
 * Returns 0 on success
 * Returns -1 on failure. An error message will be printed to describe the error
 * Returns -2 if the given athlete could not be found
 * --------------------------------------------------------------------------------------------------
 */
int LoadFromDatabase_Athlete(int fiscode, Athlete* athlete)
{
    if (athlete == 0) {
        fprintf(stderr, "[%ld] Failed to load athlete from the database: The parameter athlete needs to be set\n", (long)getpid());
        return -1;
    }


    // ---------------------------------------------------------------------------
    // Load the file that stores all the athletes
    // ---------------------------------------------------------------------------
    char file[] = DB_ATHLETES;
    char* buffer = 0;
    int buffer_size = 0;
    if (LoadFile(file, &buffer, &buffer_size) < 0) {
        if (buffer) {
            free(buffer);
        }
        return -1;  // The LoadFile function will print the error message
    }


    // ---------------------------------------------------------------------------
    // Loop though the buffer and look for the athlete with the given fiscode
    // ---------------------------------------------------------------------------
    bool foundAthlete = false;
    int currentByte = 0;
    while (currentByte < buffer_size)
    {
        // Make sure the following 4 bytes can be read
        if (currentByte + 3 >= buffer_size) {
            break;
        }

        // Extract and combines the bytes that stores the fiscode
        unsigned char bytes[4];
        bytes[0] = buffer[currentByte++];
        bytes[1] = buffer[currentByte++];
        bytes[2] = buffer[currentByte++];
        bytes[3] = buffer[currentByte++];
        unsigned int combined_bytes = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        // If this is not the requested athlete: Move past all the data for this current athlete
        if (fiscode != combined_bytes) 
        {
            currentByte += 4;  // Skip the compid field
            int string_counter = 0;
            while (string_counter < 6 && currentByte < buffer_size) {
                if (buffer[currentByte++] == '\0') {
                    string_counter++;  // Skip all string fields 
                }
            }
            continue;
        }
        else
        {
            // Make sure the following 4 bytes can be read
            if (currentByte + 3 >= buffer_size) {
                break;
            }
            
            // Set the fiscode
            athlete->fiscode = combined_bytes; 

            // Read the compid
            bytes[0] = buffer[currentByte++];
            bytes[1] = buffer[currentByte++];
            bytes[2] = buffer[currentByte++];
            bytes[3] = buffer[currentByte++];
            athlete->compid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

            // Read the firstname
            int str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                athlete->firstname[str_index++] = buffer[currentByte - 1];
            }
            athlete->firstname[str_index] = '\0';

            // Read the lastname
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                athlete->lastname[str_index++] = buffer[currentByte - 1];
            }
            athlete->lastname[str_index] = '\0';

            // Read the nation
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                athlete->nation[str_index++] = buffer[currentByte - 1];
            }
            athlete->nation[str_index] = '\0';

            // Read the birthdate
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 32) {
                athlete->birthdate[str_index++] = buffer[currentByte - 1];
            }
            athlete->birthdate[str_index] = '\0';

            // Read the gender
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 8) {
                athlete->gender[str_index++] = buffer[currentByte - 1];
            }
            athlete->gender[str_index] = '\0';

            // Read the club
            str_index = 0;
            while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                athlete->club[str_index++] = buffer[currentByte - 1];
            }
            athlete->club[str_index] = '\0';

            foundAthlete = true;
            break;
        }
    }

    if (buffer) {
        free(buffer);
    }

    if (!foundAthlete) {
        fprintf(stderr, "[%ld] Failed to load athlete from the database: Could not find fiscode: %d\n", (long)getpid(), fiscode);
        return -2;
    }

    return 0;
}




