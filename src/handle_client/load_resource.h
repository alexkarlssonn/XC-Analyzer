
#pragma once

#define RESOURCE_PATH "./resources"
#define DEFAULT_FILE "/index.html"

int load_resource(char* path, char** buffer, int* size, int* status_code);
