
# XC Analyzer  
  
## How to build
The program has no dependencies or requiered libraries. Everything needed is inside this repository, and should allow you to build the program. On Mac and Linux, just run the shell script to build everything.  
This program is intended to run as a Linux server so there is no support for having it build on Windows. You can stil get it to build, but you probably need to do some minor changes to the program.  
  
```  
sh build
```  
  

## About
This is the server for my web application XC-Analyzer.  
XC-Analyzer is built to present results, athlete information, and statistics for the sport cross country skiing.
The basic idea is to provide a database for all existing results and athlete information, as well as statistics that can be used to analyze and track any athletes progress.  
  
The entire program is written in C/C++ and is meant to run on a dedicated machine as an HTTP server.  
The reason I did is is because I have been very interested in learning how servers and internet architecture works in general. After a while, I decided to build this web app from scratch as a learning experiece.  
  
When starting the program, the server will listen on a given socket (80 as default), and all incomming connections will be handled on seperate child processes. Once the request has been processed, an http response will be sent back to the client. I have tried to make the server do as much of the work as possible before sending back the requested resources, in order to improve performance. But most of the app is not static html pages, so there is still some javascript that runs in the browser once the resource has been received.  
  
The data handling for the server (database) is also written by myself, and the data is stored in my own custom binary format. This was also done as a learning experience, but the idea came about when I realized that the server was way to slow in finding, parsing and sending back the data. I managed to get the server much much faster after restructuring the database, and writing it myself.
  


## Checklists and todo-lists
  
 * [ ] Database: Include all races, not just for 2014-2021
 * [ ] Database: Implement an easy way to update the database with newer races once new results gets added
 * [ ] Database: Add fispoints and fispoints lists to the database
  
 * [ ] Athlete Page: Implement a graph to show race progress over a season and/or across seasons
 * [ ] Athlete Page: Make it possible to exclude individual races from the statistics     
  
 * [ ] Clean up the code for the API Calls
 * [ ] Frontend: Make the pages responsive and look good on smaller and mobile devices  
 * [ ] Homepage: Add more search options. (Search for races, athletes, show top athletes, etc.)    
 * [ ] Homepage: Add filter options for the results  
  
 * Long Term Goals:  
 * [ ] Athlete Page: Compare athletes against each other  
 * [ ] Athlete Page: Implement fispoints submenu once fispoints has been added to the database


BUGS:  
 * [ ] Athelte Page, Summary section: When filtering, and no races gets selected, the summary section shows NaN and var large values. Fix this!    
 * [ ] Results are missing the pursuit time  
 * [X] Raceids for AnalyzeQual is wrong. They do no match the race they are refering to (FIXED: there was an error in one of the loops in the backend that returns the raceid)  


  

