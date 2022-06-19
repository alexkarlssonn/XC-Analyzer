
# XC Analyzer  
  
## How to build
This server is built on a MAC but should work fine on Linux. On Windows, you probably need to change the build file and/or the compiler used.  
On Mac simple run the shell script to build everything.  
  
```  
sh build
```  
  

## About
Eveything is work in progress so things might be very messy and not very well documented.  
  
The goal of this project is to build a web application that can anlyze the progress of a given athlete.  
The basic idea is to present data from all FIS competiton the athlete has attended, and present a graph that shows his/hers progress.  
Some of the data that I want to include is: Difference from winner in time, percentage and rank, as well as a metric that indicate how "competitive" each competition was.  
  
Most of this code is currently the backend. I have been very interested in learning how servers and internet architecture works in general, so I decided to build eveything from scratch in C/C++ as a learning experiece. The server will listen on a given socket (80 as default), and all incomming connections will be handled on seperate child processes. Below is a list of the paths and API call that currently exist, but keep in mind that everything is work in progress:  
  
Paths and API calls:  
 * / (or /index.html)  
 * /test  
 * /admin/index  
 * /api/athlete/fiscode/{FISCODE} 
 * /api/athletes/firstname/{NAME} 
 * /api/athletes/lastname/{NAME} 
 * /api/athletes/fullname/{FIRSTNAME}/{LASTNAME} 
 * /api/raceids/fiscode/{FISCODE}       (Returns a list of all raceids that exist in my DB for the given athlete)  
 * /api/raceinfo/raceid/{RACEID}        (Returns all race information for the given raceid)  
 * /api/raceresults/raceid/{RACEID}     (Returns the list of results for the given raceid)  
 * /api/analyze/qual/fiscode/{FISCODE}  (Analyzes and returns all sprint qualifications for the given athlete)  
 





VERSION CHECKLIST:  
 * [X] v0.2 - API calls for getting athlete
 * [X] v0.3 - Clean up database
 * [X] v0.4 - API call for getting all resultids for an athlete. API call for result by raceid
 * [X] v0.5 - API calls for getting analyzed results for an athlete
 * [X] v0.6 - Cleanup, convert database to binary format instead of slow JSON format 
 * [ ] v0.7 - Write some tests, bugfixes, etc.
 * [ ] v0.8 - Simple frontend that can interact with API calls

 * [ ] v1.0 - First proper release! Decent looking frontend. Can look up athletes and analyze his/hers results with decent looking statistics. Can also filter which results to analyze 
  
  
GENERAL TODOS:  
[ ] Blacklist css and js files from the resource folders when sending requests to the server  


  

BUGS:  
Results are missing the pursuit time  



TODO v0.7:  
[ ] Test ALL possible code paths and error handlers for all API calls!  
[X] Write test code that can easily be expanded later 
[ ] Fix up the folder layout. Inside this "backend" folder should everything releated to the backend be  
    Outside the backend folder, there should be a folder for "tests", and also move the convertDB folder out there  
  



  

TODO v0.6:  
[X] Converted database to my own custom format  
[X] General cleanup and refactoring  
[X] Implement my own util functions, like string builders etc.  
[X] Rewrite all API calls to use the new custom file format    
  
  
  
The current step I'm working on is converting the API calls to use the new db that has the new binary file format  
See the folder "convert_datatbase" for code that converts the database from JSON to the new binary format.  
API CALLS THAT HAS BEEN CONVERTED:  
[X] Athlete by fiscode  
[X] Athlete by firstname  
[X] Athlete by lastname  
[X] Athlete by fullname  
[X] Raceids  
[X] Race info  
[X] Race results  
[X] Analyzed results  
  







