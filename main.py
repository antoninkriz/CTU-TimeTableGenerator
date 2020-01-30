from io import open
from time import process_time
from copy import deepcopy
from pprint import PrettyPrinter
from json import load
from random import shuffle
from dateutil import parser

# SETTINGS
FILE = "rozvrh.json"
NO_730 = True
NO_915 = True
RANDOMIZE = False

# INIT and CONSTS
START = 0
EMPTY = "    "
PP = PrettyPrinter(indent=2)


# Normalizes data from the json
def create_json(inp):
    struct = {}

    # Builds an object with the event information
    def fmt(ev, class_form):
        time_old = parser.parse(ev["starts_at"])
        time_new = (time_old.weekday(), time_old.hour, time_old.minute)
        return {
            "cls": ev["links"]["course"],  # Class name
            "tpe": class_form,  # Type of the cass
            "beg": [time_new],  # Day and start time
            "rom": ev["links"]["room"],  # Classroom
            "cap": ev["capacity"],  # Capacity
            "par": ev["parallel"],  # Parallel number
        }

    # Divide lectures tutorials and laboratories
    for key in inp:
        struct.setdefault(key, {
            "lecture": [fmt(ev, "lecture") for ev in
                        filter(lambda it: it["event_type"] == "lecture", inp[key]["events"])],
            "tutorial": [fmt(ev, "tutorial") for ev in
                         filter(lambda it: it["event_type"] == "tutorial", inp[key]["events"])],
            "laboratory": [fmt(ev, "laboratory") for ev in
                           filter(lambda it: it["event_type"] == "laboratory", inp[key]["events"])],
        })

    # Combine classes for same parallels into one
    for sub in struct:
        for clType in struct[sub]:
            for ev in struct[sub][clType]:
                times = ev["beg"]

                for i in struct[sub][clType]:
                    if ev["par"] == i["par"] and ev["beg"][0] != i["beg"][0]:
                        times.append(i["beg"][0])
                        struct[sub][clType].remove(i)

                ev["beg"] = times

    return struct


# Flatten normalized JSON
def flatten_data(struct):
    flattened = []
    flattened_combinations = 1
    for sbj in struct:
        for class_type in struct[sbj]:
            if len(struct[sbj][class_type]) > 0:
                flattened.append(struct[sbj][class_type])
                flattened_combinations *= len(flattened[-1])
    return flattened, flattened_combinations


# Creates a new instance of the time table
def create_table():
    return {
        0: [  # MON
            EMPTY,  # 7:30
            EMPTY,  # 9:15
            EMPTY,  # 11:00
            EMPTY,  # 12:45
            EMPTY,  # 14:30
            EMPTY,  # 16:15
            EMPTY  # 18:00
        ],
        1: [  # TUE
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY
        ],
        2: [  # WED
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY
        ],
        3: [  # THU
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY
        ],
        4: [  # FRI
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY,
            EMPTY
        ]
    }


# Converts time from format (weekday, hour, minutes) to an index in the time table
def time_to_index(trp):
    if trp[1] == 7 and trp[2] == 30:
        i = 0
    elif trp[1] == 9 and trp[2] == 15:
        i = 1
    elif trp[1] == 11 and trp[2] == 0:
        i = 2
    elif trp[1] == 12 and trp[2] == 45:
        i = 3
    elif trp[1] == 14 and trp[2] == 30:
        i = 4
    elif trp[1] == 16 and trp[2] == 15:
        i = 5
    elif trp[1] == 18 and trp[2] == 0:
        i = 6
    else:
        # Hack for BI-CS1 classes to make things simple. No need to attend these anyway.
        if trp[1] == 13 and trp[2] == 30:
            return time_to_index((trp[0], 14, 30))
        elif trp[1] == 11 and trp[2] == 45:
            return time_to_index((trp[0], 12, 45))

        raise ValueError("Invalid class time!", trp)
    return trp[0], i


# Calc score for a given table, lower is better
def score_table(tbl):
    scr = 0
    mtp = 0

    # Calc total time spent in school
    for day in tbl:
        i = 0
        for hour in tbl[day]:
            if hour == EMPTY:
                i += 1
            else:
                break
        j = len(tbl[day])
        for hour in reversed(tbl[day]):
            if hour == EMPTY:
                j -= 1
            else:
                break

        if j - i > 0:
            scr += (j - i) * 10
            mtp += 1

    total = scr * mtp

    # Scoring of free days, days with lectures only and so on
    for day in tbl:
        # Free days
        if sum(c == EMPTY for c in tbl[day]) == len(tbl[day]):
            total -= 20

        # Days with only lectures
        count = 0
        lectures_only = True
        for hour in tbl[day]:
            if hour == EMPTY:
                continue

            if hour[-1] == "l":
                count += 1
            else:
                lectures_only = False
                break

        if lectures_only:
            if count == 1:
                total -= 10
            else:
                total -= 5

    # NO730 or NO915
    if NO_730 or NO_915:
        for day in tbl:
            if tbl[day][0] != EMPTY:
                total += 5
            elif tbl[day][1] == EMPTY and sum(c == EMPTY for c in tbl[day]) > 0:
                total -= 1

    return total


# Recursively try all possible combinations
def try_new(res, arr, tbl_old, flat_combinations):
    # For each event in the first parallel-class-type-thing array
    for ev in arr[0]:
        # Create a new time table to try this stuff on
        tbl = deepcopy(tbl_old)

        added_time = False
        tested_times = []

        # For each class in such parallel
        for tme in ev["beg"]:
            tb_id = time_to_index(tme)

            # Can this class be added to the time table?
            if tbl[tb_id[0]][tb_id[1]] != EMPTY:
                added_time = False
                break

            # Yep, add the class to the time table
            added_time = True
            tested_times.append(tme)
            tbl[tb_id[0]][tb_id[1]] = ev["cls"][-3:] + ("t" if ev["tpe"] == "tutorial" else
                                                        "l" if ev["tpe"] == "lecture" else
                                                        "b")

        # Was the parallel-class-type-thing added to the time table?
        if added_time:
            # Current parallel-class-type-thing was added successfully, lets move to the next one
            arr_new = deepcopy(arr)
            arr_new = arr_new[1:]

            # Check if there are more parallel-class-type-things left
            if len(arr_new) == 0:
                # Score the table and append to the results
                score = score_table(tbl)
                res.append((score, tbl))

                # Print info about the progress
                if len(res) % 1000 == 0:
                    # How long did it take from the start?
                    tmr = process_time() - START
                    # Percentage progress, in reality only around 1 % is checked
                    proc = (len(res) / flat_combinations) * 100
                    print("{0:.5f} - {1:.3f} - {2}".format(proc, tmr, len(res)))
            else:
                # There are, let's try them
                try_new(res, arr_new, tbl, flat_combinations)


# Print results
def print_res(res, tmr):
    res.sort(key=lambda tup: tup[0])
    all_low = list(filter(lambda x: x[0] == res[0][0], res))
    print("Found", len(res), "possible timetables")
    print("Execution stopped after", tmr, "seconds")
    print("Results:")
    print("-Best:")
    print(res[1])
    print("-Worst:")
    print(res[-1])
    print("-All best (", len(all_low), "):")
    PP.pprint(all_low)


# Main
def main(start):
    # Load JSON
    file = open(FILE, "r")
    data = load(file)

    # Normalize JSON
    work = create_json(data)
    table = create_table()

    # Flatten data
    flat, flat_combinations = flatten_data(work)

    # Sort or randomize input order
    for f in flat:
        if RANDOMIZE:
            shuffle(f)
        else:
            f.sort(key=lambda x: x["beg"][0])
    if RANDOMIZE:
        shuffle(flat)
    else:
        flat.sort(key=lambda x: x[0]["cls"])

    print("Data parsed after", start, "seconds")
    print("Maximum combinations:", flat_combinations)
    print("[% of combinations tried] - [time spent in seconds] - [number of total results]")

    res = []
    start = process_time()
    try:
        try_new(res, flat, table, flat_combinations)
    except KeyboardInterrupt:
        end = process_time()
        print_res(res, end - start)
    else:
        end = process_time()
        print_res(res, end - start)


main(START)

