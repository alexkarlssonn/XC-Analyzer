
#include "ConvertDB.h"

#include "../src/libs/Restart.h"
#include "../src/libs/cJSON.h"
#include "../src/util/RaceTime.h"
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

    int total_counter = 0; 
    int total_json_size = 0;
    int all_size = 0;
    unsigned char* all_buffer = (unsigned char*) malloc(100000000 * sizeof(char));
    if (all_buffer == 0) {
        fprintf(stderr, "Failed to allocate buffer to store ALL formated race result data\n");
        return -1;
    }


    // -------------------------------------------------------------
    // Convert all 7 JSON files to the new custom file format
    // -------------------------------------------------------------
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
            fprintf(stderr, "Failed to allocate buffer for storing new formated data\n");
            cJSON_Delete(json);
            return -1;
        }
        total_json_size += loadedFileSize;
        
        // ------------------------------------------------------------------------------------
        // Loop through all races in the cJSON object 
        // ------------------------------------------------------------------------------------
        int counter = -1;
        cJSON* races = cJSON_GetObjectItemCaseSensitive(json, "races");
        cJSON* race = NULL;
        cJSON_ArrayForEach(race, races)
        {
            counter++;
            total_counter++;
            cJSON* raceid_field = cJSON_GetObjectItemCaseSensitive(race, "raceid");
            cJSON* ranks = cJSON_GetObjectItemCaseSensitive(race, "result");

            // Write the raceid (u32) to the buffer
            unsigned int raceid_u32 = atoi(raceid_field->valuestring);
            buffer[size] = (raceid_u32 & 0xFF);
            buffer[++size] = (raceid_u32 >> 8) & 0xFF;
            buffer[++size] = (raceid_u32 >> 16) & 0xFF;
            buffer[++size] = (raceid_u32 >> 24) & 0xFF;

            // Write the number of ranks for this race (u16) to the buffer
            unsigned short number_of_ranks = (unsigned short)cJSON_GetArraySize(ranks);
            buffer[++size] = (number_of_ranks & 0xFF);
            buffer[++size] = (number_of_ranks >> 8) & 0xFF;            


            // Loop through each rank for the current race result list 
            cJSON* rank = NULL;
            cJSON_ArrayForEach(rank, ranks)
            {
                cJSON* field_rank =      cJSON_GetObjectItemCaseSensitive(rank, "rank");
                cJSON* field_bib =       cJSON_GetObjectItemCaseSensitive(rank, "bib");
                cJSON* field_fiscode =   cJSON_GetObjectItemCaseSensitive(rank, "fiscode");
                cJSON* field_athlete =   cJSON_GetObjectItemCaseSensitive(rank, "athlete");
                cJSON* field_year =      cJSON_GetObjectItemCaseSensitive(rank, "year");
                cJSON* field_nation =    cJSON_GetObjectItemCaseSensitive(rank, "nation");
                cJSON* field_time =      cJSON_GetObjectItemCaseSensitive(rank, "time");
                cJSON* field_diff =      cJSON_GetObjectItemCaseSensitive(rank, "diff");
                cJSON* field_fispoints = cJSON_GetObjectItemCaseSensitive(rank, "fispoints");

                unsigned short rank_u16 = 0x0000;
                if (field_rank->valuestring != 0 && strlen(field_rank->valuestring) > 0)
                    rank_u16 = (unsigned short)atoi(field_rank->valuestring);
                
                unsigned short bib_u16 = 0x0000; 
                if (field_bib->valuestring != 0 && strlen(field_bib->valuestring) > 0)
                    bib_u16 = (unsigned short)atoi(field_bib->valuestring);
                
                unsigned int fiscode_u32 = 0x00000000; 
                if (field_fiscode->valuestring != 0 && strlen(field_fiscode->valuestring) > 0)
                    fiscode_u32 = atoi(field_fiscode->valuestring);
                
                unsigned int time_u32 = 0x00000000;
                if (field_time->valuestring != 0 && strlen(field_time->valuestring) > 0)
                    time_u32 = RaceTime_string_to_ms(field_time->valuestring);
                
                unsigned short year_u16 = 0x0000; 
                if (field_year->valuestring != 0 && strlen(field_year->valuestring) > 0)
                    year_u16 = (unsigned short)atoi(field_year->valuestring);
                
                unsigned int diff_u32 = 0x00000000;
                if (field_diff->valuestring != 0 && strlen(field_diff->valuestring) > 0 &&
                    field_time->valuestring != 0 && strlen(field_time->valuestring) > 0) {
                    if (strcmp(field_time->valuestring, field_diff->valuestring) != 0) {
                        diff_u32 = RaceTime_string_to_ms(field_diff->valuestring);
                    }
                }
                
                // Write the rank (u16) and bib (u16) to the buffer in individual bytes (little endian)
                buffer[++size] = (rank_u16 & 0xFF);
                buffer[++size] = (rank_u16 >> 8) & 0xFF;
                buffer[++size] = (bib_u16 & 0xFF);
                buffer[++size] = (bib_u16 >> 8) & 0xFF;

                // Write the fiscode (u32) to the buffer in individual bytes (little endian)
                buffer[++size] = (fiscode_u32 & 0xFF);
                buffer[++size] = (fiscode_u32 >> 8) & 0xFF;
                buffer[++size] = (fiscode_u32 >> 16) & 0xFF;
                buffer[++size] = (fiscode_u32 >> 24) & 0xFF;

                // Write the time (u32) to the buffer in individual bytes (little endian)
                buffer[++size] = (time_u32 & 0xFF);
                buffer[++size] = (time_u32 >> 8) & 0xFF;
                buffer[++size] = (time_u32 >> 16) & 0xFF;
                buffer[++size] = (time_u32 >> 24) & 0xFF;
                
                // Write the diff time (u32) to the buffer in individual bytes (little endian)
                buffer[++size] = (diff_u32 & 0xFF);
                buffer[++size] = (diff_u32 >> 8) & 0xFF;
                buffer[++size] = (diff_u32 >> 16) & 0xFF;
                buffer[++size] = (diff_u32 >> 24) & 0xFF;

                // Write the year (u16) to the buffer in individual bytes (little endian)
                buffer[++size] = (year_u16 & 0xFF);
                buffer[++size] = (year_u16 >> 8) & 0xFF;


                // Write the athlete string to the buffer
                char* athlete_str = 0;
                if (field_athlete->valuestring != 0 && strlen(field_athlete->valuestring) > 0) {
                    athlete_str = field_athlete->valuestring;
                    writeStringToBuffer(&buffer, &size, athlete_str);
                } else {
                    char* str = 0;
                    str = (char*) malloc(2 * sizeof(char));
                    if (str != 0) {
                        str[0] = ' ';
                        str[1] = '\0';    
                        writeStringToBuffer(&buffer, &size, str);
                        free(str);
                    }
                }
                
                // Write the nation string to the buffer
                char* nation_str = 0;
                if (field_nation->valuestring != 0 && strlen(field_nation->valuestring) > 0) {
                    nation_str = field_nation->valuestring;
                    writeStringToBuffer(&buffer, &size, nation_str);
                } else {
                    char* str = 0;
                    str = (char*) malloc(2 * sizeof(char));
                    if (str != 0) {
                        str[0] = ' ';
                        str[1] = '\0';    
                        writeStringToBuffer(&buffer, &size, str);
                        free(str);
                    }
                }

                // Write the fispoints to the buffer
                char* fispoints_str = 0;
                if (field_fispoints->valuestring != 0 && strlen(field_fispoints->valuestring) > 0) {
                    fispoints_str = field_fispoints->valuestring;
                    writeStringToBuffer(&buffer, &size, fispoints_str);
                } else {
                    char* str = 0;
                    str = (char*) malloc(2 * sizeof(char));
                    if (str != 0) {
                        str[0] = ' ';
                        str[1] = '\0';    
                        writeStringToBuffer(&buffer, &size, str);
                        free(str);
                    }
                }
            
            }
            size++;
        }

        // Copy the buffer into the all_buffer
        for (int i = 0; i < size; i++) {
            all_buffer[all_size++] = buffer[i];
        }
        
        fprintf(stderr, "%d races was converted to the new file format (%d bytes)\n", counter+1, size);
        cJSON_Delete(json);
        if (buffer != 0)
            free(buffer);
    }
  

    // -------------------------------------------------------------------------
    // Save the all_buffer to the given file
    // -------------------------------------------------------------------------
    char outputFile[] = "./output/races-results.txt";
    int fd;
    if ((fd = r_open3(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 644)) == -1) {
        perror("Failed to open output file");
        if (all_buffer != 0)
            free(all_buffer);
        return -1;
    }
    int byteswritten = 0;
    if ((byteswritten = r_write(fd, all_buffer, all_size)) < all_size) {
        perror("Failed to write entire buffer to the output file");
        if (all_buffer != 0)
            free(all_buffer);
        return -1;
    }

    float percent = (1 - (float)all_size / total_json_size) * 100;
    fprintf(stderr, "\n%d races was written to %s successfully!\n", total_counter, outputFile);
    fprintf(stderr, "Total size of json data:      %d bytes\n", total_json_size);
    fprintf(stderr, "Total size of converted data: %d bytes\n", all_size);
    fprintf(stderr, "Reduced the size by %d bytes (%.2f%%)\n", (total_json_size - all_size), percent);
    
    if (all_buffer != 0)
       free(all_buffer);
    close(fd); 
    
    return 0;
}



/**
 * --------------------------------------------------------------------------------
 * Loads all race results that are stored in the new custom file format
 * Once the race results has been loaded they will be printed 
 * 
 * This function is a "proof of concept" to show and test that the data can be read and parsed
 * successfully from this new custom file format
 * --------------------------------------------------------------------------------
 */
int load_and_print_racesResults(bool printAll)
{
    // Open and read all bytes from the file
    char file[] = "./output/races-results.txt";
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
    fprintf(stderr, "FILE SIZE: %d\n", filesize);
    
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


    // Parse the file and print out all athletes
    for (int i = -1; i < bytesread;)
    {
        // RACEID
        unsigned char bytes[4];
        bytes[0] = buffer[++i];
        bytes[1] = buffer[++i];
        bytes[2] = buffer[++i];
        bytes[3] = buffer[++i];
        unsigned int combined_raceid = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
        if (printAll) 
            fprintf(stderr, "\nRaceid: %d | ", combined_raceid);
     
        // NUMBER OF RANKS
        bytes[0] = buffer[++i];
        bytes[1] = buffer[++i];
        unsigned int number_of_ranks = bytes[0] | (bytes[1] << 8); 
        if (printAll) 
            fprintf(stderr, "Num_of_ranks: %d\n", number_of_ranks);
   

        // LOOP THOUGH EACH RANK FOR THE CURRENT RACE RESULT LIST
        for (int j = 0; j < number_of_ranks; j++) 
        {
            // RANK
            bytes[0] = buffer[++i];
            bytes[1] = buffer[++i];
            unsigned int combined_rank = bytes[0] | (bytes[1] << 8);
            if (printAll) 
                fprintf(stderr, "    Rank: %d | ", combined_rank);
            
            // BIB
            bytes[0] = buffer[++i];
            bytes[1] = buffer[++i];
            unsigned int combined_bib = bytes[0] | (bytes[1] << 8);
            if (printAll) 
                fprintf(stderr, "Bib: %d | ", combined_bib);
            
            // FISCODE
            bytes[0] = buffer[++i];
            bytes[1] = buffer[++i];
            bytes[2] = buffer[++i];
            bytes[3] = buffer[++i];
            unsigned int combined_fiscode = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            if (printAll) 
                fprintf(stderr, "Fiscode: %d | ", combined_fiscode);
            
            // TIME
            bytes[0] = buffer[++i];
            bytes[1] = buffer[++i];
            bytes[2] = buffer[++i];
            bytes[3] = buffer[++i];
            unsigned int combined_time = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            if (printAll) 
                fprintf(stderr, "Time: %d | ", combined_time);
            
            // DIFF
            bytes[0] = buffer[++i];
            bytes[1] = buffer[++i];
            bytes[2] = buffer[++i];
            bytes[3] = buffer[++i];
            unsigned int combined_diff = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            if (printAll) 
                fprintf(stderr, "Diff: %d | ", combined_diff);
            
            // YEAR
            bytes[0] = buffer[++i];
            bytes[1] = buffer[++i];
            unsigned int combined_year = bytes[0] | (bytes[1] << 8);
            if (printAll) 
                fprintf(stderr, "Year: %d | ", combined_year);
            
            // ATHLETE
            char string[256];
            int index = 0;
            i++;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s | ", string);
            
            // NATION
            index = 0;
            i++;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s | ", string);
            
            // FISPOINTS
            index = 0;
            i++;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s |\n", string);
        }
    }
    
        
    fprintf(stderr, "\nDONE! %d bytes was read and processed\n", bytesread);
    if (buffer != 0)
        free(buffer);
    return 0;
}




