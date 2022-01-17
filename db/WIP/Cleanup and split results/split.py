import json

def extract_raceid(json):
    try:
        return int(json['raceid'])
    except KeyError:
    	print("KeyError: " + json["raceid"])
    	return 0



# ==================================================
# Load all results
# ==================================================
resultsJson = {}
inputFileName = "results.json"
with open(inputFileName, "r") as fromFile:
	resultsJson = json.load(fromFile)
	print(str(len(resultsJson['results'])) + " results was loaded from the file: '" + inputFileName + "'\n")
#results_size = len(resultsJson["results"])


# Sort all races by their raceid
# Raceid range: 22367 - 39155

print("Sorting loaded results by their raceids...")
resultsJson['results'].sort(key=extract_raceid, reverse=False)	
print("Sorting complete!\n")


# ===================================================
# Split the loaded races into:
# races-info.json
#
# And save the result list for each race to:
# races-results.json
# ===================================================
races_info = {}
races_info["races"] = []

races_results_1 = {}
races_results_1["races"] = []
races_results_2 = {}
races_results_2["races"] = []
races_results_3 = {}
races_results_3["races"] = []
races_results_4 = {}
races_results_4["races"] = []
races_results_5 = {}
races_results_5["races"] = []
races_results_6 = {}
races_results_6["races"] = []
races_results_7 = {}
races_results_7["races"] = []

# ------------------------------------------------------------------------------------------------
for race in resultsJson["results"]:

	raceid = race["raceid"]          # Ok
	codex = race["codex"]            # Ok
	date = race["date"]              # Ok
	nation = race["nation"]          # Sometimes empty, sometimes part of location
	location = race["location"]      # Sometimes empty, sometimes weird
	category = race["category"]      # Ok
	discipline = race["discipline"]  # Ok
	hasDetails = race["hasDetails"]  # Ok
	results = race["results"]        # Ok


	racetype = ""  # SQ, DM, DP, DI
	gender = ""
	# ------------------------------------------------------------------------
	for substr in discipline.split():
		s = substr.lower()
		# Set race type
		if s == "qual": 
			racetype = "SQ"
		if racetype == "" and (s == "mst" or s == "skiathlon" or s == "marathon"):
			racetype = "DM"
		if racetype == "" and (s == "pursuit"):
			racetype = "DP"
		
		# Set gender
		if s == "men" or s == "men's" or s == "mens":
			gender = "M"
		if s == "women" or s == "women's" or s == "womens" or s == "woman":
			gender = "F"
	# ------------------------------------------------------------------------
	if racetype == "":
		for substr in discipline.split():
			s = substr.lower()
			if s == "sp" or s == "sprint":
				racetype = "SO"
	# ------------------------------------------------------------------------
	if racetype == "":
		for substr in discipline.split():
			s = substr.lower()
			if s == "km":
				racetype = "DI"
	# ------------------------------------------------------------------------


	# RACES INFO
	# Add two new field to each race
	# "type": ["SQ" / "SO" / "DI" / "DM" / "DP" / ""]
	# "gender": ["M" / "F" / ""]
	race_modified = race
	race_modified.pop("results", None)
	race_modified["type"] = racetype
	race_modified["gender"] = gender
	races_info["races"].append(race_modified)


	# RACES RESULTS
	# Create a new object with just the result list that will be saved in its own file
	race_resultlist = {}
	race_resultlist["raceid"] = raceid
	race_resultlist["result"] = results

	if int(raceid) < 23000:
		races_results_1["races"].append(race_resultlist)
	elif int(raceid) >= 23000 and int(raceid) < 26000:
		races_results_2["races"].append(race_resultlist)
	elif int(raceid) >= 26000 and int(raceid) < 29000:
		races_results_3["races"].append(race_resultlist)
	elif int(raceid) >= 29000 and int(raceid) < 32000:
		races_results_4["races"].append(race_resultlist)
	elif int(raceid) >= 32000 and int(raceid) < 35000:
		races_results_5["races"].append(race_resultlist)
	elif int(raceid) >= 35000 and int(raceid) < 38000:
		races_results_6["races"].append(race_resultlist)
	elif int(raceid) >= 38000 and int(raceid) < 41000:
		races_results_7["races"].append(race_resultlist)
	elif int(raceid) >= 41000:
		print("Raceid over or equal to 41000: " + raceid)
	
	
# ------------------------------------------------------------------------------------------------

print("Saving json objects to files...")

file = "races-info.json"
with open(file, "w") as toFile:
	json.dump(races_info, toFile, indent=4)
	print(str(len(races_info["races"])) + " races saved to: " + file)


file = "races-results-0-22999.json"
with open(file, "w") as toFile:
	json.dump(races_results_1, toFile, indent=4)
	print(str(len(races_results_1["races"])) + " races saved to: " + file)

file = "races-results-23000-25999.json"
with open(file, "w") as toFile:
	json.dump(races_results_2, toFile, indent=4)
	print(str(len(races_results_2["races"])) + " races saved to: " + file)

file = "races-results-26000-28999.json"
with open(file, "w") as toFile:
	json.dump(races_results_3, toFile, indent=4)
	print(str(len(races_results_3["races"])) + " races saved to: " + file)

file = "races-results-29000-31999.json"
with open(file, "w") as toFile:
	json.dump(races_results_4, toFile, indent=4)
	print(str(len(races_results_4["races"])) + " races saved to: " + file)

file = "races-results-32000-34999.json"
with open(file, "w") as toFile:
	json.dump(races_results_5, toFile, indent=4)
	print(str(len(races_results_5["races"])) + " races saved to: " + file)

file = "races-results-35000-37999.json"
with open(file, "w") as toFile:
	json.dump(races_results_6, toFile, indent=4)
	print(str(len(races_results_6["races"])) + " races saved to: " + file)

file = "races-results-38000-40999.json"
with open(file, "w") as toFile:
	json.dump(races_results_7, toFile, indent=4)
	print(str(len(races_results_7["races"])) + " races saved to: " + file)







