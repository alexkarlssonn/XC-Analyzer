
#include "LoadFile.h"

#include "./libs/Restart.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * --------------------------------------------------------------------------------------------------
 * Load a given file into a buffer
 *
 * path: The path to the file to load
 * buffer: Where the content of the file will be written to
 * size: The size of the buffer once the content has been loaded
 *
 * Returns 0 on success
 * Returns -1 on internal errors
 * Returns -2 if the file could not be opened
 * --------------------------------------------------------------------------------------------------
 */
int LoadFile(char* path, char** buffer, int* size)
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
	        return -2;
	    }
	} 
	
	int filesize = (int) lseek(fd, 0, SEEK_END); 
	if ((int)lseek(fd, 0, SEEK_SET) == -1) 
	{ 
	    fprintf(stderr, "[%ld] Failed to move file offset back to beginning after getting the file size: %s\n", (long)getpid(), strerror(errno));
	    if (r_close(fd) == -1) 
	        fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));
	    return -1;
	}
	
	char* bytes;
	if ((bytes = (char*) malloc( (filesize+1) * sizeof(char))) == 0) 
	{
	    fprintf(stderr, "[%ld] Failed to allocate memory to read the file content\n", (long)getpid());
	    if (r_close(fd) == -1) 
	        fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));
	    return -1;
	}
	
	int bytesread;
	if ((bytesread = readblock(fd, bytes, filesize)) <= 0) 
	{
	    fprintf(stderr, "[%ld] Failed to read the requested file: %s: %s\n", (long)getpid(), path, strerror(errno));
	    if (r_close(fd) == -1) 
	        fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));
	    return -1;
	}
	
	if (r_close(fd) == -1)
	    fprintf(stderr, "[%ld] Failed to close: %s: %s\n", (long)getpid, path, strerror(errno));

	bytes[bytesread] = '\0';
	*buffer = bytes;  // The buffer needs to be manually freed later
	if (size) {
		*size = bytesread;
	}

	return 0;
}
