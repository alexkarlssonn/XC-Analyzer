





TODO v0.4: 
[X] API call for getting raceids for athlete
[X] API call for getting race-info for a given raceid
[X] API call for geting race-results for a given raceid
[X] Clean up the athelte-races.json
    Some athletes have invalid races under their fiscode.
    Ex. Fiscode 12 has a few raceids, that are valid raceids, but he hasn't actually done these races. 


TODO v0.5:
[ ] Add ./db to gitignore
[ ] API calls for getting analyzed race results for a given athlete





GOAL WITH VERSIONS (push and create a branch for each version on github):
 * [X] v0.2 - API calls for getting athlete
 * [X] v0.3 - Clean up database
 * [X] v0.4 - API call for getting all resultids for an athlete. API call for result by raceid
 * [ ] v0.5 - API calls for getting analyzed results for an athlete
 * [ ] v0.6 - Backend cleanup, writing test code, bugfixes, etc.
 * [ ] v0.7 - Super simple frontend that can interact with API calls

 * [ ] v1.0 - First proper release! Decent looking frontend. Can look up athletes and analyze his/hers results with decent looking statistics. Can also filter which results to analyze 


DATABASE:  
 * athletes.json           - Stores all athletes
 * athletes-races.json     - Stores a list of all races (raceids) for each athlete 
 * races-info.json         - Stores the race info for each race
 * races-result-XX-YY.json - Stores the result list for each races with a raceids between XX and YY


