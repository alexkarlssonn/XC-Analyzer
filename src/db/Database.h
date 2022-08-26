
#pragma once

#define DB_RACE_INFO      "./db/races-info.bin"
#define DB_RACE_RESULTS   "./db/races-results.bin"

typedef struct {
    unsigned int codex;
    char date[256];
    char nation[256];
    char location[256];
    char category[256];
    char discipline[256];
    char type[256];
    char gender[8];
} RaceInfo;

typedef struct {
    unsigned int rank;
    unsigned int bib;
    unsigned int fiscode;
    unsigned int time;
    unsigned int diff;
    unsigned int year;
    char name[256];
    char nation[256];
    char fispoints[16];
} ResultElement;


int LoadFromDatabase_RaceInfo(int raceid, RaceInfo* race_info);
int LoadFromDatabase_RaceResults(int raceid, ResultElement** results, int* results_size);


