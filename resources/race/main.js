
/**
 * ------------------------------------------------------------------
 * Send an http request with the given method to the given url
 *
 * method: The HTTP method to use (example. "GET")
 * url: The path for the request (the domain and port gets added automatically)
 * callback: The callback function once the response has been received
 * ------------------------------------------------------------------
 */
function http_async(method, url, callback)
{
    let http = new XMLHttpRequest();
    http.onreadystatechange = function() {
        if (http.readyState == 4) {
            callback(http.responseText, http.status);
        }
    }
    http.open(method, url, true);
    http.send(null);
}


/**
 * ------------------------------------------------------------------------
 * Helper function for padding a number with zeroes.
 * Used when displaying the racetime from the database
 * ------------------------------------------------------------------------
 */
function padTo2Digits(num)
{
    return num.toString().padStart(2, '0');
}

/**
 * ------------------------------------------------------------------------
 * Converts a given number of milliseconds to a "racetime string" (either mm:ss.hh or ss.hh)
 * Used when displaying the racetime returned from the database
 * ------------------------------------------------------------------------
 */
function convert_ms_to_time(ms)
{
    let hundreths = Math.floor(ms / 10);
    let seconds = Math.floor(ms / 1000);
    let minutes = Math.floor(ms / 60000);
    hundreths = hundreths % 100;
    seconds = seconds % 60;
    minutes = minutes % 60;
    if (minutes > 0)
        return `${padTo2Digits(minutes)}:${padTo2Digits(seconds)}.${padTo2Digits(hundreths)}`;
    else
        return `${padTo2Digits(seconds)}.${padTo2Digits(hundreths)}`;
}


/**
 * -----------------------------------------------------------------------
 * Main function
 * -----------------------------------------------------------------------
 */
window.onload = function() 
{
    let race_info = document.getElementById("race-info");
    let race_results = document.getElementById("race-results");

    
    // Extract the raceid from the url
    let raceid = "";
    let substrings = window.location.href.split("/");
    if (substrings.length > 1) {
        raceid = substrings[substrings.length - 1];
    }

    let race_info_url = "http://" + window.location.host + "/api/raceinfo/raceid/" + raceid;
    http_async("GET", race_info_url, function(body, status_code) {
        if (status_code == 200) {
            let json = JSON.parse(body);
            if (json) 
            {
                let date_div = document.createElement("div");
                race_info.appendChild(date_div);
                date_div.innerHTML = json.date;

                let discipline_div = document.createElement("div");
                race_info.appendChild(discipline_div);
                discipline_div.innerHTML = json.discipline;

                let location_div = document.createElement("div");
                race_info.appendChild(location_div);
                location_div.innerHTML = json.location + " " + json.nation;
            }
        }
        else 
        {
            race_info.innerHTML = "Could not find the race with raceid: " + raceid;
        }
    });


    let race_results_url = "http://" + window.location.host + "/api/raceresults/raceid/" + raceid;
    http_async("GET", race_results_url, function(body, status_code) {
        if (status_code == 200) {
            let json = JSON.parse(body);
            if (json) {
                if (json.results) 
                {
                    results = json.results;
                    for (let i = 0; i < results.length; i++) {
                        result = results[i];
                        if (result) 
                        {
                            let result_div = document.createElement("div");
                            race_results.appendChild(result_div);

                            let rank_div = document.createElement("div");
                            race_results.appendChild(rank_div);
                            rank_div.innerHTML = "Rank: " + result.rank;

                            let name_div = document.createElement("div");
                            race_results.appendChild(name_div);
                            name_div.innerHTML = result.athlete;
                            
                            let fiscode_div = document.createElement("div");
                            race_results.appendChild(fiscode_div);
                            fiscode_div.innerHTML = "Fiscode: " + result.fiscode;

                            let nation_div = document.createElement("div");
                            race_results.appendChild(nation_div);
                            nation_div.innerHTML = result.nation;

                            let born_div = document.createElement("div");
                            race_results.appendChild(born_div);
                            born_div.innerHTML = result.year;

                            let racetime = convert_ms_to_time(result.time);
                            let difftime = convert_ms_to_time(result.diff);
                            if (result.diff > 0) { difftime = "+" + difftime; }
                            let time_div = document.createElement("div");
                            race_results.appendChild(time_div);
                            time_div.innerHTML = "Time: " + racetime + " (" + difftime + ")";

                            let fispoints_div = document.createElement("div");
                            race_results.appendChild(fispoints_div);
                            fispoints_div.innerHTML = "Fispoints: " + result.fispoints;

                            let br = document.createElement("br");
                            race_results.appendChild(br);
                        }
                    }
                }
            }
        }
        else 
        {
            race_info.innerHTML = "Could not find any results for the race with raceid: " + raceid;
        }
    });
};





