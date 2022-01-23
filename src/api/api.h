
#pragma once


#define DB_ATHLETES                "./db/athletes.json"
#define DB_ATHLETES_RACES          "./db/athletes-races.json"
#define DB_RACE_INFO               "./db/races-info.json"
#define DB_RACE_RESULT_0_22999     "./db/races-results/races-results-0-22999.json"
#define DB_RACE_RESULT_23000_25999 "./db/races-results/races-results-23000-25999.json"
#define DB_RACE_RESULT_26000_28999 "./db/races-results/races-results-26000-28999.json"
#define DB_RACE_RESULT_29000_31999 "./db/races-results/races-results-29000-31999.json"
#define DB_RACE_RESULT_32000_34999 "./db/races-results/races-results-32000-34999.json"
#define DB_RACE_RESULT_35000_37999 "./db/races-results/races-results-35000-37999.json"
#define DB_RACE_RESULT_38000_40999 "./db/races-results/races-results-38000-40999.json"


/*
 * Api calls for getting athlete
 * Function definitionns can be found inside athlete.cpp
 */
int api_getAthlete_fiscode(int socket, char* fiscode);
int api_getAthlete_firstname(int socket, char* firstname);
int api_getAthlete_lastname(int socket, char* lastname);
int api_getAthlete_fullname(int socket, char* fullname);
int getAthletesByName(int socket, const char* FIELD_NAME, char* name);


/*
 * Api calls for getting data about races, and all raceids for an athlete
 * Function definitions can be found iniside races.cpp
 */
int api_getRaceInfo(int socket, char* raceid);
int api_getRaceResult(int socket, char* raceid);
int api_getAthletesRaceids(int socket, char* fiscode);


/*
 * Api calls for getting analyzed results for a given athlete
 * Function defenitions can be found inside analyzed.cpp
 */
int api_getAnalyzedResults_qual(int socket, char* fiscode);




