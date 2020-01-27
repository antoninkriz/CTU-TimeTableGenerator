# CTU-TimeTableGenerator
Generates the best possible time table

## What can it do?
The program generates all possible time tables for given classes and ranks them based from the best (lowest amount of time spent in school) to the worst following these rules:

- No. of "standard classes hours" from the first one to the last one in a day, times 10
- Free day = -20 points
- Day only with one lecture = -10 points
- Day with lectures only = -5 points
- NO_730 = +5 points when the first class starts at 7:30
- NO_915 = +1 point when the first class starts at 9:15

Parameters `NO_730` and `NO_915` can be enabled/disabled in the code. Please support the [730ne.cz](https://730ne.cz/) petition.

## What is NOT supported

- time table is generated only for 7 days, not taking in count another week
- only standard class times are supported (exception for BI-CS1, see `time_to_index` or `timeToIndex` functions)
- and probably much, much more


## How to use
Spoof HTTP requests to `https://timetable.fit.cvut.cz/new/api/sirius/courses/CLASS_NAME/events?...` from [timetable.fit.cvut.cz](https://timetable.fit.cvut.cz/new/) Using Chrome Dev tools and copy paste them to the `rozvrh.json` file following this structure:

```json
{
    "FI-KSA": {"events":[...]},
    "BI-LIN": <copy pasted response from timetables>,
    ...
}
```

### Python
Place file `rozvrh.json` into same folder where's the script and run it. It's slow for bigger input. Very slow - around 90 minutes minutes with complete data. Be aware.

### C++
Compile with `g++ -O3 main.cpp` and pipe the input file to stdin of the program using `cat rozvrh.json | ./a,out`. This one is actually fast, using the same algorithm.

## I am very generous and I want to help
Don't be afraid to create an issue or a pull request!
