
#include "CreatePages.h"


// TODO: The ascii conversion back to utf-8 should already be in the database - Convert the database instead of here


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
int WriteToBuffer(char* string, char** buffer, int buffer_size, int* currentByte)
{
    if (string == 0 || *buffer == 0 || buffer_size == 0 || (*currentByte >= buffer_size)) {
        return -1;
    }

    while (*string != '\0') {
        if (*currentByte >= buffer_size) {
            break;
        }
    
        if (*string == '\xC3') {
            if (*(string + sizeof(char)) != '\0') 
            {
                string++;
                if (*(string) == '\xA5') {
                    (*buffer)[*currentByte] = '\xE5';  // å
                }
                else if (*(string) == '\x85') {
                    (*buffer)[*currentByte] = '\xC5'; // Å
                }
                else if (*(string) == '\xA4') {
                    (*buffer)[*currentByte] = '\xE4'; // ä
                }
                else if (*(string) == '\x84') {
                    (*buffer)[*currentByte] = '\xC4';  // Ä
                }
                else if (*(string) == '\xB6') {
                    (*buffer)[*currentByte] = '\xF6'; // ö
                }
                else if (*(string) == '\x96') {
                    (*buffer)[*currentByte] = '\xD6';  // Ö
                } 
                else if (*(string) == '\xA9') {
                    (*buffer)[*currentByte] = '\xE9';  // é
                } 
                else {
                    (*buffer)[*currentByte] = ' ';
                } 
                (*currentByte)++;
                string++;
                continue;
            } 
        }

        (*buffer)[*currentByte] = *string;
        (*currentByte)++;
        string++;
    }

    return 0;
}

// The new one above tries to convert weird ascii to utf-8
int __WriteToBuffer(char* string, char** buffer, int buffer_size, int* currentByte)
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

