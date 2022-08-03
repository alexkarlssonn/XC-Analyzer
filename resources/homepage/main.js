


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
    let search_results = document.getElementById("search-results");
    
    input_submit.onclick = function() 
    {
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
                    for (let i = 0; i < athletes.length; i++) {
                        let div = document.createElement("div");
                        search_results.appendChild(div);
                        div.innerHTML = athletes[i].fiscode + ", " + athletes[i].firstname + " " + athletes[i].lastname;
                        let a = document.createElement("a");
                        a.setAttribute("href", window.location.href + "athlete/" + athletes[i].fiscode);
                        a.innerHTML = "Link";
                        div.appendChild(a);
                    }
                } else {
                    let div = document.createElement("div");
                    search_results.appendChild(div);
                    div.innerHTML = json.fiscode + ", " + json.firstname + " " + json.lastname;
                    let a = document.createElement("a");
                    a.setAttribute("href", window.location.href + "athlete/" + json.fiscode);
                    a.innerHTML = "Link";
                    div.appendChild(a);
                }
            } else {
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





