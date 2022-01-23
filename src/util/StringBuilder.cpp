

#include "StringBuilder.h"
#include <stdlib.h>


/* 
 * -------------------------------------------------------------------------
 * Calculates the size of a null terminated string.
 * The size is not counting the null terminated character.
 * The parameter str HAS to be a null terminated string, or else the behaviour is undefined
 * Returns the string size on success
 * Returns -1 on failure
 * -------------------------------------------------------------------------
 */
int getStringSize(char* str)
{
    int str_size = 0;
    char* p = str;
    
    while (*p != '\0') 
    {
        str_size++;
        p++;

        // A limit in case the null terminating character is never found
        if (str_size >= 1000000)
            return -1;  
    }
    
    return str_size;
}


/*
 * -------------------------------------------------------------------------
 * INIT
 *
 * Initiates a string builder 
 * Allocates memory using malloc for the given string builder and sets the string builders size on success
 * Returns 0 on success, and -1 on failure
 * -------------------------------------------------------------------------
 */
int str_init(StringBuilder* builder, int size)
{
    if (builder == 0 || size <= 0 || builder->string != 0)
        return -1;
    
    builder->string = (char*) malloc(size * sizeof(char));
    if (builder->string == 0)
        return -1;
    
    builder->maxsize = size;
    builder->size = 0;
    
    return 0;
}


/*
 * -------------------------------------------------------------------------
 * DESTORY
 *
 * Destorys a string builder
 * Calls free on the allocated memory for the given string builder
 * Returns 0 on success and -1 on failure
 * -------------------------------------------------------------------------
 */
int str_destroy(StringBuilder* builder)
{
    if (builder == 0 || builder->string == 0)
        return -1;
    
    free(builder->string);
    builder->maxsize = 0;
    builder->size = 0;
    
    return 0;
}


/**
 * -------------------------------------------------------------------------
 * SET
 *
 * Sets the string builder to the given string
 * The given string size (str_size) HAS to be the size of str, or else undefined behaviour may occur
 * The string (str) doesn't need to be null terminated
 * Returns 0 on success and -1 on failure
 * -------------------------------------------------------------------------
 */
int str_set(StringBuilder* builder, char* str, int str_size)
{
    if (builder == 0 || builder->string == 0 || str == 0 || str_size == 0 || (builder->maxsize < str_size))
        return -1;
    
    // Copy the characters from str to builder
    // Once all characters have been copied, set the remaining characters to 0
    for (int i = 0; i < builder->maxsize; i++) 
    {
        if (i < (str_size - 1))
            builder->string[i] = str[i];
        else
            builder->string[i] = 0;
    }

    builder->size = str_size;
    return 0;
}


/**
 * -------------------------------------------------------------------------
 * SET
 *
 * Sets the string builder to the given null terminated string
 * The given string (str) HAS to be a null terminated string, or else undefined behaviour may occur
 * Returns 0 on success and -1 on failure
 * -------------------------------------------------------------------------
 */
int str_set(StringBuilder* builder, char* str)
{
    if (builder == 0 || builder->string == 0 || str == 0) 
        return -1;

    int str_size = getStringSize(str);
    if (str_size == -1)
        return -1;

    if (builder->maxsize < str_size)
        return -1;

    // Copy the characters from str to builder
    // Once all characters have been copied, set the remaining characters to 0
    for (int i = 0; i < builder->maxsize; i++) 
    {
        if (i < (str_size - 1))
            builder->string[i] = str[i];
        else
            builder->string[i] = 0;
    }

    builder->size = str_size;
    return 0;
}


/**
 * -------------------------------------------------------------------------
 * APPEND
 *
 * Appends the given string (str) to the string builder
 * The given string size (str_size) HAS to be the size of str, or else undefined behaviour may occur
 * The string (str) doesn't need to be null terminated
 * Returns 0 on success and -1 on failure
 * -------------------------------------------------------------------------
 */
int str_append(StringBuilder* builder, char* str, int str_size)
{
    if (builder == 0 || builder->string == 0 || str == 0 || str_size <= 0 || ((builder->size + str_size) > builder->maxsize))
        return -1;

    // Appends str to builder 
    // Also updates size for builder
    int start = builder->size;
    for (int i = start; i < builder->maxsize; i++)
    {
        if ((i - start) < str_size)
            builder->string[i] = str[i - start];
        else
            builder->string[i] = 0;
    }

    builder->size += str_size;
    return 0;
}


/**
 * -------------------------------------------------------------------------
 * APPEND
 *
 * Appends the given null terminated string to builder 
 * The given string (str) HAS to be a null terminated string, or else undefined behaviour may occur
 * Returns 0 on success and -1 on failure
 * -------------------------------------------------------------------------
 */
int str_append(StringBuilder* builder, char* str)
{
    if (builder == 0 || builder->string == 0 || str == 0) 
        return -1;

    int str_size = getStringSize(str);
    if (str_size == -1)
        return -1;

    if ((builder->size + str_size) > builder->maxsize)
        return -1;

    // Appends str to builder 
    // Also updates size for builder
    int start = builder->size;
    for (int i = start; i < builder->maxsize; i++)
    {
        if ((i - start) < str_size)
            builder->string[i] = str[i - start];
        else
            builder->string[i] = 0;
    }

    builder->size += str_size;
    return 0;
}


/**
 * -------------------------------------------------------------------------
 * PUT
 *
 * Appends the given character to builder
 * If the builder is already full, -1 will be returned to indicate an error 
 * Returns 0 on success and -1 on failure
 * -------------------------------------------------------------------------
 */
int str_put(StringBuilder* builder, char ch)
{
    if (builder == 0 || builder->string == 0 || ((builder->size + 1) > builder->maxsize))
        return -1;

    builder->string[builder->size] = ch;
    builder->size += 1;
    return 0;
}








