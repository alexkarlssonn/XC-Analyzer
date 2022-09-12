
#include "CreatePages.h"

#include "../LoadFile.h"
#include "../db/Database.h"
#include "../util/RaceTime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int GetRaceData_FromRaceInfo(RaceData* raceData, unsigned int raceid, int* start_byte, char* race_info_buffer, int race_info_buffer_size);
static int GetRaceData_FromRaceResults(RaceData* raceData, unsigned int raceid, int* start_byte, unsigned int fiscode, char* race_results_buffer, int race_results_buffer_size);


/**
 * -------------------------------------------------------------------------------------------
 * Dynamically creates the web page for a given athlete
 *
 * fiscode: The fiscode of the athlete to use when creating the page
 * PageBuffer: The buffer that will store the dynamically generated page. Needs to be manually freed later!
 * PageBuffer_size: The size of the PageBuffer once it has been created
 *
 * Returns 0 on success
 * Returns -1 on failure
 * Returns -2 if the given athlete could not be found in the database
 * -------------------------------------------------------------------------------------------
 */
int CreatePage_Athlete(int fiscode, char** PageBuffer, int* PageBuffer_size)
{
    if (*PageBuffer != 0) {
        fprintf(stderr, "[%ld] Failed to create page for Athlete: the buffer parameter needs to be set to 0\n", (long)getpid());
        return -1;
    }

    // -----------------------------------------------------------------------------
    // Load the requierd data from the database
    // -----------------------------------------------------------------------------
    int res = 0;
    Athlete athlete;
    if ((res = LoadFromDatabase_Athlete(fiscode, &athlete)) < 0) {
        // The LoadFromDatabase function will print the error message
        // If it returned -1, then it is a server error.
        // If it returned -2, then the athlete was not found
        return res;
    }

    unsigned int* raceids = 0;
    int number_of_raceids = 0;
    if ((res = LoadFromDatabase_RaceIds(fiscode, &raceids, &number_of_raceids)) < 0) {
        // The LoadFromDatabase function will print the error message
        // If it returned -1, then it is a server error.
        // If it returned -2, then the raceids/athlete was not found
        return res;
    }


    // -----------------------------------------------------------------------------
    // Load the race files, and the template file
    // -----------------------------------------------------------------------------
    char FILE_RACEINFO[] = DB_RACE_INFO;
    char* race_info_buffer = 0;
    int race_info_buffer_size = 0;

    if (LoadFile(FILE_RACEINFO, &race_info_buffer, &race_info_buffer_size) < 0) 
    {
        if (raceids) { free(raceids); }
        if (race_info_buffer) { free(race_info_buffer); }
        return -1;  // The LoadFile function will print the error message
    }

    char FILE_RESULTS[] = DB_RACE_RESULTS;
    char* race_results_buffer = 0;
    int race_results_buffer_size = 0;
    
    if (LoadFile(FILE_RESULTS, &race_results_buffer, &race_results_buffer_size) < 0) 
    {
        if (raceids) { free(raceids); }
        if (race_info_buffer) { free(race_info_buffer); }
        if (race_results_buffer) { free(race_results_buffer); }
        return -1;  // The LoadFile function will print the error message
    }
    
    char FILE_TEMPLATE[] = TEMPLATE_ATHLETE;
    char* template_buffer = 0;
    int template_buffer_size = 0;
    
    if (LoadFile(FILE_TEMPLATE, &template_buffer, &template_buffer_size) < 0) 
    {
        // The LoadFile function will print the error message
        if (raceids) { free(raceids); }
        if (race_info_buffer) { free(race_info_buffer); }
        if (race_results_buffer) { free(race_results_buffer); }
        if (template_buffer) { free(template_buffer); }
        return -1;
    }


    // --------------------------------------------------------------------------------------------
    // Allocate memory for the PageBuffer
    // Using the size of the template file as the base, then it adds additional bytes for the
    // athlete info. It also adds additional bytes for each raceids, to make sure all races
    // can be added to the buffer when the placeholders gets replaced
    // --------------------------------------------------------------------------------------------
    int sprint_stats_memory_size = (number_of_raceids * 1024);    
    int athlete_memory_size = (sizeof(Athlete) + 256);
    *PageBuffer_size = template_buffer_size + athlete_memory_size + sprint_stats_memory_size;

    if ((*PageBuffer = (char*) malloc(*PageBuffer_size * sizeof(char))) == 0) 
    {
        fprintf(stderr, "[%ld] Failed to create page for Athlete: failed to allocate memory for the page\n", (long)getpid());
        if (raceids) { free(raceids); }
        if (race_info_buffer) { free(race_info_buffer); }
        if (race_results_buffer) { free(race_results_buffer); }
        if (template_buffer) { free(template_buffer); }
        return -1;
    }


    // --------------------------------------------------------------------------------------------
    // Copy the template file to the PageBuffer and replace the 
    // placeholders with the data that was loaded from the database
    // --------------------------------------------------------------------------------------------
    int PageBuffer_currentByte = 0;  // Represents the current position in the PageBuffer
    int currentByte = 0;             // Represents the current position in the buffer (template file)
    while (currentByte < template_buffer_size)
    {
        // '@' Indicates a placeholder in the template file
        // If a placeholder is not detected, then copy the current byte from the buffer to the PageBuffer
        if (template_buffer[currentByte] != '@') {
            if (PageBuffer_currentByte < *PageBuffer_size) {
                (*PageBuffer)[PageBuffer_currentByte] = template_buffer[currentByte];
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

        while (currentByte < template_buffer_size) {
            if (template_buffer[currentByte] == '}') {
                placeholder = &(template_buffer[placeholder_start]);
                template_buffer[currentByte] = '\0';
                currentByte++;
                break;
            }
            currentByte++;
        }

        // Replace the placeholder with the given data from the database
        if (placeholder)
        {
            if (strcmp(placeholder, "ATHLETE_FISCODE") == 0)
            {
                char fiscode_str[16];
                sprintf(fiscode_str, "%d", fiscode);
                WriteToBuffer(&(fiscode_str[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "ATHLETE_COMPID") == 0)
            {
                char compid_str[16];
                sprintf(compid_str, "%d", athlete.compid);
                WriteToBuffer(&(compid_str[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "ATHLETE_FIRSTNAME") == 0) 
            {
                char* p = &(athlete.firstname[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "ATHLETE_LASTNAME") == 0) 
            {
                char* p = &(athlete.lastname[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "ATHLETE_BIRTHDATE") == 0) 
            {
                char* p = &(athlete.birthdate[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "ATHLETE_NATION") == 0) 
            {
                char* p = &(athlete.nation[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "ATHLETE_CLUB") == 0) 
            {
                char* p = &(athlete.club[0]);
                WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
            }
            else if (strcmp(placeholder, "ATHLETE_GENDER") == 0) 
            {
                if (strcmp(athlete.gender, "M") == 0) {
                    char male[] = "Male"; 
                    WriteToBuffer(&(male[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                }
                else if (strcmp(athlete.gender, "F") == 0) {
                    char female[] = "Female"; 
                    WriteToBuffer(&(female[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                }
            }
            // --------------------------------------------------------------
            // Creates a javascript array with all races
            // --------------------------------------------------------------
            else if (strcmp(placeholder, "ATHLETE_RACES") == 0)
            {
                int raceinfo_currentByte = 0;     // Tracks the last visited index inside the buffer in order to speed up the next search
                int raceresults_currentByte = 0;  // Tracks the last visited index inside the buffer in order to speed up the next search
                
                for (int i = 0; i < number_of_raceids; i++)
                {
                    unsigned int raceid = raceids[i];
                    RaceData raceData;

                    // Find the race info data for the current raceid
                    if (GetRaceData_FromRaceInfo(&raceData, raceid, &raceinfo_currentByte, race_info_buffer, race_info_buffer_size) == -1) {
                        fprintf(stderr, "[%ld] Warning: Race skipped while creating javascript array: Failed to find race info for race %u in the database\n", (long)getpid(), raceid);
                        continue;   
                    }
            
                    // Find the race result data for the current raceid, from the perspective of the given athlete
                    if (GetRaceData_FromRaceResults(&raceData, raceid, &raceresults_currentByte, fiscode, race_results_buffer, race_results_buffer_size) == -1) {
                        fprintf(stderr, "[%ld] Warning: Race skipped while creating javascript array: Failed to find race results for race %u in the database\n", (long)getpid(), raceid);
                        continue;   
                    }
                    
                    // START OF OBJECT
                    char object_start[] = "{";
                    WriteToBuffer(&(object_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);

                    // DATE
                    {
                        char start[] = "\"date\": \"";
                        char end[] = "\",";
                        char* p = &(raceData.date[0]);

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // RACEID
                    {
                        char start[] = "\"raceid\": \"";
                        char end[] = "\",";
                        char raceid_str[16];
                        sprintf(raceid_str, "%u", raceid);
                        
                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(raceid_str[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // LOCATION
                    {
                        char start[] = "\"location\": \"";
                        char end[] = "\",";
                        char* p = &(raceData.location[0]);

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // NATION
                    {
                        char start[] = "\"nation\": \"";
                        char end[] = "\",";
                        char* p = &(raceData.nation[0]);

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // CATEGORY
                    {
                        char start[] = "\"category\": \"";
                        char end[] = "\",";
                        char* p = &(raceData.category[0]);

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // DISCIPLINE
                    {
                        char start[] = "\"discipline\": \"";
                        char end[] = "\",";
                        char* p = &(raceData.discipline[0]);

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // TYPE
                    {
                        char start[] = "\"type\": \"";
                        char end[] = "\",";
                        char* p = &(raceData.type[0]);

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // PARTICIPANTS
                    {
                        char start[] = "\"participants\": \"";
                        char end[] = "\",";
                        char participants[16];
                        sprintf(participants, "%u", raceData.participants);
                        
                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(participants[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // RANK
                    {
                        char start[] = "\"rank\": \"";
                        char end[] = "\",";
                        char rank[16];
                        sprintf(rank, "%u", raceData.rank);
                        
                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(rank[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // TIME 
                    {
                        char start[] = "\"time\": ";
                        char end[] = ",";
                        char time[16];
                        sprintf(time, "%u", raceData.time);
                        
                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(time[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // TIME STRING
                    {
                        char start[] = "\"time_str\": \"";
                        char end[] = "\",";
                        char* time = RaceTime_ms_to_string(raceData.time);
                        
                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (time != 0) {
                            WriteToBuffer(time, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (time) { free(time); }
                    }
                    
                    // DIFF 
                    {
                        char start[] = "\"diff\": ";
                        char end[] = ",";
                        char diff[16];
                        sprintf(diff, "%u", raceData.diff);
                        
                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(diff[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // DIFF STRING
                    {
                        char start[] = "\"diff_str\": \"";
                        char end[] = "\",";
                        char* diff = RaceTimeDiff_ms_to_string(raceData.diff);

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (diff && raceData.diff != 0) {
                            WriteToBuffer(&(diff[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (diff) { free(diff); }
                    }

                    // DIFF PERCENTAGE
                    {
                        char start[] = "\"diff_per\": \"";
                        char end[] = "\",";
                        
                        char diff_per[16];
                        unsigned int time_winner = raceData.time - raceData.diff;
                        float diff = (((float) raceData.time / time_winner) - 1) * 100;
                        sprintf(diff_per, "%.2f", diff);
                        
                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (raceData.diff != 0) {
                            WriteToBuffer(&(diff_per[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                            char percentageSign[] = "%";
                            WriteToBuffer(&(percentageSign[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                   
                    // FIS POINTS
                    {
                        char start[] = "\"fispoints\": \"";
                        char end[] = "\",";
                        char* p = &(raceData.fispoints[0]);

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (p && *p != ' ' && *p != '\0') {
                            WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // AVG FIS POINTS
                    {
                        //
                        // TODO: Implement avg fispoints once that has been added to the Database
                        //
                        char start[] = "\"avg_fispoints\": \"";
                        char end[] = "\"";
                        char no_avg[] = "-";

                        WriteToBuffer(&(start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(no_avg[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    
                    // END OF OBJECT
                    char end[] = "}";
                    WriteToBuffer(&(end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                   
                    if (i < number_of_raceids - 1) {
                        char end2[] = ",\n";
                        WriteToBuffer(&(end2[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                }
            }
            // -------------------------------------------------------------------------------------------------------
            // Dyncamically creates html elements for all sprint races
            //
            // NOTE: THIS IS DEPRECATED! 
            //       IT IS BETTER TO USE THE JAVASCRIPT ARRAY ABOVE AND CREATE THE HTML ELEMENTS IN THE BROWSER INSTEAD
            //       THIS WILL ALLOW SORTING AND DYNAMIC CHANGES TO THE UI IN THE FUTURE
            // -------------------------------------------------------------------------------------------------------
            else if (strcmp(placeholder, "ANALYZED_RACES_SPRINT") == 0)
            {
                int raceinfo_currentByte = 0;     // Tracks the last visited index inside the buffer in order to speed up the next search
                int raceresults_currentByte = 0;  // Tracks the last visited index inside the buffer in order to speed up the next search
                
                int sprint_race_counter = 0;
                for (int i = 0; i < number_of_raceids; i++)
                {
                    unsigned int raceid = raceids[i];
                    RaceData raceData;

                    // Find the race info data for the current raceid
                    if (GetRaceData_FromRaceInfo(&raceData, raceid, &raceinfo_currentByte, race_info_buffer, race_info_buffer_size) == -1) {
                        fprintf(stderr, "[%ld] Warning: Race skipped while creating Athlete Page: Failed to find race info for race %u in the database\n", (long)getpid(), raceid);
                        continue;   
                    }

                    // Only use races that are of the type: Sprint Qualification
                    if (strcmp(raceData.type, "SQ") != 0) {
                        continue;
                    }
                    sprint_race_counter++;
            
                    // Find the race result data for the current raceid, from the perspective of the given athlete
                    if (GetRaceData_FromRaceResults(&raceData, raceid, &raceresults_currentByte, fiscode, race_results_buffer, race_results_buffer_size) == -1) {
                        fprintf(stderr, "[%ld] Warning: Race skipped while creating Athlete Page: Failed to find race results for race %u in the database\n", (long)getpid(), raceid);
                        continue;   
                    }
                    
                    
                    // Start writing each race to the PageBuffer enclosed in relevant html-tags
                    char a_start_even[] = "<a class='sprint-result even'>\n";
                    char a_start_odd[] = "<a class='sprint-result odd'>\n";
                    char a_end[] = "</a>\n";
                    char div_end[] = "</div>\n";

                    if (sprint_race_counter % 2 == 0) {
                        WriteToBuffer(&(a_start_even[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    } else {
                        WriteToBuffer(&(a_start_odd[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // DATE
                    {
                        char div_start[] = "<div class='sprint-result-field date-field'>";
                        char* p = &(raceData.date[0]);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // RACEID
                    {
                        char div_start[] = "<div class='sprint-result-field raceid-field'>";
                        char raceid_str[16];
                        sprintf(raceid_str, "%u", raceid);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(raceid_str[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // LOCATION
                    {
                        char div_start[] = "<div class='sprint-result-field location-field'>";
                        char location[512];
                        strcpy(location, raceData.location);
                        strcat(location, " (");
                        strcat(location, raceData.nation);
                        strcat(location, ")");

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(location[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // CATEGORY
                    {
                        char div_start[] = "<div class='sprint-result-field category-field'>";
                        char* p = &(raceData.category[0]);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // PARTICIPANTS
                    {
                        char div_start[] = "<div class='sprint-result-field participants-field'>";
                        char participants[16];
                        sprintf(participants, "%u", raceData.participants);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(participants[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // RANK
                    {
                        char div_start[] = "<div class='sprint-result-field rank-field'>";
                        char rank[16];
                        sprintf(rank, "%u", raceData.rank);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(rank[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }

                    // TIME
                    {
                        char div_start[] = "<div class='sprint-result-field time-field'>";
                        char* time = RaceTime_ms_to_string(raceData.time);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (time) {
                            WriteToBuffer(&(time[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (time) { free(time); }
                    }

                    // DIFF
                    {
                        char div_start[] = "<div class='sprint-result-field diff-field'>";
                        char no_diff[] = "-";
                        char* diff = RaceTimeDiff_ms_to_string(raceData.diff);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (diff && raceData.diff != 0) {
                            WriteToBuffer(&(diff[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        } else {
                            WriteToBuffer(&(no_diff[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (diff) { free(diff); }
                    }

                    // DIFF PERCENTAGE
                    {
                        char div_start[] = "<div class='sprint-result-field diffpercentage-field'>";
                        char no_diff[] = "-";
                        char diff_per[16];

                        unsigned int time_winner = raceData.time - raceData.diff;
                        float diff = (((float) raceData.time / time_winner) - 1) * 100;
                        sprintf(diff_per, "%.2f", diff);
                        
                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (raceData.diff == 0) {
                            WriteToBuffer(&(no_diff[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        } else {
                            WriteToBuffer(&(diff_per[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                            char percentageSign[] = "%";
                            WriteToBuffer(&(percentageSign[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                   
                    // FIS POINTS
                    {
                        char div_start[] = "<div class='sprint-result-field fispoints-field'>";
                        char no_points[] = "-";
                        char* p = &(raceData.fispoints[0]);

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        if (p && *p != ' ' && *p != '\0') {
                            WriteToBuffer(p, PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        } else {
                            WriteToBuffer(&(no_points[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        }
                        WriteToBuffer(&(div_end[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                    }
                    
                    // AVG FIS POINTS
                    {
                        //
                        // TODO: Implement avg fispoints once that has been added to the Database
                        //
                        char div_start[] = "<div class='sprint-result-field avg-field'>";
                        char no_avg[] = "-";

                        WriteToBuffer(&(div_start[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
                        WriteToBuffer(&(no_avg[0]), PageBuffer, *PageBuffer_size, &PageBuffer_currentByte);
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

    if (raceids) { free(raceids); }
    if (race_info_buffer) { free(race_info_buffer); }
    if (race_results_buffer) { free(race_results_buffer); }
    if (template_buffer) { free(template_buffer); }
    
    return 0;
}


/**
 * -------------------------------------------------------------------------------------------------------------------------------
 *  Gets the race info for the given race id
 *  The function will look though the race_info_buffer for the given race id, and the requierd data will then be stored in the RaceData object
 *
 *  raceData: The object that will store the race info data
 *  raceid: The id of the requested race
 *  start_byte: The position in the buffer the search should start from. 
 *              This index should be stored from the last time this function was called, in order to speed up the current search 
 *  race_info_buffer: This buffer should contain the full file from the database that contains all the data related to race info
 *  race_info_buffer_size: The size of the buffer
 *
 *  Returns 0 on success, the race was found and the data was stored inside the RaceData object
 *  Returns -1 if the requested race was not found
 * -------------------------------------------------------------------------------------------------------------------------------
 */
static int GetRaceData_FromRaceInfo(RaceData* raceData, unsigned int raceid, int* start_byte, char* race_info_buffer, int race_info_buffer_size)
{
    int currentByte = *start_byte;
    while (currentByte < race_info_buffer_size)
    {
        // Make sure it is possible to read the next 4 bytes
        if (currentByte + 3 >= race_info_buffer_size) {
            break;
        }

        // Read the current raceid
        unsigned char bytes[4];
        bytes[0] = race_info_buffer[currentByte++];
        bytes[1] = race_info_buffer[currentByte++];
        bytes[2] = race_info_buffer[currentByte++];
        bytes[3] = race_info_buffer[currentByte++];
        unsigned int currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        if (currentRaceid != raceid)
        {
            // Skip past all the bytes for the current race
            currentByte += 4;
            int string_counter = 0;
            while (string_counter < 7 && currentByte < race_info_buffer_size) {
                if (race_info_buffer[currentByte++] == '\0') {
                    string_counter++;
                }
            }
        }
        else 
        {
            // Skip the codex
            currentByte += 4;

            // Date
            int str_index = 0;
            while (currentByte < race_info_buffer_size && race_info_buffer[currentByte++] != '\0' && str_index < 256) {
                raceData->date[str_index++] = race_info_buffer[currentByte - 1];
            }
            raceData->date[str_index] = '\0';

            // Nation
            str_index = 0;
            while (currentByte < race_info_buffer_size && race_info_buffer[currentByte++] != '\0' && str_index < 256) {
                raceData->nation[str_index++] = race_info_buffer[currentByte - 1];
            }
            raceData->nation[str_index] = '\0';

            // Location
            str_index = 0;
            while (currentByte < race_info_buffer_size && race_info_buffer[currentByte++] != '\0' && str_index < 256) {
                raceData->location[str_index++] = race_info_buffer[currentByte - 1];
            }
            raceData->location[str_index] = '\0';

            // Category
            str_index = 0;
            while (currentByte < race_info_buffer_size && race_info_buffer[currentByte++] != '\0' && str_index < 256) {
                raceData->category[str_index++] = race_info_buffer[currentByte - 1];
            }
            raceData->category[str_index] = '\0';

            // Discipline
            str_index = 0;
            while (currentByte < race_info_buffer_size && race_info_buffer[currentByte++] != '\0' && str_index < 256) {
                raceData->discipline[str_index++] = race_info_buffer[currentByte - 1];
            }
            raceData->discipline[str_index] = '\0';

            // Type
            str_index = 0;
            while (currentByte < race_info_buffer_size && race_info_buffer[currentByte++] != '\0' && str_index < 256) {
                raceData->type[str_index++] = race_info_buffer[currentByte - 1];
            }
            raceData->type[str_index] = '\0';

            // Skip the last string field
            while (currentByte < race_info_buffer_size && race_info_buffer[currentByte++] != '\0') {}

            // The requested race was found
            *start_byte = currentByte;
            return 0;
        }
    }

    *start_byte = 0;
    return -1;
}


/**
 * -------------------------------------------------------------------------------------------------------------------------------
 *  Gets the race result data for the given race id, and from the perspective of the athlete with the given fiscode
 *  The function will look though the race_results_buffer for the given race id, and the requierd result data will then be stored in the RaceData object
 *
 *  raceData: The object that will store the race results data
 *  raceid: The id of the requested race
 *  fiscode: The fiscode of the requested athlete
 *  start_byte: The position in the buffer the search should start from. 
 *              This index should be stored from the last time this function was called, in order to speed up the current search 
 *  race_results_buffer: This buffer should contain the full file from the database that contains all the data related to race results
 *  race_results_buffer_size: The size of the buffer
 *
 *  Returns 0 on success, the race was found and the data was stored inside the RaceData object
 *  Returns -1 if the requested race was not found
 * -------------------------------------------------------------------------------------------------------------------------------
 */
static int GetRaceData_FromRaceResults(RaceData* raceData, unsigned int raceid, int* start_byte, unsigned int fiscode, char* race_results_buffer, int race_results_buffer_size)
{
    int currentByte = *start_byte;
    while (currentByte < race_results_buffer_size)
    {
        // Make sure the following 6 bytes can be read
        if (currentByte + 5 >= race_results_buffer_size) {
            break;
        }

        // Read the current raceid and how many ranks the result list has
        unsigned char bytes[4];
        bytes[0] = race_results_buffer[currentByte++];
        bytes[1] = race_results_buffer[currentByte++];
        bytes[2] = race_results_buffer[currentByte++];
        bytes[3] = race_results_buffer[currentByte++];
        unsigned int currentRaceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

        bytes[0] = race_results_buffer[currentByte++];
        bytes[1] = race_results_buffer[currentByte++];
        unsigned int numberOfRanks = bytes[0] | (bytes[1] << 8);

        if (currentRaceid != raceid)
        {
            // Skip past all the bytes for the current race
            for (int i = 0; i < numberOfRanks; i++) {
                currentByte += 18;
                int stringCounter = 0;
                while (stringCounter < 3 && currentByte < race_results_buffer_size) {
                    if (race_results_buffer[currentByte++] == '\0') {
                        stringCounter++;
                    }
                }
            }
        }
        /*
         * THIS CODE PATH IS PROBABLY UNNECCESSARY
        else if (numberOfRanks == 0)
        {
            // The requested race was found, but it has no results stored
            *start_byte = -1;
            return -1;
        }
        */
        else
        {
            // The requested race was found, so loop through all ranks in the result list
            for (int i = 0; i < numberOfRanks; i++)
            {
                // Make sure it is possible to read the following 8 bytes
                if (currentByte + 7 >= race_results_buffer_size) {
                    break;
                }

                bytes[0] = race_results_buffer[currentByte++];
                bytes[1] = race_results_buffer[currentByte++];
                unsigned int rank = bytes[0] | (bytes[1] << 8);

                bytes[0] = race_results_buffer[currentByte++];
                bytes[1] = race_results_buffer[currentByte++];
                unsigned int bib = bytes[0] | (bytes[1] << 8);

                bytes[0] = race_results_buffer[currentByte++];
                bytes[1] = race_results_buffer[currentByte++];
                bytes[2] = race_results_buffer[currentByte++];
                bytes[3] = race_results_buffer[currentByte++];
                unsigned int currentFiscode = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                if (currentFiscode != fiscode)
                {
                    // Skip past all the bytes for the current rank
                    currentByte += 10;
                    int stringCounter = 0;
                    while (stringCounter < 3 && currentByte < race_results_buffer_size) {
                        if (race_results_buffer[currentByte++] == '\0') {
                            stringCounter++;
                        }
                    }
                }
                else 
                {
                    // Make sure it is possible to read the following 10 bytes
                    if (currentByte + 9 >= race_results_buffer_size) {
                        break;
                    }
                    
                    bytes[0] = race_results_buffer[currentByte++];
                    bytes[1] = race_results_buffer[currentByte++];
                    bytes[2] = race_results_buffer[currentByte++];
                    bytes[3] = race_results_buffer[currentByte++];
                    raceData->time = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                    bytes[0] = race_results_buffer[currentByte++];
                    bytes[1] = race_results_buffer[currentByte++];
                    bytes[2] = race_results_buffer[currentByte++];
                    bytes[3] = race_results_buffer[currentByte++];
                    raceData->diff = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);

                    bytes[0] = race_results_buffer[currentByte++];
                    bytes[1] = race_results_buffer[currentByte++];
                    unsigned int year = bytes[0] | (bytes[1] << 8);

                    // Skip the name and nation field
                    int stringCounter = 0;
                    while (stringCounter < 2 && currentByte < race_results_buffer_size) {
                        if (race_results_buffer[currentByte++] == '\0') {
                            stringCounter++;
                        }
                    }
                    
                    int str_index = 0;
                    while (currentByte < race_results_buffer_size && race_results_buffer[currentByte++] != '\0' && str_index < 256) {
                        raceData->fispoints[str_index++] = race_results_buffer[currentByte - 1];
                    }
                    raceData->fispoints[str_index] = '\0';

                    raceData->rank = rank;
                    raceData->participants = numberOfRanks;


                    // Since the parameter "start_byte" is used to keep track of the current byte (in order to speed up the search for the next race)
                    // We need to finish looping though the result list, so that the byte index points to the start of a new race during the next search
                    i++;
                    while (i < numberOfRanks)
                    {
                        // If the end of the buffer is reached: set the byte index to 0 so the next search start from the beginning
                        if (currentByte + 17 >= race_results_buffer_size) {
                            *start_byte = 0;
                            return 0;
                        }

                        // Skip past all the bytes for the current rank
                        i++;
                        currentByte += 18;
                        int stringCounter = 0;
                        while (stringCounter < 3 && currentByte < race_results_buffer_size) {
                            if (race_results_buffer[currentByte++] == '\0') {
                                stringCounter++;
                            }
                        }
                    }
                    
                    // The requested race and athlete was found, and the data was stored in the RaceData object
                    *start_byte = currentByte;
                    return 0;
                }
            }


            // The requested race was found but not the requested athlete
            if (currentByte >= race_results_buffer_size) {
                *start_byte = 0;  // If the end of the buffer is reached: set the byte index to 0 so the next search start from the beginning
            } else {
                *start_byte = currentByte;
            }
            return -1;
        }
    }    

    // The requested race was not found
    *start_byte = 0;
    return -1;
}












