
#include "ConvertDB.h"

/**
 * ----------------------------------------------------------------------------------------------------------------
 * This program is built to convert the database into my own custom file format.
 * The databse is currently storing all its data in JSON format, which is easy to read, but very slow.
 * The new custom file format is storing all data in binary, which reduces the file sizes, but mainly speed up the 
 * time it takes to search through the file data to find the requested resources.
 *
 * Each function call converts one part of the database, in case you want to run/convert indivdual parts only.
 * The new files will be saved to the ./output directory
 * There is also a "load_and_print" function for each "file" of the database that can be called to test that the new
 * file format can be loaded, parsed and printed correctly.
 * ----------------------------------------------------------------------------------------------------------------
 */
int main()
{
    //convert_athletes_from_json_to_custom_format(false, false);  // bool toogles: "print_athletes" and "print_buffer"
    //load_and_print_athletes(false);                             // bool toogles "printAthletes"
    
    //convert_athletesRaces_from_json_to_custom_format(false);
    //load_and_print_athletesRaces(false);

    //convert_racesInfo_from_json_to_custom_format(false);
    //load_and_print_racesInfo(true);

    //convert_racesResults_from_json_to_custom_format(true);
    load_and_print_racesResults(true);

    return 0;
}

