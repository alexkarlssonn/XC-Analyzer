
#pragma once

#define TEMPLATE_ATHLETE "./resources/athlete/template.html"
#define TEMPLATE_RACE    "./resources/race/template.html"

// Used for displaying a single race in the statistics menu
typedef struct {
    char date[256];
    char location[256];
    char nation[256];
    char category[256];
    char discipline[256];
    char type[256];
    unsigned int participants = 0;
    unsigned int rank = 0;
    unsigned int time = 0;
    unsigned int diff = 0;
    char fispoints[16];
} RaceData;


int CreatePage_Athlete(int fiscode, char** PageBuffer, int* PageBuffer_size);
int CreatePage_RaceResults(int raceid, char** PageBuffer, int* PageBuffer_size);

// Util functions
int WriteToBuffer(char* string, char** buffer, int buffer_size, int* currentByte);


