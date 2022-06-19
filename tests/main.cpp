

#include "../src/libs/Restart.h"
#include "../src/libs/uici.h"
#include "../src/handle_client/load_resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HOST "127.0.0.1"
#define DEFAULT_PORT 80
#define BUFFER_SIZE 32000

/*
 * TODO:
 * Test API calls
 * Tests with almost correct path but should be failing
 * Test with junk input, trying to break server
 * Test with multiple requests at the same time (stress test)
 */



int run_test(char* host, int port, char* request, int expectedCode, char* expectedResource);
int validate_response(int socket, int expectedCode, char* expectedBody, int expectedBody_size);



int main(int argc, char** argv)
{ 
    // If an argument was given, then set the port to the given argument
    int server_port = DEFAULT_PORT;
    if (argc == 2) {
        int arg_port = atoi(argv[1]);
        if (arg_port >= 0 || arg_port >= 65535) {
            server_port = arg_port;
        }
    }
    fprintf(stderr, "Running tests against server on port: %d\n\n", server_port);

    

    
    // Run tests
    int failed_tests = 0;
    int passed_tests = 0;

    // ------------------------------------------------------
    // TEST: Default path
    // ------------------------------------------------------
    if (run_test(HOST, server_port, "GET / HTTP/1.1\n", 200, "../resources/index.html\0") == -1) {
        fprintf(stderr, "Test failed for request: GET / HTTP/1.1\n\n");
        failed_tests++;
    } else {
        passed_tests++;
    }
    
    if (run_test(HOST, server_port, "GET /index HTTP/1.1\n", 200, "../resources/index.html\0") == -1) {
        fprintf(stderr, "Test failed for request: GET /index HTTP/1.1\n\n");
        failed_tests++;
    } else {
        passed_tests++;
    }

    if (run_test(HOST, server_port, "GET /index.html HTTP/1.1\n", 200, "../resources/index.html\0") == -1) {
        fprintf(stderr, "Test failed for request: GET /index.html HTTP/1.1\n\n");
        failed_tests++;
    } else {
        passed_tests++;
    }


    // ------------------------------------------------------
    // TEST: Admin page
    // ------------------------------------------------------
    if (run_test(HOST, server_port, "GET /admin/index HTTP/1.1\n", 200, "../resources/admin/index.html\0") == -1) {
        fprintf(stderr, "Test failed for request: GET /admin/index HTTP/1.1\n\n");
        failed_tests++;
    } else {
        passed_tests++;
    }
    
    if (run_test(HOST, server_port, "GET /admin/index.html HTTP/1.1\n", 200, "../resources/admin/index.html\0") == -1) {
        fprintf(stderr, "Test failed for request: GET /admin/index.html HTTP/1.1\n\n");
        failed_tests++;
    } else {
        passed_tests++;
    }



    fprintf(stderr, "Passed tests: %d/%d\n", passed_tests, (passed_tests + failed_tests));
    fprintf(stderr, "Failed tests: %d/%d\n", failed_tests, (passed_tests + failed_tests));


    return 0;
}




/**
 * --------------------------------------------------------------------------------------
 * Runs a test against the server
 *
 * host: The ip address of the server to connect to
 * port: The port that is used when connecting to the server
 * request: The request that should be sent to the server
 * expectedCode: The expected HTTP status code that the server should respond with
 * expectedResource: The file path to the resource that the server should respons with 
 *
 * Return 0 on success
 * Return -1 on failure and prints a message of what failed 
 * --------------------------------------------------------------------------------------
 */
int run_test(char* host, int port, char* request, int expectedCode, char* expectedResource)
{
    // Connect to the host with the given port
    int socket = u_connect(port, host);
    if (socket == -1) {
        fprintf(stderr, "Failed to connect to %s on port %d. Make sure the server is running on the given address and port and try again\n", host, port);
        return -1;
    }

    // Load the resource for the expected response
    char* expectedBody = 0;
    int expectedBody_size = 0;
    int status_code = 0;
    if (load_resource(expectedResource, &expectedBody, &expectedBody_size, &status_code) == -1 || expectedBody == 0) {
        fprintf(stderr, "Failed to run test: Could not load resource to compare against expected server response: %s\n", expectedResource);
        r_close(socket);
        return -1;
    } 
    
    // Send the given request to the server and validate the response
    if (r_write(socket, request, 256) == -1) {
        fprintf(stderr, "Failed to send request to server: %s\n", request);
        r_close(socket);
        return -1;
    }
    if (validate_response(socket, 200, expectedBody, expectedBody_size) == -1) {
        fprintf(stderr, "Invalid server response to the request: %s", request);
        free(expectedBody);
        r_close(socket);
        return -1;
    }

    free(expectedBody);
    r_close(socket);
    return 0;
}


/**
 * --------------------------------------------------------------------------------------
 * Reads and validates the reponse from the server
 *
 * socket: The file descriptor that is used to communicate with the server
 * expectedCode: The expected HTTP status code that the server should respond with
 * expectedBody: The expected message the server should respons with
 *
 * Return 0 on success
 * Return -1 on failure and prints a message of what went wrong when validating the response
 * --------------------------------------------------------------------------------------
 */
int validate_response(int socket, int expectedCode, char* expectedBody, int expectedBody_size)
{
    char buffer[BUFFER_SIZE];
    int bytes;
    if ((bytes = r_read(socket, buffer, BUFFER_SIZE)) == -1) {
        fprintf(stderr, "Failed to read response from server\n");
        return -1;
    }
    if (bytes < 2) {
        fprintf(stderr, "Failed to read response from server: The response is less than 2 bytes\n");
        return -1;
    }
    
    // Create substrings from the server response
    char* protocol = 0;
    char* status = 0;
    char* description = 0;
    char* body = 0;
    for (int i = 0; i < bytes; i++)
    {
        // Set the start of the protocol substring
        if (i == 0) {
            protocol = &buffer[i];
        }

        // Set the end of the protocol substring, and the start of status substring
        if (buffer[i] == ' ' && status == 0 && description == 0 && body == 0) {
            buffer[i] = '\0';
            if (i + 1 < bytes) {
                status = &buffer[i + 1];
            }
        }

        // Set the end of the status substring, and the start of the description substring
        if (buffer[i] == ' ' && status != 0 && description == 0 && body == 0) {
            buffer[i] = '\0';
            if (i + 1 < bytes) {
                description = &buffer[i + 1];
            }
        }

        // Set the end of the description substring
        if (buffer[i] == '\n' && status != 0 && description != 0 && body == 0) {
            buffer[i] = '\0';
            
            // Skip all the lines until the response body starts
            int counter = 0;
            while (i < bytes && counter < 4) {
                if (buffer[++i] == '\n') {
                    counter++;
                }
            }
            // Set tthe start of the response body substring
            if (counter == 4 && i + 1 < bytes) {
                body = &buffer[i + 1];
            }
        }
    }

    //fprintf(stderr, "\n%s\n%s\n%s\n%s\n", protocol, status, description, body);
    //fprintf(stderr, "\n--------------------\n%s\n-------------------------\n%s\n", expectedBody, body);

    // Validate the status code
    int status_int = atoi(status);
    if (status_int != expectedCode) {
        fprintf(stderr, "Invalid server response: The status code: %d does not match the requested: %d\n", status_int, expectedCode);
        return -1;
    }

    // Validate the response body
    if (expectedBody != 0) {
        int smallestSize = 0;
        expectedBody_size < bytes ? smallestSize = expectedBody_size : smallestSize = bytes;
        if (strncmp(expectedBody, body, smallestSize) != 0) {
            fprintf(stderr, "Inavlid server response: The response body does not match the requested body\n");
            return -1;
        }
    }

    return 0;
}



