#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <list>
#include <chrono>
#include <ctime>
#include "json.hpp"

// https://730ne.cz/ - Lower the priority of
#define NO_730 (true)
#define NO_915 (true)

#define EMPTY ("")

using std::cin;
using std::cout;
using std::endl;
using std::fixed;
using std::setprecision;
using std::invalid_argument;
using std::copy;
using std::tie;
using std::string;
using std::tuple;
using std::vector;
using std::list;
using std::map;
using std::sort;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::tm;
using nlohmann::json;

/// Time of start of the program
static auto START = high_resolution_clock::now();

/// Get last N chars of a string
/// \param input - string to get the substring from
/// \param n - numebr of chars to get
/// \return strign composed of last N chars or an empty string
static string lastN(const string &input, int n) {
    int inputSize = input.size();
    return (n > 0 && inputSize > n) ? input.substr(inputSize - n) : "";
}

/// Form of the class
enum ClassForm {
    Lecture,
    Tutorial,
    Laboratory
};

/// Event object holding data about a single class-parallel-thingy
class Event {
public:
    bool operator==(const Event &e) {
        return this->p_name == e.p_name &&
               this->p_form == e.p_form &&
               this->p_begin == e.p_begin &&
               this->p_room == e.p_room &&
               this->p_capacity == e.p_capacity &&
               this->p_parallel == e.p_parallel;
    }

    bool operator!=(Event e) {
        bool r = e == *this;
        return r ^ 1u;
    }

    /// Parses class form
    /// \param inp - a string
    /// \return ClassForm or throws an exception when invalid
    static ClassForm parseClassForm(const string &inp) {
        if (inp == "lecture")
            return ClassForm::Lecture;
        else if (inp == "tutorial")
            return ClassForm::Tutorial;
        else if (inp == "laboratory")
            return ClassForm::Laboratory;
        throw invalid_argument("Invalid class type");
    }

    /// Name of the class
    string p_name;

    /// Form of the class
    ClassForm p_form;

    /// List of beginning times
    vector<tuple<int, int, int>> p_begin;

    /// Room name
    string p_room;

    /// Capacity of the class
    int p_capacity;

    /// Parallel "number"
    string p_parallel;

    /// Default constructor
    /// \param inp - JSON of an event taken from the Sirius or timetable system
    explicit Event(const json &inp) {
        auto links = inp["links"];

        p_name = links["course"].get<string>();
        p_form = parseClassForm(inp["event_type"]);
        p_room = links.contains("room") ? links["room"] : "";
        p_capacity = inp["capacity"].get<int>();
        p_parallel = inp["parallel"].get<string>();

        tm time = {0};
        strptime(inp["starts_at"].get<string>().data(), "%FT%X", &time);
        p_begin.emplace_back(time.tm_wday - 1, time.tm_hour,
                             time.tm_min); // MON = 1, TUE = 2, ... so let's start from 0
    }

    /// Copy an event
    /// \param old - old event to copy
    Event(const Event &old) {
        p_name = old.p_name;
        p_form = old.p_form;
        p_begin = old.p_begin;
        p_room = old.p_room;
        p_capacity = old.p_capacity;
        p_parallel = old.p_parallel;
    }
};

class Table {
public:
    int score = INT32_MAX;

    vector<vector<string>> table = {
            {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
            {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
            {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
            {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
            {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY}
    };

    Table() = default;

    Table(const Table &old) {
        table = old.table;
    }

    static tuple<int, int> timeToIndex(const tuple<int, int, int> &time) {
        int day, hours, minutes, index;
        tie(day, hours, minutes) = time;
        if (hours == 7 && minutes == 30)
            index = 0;
        else if (hours == 9 && minutes == 15)
            index = 1;
        else if (hours == 11 && minutes == 0)
            index = 2;
        else if (hours == 12 && minutes == 45)
            index = 3;
        else if (hours == 14 && minutes == 30)
            index = 4;
        else if (hours == 16 && minutes == 15)
            index = 5;
        else if (hours == 18 && minutes == 0)
            index = 6;
        else {
            // Hack for non "standard time" classes like BI-CS1 classes to keep things simple.
            if (hours == 13 && minutes == 30)
                index = 4;
            else if (hours == 11 && minutes == 45)
                index = 3;
            else
                throw invalid_argument("Invalid class time");
        }

        return {day, index};
    }

    /// Update the score of a table
    void scoreTable() {
        int currentScore = 0, multiply = 0;

        // Score based on total time spent in school
        for (auto &day : this->table) {
            int i = 0;
            for (auto &hour : day) {
                if (hour.empty())
                    i++;
                else
                    break;
            }

            int j = day.size();
            for (auto p = day.size(); p-- > 0;) {
                if (day[p].empty())
                    j--;
                else
                    break;
            }

            if (j - i > 0) {
                currentScore += (j - i) * 10;
                multiply++;
            }
        }

        int total = currentScore * multiply;

        // Scoring of free days, days with lectures only, ...
        for (auto &day : this->table) {
            // Free days
            int daySum = 0;
            for (auto &hour : day)
                if (!hour.empty())
                    daySum++;

            if (daySum == 0)
                total -= 20;

            // Lectures only
            int count = 0;
            bool lecturesOnly = true;
            for (auto &hour : day) {
                if (hour.empty())
                    continue;

                if (*(hour.end()) == 'l') {
                    count += 1;
                } else {
                    lecturesOnly = false;
                    break;
                }
            }

            if (lecturesOnly) {
                if (count == 1)
                    total -= 10;
                else
                    total -= 5;
            }
        }

        if (NO_730 || NO_915) {
            for (auto &day : this->table) {
                int daySum = 0;
                for (auto &hour : day)
                    if (!hour.empty())
                        daySum++;
                if (!day[0].empty()) {
                    // Lower score for days with 7:30 classes
                    if (NO_730)
                        total += 5;
                } else if (day[1].empty() && daySum > 0) {
                    // Lower score for days with 9:15 classes
                    if (NO_915)
                        total -= 1;
                }
            }
        }

        // Update the score
        score = total;
    }

    /// Pretty-print the table and its score
    void printTable() {
        cout << "- Score: " << score << endl;
        for (auto &day : table) {
            cout << "\t| ";
            for (auto &val : day) {
                if (val.empty())
                    cout << "     | ";
                else
                    cout << val.substr(3, 4) << " | ";
            }
            cout << endl;
        }
    }

    bool operator<(const Table &other) {
        return score < other.score;
    }

    static bool sortInstances(Table *a, Table *b) {
        return *a < *b;
    }
};

/// Parse the JSON into some usable form
/// \param inp - Input JSON
/// \return parsed data, throws on parsing error
map<string, map<ClassForm, list<Event>>> *createJson(const json &inp) {
    auto data = new map<string, map<ClassForm, list<Event>>>;

    // Parse each event and categorize it
    for (auto&[key, array] : inp.items())
        for (auto &value : array["events"])
            (*data)[value["links"]["course"]][Event::parseClassForm(value["event_type"])].emplace_back(value);

    // Combine same parallels into a single events
    for (auto &subject : *data) {
        for (auto &form : subject.second) {
            for (auto &event : form.second) {
                for (auto it = form.second.begin(); it != form.second.end(); ++it) {
                    if (event.p_parallel == (*it).p_parallel && event != *it) {
                        event.p_begin.emplace_back((*it).p_begin[0]);
                        it = form.second.erase(it);
                    }
                }
            }
        }
    }

    return data;
}

/// Flattens events into pure lists
/// \param data - parsed JSON
/// \return flattened list of lists
list<list<Event>> *flattenData(const map<string, map<ClassForm, list<Event>>> &data) {
    auto flattened = new list<list<Event>>;

    for (auto &subject : data) {
        for (auto &classType : subject.second) {
            flattened->emplace_back(classType.second);
        }
    }

    return flattened;
}

/// Recursively create, score and save all possible time tables
/// \param results - input-output vector of all valid results
/// \param arr - list containing list of all class-form-parallel things
/// \param tblOld - previous level table to copy and edit
/// \param flatCombinations - total number of possible combinations
void solve(vector<Table *> &results, const list<list<Event>> &arr, const Table *tblOld, const long flatCombinations) {
    // For each event in the first parallel-class-form-thing array
    auto firstSubject = *(arr.begin());
    for (auto &e : firstSubject) {
        // Create a copy of the previous level table
        auto table = new Table(*tblOld);
        bool addedTime = false;

        // For each class-time in a current parallel
        for (auto &time : e.p_begin) {
            int x, y;
            tie(x, y) = Table::timeToIndex(time);

            // Can this time be added to the time table without a colission?
            if (!table->table[x][y].empty()) {
                // Nope, colission would occur
                addedTime = false;
                break;
            }

            // Yep, add the class to the time table
            addedTime = true;
            table->table[x][y] = e.p_name + (e.p_form == ClassForm::Tutorial
                                             ? "t" : e.p_form == ClassForm::Laboratory
                                                     ? "b" : "l");
        }

        // Was the parallel-class-type-thing was successfully added
        if (addedTime) {
            // Yes, let's move to the another level if possible
            list<list<Event>> arrNew = arr;
            arrNew.pop_front();

            // No more classes to add, time table is complete. Let's save it.
            if (arrNew.empty()) {
                // Score the time table and save it
                table->scoreTable();
                results.push_back(table);

                // Print progress
                if (results.size() % 1000 == 0) {
                    // How long did it take from the start?
                    auto tmr = duration_cast<milliseconds>(high_resolution_clock::now() - START).count();
                    // Percentage progress of all combinations
                    auto prc = (results.size() / (double) flatCombinations) * 100;
                    cout << setprecision(5) << fixed << prc << " - " << (tmr / 1000.0)
                         << " - " << results.size() << endl;
                }
            } else {
                // There are more classes to add, let's try them
                solve(results, arrNew, table, flatCombinations);
                delete table;
            }
        } else {
            delete table;
        }
    }
}

int main() {
    // Read JSON
    json j;
    try {
        cin >> j;
    } catch (error_t e) {
        cout << "Json can not be parsed";
        return 1;
    }

    // Parse and normalize the data
    auto input = createJson(j);
    auto flat = flattenData(*input);
    auto table = new Table;

    // Calculate number of possible combinations
    auto flatCombinations = 1L;
    for (auto &f : *flat)
        flatCombinations *= f.size();

    auto end = duration_cast<milliseconds>(high_resolution_clock::now() - START).count();

    cout << "Data parsed after " << ((double) end / 1000.0) << " seconds" << endl;
    cout << "Maximum combinations: " << flatCombinations << endl;
    cout << "[% of combinations tried] - [time spent in seconds] - [number of total results]" << endl;

    auto results = new vector<Table *>;
    START = high_resolution_clock::now();

    // Build all time tables and sort them descending, starting with the best ones
    solve(*results, *flat, table, flatCombinations);
    sort(results->begin(), results->end(), Table::sortInstances);

    end = duration_cast<milliseconds>(high_resolution_clock::now() - START).count();
    cout << "Found " << results->size() << " possible timetables" << endl;
    cout << "Execution stopped after " << end / 1000.0 << " seconds" << endl;
    cout << "Results:" << endl;

    // Print all best time tables
    if (!results->empty()) {
        int min = (*results->begin())->score;
        for (auto &tbl : *results) {
            if (tbl->score != min)
                break;
            tbl->printTable();
        }
    }

    for (auto res : *results)
        delete res;

    delete input;
    delete flat;
    delete table;
    delete results;
    return 0;
}

