# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_datetime.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_datetime.cppm"

export module datetime:datetime;

import std;
import :date;
import :duration;
import :types;

export namespace datetime {

   class DateTime {
    public:




        DateTime();





        DateTime(const string& dateTime);




        DateTime(const DateTime& other);


        DateTime operator+(DateTime otherDateTime) const;
        DateTime operator+(Date date) const;
        DateTime operator+(Duration duration) const;
        DateTime operator+(DurationNanoSeconds duration) const;
        DateTime operator+(DurationSeconds duration) const;
        DateTime operator+(DurationMinutes duration) const;
        DateTime operator+(DurationHours duration) const;


        DateTime operator-(Duration duration) const;





        string toString() const;


        bool operator<(const DateTime& other) const;
        bool operator>(const DateTime& other) const;
        bool operator<=(const DateTime& other) const;
        bool operator>=(const DateTime& other) const;
        bool operator==(const DateTime& other) const;
        bool operator!=(const DateTime& other) const;


        long long toMillisecondsSinceEpoch() const;





        static DateTime now();





        static string fromEpochTime(long long epochTime, bool isNanoseconds = false);
        static string fromEpochTime(std::uint64_t epochTime, bool isNanoseconds = false);

    private:
        int year;
        int month;
        int day;
        long long nanoseconds;
    };
};
