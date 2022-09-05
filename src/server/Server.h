
#pragma once

#define REQUEST_MAX_SIZE 32768
#define LINE_MAX_SIZE 256
#define CONNECTION_CLOSE "close"
#define CONNECTION_ALIVE "keep-alive"
#define TYPE_HTML "text/html; charset=iso-8859-1"
#define TYPE_JSON "application/json"

typedef struct {
    char* buffer = 0;  
    int buffer_size = 0;

    // Substrings - Pointers to different locations in the buffer above
    char* protocol = 0;
    char* method = 0;
    char* server = 0;
    char* port = 0;
    char* path = 0;
    char* query = 0;
    char* headers = 0;
    char* body = 0;
} Request;

int ReadClientRequest(int socket, Request* request);
int HandleClientRequest(int socket, Request* request);
int SendHttpResponse(int socket, int statuscode, const char* connection, const char* type, const char* body);

