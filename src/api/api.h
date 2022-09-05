
#pragma once

#include "../libs/cJSON.h"

#define DB_ATHLETES       "./db/athletes.bin"
#define DB_ATHLETES_RACES "./db/athletes-races.bin"
#define DB_RACE_INFO      "./db/races-info.bin"
#define DB_RACE_RESULTS   "./db/races-results.bin"


int load_resource(char* path, char** buffer, int* size, int* status_code);


/* ===============================================================
 * Api calls for getting athletes
 * Function definitionns can be found inside "athlete.cpp"
 =============================================================== */
int api_getAthlete_firstname(int socket, char* firstname);
int api_getAthlete_lastname(int socket, char* lastname);
int api_getAthlete_fullname(int socket, char* fullname);
int api_getAthlete_fiscode(int socket, char* fiscode);


/* ===============================================================
 * Api calls for getting data about races, and all raceids for an athlete
 * Function definitions can be found iniside "races.cpp"
 =============================================================== */
int api_getAthletesRaceids(int socket, char* fiscode);
int api_getRaceInfo(int socket, char* raceid);
int api_getRaceResult(int socket, char* raceid);


/* ===============================================================
 * Api calls for getting analyzed results for a given athlete
 * Function defenitions can be found inside "analyzed.cpp"
 =============================================================== */
int api_getAnalyzedResults_qual(int socket, char* fiscode_str);


/* ===============================================================
 * Other functions used in most of the api calls
 =============================================================== */
int validate_and_convert_parameter(char* param);



