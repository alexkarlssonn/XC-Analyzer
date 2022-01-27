
#pragma once

#include "../src/libs/cJSON.h"

// ----------------------------------------
// athletes.cpp
// ----------------------------------------
int convert_athletes_from_json_to_custom_format(bool print_athletes, bool print_buffer);
int load_and_print_athletes(bool printAll);


// ----------------------------------------
// athletes-races.cpp
// ----------------------------------------
int convert_athletesRaces_from_json_to_custom_format(bool print_buffer);
int load_and_print_athletesRaces(bool printAll);


// ----------------------------------------
// races-info.cpp
// ----------------------------------------
int convert_racesInfo_from_json_to_custom_format(bool print_buffer);
int load_and_print_racesInfo(bool printAll);
   

// ----------------------------------------
// races-info.cpp
// ----------------------------------------
int convert_racesResults_from_json_to_custom_format(bool print_buffer);

// ----------------------------------------
// util.cpp
// ----------------------------------------
cJSON* loadAndParse(const char* file, int* size);
void writeStringToBuffer(unsigned char** buffer, int* size, char* string);
void printBuffer(unsigned char** buffer, int size);


