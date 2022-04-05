
#include "parse_requestline.h"
#include "../util/StringUtil.h"

/**
 * ----------------------------------------------------------------------------
 * Parses the initial HTTP request line
 *  Returns 0 on success which means atleast the following 3 tokens was parsed and set: command, path and protocol
 *  Returns -1 on failure
 *  
 *  The first two parameters whould be the line and it's length. The line should end with a newline character.
 *  The other parameters will represent the different tokens (substrings) inside line, once it has been parsed.
 *  These parameters are pointers that will point to the start of their respective tokens. 
 *  Each of these tokens will be set to end with a '\0' character once line has been parsed.
 *  
 *  The server and port tokens are optional, and will be set to 0 if they are not found.
 *  The command, path and protocol tokens will also be set to 0 if the functions fails.
 * ----------------------------------------------------------------------------
 */
int parse_requestline(char* line, int len, char** command, char** server, char** path, char** protocol, char** port)
{
    *command = 0;
    *server = 0;
    *path = 0;
    *protocol = 0;
    *port = 0;
    
    // =======================================================================
    // COMMAND, PATH and PROTOCOL TOKEN
    // =======================================================================
    int tokens = 0;
    bool newline = false;
    bool creatingToken = false;

    for (int i = 0; i < len; i++) {
        if (line[i] == '\n')
            newline = true;
        
        // Start creating a token if a new string was detected, and no token is currently being created
        if (!creatingToken && (line[i] != ' ') && (line[i] != '\n') && (line[i] != '\r') && (line[i] != '\t')) {
            creatingToken = true;
            if (tokens == 0) 
                *command = &(line[i]);
            else if (tokens == 1) 
                *path = &(line[i]);
            else if (tokens == 2) 
                *protocol = &(line[i]);
        }
        // Finish creating the current token if the end of a string is detected 
        else if (creatingToken && (line[i] == ' ' || line[i] == '\n' || line[i] == '\r' || line[i] == '\t')) {
            creatingToken = false;
            line[i] = '\0';
            tokens++;
        }
        
        if (newline) 
            break;
    } 

    // Parsing failed if not excatly 3 tokens was created successfully
    if (tokens != 3 || creatingToken) {
        *command = 0;
        *path = 0;
        *protocol = 0;
        return -1;
    }

    // =======================================================================
    // SERVER TOKEN (optional)
    // =======================================================================
    char* p = *path;
    int counter = 0;
    bool startsWithHttp = true;
    bool serverTokenCreated = false;
    bool creatingPortToken = false;

    while (*p != '\0' && startsWithHttp)
    {
        if (counter == 0 && *p != 'h') startsWithHttp = false;
        if (counter == 1 && *p != 't') startsWithHttp = false;
        if (counter == 2 && *p != 't') startsWithHttp = false;
        if (counter == 3 && *p != 'p') startsWithHttp = false;
        if (counter == 4 && *p != ':') startsWithHttp = false;
        if (counter == 5 && *p != '/') startsWithHttp = false;
        if (counter == 6 && *p != '/') startsWithHttp = false;
        
        // Move every character back one step if the path-strings starts with "http://"
        // Do this until the server token has been created, and also set the server pointer to start after "http://"
        if (counter > 6 && startsWithHttp) {
            if (*server == 0)
                *server = (p - sizeof(char));
            if (!serverTokenCreated) 
                *(p - sizeof(char)) = *p;
        }
       
        // If server token is being created and a '/' char was found, then set it to a null-character which marks the end of the server token
        // Also update the path token to start after the server token
        if (counter > 6 && startsWithHttp && *p == '/') {
            if (!serverTokenCreated) {
                *(p - sizeof(char)) = '\0'; 
                serverTokenCreated = true;
                *path = p;
            }
        }
        
        p++;
        counter++;
    }

    if (*server != 0 && !serverTokenCreated)
        *server = 0;

    // =======================================================================
    // PORT TOKEN (optional)
    // =======================================================================
    if (*server != 0) {
        p = *server;
        while (*p != '\0') {    
            // Clear the port token if a non-digit shows up during creation
            // Also set the port token to the digit after the first ':' character
            if (*port != 0 && !is_digit(*p))
                *port = 0;
            if (*p == ':' && *port == 0 && is_digit(*(p + sizeof(char)))) 
                *port = (p + sizeof(char));
            p++;
        }

        // If a port was given, then the server token should end before the port
        if (*port != 0) {
            p = *server;
            while (*p != '\0') {
                if (*p == ':')
                    *p = '\0';
                p++;
            }
        }
    }

    return 0;
}


