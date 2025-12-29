# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/src/date.cpp"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/src/date.cpp"

module datetime;

import std;

namespace datetime {
    using string = std::string;
    using SystemClock = std::chrono::system_clock;

    Date::Date(const string& date) {

        std::istringstream iss(date);
        char dash1, dash2;

        if (!(iss >> year >> dash1 >> month >> dash2 >> day) || dash1 != '-' || dash2 != '-') {
            throw std::invalid_argument("Invalid date format. Expected yyyy-mm-dd");
        }


        if (year < 1900 || year > 3000) {
            throw std::invalid_argument("Year must be between 1900 and 3000");
        }
        if (month < 1 || month > 12) {
            throw std::invalid_argument("Month must be between 1 and 12");
        }
        if (day < 1 || day > 31) {
            throw std::invalid_argument("Day must be between 1 and 31");
        }
    }

    Date::Date(TimePointNanoSeconds timePointNanoSeconds) {

        auto time_t_val = SystemClock::to_time_t(timePointNanoSeconds);
        auto tm_val = *std::gmtime(&time_t_val);

        year = tm_val.tm_year + 1900;
        month = tm_val.tm_mon + 1;
        day = tm_val.tm_mday;
    }

    Date::Date(TimePointSeconds timePointSeconds) {

        auto nanoTimePoint = std::chrono::time_point_cast<std::chrono::nanoseconds>(timePointSeconds);
        *this = Date(nanoTimePoint);
    }

    Date::Date(TimePointMinutes timePointMinutes) {

        auto nanoTimePoint = std::chrono::time_point_cast<std::chrono::nanoseconds>(timePointMinutes);
        *this = Date(nanoTimePoint);
    }

    Date::Date(const Date& other) {
        year = other.year;
        month = other.month;
        day = other.day;
    }

    Date Date::operator+(Date otherDate) const {


        Date result(*this);
        result.day += otherDate.day;
        result.month += otherDate.month;
        result.year += otherDate.year;


        while (result.month > 12) {
            result.month -= 12;
            result.year++;
        }


        while (result.day > 30) {
            result.day -= 30;
            result.month++;
            if (result.month > 12) {
                result.month = 1;
                result.year++;
            }
        }

        return result;
    }

    DateTime Date::operator+(Duration duration) const {

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(4) << year << "-" << std::setw(2) << month << "-"
            << std::setw(2) << day << " "
            << "00:00:00";

        DateTime dateTime(oss.str());
        return dateTime + duration;
    }

    DateTime Date::operator-(Duration duration) const {

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(4) << year << "-" << std::setw(2) << month << "-"
            << std::setw(2) << day << " "
            << "00:00:00";

        DateTime dateTime(oss.str());
        return dateTime - duration;
    }

    string Date::toString() const {
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(4) << year << "-" << std::setw(2) << month << "-"
            << std::setw(2) << day;

        return oss.str();
    }

    int Date::getYear() const { return year; }

    int Date::getMonth() const { return month; }

    int Date::getDay() const { return day; }

    Date Date::today() {
        auto now = SystemClock::now();

        auto secondsTimePoint = std::chrono::time_point_cast<std::chrono::seconds>(now);
        return Date(secondsTimePoint);
    }

    long long Date::toMillisecondsSinceEpoch() const {
        using namespace std::chrono;


        std::chrono::year_month_day ymd{std::chrono::year{year},
            std::chrono::month{static_cast<unsigned int>(month)},
            std::chrono::day{static_cast<unsigned int>(day)}};


        sys_days daysSinceEpoch{ymd};
        std::chrono::milliseconds msSinceEpoch =
            duration_cast<std::chrono::milliseconds>(daysSinceEpoch.time_since_epoch());

        return msSinceEpoch.count();
    }

    Date Date::firstOfMonth() const {
        std::ostringstream theFirstOfMonth;
        theFirstOfMonth << getYear() << "-" << getMonth() << "-01";
        return Date(theFirstOfMonth.str());
    }

    Date Date::daysAgo(int days) const {

        std::tm tm_val = {};
        tm_val.tm_year = year - 1900;
        tm_val.tm_mon = month - 1;
        tm_val.tm_mday = day;
        tm_val.tm_hour = 0;
        tm_val.tm_min = 0;
        tm_val.tm_sec = 0;
        tm_val.tm_isdst = 0;


        std::time_t time_val = std::mktime(&tm_val);


        time_val -= (days * 86400);


        std::tm* result_tm = std::gmtime(&time_val);


        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(4) << (result_tm->tm_year + 1900) << "-" << std::setw(2)
            << (result_tm->tm_mon + 1) << "-" << std::setw(2) << result_tm->tm_mday;

        return Date(oss.str());
    }


    bool Date::operator==(const Date& other) const {
        return this->toMillisecondsSinceEpoch() == other.toMillisecondsSinceEpoch();
    }

    bool Date::operator!=(const Date& other) const { return !(*this == other); }

    bool Date::operator<(const Date& other) const {
        return this->toMillisecondsSinceEpoch() < other.toMillisecondsSinceEpoch();
    }

    bool Date::operator<=(const Date& other) const { return *this < other || *this == other; }

    bool Date::operator>(const Date& other) const { return !(*this <= other); }

    bool Date::operator>=(const Date& other) const { return !(*this < other); }
};
