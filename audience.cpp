/*
* Martyna Staniec-Wosko
*/

#include <ctime>
#include <exception>
#include <fstream>
#include <iostream>
#include <istream>
#include <map>
#include <set>
#include <string>

using namespace std;


class DateTime {
public:
    DateTime() {}
    DateTime(const tm& time): dateTime(time) {}

    //YYYYMMDDHHMMSS
    DateTime(const string& str) {
        if (NULL == strptime(str.c_str(), "%Y%m%d%H%M%S", &dateTime)) {
            cout << "ERROR date " << str <<endl;
        }
    }

    string toString () const {
        short kSize = 256;
        char buffer[kSize] = {0};
        size_t res = std::strftime(buffer, kSize, "%Y%m%d%H%M%S", &dateTime);
        return string(buffer);
    }

    string operator-(const DateTime& oth) {
        tm otherDateTime(oth.dateTime);
        int sec = (int)difftime(mktime(&dateTime), mktime(&otherDateTime));
        return to_string(++sec);
    }

    tm getMidnightTimeFromDate() {
        tm dt(dateTime);
        dt.tm_hour = 23;
        dt.tm_min = 59;
        dt.tm_sec = 59;
        return dt;
    }

    tm getSecEarlierFromDate() {
        tm dt(dateTime);
        if (dt.tm_sec == 0) {
            dt.tm_sec = 59;
            if (dt.tm_min == 0) {
                dt.tm_min = 59;
                if (dt.tm_hour == 0) {
                    dt.tm_hour = 23;
                } else {
                    dt.tm_hour--;
                }
            } else {
                dt.tm_min--;
            }
        } else {
            dt.tm_sec--;
        }
        return dt;
    }

    static bool isSameDay(const string& day1, const string& day2) {
        for (int i=0; i<8; ++i) {
            if (day1[i] != day2[i]) {
                return false;
            }
        }
        return true;
    }

    static bool isValidDateTime(const string& str) {
        tm dt;
        if (NULL == strptime(str.c_str(), "%Y%m%d%H%M%S", &dt)) {
            return false;
        }
        // todo: better validation needed ex. for Feb 29 2021
        if (dt.tm_hour >= 24 || dt.tm_min >= 60 || dt.tm_sec >= 60) {
            return false;
        }
        if (mktime(&dt) == -1) {
            return false;
        }
        return true;
    }

private:
    tm dateTime;
};


class ActivityData {
public:
    struct ActivityDataMissing: public exception {
        const char* what() const throw () {
            return "ActivityData channel or start time missing!";
        }
    };

    struct ActivityDataInvalid: public exception {
        const char* what() const throw () {
            return "ActivityData start time incorrect value!";
        }
    };

    ActivityData(const string& channel, const string& startTime, const string& activity):
        channel(channel), startTime(startTime), activity(activity) {
        if (channel.empty() || startTime.empty()) {
            throw ActivityDataMissing();
        }
        if (!DateTime::isValidDateTime(startTime)) {
            throw ActivityDataInvalid();
        }
    }

    bool operator<(const ActivityData& other) const {
        return startTime < other.startTime;
    }

    friend ostream& operator<<(ostream& outs, const ActivityData& d) {
        outs << d.channel << "|" << d.startTime << "|" << d.activity << "|" << d.endTime << "|" << d.duration;
        return outs;
    }

    void calcDurationUntil(const ActivityData& next) {
        DateTime start(startTime);
        DateTime end;
        if (DateTime::isSameDay(startTime, next.getTime())) {
            end = DateTime(next.getTime()).getSecEarlierFromDate();
        } else {
            end = DateTime(start.getMidnightTimeFromDate());
        }
        endTime = end.toString();
        duration = end - start;
    }

    void calcDurationEndDay() {
        DateTime start(startTime);
        DateTime end(start.getMidnightTimeFromDate());
        endTime = end.toString();
        duration = end - start;
    }

    const string& getTime() const{
        return startTime;
    }

private:
    string channel;
    string startTime;
    string activity;
    string endTime;
    string duration;
};


class AudienceDataCollection {
public:
    void addData(const string& homeNo, const string& channel, const string& startTime, const string& activity) {
        if (homeNo.empty()) {
            cout << "Data read error: Missing home number!" << endl;
            return;
        }
        try {
            ActivityData newData (channel, startTime, activity);

            AudienceData::iterator it = data.find(homeNo);
            if (it != data.end()) { // add to existing
                it->second.insert(newData);
            } else { // create new
                HomeActivity homeSet;
                homeSet.insert(newData);
                data[homeNo] = homeSet;
            }
        } catch (ActivityData::ActivityDataMissing err) {
            cout << "Data read error: " << err.what() << endl;
            return;
        } catch (ActivityData::ActivityDataInvalid err) {
            cout << "Data read error: " << err.what() << " " << startTime << endl;
            return;
        }
    }

    void processData() {
        for (AudienceData::iterator it = data.begin(); it != data.end(); ++it) {
            HomeActivity::iterator prevHit = it->second.begin();
            HomeActivity::iterator nextHit = it->second.begin();
            for (++nextHit; nextHit != it->second.end(); ++nextHit) {
                ActivityData& ad = const_cast<ActivityData&>(*prevHit);
                ad.calcDurationUntil(*nextHit);
                prevHit = nextHit;
            }
            if (prevHit != it->second.end()) {
                ActivityData& ad = const_cast<ActivityData&>(*prevHit);
                ad.calcDurationEndDay();
            }
        }
    }

    friend ostream& operator<<(ostream& outs, const AudienceDataCollection& adc) {
        for (AudienceData::const_iterator it = adc.data.begin(); it != adc.data.end(); ++it) {
            for (HomeActivity::const_iterator hit = it->second.begin(); hit != it->second.end(); ++hit) {
                outs << it->first << "|" << *hit << endl;
            }
        }
        return outs;
    }

private:
    typedef set<ActivityData> HomeActivity;
    typedef map<string, HomeActivity> AudienceData;

    AudienceData data;
};


class AudienceDataProcessor {
public:
    AudienceDataProcessor() {}
    ~AudienceDataProcessor() {}

    bool readInput(const string& inFilename) {
        fstream inFile (inFilename);
        if (!inFile.is_open()) {
            return false;
        }
        string homeNo, channel, startTime, activity;
        bool isHeaderRead (false);
        while (!inFile.eof()) {
            getline(inFile, homeNo, '|');
            getline(inFile, channel, '|');
            getline(inFile, startTime, '|');
            getline(inFile, activity, '\n');
            if (!isHeaderRead) {
                // skip storing of header line
                isHeaderRead = true;
                continue;
            }
            dataCollection.addData(homeNo, channel, startTime, activity);
        }
        inFile.close();
        return true;
    }

    bool writeOutput(const string& outFilename) {
        ofstream outFile (outFilename);
        if (!outFile.is_open()) {
            return false;
        }
        outFile << "HomeNo|Channel|Starttime|Activity|EndTime|Duration" << endl;
        outFile << dataCollection;
        outFile.close();
        return true;
    }

    void processData() {
        dataCollection.processData();
    }

private:
    AudienceDataCollection dataCollection;
};


int main (int argc, char** argv) {
    if (argc < 3) {
        cout << "Missing arguments: <input-statements-file> <output-sessions-file>" << endl;
        return 1;
    }

    AudienceDataProcessor processor;
    if (!processor.readInput(argv[1])) {
        cout << "Cannot open <input-statements-file>: " << argv[1] << endl;
        return 1;
    }
    processor.processData();
    if (!processor.writeOutput(argv[2])) {
        cout << "Failed to create <output-sessions-file>:: " << argv[2] << endl;
        return 1;
    }

    return 0;
}
