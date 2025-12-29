# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_date.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_date.cppm"

export module datetime:date;

import std;
import :duration;
import :types;

export namespace datetime {
    class DateTime;

    class Date {
    public:




        Date(const string& date);




        Date(TimePointNanoSeconds timePointNanoSeconds);




        Date(TimePointSeconds timePointSeconds);




        Date(TimePointMinutes timePointMinutes);




        Date(const Date& other);





        static Date today();





        Date operator+(Date otherDate) const;




        DateTime operator+(Duration duration) const;




        DateTime operator-(Duration duration) const;




        string toString() const;


        bool operator==(const Date& other) const;
        bool operator!=(const Date& other) const;
        bool operator<(const Date& other) const;
        bool operator<=(const Date& other) const;
        bool operator>(const Date& other) const;
        bool operator>=(const Date& other) const;




        long long toMillisecondsSinceEpoch() const;

        Date firstOfMonth() const;


        Date daysAgo(int days) const;


        int getYear() const;
        int getMonth() const;
        int getDay() const;

    private:
        int year;
        int month;
        int day;
    };

};
