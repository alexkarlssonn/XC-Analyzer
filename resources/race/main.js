
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
                console.log(json);
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
            if (json) 
            {
                console.log(json);
            }
        }
        else 
        {
            race_info.innerHTML = "Could not find any results for the race with raceid: " + raceid;
        }
    });
};





