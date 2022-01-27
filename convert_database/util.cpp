
#include "ConvertDB.h"

#include "../src/libs/Restart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * ------------------------------------------------------------------------------------
 * Loads the given file and parses it to cJSON and returns a pointer to it
 * On failure, 0 will be returned and an error message printed
 * ------------------------------------------------------------------------------------
 */
cJSON* loadAndParse(const char* file, int* size)
{
    int fd; 
    if ((fd = r_open2(file, O_RDONLY)) == -1) {
        perror("Failed to open file");
        return 0;
    } 
    int filesize = (int) lseek(fd, 0, SEEK_END); 
    if ((int)lseek(fd, 0, SEEK_SET) == -1) { 
        perror("Failed to move file offset back to the beginning");
        return 0;
    }
    *size = filesize;
    char* bytes = 0;
    if ((bytes = (char*) malloc( (filesize+1) * sizeof(char))) == 0) {
        perror("Failed to allocate buffer for loading file");
        return 0;
    }
    if ((readblock(fd, bytes, filesize)) <= 0) {
        perror("Failed to read content of file into buffer");
        if (bytes != 0)
            free(bytes);
        return 0;
    }
    if (r_close(fd) == -1) {
        fprintf(stderr, "Failed to close the file\n");
    }
    cJSON* json = cJSON_Parse(bytes);
    if (bytes != 0) 
        free(bytes);
    if (json == NULL) {
        perror("Failed to parse file content to JSON");
        cJSON_Delete(json);
        return 0;
    }

    return json;
}


/**
 * ------------------------------------------------------------------------------------
 * Writes the string into the buffer, and updates size along the way
 * ------------------------------------------------------------------------------------
 */
 void writeStringToBuffer(unsigned char** buffer, int* size, char* string)
{
    if (*buffer == 0)
        return;
    
    int index = *size;
    if (string != 0 && strlen(string) > 0) {
        for (int i = 0; i < strlen(string); i++) {
            (*buffer)[++index] = string[i];
        }
    }
    (*buffer)[++index] = '\0';
    *size = index;
    return;
}


/**
 * ------------------------------------------------------------------------------------
 * Prints the content of the buffer byte by byte in hex 
 * ------------------------------------------------------------------------------------
 */
void printBuffer(unsigned char** buffer, int size)
{
    fprintf(stderr, "\nPrinting contents of buffer:\n");
    for (int i = 0; i < size; i++) {
        if (i % 32 == 0 && i != 0) 
            fprintf(stderr, "\n");
        if (i % 4 == 0 && i != 0)
            fprintf(stderr, "| ");
        fprintf(stderr, "%02x ", (*buffer)[i]);
    }
    fprintf(stderr, "\n");
}


