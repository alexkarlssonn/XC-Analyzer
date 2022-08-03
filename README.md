
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
 



## Checklists and todo-lists

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
[X] Create a controller that maps requested paths to the physical filepaths inside this folder  
[ ] For all API Calls: On success when they print the success message, they should include the request in that message  
[ ] Basically what the point above said, but make sure the server prints out relevant, helpfull and accurate messages  
[ ] When a 404 not found occurs and that specific error page gets sent back, make sure its not sent back with the status code 200 like it is now


BUGS:  
[ ] Results are missing the pursuit time  
[X] Raceids for AnalyzeQual is wrong. They do no match the race they are refering to (FIXED: there was an error in one of the loops in the backend that returns the raceid)  



TODO v0.7:  
[X] Write test code that can easily be expanded later 
[X] Fix up the project layout. 
    
  

