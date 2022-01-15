
#pragma once

#define DB_ATHLETES "./db/athletes.json"

int api_getAthlete_fiscode(int socket, char* fiscode);
int api_getAthlete_firstname(int socket, char* firstname);
int api_getAthlete_lastname(int socket, char* lastname);
int getAthletesByName(int socket, const char* FIELD_NAME, char* name);
int api_getAthlete_fullname(int socket, char* fullname);


