
#pragma once

#define DB_RACE_INFO "./db/races-info.json"
#define DB_RACE_RESULT_0_22999 "./db/races-results/races-results-0-22999.json"
#define DB_RACE_RESULT_23000_25999 "./db/races-results/races-results-23000-25999.json"
#define DB_RACE_RESULT_26000_28999 "./db/races-results/races-results-26000-28999.json"
#define DB_RACE_RESULT_29000_31999 "./db/races-results/races-results-29000-31999.json"
#define DB_RACE_RESULT_32000_34999 "./db/races-results/races-results-32000-34999.json"
#define DB_RACE_RESULT_35000_37999 "./db/races-results/races-results-35000-37999.json"
#define DB_RACE_RESULT_38000_40999 "./db/races-results/races-results-38000-40999.json"

int api_getRaceInfo(int socket, char* raceid);
int api_getRaceResult(int socket, char* raceid);

