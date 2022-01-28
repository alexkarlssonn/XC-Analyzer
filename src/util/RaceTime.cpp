
#include "RaceTime.h"

#include "StringBuilder.h"
#include <string.h>
#include <stdlib.h>


/*
 * ----------------------------------------------------------------
 * Convert a string that represents the race time into milliseconds as a long type
 * The string (str) HAS to be a null terminated string, or else undefined behaviour may occur
 * Returns the time in milliseconds on success, and -1 on failure
 * ----------------------------------------------------------------
 */
unsigned int RaceTime_string_to_ms(char* time_string)
{
    if (time_string == 0)
        return -1;

    // Copy the given time_string since it needs to be modified
    char* str = 0;
    int str_size = strlen(time_string);
    str = (char*) malloc(str_size * sizeof(char));
    if (str == 0)
        return -1;
    strcpy(str, time_string);
    
    
    // Count the number of colons in the given string
    int colons = 0;
    char* p = str;
    while (*p != '\0') {
        if (*p == ':')
            colons += 1;
        p++;
    }
    // Validate if the given string has 0-2 colons
    if (colons < 0 && colons > 2) {
        if (str != 0)
            free(str);
        return -1;
    }


    // Time diffs starts with a '+' character
    // If that is the case, then skip it and start the string on the 2nd character
    p = str;
    if (p[0] == '+')
        p++;

    // This part of the code creates substrings for the hours, minutes, seconds and tenths in the time string
    // The substrings are created by having the pointers below point to different places in the string, 
    // and by replacing the delimiters with the null terminating character, to mark the end of the substrings
    char* hours = 0;
    char* minutes = 0;
    char* seconds = 0;
    char* tenths = 0;

    // Set either the seconds, minutes or hours to point to the start of the string
    // depending on how many colons the string had
    if (colons == 0)
        seconds = p;
    else if (colons == 1)
        minutes = p;
    else if (colons == 2)
        hours = p;

    // Loop through the entire stiring and look for the delimiters (':' and '.')
    while (*p != '\0') 
    {
        // TWO COLONS
        // Set the minutes-pointer first, and then the seconds-pointer
        if (*p == ':' && colons == 2) {
            if (minutes == 0)
                minutes = (p + sizeof(char));
            else
                seconds = (p + sizeof(char));
            *p = '\0';
        }

        // ONE COLON
        // Set the seconds-pointer to start after the colon
        if (*p == ':' && colons == 1) {
            seconds = (p + sizeof(char));
            *p = '\0';
        }

        // Set the tenths-ponter to start after the decimal sign
        if (*p == '.') { 
            tenths = (p + sizeof(char));
            *p = '\0';
        }
        
        p++;
    }

    unsigned int ms = 0;

    // Convert the string for hours to milliseconds
    if (hours != 0) {
        int value = atoi(hours);
        if (value <= 0) value = 0;
        ms += (value * 3600000);
    }
    
    // Convert the string for minutes to milliseconds
    if (minutes != 0) {
        int value = atoi(minutes);
        if (value <= 0) value = 0;
        ms += (value * 60000);
    }
    
    // Convert the string for seconds to milliseconds
    if (seconds != 0) {
        int value = atoi(seconds);
        if (value <= 0) value = 0;
        ms += (value * 1000);
    }

    // Convert the string for tenths/hundreths/milliseconds to milliseconds 
    if (tenths != 0) {
        int tenths_size = strlen(tenths);
        int value = atoi(tenths);
        if (value <= 0) value = 0;
        if (tenths_size == 1)
            ms += (value * 100);
        else if (tenths_size == 2)
            ms += (value * 10);
        else if (tenths_size == 3)
            ms += value;
    }

    if (str != 0)
        free(str);

    return ms;
}


/*
 * ----------------------------------------------------------------
 * Convert a race time in milliseconds to string format 
 * The string that gets retuned has been dynamically allocated and needs to be freed manually later
 * Return a pointer to the string that has been dynamically allocated
 * Returns 0 on failure
 * ----------------------------------------------------------------
 */
char* RaceTime_ms_to_string(long time)
{
    if (time < 0)
        return 0;

    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms = time;
    seconds = ms / 1000;
    ms %= 1000;
    minutes = seconds / 60;
    seconds %= 60;
    hours = minutes / 60;
    minutes %= 60;

    
    StringBuilder string;
    if ((str_init(&string, 16)) == -1)
        return 0;

    // Add hours to the string builder 
    if (hours > 0) {
        if (hours >= 10) {
            str_put(&string, ('0' + ((int)(hours / 10))));
            str_put(&string, ('0' + (hours % 10)));
            str_put(&string, ':');
        } else {
            str_put(&string, ('0' + hours)); 
            str_put(&string, ':');
        }
    }

    // Add minutes to the string builder
    if (minutes <= 0) {
        str_put(&string, '0');
        str_put(&string, '0');
        str_put(&string, ':');
    } 
    else if (minutes > 0 && minutes < 10) {
        if (hours > 0) {
            str_put(&string, '0');
            str_put(&string, ('0' + minutes));
            str_put(&string, ':');
        } else {
            str_put(&string, ('0' + minutes));
            str_put(&string, ':');
        }
    } 
    else {
        str_put(&string, ('0' + ((int)(minutes / 10))));
        str_put(&string, ('0' + (minutes % 10)));
        str_put(&string, ':');
    }

    // Add seconds to the string builder
    if (seconds <= 0) {
        str_put(&string, '0');
        str_put(&string, '0');
        str_put(&string, '.');
    } 
    else if (seconds > 0 && seconds < 10) {
        str_put(&string, '0');
        str_put(&string, ('0' + seconds));
        str_put(&string, '.');
    } 
    else {
        str_put(&string, ('0' + ((int)(seconds / 10))));
        str_put(&string, ('0' + (seconds % 10)));
        str_put(&string, '.');
    }

    // Add milliseconds to the string builder
    if (ms <= 0)
        str_put(&string, '0');
    else {
        int tenths = (int)(ms / 100);
        int hundreths = (int) (ms / 10) - (tenths * 10);
        if (hundreths < 0) hundreths = 0;
        str_put(&string, ('0' + tenths));
        str_put(&string, ('0' + hundreths));
    }
    
    str_put(&string, '\0');
    
    return string.string; 
}


/*
 * ----------------------------------------------------------------
 * Converts a percent value as a float, into a string
 * The percentage value should represent how far behind someone is the winner in a race
 * For example, a value of 1.000 means that it's the winning time, and 1.024 means the athlete is 2.4% behinds
 * Returns a string that has been dynamically allocated, this has to be manually freed later
 * ----------------------------------------------------------------
 */
char* RaceTime_percentage_to_string(float value)
{
    StringBuilder string;
    str_init(&string, 8);

    if (value <= 1)
    {
        str_put(&string, '0'); 
        str_put(&string, '0'); 
        str_put(&string, '.'); 
        str_put(&string, '0'); 
        str_put(&string, '0'); 
    }
    else 
    {
        value -= 1;
        value *= 10;
        int first = ((int)value) % 10;
        value -= first;
        value *= 10;
        int second = ((int)value) % 100;
        value -= second;
        value *= 10;
        int third = ((int)value) % 100;
        value -= third;
        value *= 10;
        int forth = ((int)value) % 1000;
        
        str_put(&string, ('0' + first));
        str_put(&string, ('0' + second));
        str_put(&string, '.');
        str_put(&string, ('0' + third));
        str_put(&string, ('0' + forth));
    }

    str_put(&string, '\0');

    return string.string;
}




