import json

athletes_raceids = {}
inputFileName = "input.json"
with open(inputFileName, "r") as fromFile:
	athletes_raceids = json.load(fromFile)
	print(str(len(athletes_raceids['athletes'])) + " athletes and their raceids was loaded from the file: '" + inputFileName + "'\n")
SIZE = str(len(athletes_raceids['athletes']))


all_races = {}
inputFileName = "races-info.json"
with open(inputFileName, "r") as fromFile:
	all_races = json.load(fromFile)
	print(str(len(all_races['races'])) + " races was loaded from the file: '" + inputFileName + "'\n")

all_raceids = []
for race in all_races["races"]:
	raceid = race["raceid"]
	all_raceids.append(raceid)
print(str(len(all_raceids)) + " raceids was extracted from all " + str(len(all_races["races"])) + " races in " + inputFileName)


total_athletes = 0
totalRaceids_before = 0
deletedRaceids = 0
validRaceids = 0

new_athletes_raceids = {}
new_athletes_raceids["athletes"] = []

# ==========================================================================================
for athlete in athletes_raceids["athletes"]:
	total_athletes += 1
	fiscode = athlete["fiscode"]
	results = athlete["results"]

	new_athlete = {}
	new_athlete["fiscode"] = fiscode
	new_athlete["races"] = []

	# Loop through all raceids for the current athlete
	# ------------------------------------------------------------
	for current_raceid in results:
		totalRaceids_before += 1

		isValidRaceid = False
		# -------------------------------
		for raceid in all_raceids:
			if current_raceid == raceid:
				isValidRaceid = True
				break
		# -------------------------------
		if (isValidRaceid == False):
			deletedRaceids += 1
		else:
			new_athlete["races"].append(current_raceid)
			validRaceids += 1
	# ------------------------------------------------------------

	# Add the athletes and his list of raceids to the new object
	new_athletes_raceids["athletes"].append(new_athlete)

	print(str(total_athletes) + "/" + SIZE)

# ==========================================================================================


print("\nTotal athletes checked: " + str(total_athletes))
print("Deleted " + str(deletedRaceids) + " out of " + str(totalRaceids_before))
print(str(validRaceids) + " valid raceids remaining ( total + deleted = " + str(totalRaceids_before - deletedRaceids) + ")")


file = "OUTPUT.json"
with open(file, "w") as toFile:
	json.dump(new_athletes_raceids, toFile, indent=4)
	print(str(len(new_athletes_raceids["athletes"])) + " athletes saved to: " + file)




