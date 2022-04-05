



GOAL WITH VERSIONS (push and create a branch for each version on github):
 * [X] v0.2 - API calls for getting athlete
 * [X] v0.3 - Clean up database
 * [X] v0.4 - API call for getting all resultids for an athlete. API call for result by raceid
 * [X] v0.5 - API calls for getting analyzed results for an athlete
 * [X] v0.6 - Cleanup, convert database to binary 
 * [ ] v0.7 - Write test code, bugfixes, etc.
 * [ ] v0.8 - Super simple frontend that can interact with API calls

 * [ ] v1.0 - First proper release! Decent looking frontend. Can look up athletes and analyze his/hers results with decent looking statistics. Can also filter which results to analyze 

  

BUGS:  
Results are missing the pursuit time  

  

TODO v0.6:  
[X] Converted database to my own custom format
[X] General cleanup and refactoring
[X] Implement my own util functions, like string builders etc.
[X] Rewrite all API calls to use the new custom file format  
  

The current step I'm working on is converting the API calls to use the new db that has the new binary file format  
API CALLS THAT HAS BEEN CONVERTED:  
[X] Athlete by fiscode
[X] Athlete by firstname
[X] Athlete by lastname
[X] Athlete by fullname
[X] Raceids
[X] Race info
[X] Race results
[X] Analyzed results

  


TODO v0.7:
[ ] Test ALL possible code paths and error handlers for all API calls!
[ ] Write test code (maybe?)
[ ] Fix up the folder layout. Inside this "backend" folder should everything releated to the backend be
    Outside the backend folder, there should be a folder for "tests", and also move the convertDB folder out there









