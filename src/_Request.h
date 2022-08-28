
#pragma once

#define REQUEST_MAX_SIZE 32768
#define PATH_MAX_SIZE 256
#define RESOURCE_PATH "./resources"
#define NOT_FOUND "/not_found/index.html"
#define DEFAULT_FILE "/index.html"

#define HTTP_GET 1
#define HTTP_POST 2
#define HTTP_PUT 3
#define HTTP_DELETE 4

#define GET_RESOURCE 1
#define GET_TEMPLATE 2
#define API_ATHLETE_FISCODE 3
#define API_ATHLETES_FIRSTNAME 4
#define API_ATHLETES_LASTNAME 5
#define API_ATHLETES_FULLNAME 6
#define API_RACEIDS_FISCODE 7
#define API_RACEINFO_RACEID 8
#define API_RACERESULTS_RACEID 9
#define API_ANALYZE_QUAL_FISCODE 10

typedef struct {
    int method = 0;
    int type = 0;
    char path[PATH_MAX_SIZE];
    int parameter_start = 0;
} Request;

//bool does_str_begin_with(char* str1, char* str2);  // TODO: Move this function to util folder (String file maybe?)
int read_and_parse_request(int socket, Request* request);
int handle_request(int socket, Request* request);
int parse_requestline(char* line, int len, char** command, char** server, char** path, char** protocol, char** port);





