import json


# ==================================================
# Load all results
# ==================================================
resultsJson = {}
inputFileName = "results.json"
with open(inputFileName, "r") as fromFile:
	resultsJson = json.load(fromFile)
	print(str(len(resultsJson['results'])) + " results was loaded from the file: '" + inputFileName + "'\n")

results_size = len(resultsJson["results"])

# ===================================================
# Split the loaded races into:
# results-info-SQ-M.json
# results-info-SQ-F.json
# results-info-D-M.json
# results-info-D-F.json
# results-info-other.json
#
# And save the result list for each race to:
# races-results.json
# ===================================================
results_SQ_M = {}
results_SQ_F = {}
results_D_M = {}
results_D_F = {}
results_SO = {}
results_other = {}
results_SQ_M["races"] = []
results_SQ_F["races"] = []
results_D_M["races"] = []
results_D_F["races"] = []
results_SO["races"] = []
results_other["races"] = []

races_results = {}
races_results["races"] = []

# ------------------------------------------------------------------------------------------------
DM = 0
DP = 0
DI = 0

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
			DM += 1
		if racetype == "" and (s == "pursuit"):
			racetype = "DP"
			DP += 1
		
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
				DI += 1
	# ------------------------------------------------------------------------


	# Add two new field to each race
	# "type": ["SQ" / "SO" / "DI" / "DM" / "DP" / ""]
	# "gender": ["M" / "F" / ""]
	race_modified = race
	race_modified.pop("results", None)
	race_modified["type"] = racetype
	race_modified["gender"] = gender


	# Create a new object with just the result list that will be saved in its own file
	race_resultlist = {}
	race_resultlist["raceid"] = raceid
	race_resultlist["result"] = results
	races_results["races"].append(race_resultlist)


	# OTHER
	if racetype == "":
		results_other["races"].append(race_modified)
		continue

	# SPRINT QUAL M
	if racetype == "SQ" and gender == "M":
		results_SQ_M["races"].append(race_modified)
		continue

	# SPRINT QUAL F
	if racetype == "SQ" and gender == "F":
		results_SQ_F["races"].append(race_modified)
		continue

	# SPRINT OTHER
	if racetype == "SO":
		results_SO["races"].append(race_modified)
		continue

	# DISTANCE M
	if racetype[0] == "D" and gender == "M":
		results_D_M["races"].append(race_modified)
		continue

	# DISTANCE F
	if racetype[0] == "D" and gender == "F":
		results_D_F["races"].append(race_modified)
		continue

	# Other mixed gender
	if racetype[0] == "D" and gender == "":
		results_other["races"].append(race_modified)
		continue
	
# ------------------------------------------------------------------------------------------------

# PRINT RESULTS
print("Sprint Qual M: " + str(len(results_SQ_M["races"])))
print("Sprint Qual F: " + str(len(results_SQ_F["races"])))
print("Sprint Other:  " + str(len(results_SO["races"])))
print("Distance M:    " + str(len(results_D_M["races"])))
print("Distance F:    " + str(len(results_D_F["races"])))
print("Other:         " + str(len(results_other["races"])))
print("Total:         " + str(len(results_SQ_M["races"]) + len(results_SQ_F["races"]) + len(results_SO["races"]) + len(results_D_M["races"]) + len(results_D_F["races"]) + len(results_other["races"])))

print("\nDistance races:\nDM:  " + str(DM) + "\nDP:  " + str(DP) + "\nDI:  " + str(DI) + "\nTot: " + str(DM + DP + DI) + " (Distance M + Distance F: " + str(len(results_D_M["races"]) + len(results_D_F["races"])) + ")")

print("\nraces-result: " + str(len(races_results["races"])))


# Save to file: SPRINT QUAL M
# outputFileName = "races-info-SQ-M.json"
# with open(outputFileName, "w") as toFile:
# 	json.dump(results_SQ_M, toFile, indent=4)
# 	print(str(len(results_SQ_M["races"])) + " races was saved to: " + outputFileName + " successfully!")

# Save to file: SPRINT QUAL F

# Save to file: DISTANCE M

# Save to file: DISTANCE F

# Save to file: OTHER + SPRINT OTHER

# Save to file: RESULT LIST FOR EACH RACE



