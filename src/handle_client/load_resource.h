
#pragma once

#define RESOURCE_PATH "./resources"
#define DEFAULT_FILE "/index.html"

int load_resource(int socket, char* path, char** buffer, int* status_code);
