
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
    let races_div = document.getElementById("races");    

    // Extract the fiscode from the url
    let fiscode = "";
    let substrings = window.location.href.split("?")
    if (substrings.length > 1) {
        fiscode = substrings[1]
    }
/*
    let fiscode = "";
    let substrings = window.location.href.split("/");
    if (substrings.length > 1) {
        fiscode = substrings[substrings.length - 1];
    }
*/

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
                let race_container = document.createElement("a");
                race_container.setAttribute("href", "http://" + window.location.host + "/race?" + race.raceid);
                race_container.classList.add("race-row");
                if (i % 2 == 0) {
                    race_container.classList.add("even");
                } else {
                    race_container.classList.add("odd");
                }


                let checkbox_div = document.createElement("div");
                race_container.appendChild(checkbox_div);
                checkbox_div.classList.add("race-row-element");
                checkbox_div.style.cssText += "flex-basis:5%;max-width:5%;min-height:3rem;";
                let checkbox = document.createElement("input");
                checkbox_div.appendChild(checkbox);
                checkbox.type = "checkbox";
                checkbox.id = "checkbox-" + race.raceid;
                checkbox.style.cssText += "min-width:50%;;min-height:3rem;"

                let date_div = document.createElement("div");
                race_container.appendChild(date_div);
                date_div.innerHTML = race.date;
                date_div.classList.add("race-row-element");
                date_div.style.cssText += "flex-basis:10%;max-width:10%;";

                let location_div = document.createElement("div");
                race_container.appendChild(location_div);
                location_div.innerHTML = race.location; 
                location_div.classList.add("race-row-element");
                location_div.style.cssText += "flex-basis:10%;max-width:10%;";

                let nation_div = document.createElement("div");
                race_container.appendChild(nation_div);
                nation_div.innerHTML = race.nation;
                nation_div.classList.add("race-row-element");
                nation_div.style.cssText += "flex-basis:10%;max-width:10%";

                let type_div = document.createElement("div");
                race_container.appendChild(type_div);
                type_div.innerHTML = race.category;
                type_div.classList.add("race-row-element");
                type_div.style.cssText += "flex-basis:25%;max-width:25%;";

                let rank_div = document.createElement("div");
                race_container.appendChild(rank_div);
                rank_div.innerHTML = race.rank;
                rank_div.classList.add("race-row-element");
                rank_div.style.cssText += "flex-basis:10%;max-width:10%;";

                let time_div = document.createElement("div");
                let time = convert_ms_to_time(race.time);
                race_container.appendChild(time_div);
                time_div.innerHTML = time;
                time_div.classList.add("race-row-element");
                time_div.style.cssText += "flex-basis:10%;max-width:10%;";

                let diff_div = document.createElement("div");
                let diff = convert_ms_to_time(race.diff);
                if (race.diff > 0) { diff = "+" + diff; }
                race_container.appendChild(diff_div);
                diff_div.innerHTML = diff;
                diff_div.classList.add("race-row-element");
                diff_div.style.cssText += "flex-basis:10%;max-width:10%;";

                let diff_percentage_div = document.createElement("div");
                let diff_percentage = ((race["diff percentage"] - 1) * 100).toFixed(2);
                if (race["diff percentage"] > 1) { diff_percentage = "+" + diff_percentage; }
                race_container.appendChild(diff_percentage_div);
                diff_percentage_div.innerHTML = diff_percentage + "%";
                diff_percentage_div.classList.add("race-row-element");
                diff_percentage_div.style.cssText += "flex-basis:10%;max-width:10%;";

                races_div.appendChild(race_container);
            }
        }
        else {
            console.log(body);
        }
    });
    


};






