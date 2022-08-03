
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
 * ---------------------------------------------------------------------
 * Main function that gets called once the window has fully loaded
 * ---------------------------------------------------------------------
 */
window.onload = function() 
{
    let athlete_info = document.getElementById("athlete-info");
    let dump = document.getElementById("dump");
    

    // Extract the fiscode from the url
    let fiscode = "";
    let substrings = window.location.href.split("/");
    if (substrings.length > 1) {
        fiscode = substrings[substrings.length - 1];
    }

    // Get and display the athlete info
    let url = window.location.protocol + "//" + window.location.host + "/api/athlete/fiscode/" + fiscode;
    http_async("GET", url, function(body, status_code) {
        if (status_code == 200) 
        {
            let json = JSON.parse(body);
            
            let name_div = document.createElement("div");
            athlete_info.appendChild(name_div);
            name_div.innerHTML = json.firstname + " " + json.lastname;

            let fiscode_div = document.createElement("div");
            athlete_info.appendChild(fiscode_div);
            fiscode_div.innerHTML = "Fiscode: " + json.fiscode;
          
            let birthdate_div = document.createElement("div");
            athlete_info.appendChild(birthdate_div);
            birthdate_div.innerHTML = "Birthdate: " + json.birthdate;

            let nation_div = document.createElement("div");
            athlete_info.appendChild(nation_div);
            nation_div.innerHTML = "Nation: " + json.nation;

            let club_div = document.createElement("div");
            athlete_info.appendChild(club_div);
            club_div.innerHTML = "Club: " + json.club;

            let gender_div = document.createElement("div");
            athlete_info.appendChild(gender_div);
            gender_div.innerHTML = "Gender: " + json.gender;
        } 
        else {
            athlete_info.innerHTML = "Could not find the given athlete";
        }
    });

   
    
    // Get an display the analyzed data
    url = window.location.protocol + "//" + window.location.host + "/api/analyze/qual/fiscode/" + fiscode;
    http_async("GET", url, function(body, status_code) {
        if (status_code == 200) 
        {
            let json = JSON.parse(body);
            let races = json.races;
            for (let i = 0; i < races.length; i++) 
            {
                race = races[i];
                
                let raceid_div = document.createElement("div");
                dump.appendChild(raceid_div);
                raceid_div.innerHTML = race.raceid;

                let date_div = document.createElement("div");
                dump.appendChild(date_div);
                date_div.innerHTML = race.date;

                let location_div = document.createElement("div");
                dump.appendChild(location_div);
                location_div.innerHTML = race.nation + ", " + race.location; 

                let type_div = document.createElement("div");
                dump.appendChild(type_div);
                type_div.innerHTML = race.category + ", " + race.type;

                let rank_div = document.createElement("div");
                dump.appendChild(rank_div);
                rank_div.innerHTML = "Rank: " + race.rank;
    
                let time_div = document.createElement("div");
                let time = convert_ms_to_time(race.time);
                dump.appendChild(time_div);
                time_div.innerHTML = "Time: " + time;

                let diff_div = document.createElement("div");
                let diff = convert_ms_to_time(race.diff);
                if (race.diff > 0) { diff = "+" + diff; }
                dump.appendChild(diff_div);
                diff_div.innerHTML = "Diff: " + diff;

                let diff_percentage_div = document.createElement("div");
                let diff_percentage = ((race["diff percentage"] - 1) * 100).toFixed(2);
                if (race["diff percentage"] > 1) { diff_percentage = "+" + diff_percentage; }
                dump.appendChild(diff_percentage_div);
                diff_percentage_div.innerHTML = "Diff percentage: " + diff_percentage + "%";

                let a = document.createElement("a");
                dump.appendChild(a);
                a.setAttribute("href", "http://" + window.location.host + "/race/" + race.raceid);
                a.innerHTML = "Link";

                let br = document.createElement("br");
                let br2 = document.createElement("br");
                dump.append(br);
                dump.append(br2);
            }
        }
        else {
            dump.innerHTML = "Could not find analyzed data";
        }
    });
    


};






