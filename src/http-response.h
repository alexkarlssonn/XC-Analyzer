
#pragma once

#define RELATIVE_PATH "./paths"  
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


// TODO: Remove these once the handleClient function has been modified to always use sendHttpResponse
//#define DEFAULT_RESPONSE_HEADER_200 "HTTP/1.1 200 OK\r\nConnection: Closed\r\nContent-Type: text/html; charset=iso-8859-1\r\n\n"
//#define DEFAULT_RESPONSE_HEADER_400 "HTTP/1.1 400 Bad Request\r\nConnection: Closed\r\n\n"
//#define DEFAULT_RESPONSE_HEADER_500 "HTTP/1.1 500 Internal Server Error\r\nConnection: Close\r\n\n"
//int createResponseHeader(char** header, int statuscode, const char* customdesc);
//void sendClientError(int socket);  
// void sendServerError(int socket);  

