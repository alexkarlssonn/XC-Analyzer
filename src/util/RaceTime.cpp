
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
long RaceTime_string_to_ms(char* time_string)
{
    int str_size = strlen(time_string);
    char* str = 0;
    str = (char*) malloc(str_size * sizeof(char));
    if (str == 0)
        return -1;
    strcpy(str, time_string);
    
    // Valid time strings has one or two colons
    // If the given strings has more or less, return -1
    int colons = 0;
    char* p = str;
    while (*p != '\0') {
        if (*p == ':')
            colons += 1;
        p++;
    }
    if (colons < 1 && colons > 2)
        return -1;

    // Time diffs starts with a '+' character
    // If that is the case, then skip it and start the string on the 2nd character
    p = str;
    if (p[0] == '+')
        p++;

    char* hours = 0;
    char* minutes = 0;
    char* seconds = 0;
    char* tenths = 0;
    if (colons == 1)
        minutes = p;
    else if (colons == 2)
        hours = p;

    while (*p != '\0') {
        // If there are two colons, then set the minutes-pointer first, and seconds-pointer the second
        if (*p == ':' && colons == 2) {
            if (minutes == 0)
                minutes = (p + sizeof(char));
            else
                seconds = (p + sizeof(char));
            *p = '\0';
        }

        // If there are one colon, then set the seconds-pointer
        if (*p == ':' && colons == 1) {
            seconds = (p + sizeof(char));
            *p = '\0';
        }

        // Set a pointer to start AFTER the decimal sign to indicate the start of the tenths/hundreths/milliseconds
        if (*p == '.') { 
            tenths = (p + sizeof(char));
            *p = '\0';
        }
        
        p++;
    }

    long ms = 0;

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




