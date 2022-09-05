
#include "api.h"

#include "../server/Server.h"
#include "../libs/Restart.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * ----------------------------------------------------------------------------
 * Load the resource that is requested in the paremter path.
 * The content of the file will be allocated dynamically and stored in the buffer parameter. This needs to be manually freed later.
 * If the function fails, it will print an error message, sets staus_code to indicate the error, and return -1.
 *
 * path: The requested resource. Should be a null terminated string and be a full path relative to the executable.
 * buffer: A pointer to the buffer that will store the content of the loaded file. 
 *         Should be null when calling this function, and needs to be manually freed later.
 * size: The number of bytes that was read and stored inside buffer.
 * status_code: The status code that should be sent back with an http response if this function fails.
 *
 *  Returns 0 on success, and sets buffer to the content of the file
 *  Returns -1 on failure, prints an error message, and sets status_code to indicate the error
 * ----------------------------------------------------------------------------
 */
int load_resource(char* path, char** buffer, int* size, int* status_code)
{
    int fd; 
    if ((fd = r_open2(path, O_RDONLY)) == -1) 
    {
        // Try to append .html if opening the file failed
        char newpath[strlen(path) + 8];
        strcpy(newpath, path);
        strcat(newpath, ".html");
        if ((fd = r_open2(newpath, O_RDONLY)) == -1) 
        {
            fprintf(stderr, "[%ld] Failed to open %s: %s\n", (long)getpid(), path, strerror(errno));
            *status_code = 404;
            return -1;
        }
    } 
    
    int filesize = (int) lseek(fd, 0, SEEK_END); 
    if ((int)lseek(fd, 0, SEEK_SET) == -1) 
    { 
        fprintf(stderr, "[%ld] Failed to move file offset back to beginning after getting the file size: %s\n", (long)getpid(), strerror(errno));
        if (r_close(fd) == -1) 
            fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));
        
        *status_code = 500;
        return -1;
    }
    
    char* bytes;
    if ((bytes = (char*) malloc( (filesize+1) * sizeof(char))) == 0) 
    {
        fprintf(stderr, "[%ld] Failed to allocate memory to read the file content\n", (long)getpid());
        if (r_close(fd) == -1) 
            fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));
        
        *status_code = 500;
        return -1;
    }
    
    int bytesread;
    if ((bytesread = readblock(fd, bytes, filesize)) <= 0) 
    {
        fprintf(stderr, "[%ld] Failed to read the requested file: %s: %s\n", (long)getpid(), path, strerror(errno));
        if (r_close(fd) == -1) 
            fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));

        *status_code = 500;
        return -1;
    }
    
    if (r_close(fd) == -1)
        fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));

    bytes[bytesread] = '\0';
    *buffer = bytes;  // The buffer needs to be manually freed later
    *size = bytesread;

    return 0;
}



