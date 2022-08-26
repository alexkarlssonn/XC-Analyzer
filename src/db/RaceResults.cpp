
#include "Database.h"

#include "../LoadFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * --------------------------------------------------------------------------------------------------
 * Loads the race results from the database for the given race
 *
 * raceid: The id of the race to look for in the database
 * results: Will contain a list of ResultElement if the requested race was found. 
 *          The memory will be allocated if the race was found, and needs to be manuelly freed later
 * results_size: The number of ResultElement that has been stored inside "results" if the race was found
 *
 * Return 0 on success, and -1 on failure. An error message will be printed to descirbe the error
 * --------------------------------------------------------------------------------------------------
 */
int LoadFromDatabase_RaceResults(int raceid, ResultElement** results, int* results_size)
{
    if (*results != 0) {
        fprintf(stderr, "[%ld] Failed to load Race Results from the database: The parameter results needs to be set to 0\n", (long)getpid());
        return -1;
    }


    // ---------------------------------------------------------------------------
    // Load the file that stores all race results
    // ---------------------------------------------------------------------------
    char file[] = DB_RACE_RESULTS;
    char* buffer = 0;
    int buffer_size = 0;
    if (LoadFile(file, &buffer, &buffer_size) == -1) {
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
        // Make sure it is possible to read the following 6 bytes
        if (currentByte + 6 >= buffer_size) {
            break;
        }

        // Read the current raceid and how many ranks the result list has
        unsigned int currentRaceid;
        unsigned int numberOfRanks;
        unsigned char bytes[4];

        bytes[0] = buffer[currentByte++];
        bytes[1] = buffer[currentByte++];
        bytes[2] = buffer[currentByte++];
        bytes[3] = buffer[currentByte++];
        currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        bytes[0] = buffer[currentByte++];
        bytes[1] = buffer[currentByte++];
        numberOfRanks = bytes[0] | (bytes[1] << 8);


        if (currentRaceid != raceid)
        {
            // Skip past all the bytes for the current race
            for (int i = 0; i < numberOfRanks; i++) {
                currentByte += 18;
                int stringCounter = 0;
                while (stringCounter < 3 && currentByte < buffer_size) {
                    if (buffer[currentByte++] == '\0') {
                        stringCounter++;
                    }
                }
            }
        }
        else
        {
            // Allocate memory for all the results
            if ((*results = (ResultElement*) malloc(numberOfRanks * sizeof(ResultElement))) == 0) {
                if (buffer) {
                    free(buffer);
                }
                fprintf(stderr, "[%ld] Failed to load Race Results from the database: failed to allocate memory for the results\n", (long)getpid());
                return -1;
            }
            
            for (int i = 0; i < numberOfRanks; i++)
            {
                // Make sure it is possible to read the following 18 bytes
                if (currentByte + 18 >= buffer_size) {
                    break;
                }

                bytes[0] = buffer[currentByte++];
                bytes[1] = buffer[currentByte++];
                (*results)[i].rank = bytes[0] | (bytes[1] << 8);

                bytes[0] = buffer[currentByte++];
                bytes[1] = buffer[currentByte++];
                (*results)[i].bib = bytes[0] | (bytes[1] << 8);

                bytes[0] = buffer[currentByte++];
                bytes[1] = buffer[currentByte++];
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                (*results)[i].fiscode = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                bytes[0] = buffer[currentByte++];
                bytes[1] = buffer[currentByte++];
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                (*results)[i].time = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
                
                bytes[0] = buffer[currentByte++];
                bytes[1] = buffer[currentByte++];
                bytes[2] = buffer[currentByte++];
                bytes[3] = buffer[currentByte++];
                (*results)[i].diff = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
                
                bytes[0] = buffer[currentByte++];
                bytes[1] = buffer[currentByte++];
                (*results)[i].year = bytes[0] | (bytes[1] << 8);

                int str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    (*results)[i].name[str_index++] = buffer[currentByte - 1];
                }
                (*results)[i].name[str_index] = '\0';

                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    (*results)[i].nation[str_index++] = buffer[currentByte - 1];
                }
                (*results)[i].nation[str_index] = '\0';

                str_index = 0;
                while (currentByte < buffer_size && buffer[currentByte++] != '\0' && str_index < 256) {
                    (*results)[i].fispoints[str_index++] = buffer[currentByte - 1];
                }
                (*results)[i].fispoints[str_index] = '\0';
            }

            // The race was found and the data has been read
            *results_size = numberOfRanks;        
            foundRace = true;
            break;
        }
    }

    if (buffer) {
        free(buffer);
    }

    if (!foundRace) {
        fprintf(stderr, "[%ld] Failed to load Race Results from the database: could not find race %d in the database\n", (long)getpid(), raceid);
        return -1;
    }

    return 0;
}







