
#include "ConvertDB.h"

#include "../src/libs/Restart.h"
#include "../src/libs/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * -------------------------------------------------------------------------------------------------
 * Converts the races info from the database that are stored as JSON objects, into my own custom file format
 * This file format will store the data as binary data which will make accessing and searching the db much faster!
 * The converted data are stored in a new file that can be found under the "output" directory
 * -------------------------------------------------------------------------------------------------
 */
int convert_racesInfo_from_json_to_custom_format(bool print_buffer)
{
    // ------------------------------------------------------------------------------
    // Load the file and allocate a buffer that will store the converted file data 
    // ------------------------------------------------------------------------------
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


    // ------------------------------------------------------------------------------
    // Loop through all races in the cJSON object 
    // ------------------------------------------------------------------------------
    int counter = -1;
    cJSON* races = cJSON_GetObjectItemCaseSensitive(json, "races");
    cJSON* race = NULL;
    cJSON_ArrayForEach(race, races)
    {
        counter++;
        char* date_str = 0;
        char* nation_str = 0;
        char* location_str = 0;
        char* category_str = 0;
        char* discipline_str = 0;
        char* type_str = 0;
        char* gender_str = 0;
        cJSON* field_raceid = cJSON_GetObjectItemCaseSensitive(race, "raceid");
        cJSON* field_codex = cJSON_GetObjectItemCaseSensitive(race, "codex");
        cJSON* field_date = cJSON_GetObjectItemCaseSensitive(race, "date");
        cJSON* field_nation = cJSON_GetObjectItemCaseSensitive(race, "nation");
        cJSON* field_location = cJSON_GetObjectItemCaseSensitive(race, "location");
        cJSON* field_category = cJSON_GetObjectItemCaseSensitive(race, "category");
        cJSON* field_discipline = cJSON_GetObjectItemCaseSensitive(race, "discipline");
        cJSON* field_type = cJSON_GetObjectItemCaseSensitive(race, "type");
        cJSON* field_gender = cJSON_GetObjectItemCaseSensitive(race, "gender");

        unsigned int raceid_u32 = atoi(field_raceid->valuestring);
        unsigned int codex_u32 = atoi(field_codex->valuestring);
        if ((date_str = field_date->valuestring) == 0) 
            fprintf(stderr, "ERROR! Empty date_str\n");
        if ((nation_str = field_nation->valuestring) == 0)
            fprintf(stderr, "ERROR! Empty nation_str\n");
        if ((location_str = field_location->valuestring) == 0)
            fprintf(stderr, "ERROR! Empty location_str\n");
        if ((category_str = field_category->valuestring) == 0) 
            fprintf(stderr, "ERROR! Empty category_str\n");
        if ((discipline_str = field_discipline->valuestring) == 0)
            fprintf(stderr, "ERROR! Empty discipline_str\n");
        if ((type_str = field_type->valuestring) == 0) 
            fprintf(stderr, "ERROR! Empty type_str\n");
        if ((gender_str = field_gender->valuestring) == 0) 
            fprintf(stderr, "ERROR! Empty gender_str\n");
    
        
        // Write the raceid (u32) to the buffer in individual bytes (little endian)
        buffer[size] = (raceid_u32 & 0xFF);
        buffer[++size] = (raceid_u32 >> 8) & 0xFF;
        buffer[++size] = (raceid_u32 >> 16) & 0xFF;
        buffer[++size] = (raceid_u32 >> 24) & 0xFF;
        
        // Write the codex (u32) to the buffer in individual bytes (little endian)
        buffer[++size] = (codex_u32 & 0xFF);
        buffer[++size] = (codex_u32 >> 8) & 0xFF;
        buffer[++size] = (codex_u32 >> 16) & 0xFF;
        buffer[++size] = (codex_u32 >> 24) & 0xFF;
    
        // Write the strings to the buffer
        writeStringToBuffer(&buffer, &size, date_str);
        writeStringToBuffer(&buffer, &size, nation_str);
        writeStringToBuffer(&buffer, &size, location_str);
        writeStringToBuffer(&buffer, &size, category_str);
        writeStringToBuffer(&buffer, &size, discipline_str);
        writeStringToBuffer(&buffer, &size, type_str);
        writeStringToBuffer(&buffer, &size, gender_str);
        size++;
    }
    
    cJSON_Delete(json);


    // Print each byte in the buffer in hex
    if (print_buffer)
        printBuffer(&buffer, size);
    fprintf(stderr, "Converted %d races!\n\n", counter+1);


    // ------------------------------------------------------------------------
    // Save the buffer to the given file
    // ------------------------------------------------------------------------
    char file[] = "./output/races-info.txt";
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
        fprintf(stderr, "All raceids was converted and written to %s successfully!\n", file);
    }    
    if (buffer != 0)
        free(buffer);
    
    return 0;
}



enum field_t {
    RACEID, CODEX, DATE, NATION, LOCATION, CATEGORY, DISCIPLINE, TYPE, GENDER
};


/**
 * --------------------------------------------------------------------------------
 * Loads all races info that are stored in the new custom file format
 * Once the file has been loaded they will be printed 
 * 
 * This function is a "proof of concept" to show and test that the data can be read and parsed
 * successfully from this new custom file format
 * --------------------------------------------------------------------------------
 */
int load_and_print_racesInfo(bool printAll)
{
    // Open and read all bytes from the file
    char file[] = "./output/races-info.txt";
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

    // Parse the file and print out all athletes
    field_t currentField = RACEID;
    for (int i = 0; i < bytesread;)
    {
        if (currentField == RACEID) 
        {
            unsigned char bytes[4];
            bytes[0] = buffer[i++];
            bytes[1] = buffer[i++];
            bytes[2] = buffer[i++];
            bytes[3] = buffer[i];
            unsigned int combined = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            if (printAll) 
                fprintf(stderr, "\nRACEID: %d | ", combined);
            currentField = CODEX;
        }
        else if (currentField == CODEX)
        {
            unsigned char bytes[4];
            bytes[0] = buffer[i++];
            bytes[1] = buffer[i++];
            bytes[2] = buffer[i++];
            bytes[3] = buffer[i];
            unsigned int combined = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            if (printAll) 
                fprintf(stderr, "%d | ", combined);
            currentField = DATE;
        }
        else if (currentField == DATE)
        {
            char string[256];
            int index = 0;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s | ", string);
            currentField = NATION;
        }
        else if (currentField == NATION)
        {
            char string[256];
            int index = 0;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s | ", string);
            currentField = LOCATION;
        }
        else if (currentField == LOCATION)
        {
            char string[256];
            int index = 0;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s | ", string);
            currentField = CATEGORY;
        }
        else if (currentField == CATEGORY)
        {
            char string[256];
            int index = 0;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s | ", string);
            currentField = DISCIPLINE;
        }
        else if (currentField == DISCIPLINE)
        {
            char string[256];
            int index = 0;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s | ", string);
            currentField = TYPE;
        }
        else if (currentField == TYPE)
        {
            char string[256];
            int index = 0;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s | ", string);
            currentField = GENDER;
        }
        else if (currentField == GENDER)
        {
            char string[256];
            int index = 0;
            while (buffer[i] != '\0') {
                string[index] = buffer[i];
                index++;
                i++;
            }
            string[index] = '\0';
            if (printAll) 
                fprintf(stderr, "%s", string);
            currentField = RACEID;
        }
        i++;
    }

    fprintf(stderr, "\nDONE! %d bytes was read and processed\n", bytesread);
    if (buffer != 0)
        free(buffer);
    return 0;

}
