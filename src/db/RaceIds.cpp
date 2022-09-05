
#include "Database.h"

#include "../LoadFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/**
 * --------------------------------------------------------------------------------------------------
 * Loads the all race ids for a given athlete from the database
 *
 * fiscode: The fiscode of the athlete to look for in the database
 * raceids: The array that will hold the race ids. This will be dynamically allocated if the athlete was found, and needs to be manually freed later!
 *
 * Returns 0 on success
 * Returns -1 on failure. An error message will be printed to describe the error
 * Returns -2 if the given athlete could not be found
 * --------------------------------------------------------------------------------------------------
 */
int LoadFromDatabase_RaceIds(int fiscode, unsigned int** raceids, int* raceids_size)
{
    if (fiscode < 0) {
        fprintf(stderr, "[%ld] Failed to load Race Ids from the database: Invalid fiscode parameter: %d\n", (long)getpid(), fiscode);
        return -1;
    }
    if (*raceids != 0) {
        fprintf(stderr, "[%ld] Failed to load Race Ids from the database: The parameter resultids needs to be set to 0\n", (long)getpid());
        return -1;
    }


    // ---------------------------------------------------------------------------
    // Load the file that stores all the race ids
    // ---------------------------------------------------------------------------
    char file[] = DB_ATHLETE_RACES;
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
    while (currentByte < buffer_size)  // Loop through all the bytes in the file
    {
        // Make sure the following 8 bytes can be read
        if (currentByte + 7 >= buffer_size) {
            break;
        }

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

        // If the current fiscode does not match the requested one, then skip over all the race field
        if (currentFiscode != fiscode) 
        {
            currentByte += (numberOfRaces * 4);
            continue;
        }
        else
        {
            // Allocate memory for all the race ids
            if (numberOfRaces > 0) {
                if ((*raceids = (unsigned int*) malloc(numberOfRaces * sizeof(unsigned int))) == 0) 
                {
                    fprintf(stderr, "[%ld] Failed to load Race Ids from the database: Failed to allocate memory for the race ids\n", (long)getpid());
                    if (buffer) {
                        free(buffer);
                    }
                    return -1;
                }
            }

            for (int i = 0; i < numberOfRaces; i++)
            {
                // Make sure the following 4 bytes can be read
                if (currentByte + 3 >= buffer_size) {
                    break;
                }

                // Read the race id for the current race
                bytes[0] = buffer[currentByte++];
                bytes[1] = buffer[currentByte++];
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                (*raceids)[i] = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            }

            *raceids_size = numberOfRaces;
            foundAthlete = true;
            break;
        }
    }


    if (buffer) {
        free(buffer);
    }

    if (!foundAthlete) {
        fprintf(stderr, "[%ld] Failed to load Race Ids from the database: could not find athlete with fiscode %d in the database\n", (long)getpid(), fiscode);
        return -2; 
    }

    return 0;
}





