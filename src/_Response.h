
#pragma once

#define LINE_MAX_SIZE 256
#define CONNECTION_CLOSE "close"
#define CONNECTION_ALIVE "keep-alive"
#define TYPE_HTML "text/html; charset=iso-8859-1"
#define TYPE_JSON "application/json" 

int send_http_response(int socket, int statuscode, const char* connection, const char* type, const char* body);
int load_resource(char* path, char** buffer, int* size, int* status_code);

