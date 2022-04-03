
#pragma once

int parse_requestline(char* line, int len, char** command, char** server, char** path, char** protocol, char** port);
