
#include "CreatePages.h"

#include "../LoadFile.h"
#include "../db/Database.h"
#include "../util/RaceTime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * ------------------------------------------------------------------------------------------------------
 * Writes the bytes inside "string" to "buffer" and updates the value of "currentByte"
 *
 * string: The string that you want to write to the buffer. It has to be a null terminated string!
 * buffer: The buffer to write to
 * buffer_size: The size of the memory that has been allocated for buffer
 * currentByte: The byte that is currently being written to at the end of buffer
 *
 * Returns 0 on success, and -1 on failure
 * ------------------------------------------------------------------------------------------------------
 */
static int WriteToBuffer(char* string, char** buffer, int buffer_size, int* currentByte)
{
    if (string == 0 || *buffer == 0 || buffer_size == 0 || (*currentByte >= buffer_size)) {
        return -1;
    }

    while (*string != '\0') {
        if (*currentByte < buffer_size) {
            (*buffer)[*currentByte] = *string;
            (*currentByte)++;
        }
        string++;
    }

    return 0;
}





// CAN RETURN -2!
int CreatePage_RaceResults(int raceid, char** PageBuffer, int* PageBuffer_size)
{
    if (*PageBuffer != 0) {
        fprintf(stderr, "[%ld] Failed to create page for Race Results: the buffer parameter needs to be set to 0\n", (long)getpid());
        return -1;
    }
    
    // -----------------------------------------------------------------------------
    // Load the race data from the database and the html template file
    // -----------------------------------------------------------------------------
    int res = 0;
    RaceInfo race_info;
    if ((res = LoadFromDatabase_RaceInfo(raceid, &race_info)) < 0) {
        // The LoadFromDatabase function will print the error message
        // If it returned -1, then it is a server error. 
        // If it returned -2, then the race was not found
        return res;  
    }

    int number_of_results = 0;
    ResultElement* results = 0;
    if (LoadFromDatabase_RaceResults(raceid, &results, &number_of_results) == -1) {
        return -1;  // The LoadFromDatabase function will print the error message
    }

    char file[] = TEMPLATE_RACE;
    char* buffer = 0;
    int buffer_size = 0;
    if (LoadFile(file, &buffer, &buffer_size) == -1) {
        // The LoadFile function will print the error message
        if (buffer) { free(buffer); }
        if (results) { free(results); }
        return -1;  
    }


    // --------------------------------------------------------------------------------------------
    // Allocate memory for the PageBuffer
    // Using an extra 256 bytes for each ResultElement to make sure it can 
    // store the template file, and the Race data that will be inserted into it
    // --------------------------------------------------------------------------------------------
    int results_memory_size = number_of_results * (sizeof(ResultElement) + 256); 
    *PageBuffer_size = buffer_size + sizeof(race_info) + results_memory_size;
    
    if ((*PageBuffer = (char*) malloc(*PageBuffer_size * sizeof(char))) == 0) {
        fprintf(stderr, "[%ld] Failed to create page for Race Results: failed to allocate memory for the page\n", (long)getpid());
        if (buffer) { free(buffer); }
        if (results) { free(results); }
        return -1;
    }


    // --------------------------------------------------------------------------------------------
    // Copy the template file to the PageBuffer and replace the placeholders 
    // with the Race data that was loaded from the database
    // --------------------------------------------------------------------------------------------
    int PageBuffer_currentByte = 0;  // Represents the current position in the PageBuffer
    int currentByte = 0;             // Represents the current position in the buffer (template file)
    while (currentByte < buffer_size)
    {
        // '@' Indicates a placeholder in the template file
        // If a placeholder is not detected, then copy the current byte from the buffer to the PageBuffer
        if (buffer[currentByte] != '@') {
            if (PageBuffer_currentByte < *PageBuffer_size) {
                (*PageBuffer)[PageBuffer_currentByte] = buffer[currentByte];
                PageBuffer_currentByte++;
            }
            currentByte++;
            continue;
        }
        
        // A placeholder was detected
        // Attempt to create a substring for the placeholder text
        currentByte += 2;
        char* placeholder = 0;
        int placeholder_start = currentByte;
        
        while (currentByte < buffer_size) {
            if (buffer[currentByte] == '}') {
                placeholder = &(buffer[placeholder_start]);
                buffer[currentByte] = '\0';
                currentByte++;
                break;
            }
            currentByte++;
        }

        // Replace the placeholder with the given data from the database
        if (placeholder) 
        {
            if (strcmp(placeholder, "RACEID") == 0)
            {
                char raceid_str[16];
                sprintf(raceid_str, "%d", raceid);
                WriteToBuffer(&(raceid_str[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "RACE_INFO_CODEX") == 0) 
            {
                char codex[16];
                sprintf(codex, "%u", race_info.codex);
                WriteToBuffer(&(codex[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "RACE_INFO_DATE") == 0)
            {
                char* p = &(race_info.date[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }    
            else if (strcmp(placeholder, "RACE_INFO_NATION") == 0)
            {
                char* p = &(race_info.nation[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "RACE_INFO_LOCATION") == 0)
            {
                char* p = &(race_info.location[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "RACE_INFO_CATEGORY") == 0)
            {
                char* p = &(race_info.category[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "RACE_INFO_DISCIPLINE") == 0)
            {
                char* p = &(race_info.discipline[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "RACE_INFO_TYPE") == 0)
            {
                char* p = &(race_info.type[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "RACE_INFO_GENDER") == 0)
            {
                char* p = &(race_info.gender[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "RACE_RESULTS") == 0)
            {
                for (int i = 0; i < number_of_results; i++)
                {
                    char a_start_even[] = "<a class='race-result even'>\n";
                    char a_start_odd[] = "<a class='race-result odd'>\n";
                    char a_end[] = "</a>\n";
                    char div_end[] = "</div>\n";

                    if (i % 2 == 0) {
                        WriteToBuffer(&(a_start_even[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    } else {
                        WriteToBuffer(&(a_start_odd[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // RANK
                    {
                        char div_start[] = "<div class='race-result-field rank-field'>";
                        char rank[16];
                        sprintf(rank, "%u", results[i].rank);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(rank[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // BIB
                    {
                        char div_start[] = "<div class='race-result-field bib-field'>";
                        char bib[16];
                        sprintf(bib, "%u", results[i].bib);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(bib[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // FISCODE
                    {
                        char div_start[] = "<div class='race-result-field fiscode-field'>";
                        char fiscode[16];
                        sprintf(fiscode, "%u", results[i].fiscode);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(fiscode[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // NAME
                    {
                        char div_start[] = "<div class='race-result-field name-field'>";
                        char* p = &(results[i].name[0]);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // YEAR
                    {
                        char div_start[] = "<div class='race-result-field year-field'>";
                        char year[16];
                        sprintf(year, "%u", results[i].year);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(year[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // NATION
                    {
                        char div_start[] = "<div class='race-result-field nation-field'>";
                        char* p = &(results[i].nation[0]);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // TIME
                    {
                        char div_start[] = "<div class='race-result-field time-field'>";
                        char* time = RaceTime_ms_to_string(results[i].time);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (time) {
                            WriteToBuffer(&(time[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);

                        if (time) { free(time); }
                    }

                    // DIFF
                    {
                        char div_start[] = "<div class='race-result-field diff-field'>";
                        char* diff = RaceTimeDiff_ms_to_string(results[i].diff);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (diff && results[i].diff != 0) {
                            WriteToBuffer(&(diff[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // FISPOINTS
                    {
                        char div_start[] = "<div class='race-result-field fispoints-field'>";
                        char* p = &(results[i].fispoints[0]);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    
                    }

                    // Write the end of the div element to the PageBuffer
                    WriteToBuffer(&(a_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                }
            }
        }
    } 

   
    // Set a null character at the end of the PageBuffer
    // Also set PageBuffer_size to be the size of the content that was written to the PageBuffer
    if (PageBuffer_currentByte < *PageBuffer_size) {
        (*PageBuffer)[PageBuffer_currentByte] = '\0';
        PageBuffer_currentByte++;
    }
    *PageBuffer_size = PageBuffer_currentByte;

    if (buffer) { free(buffer); }
    if (results) { free(results); }
    return 0;
}









