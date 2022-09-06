


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
 * --------------------------------------------------------------------
 * Searches for an athlete in the database for the given parameters
 *
 * fiscode: The fiscode to search for
 * firstname: The firstname to search for
 * lastname: The lastname to search for
 * callback: The function to call once the response to the request has been received. Should be given two parameters: "status_code" and "body"
 * --------------------------------------------------------------------
 */
function search_athlete(fiscode, firstname, lastname, callback) 
{
    url = "";
    search_by_name = false;
    if (fiscode) {
        url = window.location.href + "api/athlete/fiscode/" + fiscode;
        http_async("GET", url, function(body, status_code) 
        {
            // If only fiscode was given, present the results of the search
            search_again = false;
            if (status_code == 200 || (!firstname && !lastname)) {
                callback(body, status_code);
            }
            // Search again if first- and/or lastname was given and nothing was found for the fiscode 
            else if (firstname && !lastname) {
                search_again = true;
                url = window.location.href + "api/athletes/firstname/" + firstname;
            } else if (!firstname && lastname) {
                search_again = true;
                url = window.location.href + "api/athletes/lastname/" + lastname;
            } else {
                search_again = true;
                url = window.location.href + "api/athletes/fullname/" + firstname + "/" + lastname;
            }
            if (search_again) {
                http_async("GET", url, function(body, status_code) {
                    callback(body, status_code);
                });
            }
        });
    }
    else if (firstname && !lastname) {
        search_by_name = true;
        url = window.location.href + "api/athletes/firstname/" + firstname;
    } else if (!firstname && lastname) {
        search_by_name = true;
        url = window.location.href + "api/athletes/lastname/" + lastname;
    } else if (firstname && lastname) {
        search_by_name = true;
        url = window.location.href + "api/athletes/fullname/" + firstname + "/" + lastname;
    }
    if (search_by_name) {
        http_async("GET", url, function(body, status_code) {
            callback(body, status_code);
        });
    }
}


function appendAthlete(athlete, isEven)
{
    if (!athlete) return;
    let results = document.getElementById("search-results");
    if (!results) return;
    
    let rowColor = "";
    if (isEven) 
        rowColor = "color:#cccccc;background-color:#2a2a2a;";
    else 
        rowColor = "color:#cccccc;background-color:#222222;";

    let row = document.createElement("a");
    row.style.cssText += ("border-bottom: 0.1rem solid #121212; display:flex; align-items:center; min-height:3rem;" + rowColor);
    row.setAttribute("href", window.location.href + "athlete?" + athlete.fiscode);
    results.append(row);

    // Fiscode
    let div_fiscode = document.createElement("div");
    div_fiscode.innerHTML = athlete.fiscode;
    div_fiscode.style.cssText += "text-align:center; flex-basis: 16.67%; max-width: 16.67%";
    row.append(div_fiscode);

    // Name
    let div_name = document.createElement("div");
    div_name.innerHTML = athlete.firstname + " " + athlete.lastname;
    div_name.style.cssText += "text-align:center; flex-basis: 16.67%; max-width: 16.67%";
    row.append(div_name);

    // Birthdate
    let div_birthdate = document.createElement("div");
    div_birthdate.innerHTML = athlete.birthdate;
    div_birthdate.style.cssText += "text-align:center; flex-basis: 16.67%; max-width: 16.67%";
    row.append(div_birthdate);

    // Nation
    let div_nation = document.createElement("div");
    div_nation.innerHTML = athlete.nation;
    div_nation.style.cssText += "text-align:center; flex-basis: 16.67%; max-width: 16.67%";
    row.append(div_nation);

    // Club
    let div_club = document.createElement("div");
    div_club.innerHTML = athlete.club;
    div_club.style.cssText += "text-align:center; flex-basis: 16.67%; max-width: 16.67%";
    row.append(div_club);
    
    // Gender
    let div_gender = document.createElement("div");
    let gender = "";
    if (athlete.gender == "M") 
        gender = "Male";
    else if (athlete.gender = "F") 
        gender = "Female";
    div_gender.innerHTML = gender;
    div_gender.style.cssText += "text-align:center; flex-basis: 16.67%; max-width: 16.67%";
    row.append(div_gender);
}



/**
 * ----------------------------------------------------------------------
 * Main function. Gets called when the page has been loaded
 *
 * ----------------------------------------------------------------------
 */
window.onload = function() 
{

    let input_fiscode = document.getElementById("input-fiscode");
    let input_firstname = document.getElementById("input-firstname");
    let input_lastname = document.getElementById("input-lastname");
    let input_submit = document.getElementById("input-submit");
    
    let results_athletes_header = document.getElementById("results-athletes-header");
    let search_results = document.getElementById("search-results");
    
    input_submit.onclick = function() 
    {
        results_athletes_header.style.display = "none";
        search_results.textContext = "";
        while(search_results.firstChild) {
            search_results.removeChild(search_results.firstChild);
        }

        search_athlete(input_fiscode.value, input_firstname.value, input_lastname.value, function(body, status_code) {
            console.log(status_code + ", " + body);
            if (status_code == 200) {
                let json = JSON.parse(body);
                if (json.athletes) {
                    athletes = json.athletes;
                    results_athletes_header.style.display = "flex";
                    for (let i = 0; i < athletes.length; i++) { 
                        appendAthlete(athletes[i], (i % 2 == 0));
                    }
                } 
                else if (json) {
                    results_athletes_header.style.display = "flex";
                    appendAthlete(json, true);
                }
                else {
                    results_athletes_header.style.display = "none";
                    search_results.innerHTML = "No athlete was found";
                }
            } else {
                results_athletes_header.style.display = "none";
                search_results.innerHTML = "No athlete was found";
            }
        });
    };

    input_fiscode.addEventListener("keypress", function(event) {
        if (event.key == "Enter") {
            event.preventDefault();
            input_submit.click();
        }
    });
    input_firstname.addEventListener("keypress", function(event) {
        if (event.key == "Enter") {
            event.preventDefault();
            input_submit.click();
        }
    });
    input_lastname.addEventListener("keypress", function(event) {
        if (event.key == "Enter") {
            event.preventDefault();
            input_submit.click();
        }
    });

};





