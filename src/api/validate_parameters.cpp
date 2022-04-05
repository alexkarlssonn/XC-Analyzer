
#include "api.h"
#include "../util/StringUtil.h"
#include <stdio.h>


/**
 * -------------------------------------------------------------------------------------
 * Validates the given parameter, and converts in into an integer.
 * 
 * param: A null terminated string containing the parameter.
 * 
 * Returns the converted parameter on success
 * Returns -1 on failure, which indicates that the parameter was either in an invalid format, or had a value that is outside of the allowed range
 * -------------------------------------------------------------------------------------
 */
int validate_and_convert_parameter(char* param)
{
	bool isValidParameter = false;
	char* p = param;
	while (*p != '\0') {
	    isValidParameter = true;
	    if (!is_digit(*p)) {
	        return -1;
	    }
	    p++;
	}

	if (!isValidParameter) {
		return -1;
	}

	int param_int;
	sscanf(param, "%d", &param_int);
	if (param_int <= 0 || param_int >= 100000000) {
	    return -1;
	}

	return param_int;
}