
#pragma once

#define TEMPLATE_ATHLETE "./resources/athlete/template.html"
#define TEMPLATE_RACE    "./resources/race/template.html"


int CreatePage_Athlete(int fiscode, char** PageBuffer, int* PageBuffer_size);
int CreatePage_RaceResults(int raceid, char** PageBuffer, int* PageBuffer_size);


// Util functions
int WriteToBuffer(char* string, char** buffer, int buffer_size, int* currentByte);


