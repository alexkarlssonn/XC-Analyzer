
#include "ConvertDB.h"

#include "../src/libs/Restart.h"
#include "../src/libs/cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>




/*
 * -----------------------------------------------------------------------------------------------
 * Converts the athletes from the database that are stored as JSON objects, into my own custom file format
 * This file format will store the athletes as binary data which will make accessing and searching the db, much faster!
 * The converted athletes are stored in a new file that can be found under the "output" directory
 * -----------------------------------------------------------------------------------------------
 */
int convert_athletes_from_json_to_custom_format(bool print_athletes, bool print_buffer)
{
    // -------------------------------------------------------------------------------
    // Loads the file and allocates a buffer that will store the converted file data
    // -------------------------------------------------------------------------------
    char file[] = "../db/athletes.json";
    int loadedFileSize = 0;
    cJSON* json = loadAndParse(file, &loadedFileSize);
    if (json == 0)
        return -1;

    int size = 0;  // The current and actual size of buffer
    unsigned char* buffer = (unsigned char*) malloc(loadedFileSize * sizeof(unsigned char));
    if (buffer == 0) {
        perror("Failed to allocate buffer for storing new formated data");
        return -1;
    }
    for (int i = 0; i < loadedFileSize; i++) 
        buffer[i] = 0;  // Make sure the buffer is filled with 0's


    // -------------------------------------------------------------------------------
    // Loops through all athletes in the cJSON object
    // -------------------------------------------------------------------------------
    int counter = -1;
    cJSON* athletes = cJSON_GetObjectItemCaseSensitive(json, "athletes");
    cJSON* athlete = NULL;
    cJSON_ArrayForEach(athlete, athletes) 
    {
        counter ++;
        char* firstname_str = 0;
        char* lastname_str = 0;
        char* nation_str = 0;
        char* birthdate_str = 0;
        char* gender_str = 0;
        char* club_str = 0;
        cJSON* field_fiscode = cJSON_GetObjectItemCaseSensitive(athlete, "fiscode");
        cJSON* field_compid = cJSON_GetObjectItemCaseSensitive(athlete, "competitorid");
        cJSON* field_firstname = cJSON_GetObjectItemCaseSensitive(athlete, "firstname");
        cJSON* field_lastname = cJSON_GetObjectItemCaseSensitive(athlete, "lastname");
        cJSON* field_nation = cJSON_GetObjectItemCaseSensitive(athlete, "nation");
        cJSON* field_birthdate = cJSON_GetObjectItemCaseSensitive(athlete, "birthdate");
        cJSON* field_gender = cJSON_GetObjectItemCaseSensitive(athlete, "gender");
        cJSON* field_club = cJSON_GetObjectItemCaseSensitive(athlete, "club");

        unsigned int fiscode_u32 = atoi(field_fiscode->valuestring);
        unsigned int compid_u32 = atoi(field_compid->valuestring);
        firstname_str = field_firstname->valuestring;
        lastname_str = field_lastname->valuestring;
        nation_str = field_nation->valuestring;
        birthdate_str = field_birthdate->valuestring;
        gender_str = field_gender->valuestring;
        club_str = field_club->valuestring;

        
        // -------------------------------------------------
        // Print all data fields for the current athlete
        // -------------------------------------------------
        if (print_athletes) {
            fprintf(stderr, "\nFiscode %d (hex: %x), Compid: %d (hex: %x)\nFirstname: %s, Lastname: %s, Nation: %s, Birthdate: %s, Gender: %s, Club: %s\n", fiscode_u32, fiscode_u32, compid_u32, compid_u32, firstname_str, lastname_str, nation_str, birthdate_str, gender_str, club_str);
        }

        // Write the fiscode (u32) to the buffer in individual bytes (little endian)
        buffer[size] = (fiscode_u32 & 0xFF);            // Write byte 1/4 of fiscode
        buffer[++size] = (fiscode_u32 >> 8) & 0xFF;   // Write byte 2/4 of fiscode
        buffer[++size] = (fiscode_u32 >> 16) & 0xFF;  // Write byte 3/4 of fiscode
        buffer[++size] = (fiscode_u32 >> 24) & 0xFF;  // Write byte 4/4 of fiscode

        // Write the compid (u32) to the buffer in individual bytes (little endian)
        buffer[++size] = (compid_u32 & 0xFF);
        buffer[++size] = (compid_u32 >> 8) & 0xFF;
        buffer[++size] = (compid_u32 >> 16) & 0xFF;
        buffer[++size] = (compid_u32 >> 24) & 0xFF;

        // Write all the strings to the buffer
        writeStringToBuffer(&buffer, &size, firstname_str);
        writeStringToBuffer(&buffer, &size, lastname_str);
        writeStringToBuffer(&buffer, &size, nation_str);
        writeStringToBuffer(&buffer, &size, birthdate_str);
        writeStringToBuffer(&buffer, &size, gender_str);
        writeStringToBuffer(&buffer, &size, club_str);
        size++;
    }
    
    cJSON_Delete(json);


    // Prints each byte in the buffer in hex
    if (print_buffer)
        printBuffer(&buffer, size); 

    fprintf(stderr, "Converted %d athletes!\n", (counter+1));

    // -----------------------------------------------------------------------
    // Save the buffer to the given file
    // -----------------------------------------------------------------------
    char outputFile[] = "./output/athletes.txt";
    int fd; 
    if ((fd = r_open2(outputFile, O_RDWR)) == -1) {
        perror("Failed to open file");
        if (buffer != 0)
            free(buffer);
        return -1;
    } 
    int byteswritten = 0;
    if ((byteswritten = r_write(fd, buffer, size)) < size) {
        perror("Failed to write entire buffer to output file");
    } else {
        fprintf(stderr, "All athletes was converted and written to %s successfully!\n", outputFile);
    }
    if (buffer != 0)
        free(buffer);

    return 0;
}



/**
 * --------------------------------------------------------------------------------
 * Enum used to respresent the different data fields for each athlete
 * Is used when parsing athlets from the file with the new custom format
 * --------------------------------------------------------------------------------
 */
enum FIELD {
    fiscode, compid, firstname, lastname, nation, birthdate, gender, club
};


/**
 * --------------------------------------------------------------------------------
 * Loads all atheltes that are stored in the new custom file format
 * Once the athletes has been loaded they will be printed 
 * 
 * This function is a "proof of concept" to show and test that the athletes can be read and parsed
 * successfully from this new custom file format
 * --------------------------------------------------------------------------------
 */
int load_and_print_athletes(bool printAll)
{
    // Open and read all bytes from the file
    char file[] = "./output/athletes.txt";
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
    FIELD currentField = fiscode;
    for (int i = 0; i < bytesread;)
    {
        if (currentField == fiscode) 
        {
            unsigned char bytes[4];
            bytes[0] = buffer[i++];
            bytes[1] = buffer[i++];
            bytes[2] = buffer[i++];
            bytes[3] = buffer[i];
            unsigned int combined = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            if (printAll) 
                fprintf(stderr, "\n%d, ", combined);
            currentField = compid;
        }
        else if (currentField == compid)
        {
            unsigned char bytes[4];
            bytes[0] = buffer[i++];
            bytes[1] = buffer[i++];
            bytes[2] = buffer[i++];
            bytes[3] = buffer[i];
            unsigned int combined = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
            if (printAll) 
                fprintf(stderr, "%d, ", combined);
            currentField = firstname;
        }
        else if (currentField == firstname)
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
                fprintf(stderr, "%s, ", string);
            currentField = lastname;
        }
        else if (currentField == lastname)
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
                fprintf(stderr, "%s, ", string);
            currentField = nation;
        }
        else if (currentField == nation)
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
                fprintf(stderr, "%s, ", string);
            currentField = birthdate;
        }
        else if (currentField == birthdate)
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
                fprintf(stderr, "%s, ", string);
            currentField = gender;
        }
        else if (currentField == gender)
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
                fprintf(stderr, "%s, ", string);
            currentField = club;
        }
        else if (currentField == club)
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
                fprintf(stderr, "%s, ", string);
            currentField = fiscode;
        }
        i++;
    }

    fprintf(stderr, "\nDONE! %d bytes was read and processed\n", bytesread);
    if (buffer != 0)
        free(buffer);
    return 0;
}




