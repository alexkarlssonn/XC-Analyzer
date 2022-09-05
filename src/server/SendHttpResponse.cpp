
#include "Server.h"

#include "../libs/Restart.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/**
 * ----------------------------------------------------------------------------
 * Constructs and sends an http response over the given socket
 * On failure, an error message will be printed and -1 will be returned.
 * Only the socket and status code are required by the function, the rest can be NULL / 0.
 * All parameters for setting the header lines needs to be null terminated to prevent unexpected errors
 *
 * socket: The file descriptor that represents the socket to send the http response over.
 * statuscode: The status code to use.
 * connection: The connection type in the header. Should be a valid connection type.
 * type: The content type in the header. Should be a valid content type.
 * body: The actual body of the HTTP response.
 *
 * Returns 0 on success
 * Return -1 on failure
 * ----------------------------------------------------------------------------
 */
int SendHttpResponse(int socket, int statuscode, const char* connection, const char* type, const char* body)
{
    if (statuscode < 100 || statuscode >= 600) {
        fprintf(stderr, "[%ld] Failed to send HTTP Response: Invalid status code\n", (long)getpid());
        return -1;
    }

    // Convert the status code into a string
    int digit3 = (statuscode / 100);
    int digit2 = ((statuscode - (digit3 * 100)) / 10);
    int digit1 = (statuscode - (digit3 * 100) - (digit2 * 10));
    char statuscode_str[5];
    statuscode_str[0] = (char) (((int)'0') + digit3);
    statuscode_str[1] = (char) (((int)'0') + digit2);
    statuscode_str[2] = (char) (((int)'0') + digit1);
    statuscode_str[3] = ' ';
    statuscode_str[4] = '\0';


    // Create the individual header lines
    char status_line[LINE_MAX_SIZE];
    strcpy(status_line, "HTTP/1.1 ");
    strcat(status_line, statuscode_str);
    if (statuscode == 200)
        strcat(status_line, "OK");
    else if (statuscode == 201)
        strcat(status_line, "Created");
    else if (statuscode == 202)
        strcat(status_line, "Accepted");
    else if (statuscode == 204)
        strcat(status_line, "No Content");
    else if (statuscode == 206)
        strcat(status_line, "Partial Content");
    else if (statuscode == 400)
        strcat(status_line, "Bad Request");
    else if (statuscode == 401)
        strcat(status_line, "Unauthorized");
    else if (statuscode == 403)
        strcat(status_line, "Forbidden");
    else if (statuscode == 404)
        strcat(status_line, "Not Found");
    else if (statuscode == 500)
        strcat(status_line, "Internal Server Error");
    else
        strcat(status_line, " No-Response-Phrase");
    strcat(status_line, "\r\n");

    char date[32];
    time_t time_now;
    struct tm ts;
    time_now = time(0);
    ts = *gmtime(&time_now);
    strftime(date, LINE_MAX_SIZE, "%a, %d %b %Y %H:%M:%S %Z", &ts);

    char date_line[LINE_MAX_SIZE];
    strcpy(date_line, "Date: ");
    strcat(date_line, date);
    strcat(date_line, "\r\n");

    char connection_line[LINE_MAX_SIZE];
    if (connection != 0) {
        strcpy(connection_line, "Connection: ");
        strcat(connection_line, connection);
        strcat(connection_line, "\r\n");
    }

    char type_line[LINE_MAX_SIZE];
    if (type != 0) {
        strcpy(type_line, "Content-Type: ");
        strcat(type_line, type);
        strcat(type_line, "\r\n");
    }

    int status_line_size = strlen(status_line);
    int date_line_size = strlen(date_line);
    int connection_line_size = 0;
    int type_line_size = 0;
    if (connection != 0)
        connection_line_size = strlen(connection_line);
    if (type != 0)
        type_line_size = strlen(type_line);

    // Combine all the individual lines into a full HTTP Header
    int HEADER_SIZE = status_line_size + date_line_size + connection_line_size + type_line_size + (sizeof(char) * 3);
    char header[HEADER_SIZE];
    strcpy(header, status_line);
    strcat(header, date_line);
    if (connection != 0)
        strcat(header, connection_line);
    if (type != 0)
        strcat(header, type_line);
    if (body != 0)
        strcat(header, "\r\n");


    // Combine the header and body into a full HTTP Response
    int BODY_SIZE = 0;
    if (body != 0)
        BODY_SIZE = strlen(body) + 1;  // The extra character is for the '\n' that will be added at the end
    int RESPONSE_SIZE = (HEADER_SIZE + BODY_SIZE);
    char http_response[RESPONSE_SIZE];
    strcpy(http_response, header);
    if (body != 0) {
        strcat(http_response, body);
        strcat(http_response, "\n");  // The body should end with a newline character
    }

    // Send the full HTTP Response over the socket
    // Using (RESPONSE_SIZE - 1) for the size, since '\0' is not needed and actually prevents javascript files from working
    if (r_write(socket, http_response, RESPONSE_SIZE - 1) == -1) {
        fprintf(stderr, "[%ld] Failed to send HTTP Response: r_write failed: %s\n", (long)getpid(), strerror(errno));
        return -1;
    }

    return 0;
}




