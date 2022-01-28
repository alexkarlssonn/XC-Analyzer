






TODO v0.6:
[ ] General cleanup and refactoring
[ ] Implement my own util functions, like string builders etc.
[ ] Reduce the executable file size as much as possible (more dynamic memory, etc. etc.)


BUGS:
Api get athletes by fullname, if lasstname is empty, then a bunch or random relay teams are returned
Results are missing the pursuit time




GOAL WITH VERSIONS (push and create a branch for each version on github):
 * [X] v0.2 - API calls for getting athlete
 * [X] v0.3 - Clean up database
 * [X] v0.4 - API call for getting all resultids for an athlete. API call for result by raceid
 * [X] v0.5 - API calls for getting analyzed results for an athlete
 * [ ] v0.6 - Backend cleanup, writing test code, bugfixes, etc.
 * [ ] v0.7 - Super simple frontend that can interact with API calls

 * [ ] v1.0 - First proper release! Decent looking frontend. Can look up athletes and analyze his/hers results with decent looking statistics. Can also filter which results to analyze 


DATABASE:  
 * athletes.json           - Stores all athletes
 * athletes-races.json     - Stores a list of all races (raceids) for each athlete 
 * races-info.json         - Stores the race info for each race
 * races-result-XX-YY.json - Stores the result list for each races with a raceids between XX and YY



DATABASE RE-WRITE:  
 * athletes.json
    (fiscode 4 bytes) (competitorid 4 bytes)
    - Fiscode is within 1 - 10,000,331 which means they can all be stored inside 4 bytes (32 bits)




