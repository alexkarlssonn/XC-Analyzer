
#pragma once

#define RESOURCE_PATH "./resources"  
#define DEFAULT_FILE "/index.html"
#define LINE_MAX_SIZE 256
#define CONNECTION_CLOSE "close"
#define CONNECTION_ALIVE "keep-alive"
#define TYPE_HTML "text/html; charset=iso-8859-1"
#define TYPE_JSON "application/json" 

int handleClient(int socket, char* response, int size);
int sendHttpResponse(int socket, int statuscode, const char* connection, const char* type, char** body);
int loadResource(int socket, char* path, char** buffer, int* status_code, char** error_message);
int parse_requestline(char* line, int len, char** command, char** server, char** path, char** protocol, char** port);
int isdigit(char ch);

