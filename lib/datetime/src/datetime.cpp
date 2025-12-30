// date_time.cpp
module datetime;

import std;

namespace datetime {
    using SystemClock  = std::chrono::system_clock;

    using NanoSeconds = std::chrono::nanoseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using hours = std::chrono::hours;

    using TimePointNanoSeconds = std::chrono::time_point<SystemClock, NanoSeconds>;
    using TimePointSeconds = std::chrono::time_point<SystemClock, seconds>;
    using TimePointMinutes = std::chrono::time_point<SystemClock, minutes>;
    using TimePointDays = std::chrono::time_point<SystemClock, hours>;

    using DurationNanoSeconds = std::chrono::nanoseconds;
    using DurationSeconds = std::chrono::seconds;
    using DurationMinutes = std::chrono::minutes;
    using DurationHours = std::chrono::hours;

    using string = std::string;

    DateTime::DateTime() {
        // Initialize with epoch time (1970-01-01 00:00:00)
        year = 1970;
        month = 1;
        day = 1;
        nanoseconds = 0;
    }

    DateTime::DateTime(const DateTime& other) 
        : year(other.year), month(other.month), day(other.day), nanoseconds(other.nanoseconds) {
    }

    DateTime::DateTime(const string& dateTime) {
        // Parse format: "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd HH:MM:SS.nnnnnnnnn"
        std::istringstream iss(dateTime);
        string datePart, timePart;
        
        if (!(iss >> datePart >> timePart)) {
            throw std::invalid_argument("Invalid DateTime format");
        }
        
        // Parse date part (yyyy-mm-dd)
        char dash1, dash2;
        std::istringstream dateStream(datePart);
        if (!(dateStream >> year >> dash1 >> month >> dash2 >> day) || 
            dash1 != '-' || dash2 != '-') {
            throw std::invalid_argument("Invalid date format");
        }
        
        // Parse time part (HH:MM:SS.nnnnnnnnn)
        int hours, minutes, seconds;
        char colon1, colon2;
        std::istringstream timeStream(timePart);
        if (!(timeStream >> hours >> colon1 >> minutes >> colon2 >> seconds) ||
            colon1 != ':' || colon2 != ':') {
            throw std::invalid_argument("Invalid time format");
        }
        
        // Convert to nanoseconds
        nanoseconds = static_cast<long long>(hours) * 3600000000000LL +
                    static_cast<long long>(minutes) * 60000000000LL +
                    static_cast<long long>(seconds) * 1000000000LL;
        
        // Check for nanoseconds part
        if (timeStream.peek() == '.') {
            timeStream.ignore(); // skip dot
            long long nanos;
            if (timeStream >> nanos) {
                nanoseconds += nanos;
            }
        }
    }

    DateTime DateTime::operator+(DateTime otherDateTime) const {
        DateTime result(*this);
        result.nanoseconds += otherDateTime.nanoseconds;
        
        // Handle nanoseconds overflow to next day
        const long long nanosPerDay = 24LL * 3600 * 1000000000;
        while (result.nanoseconds >= nanosPerDay) {
            result.nanoseconds -= nanosPerDay;
            // Simple day increment (doesn't handle month/year overflow)
            result.day++;
        }
        
        return result;
    }

    DateTime DateTime::operator+(Date date) const {
        DateTime result(*this);
        result.year += date.getYear() - 1970;  // Subtract epoch year since Date constructor uses absolute values
        result.month += date.getMonth() - 1;   // Subtract 1 since we're adding relative values
        result.day += date.getDay() - 1;       // Subtract 1 since we're adding relative values
        
        // Handle month overflow
        while (result.month > 12) {
            result.month -= 12;
            result.year++;
        }
        
        // Handle day overflow (simplified - assumes 30 days per month)
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
    DateTime DateTime::operator+(Duration duration) const {
        DateTime result(*this);
        result.nanoseconds += duration.getNanoseconds().count();
        
        // Handle overflow to next day
        const long long nanosPerDay = 24LL * 3600 * 1000000000;
        while (result.nanoseconds >= nanosPerDay) {
            result.nanoseconds -= nanosPerDay;
            result.day++;
        }
        
        return result;
    }
    DateTime DateTime::operator+(DurationNanoSeconds duration) const {
        DateTime result(*this);
        result.nanoseconds += duration.count();
        
        // Handle overflow to next day
        const long long nanosPerDay = 24LL * 3600 * 1000000000;
        while (result.nanoseconds >= nanosPerDay) {
            result.nanoseconds -= nanosPerDay;
            result.day++;
        }
        
        return result;
    }
    DateTime DateTime::operator+(DurationSeconds duration) const {
        return *this + std::chrono::duration_cast<DurationNanoSeconds>(duration);
    }
    DateTime DateTime::operator+(DurationMinutes duration) const {
        return *this + std::chrono::duration_cast<DurationNanoSeconds>(duration);
    }
    DateTime DateTime::operator+(DurationHours duration) const {
        return *this + std::chrono::duration_cast<DurationNanoSeconds>(duration);
    }

    DateTime DateTime::operator-(Duration duration) const {
        DateTime result(*this);
        result.nanoseconds -= duration.getNanoseconds().count();
        
        // Handle underflow to previous day
        const long long nanosPerDay = 24LL * 3600 * 1000000000;
        while (result.nanoseconds < 0) {
            result.nanoseconds += nanosPerDay;
            result.day--;
            // Note: This is a simplified implementation
            // In a production system, you'd want proper calendar arithmetic
        }
        
        return result;
    }

    string DateTime::toString() const {
        // Convert nanoseconds back to time components
        long long totalNanos = nanoseconds;
        long long hours = totalNanos / (3600 * 1000000000LL);
        totalNanos %= (3600 * 1000000000LL);
        long long minutes = totalNanos / (60 * 1000000000LL);
        totalNanos %= (60 * 1000000000LL);
        long long seconds = totalNanos / 1000000000LL;
        long long remainingNanos = totalNanos % 1000000000LL;
        
        std::ostringstream oss;
        oss << std::setfill('0') 
            << std::setw(4) << year << "-"
            << std::setw(2) << month << "-"
            << std::setw(2) << day << " "
            << std::setw(2) << hours << ":"
            << std::setw(2) << minutes << ":"
            << std::setw(2) << seconds;
        
        if (remainingNanos > 0) {
            oss << "." << std::setw(9) << remainingNanos;
        }
        
        return oss.str();
    }

    bool DateTime::operator<(const DateTime& other) const {
        if (year != other.year) return year < other.year;
        if (month != other.month) return month < other.month;
        if (day != other.day) return day < other.day;
        return nanoseconds < other.nanoseconds;
    }

    bool DateTime::operator>(const DateTime& other) const {
        return other < *this;
    }

    bool DateTime::operator<=(const DateTime& other) const {
        return !(*this > other);
    }

    bool DateTime::operator>=(const DateTime& other) const {
        return !(*this < other);
    }

    bool DateTime::operator==(const DateTime& other) const {
        return year == other.year && 
            month == other.month && 
            day == other.day && 
            nanoseconds == other.nanoseconds;
    }

    bool DateTime::operator!=(const DateTime& other) const {
        return !(*this == other);
    }

    long long DateTime::toMillisecondsSinceEpoch() const {
        using namespace std::chrono;

        // Convert the date part to sys_days (midnight UTC)
        year_month_day ymd{
            std::chrono::year{this->year},
            std::chrono::month{static_cast<unsigned int>(this->month)},
            std::chrono::day{static_cast<unsigned int>(this->day)}
        };
        sys_days datePoint{ymd};

        // Convert nanoseconds since midnight into a duration
        std::chrono::nanoseconds nanosSinceMidnight{this->nanoseconds};

        // Combine the date and time into a sys_time<nanoseconds>
        sys_time<std::chrono::nanoseconds> dateTimePoint = datePoint + nanosSinceMidnight;

        // Convert to milliseconds since epoch
        std::chrono::milliseconds msSinceEpoch = duration_cast<std::chrono::milliseconds>(dateTimePoint.time_since_epoch());

        return msSinceEpoch.count();
    }

    DateTime DateTime::now() {
        // Get current system time
        auto now = SystemClock::now();
        
        // Convert to time_t for date components
        auto time_t_now = SystemClock::to_time_t(now);
        std::tm* tm_now = std::gmtime(&time_t_now);
        
        // Extract date components
        int year = tm_now->tm_year + 1900;  // tm_year is years since 1900
        int month = tm_now->tm_mon + 1;     // tm_mon is 0-11
        int day = tm_now->tm_mday;
        
        // Extract time components and convert to nanoseconds
        int hours = tm_now->tm_hour;
        int minutes = tm_now->tm_min;
        int seconds = tm_now->tm_sec;
        
        // Calculate nanoseconds since midnight
        long long nanoseconds = static_cast<long long>(hours) * 3600000000000LL +
                            static_cast<long long>(minutes) * 60000000000LL +
                            static_cast<long long>(seconds) * 1000000000LL;
        
        // Add sub-second precision if available
        auto duration_since_epoch = now.time_since_epoch();
        auto seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(duration_since_epoch);
        auto nanoseconds_part = std::chrono::duration_cast<std::chrono::nanoseconds>(duration_since_epoch - seconds_since_epoch);
        nanoseconds += nanoseconds_part.count();
        
        // Create and return DateTime object
        DateTime result;
        result.year = year;
        result.month = month;
        result.day = day;
        result.nanoseconds = nanoseconds;
        
        return result;
    }

    string DateTime::fromEpochTime(long long epochTime, bool isNanoseconds) {
        using namespace std::chrono;
        
        // Convert to nanoseconds if needed
        long long nanosSinceEpoch;
        if (isNanoseconds) {
            nanosSinceEpoch = epochTime;
        } else {
            // Convert milliseconds to nanoseconds
            nanosSinceEpoch = epochTime * 1000000LL;
        }
        
        // Create time point from nanoseconds since epoch
        sys_time<std::chrono::nanoseconds> timePoint{std::chrono::nanoseconds{nanosSinceEpoch}};
        
        // Convert to time_t for date components
        auto time_t_val = system_clock::to_time_t(time_point_cast<system_clock::duration>(timePoint));
        std::tm* tm_val = std::gmtime(&time_t_val);
        
        // Extract date components
        int year = tm_val->tm_year + 1900;  // tm_year is years since 1900
        int month = tm_val->tm_mon + 1;     // tm_mon is 0-11
        int day = tm_val->tm_mday;
        
        // Extract time components
        int hours = tm_val->tm_hour;
        int minutes = tm_val->tm_min;
        int seconds = tm_val->tm_sec;
        
        // Calculate nanoseconds since midnight
        long long nanoseconds = static_cast<long long>(hours) * 3600000000000LL +
                            static_cast<long long>(minutes) * 60000000000LL +
                            static_cast<long long>(seconds) * 1000000000LL;
        
        // Add sub-second precision
        auto duration_since_epoch = timePoint.time_since_epoch();
        auto seconds_since_epoch = duration_cast<std::chrono::seconds>(duration_since_epoch);
        auto nanoseconds_part = duration_cast<std::chrono::nanoseconds>(duration_since_epoch - seconds_since_epoch);
        nanoseconds += nanoseconds_part.count();
        
        // Format as string: "yyyy-mm-dd HH:MM:SS.nnnnnnnnn"
        std::ostringstream oss;
        oss << std::setfill('0') 
            << std::setw(4) << year << "-"
            << std::setw(2) << month << "-"
            << std::setw(2) << day << " "
            << std::setw(2) << hours << ":"
            << std::setw(2) << minutes << ":"
            << std::setw(2) << seconds;
        
        // Add nanoseconds part if present
        long long remainingNanos = nanoseconds % 1000000000LL;
        if (remainingNanos > 0) {
            oss << "." << std::setw(9) << remainingNanos;
        }
        
        return oss.str();
    }

    string DateTime::fromEpochTime(std::uint64_t epochTime, bool isNanoseconds) {
        // Convert uint64_t to long long and delegate to the other overload
        return fromEpochTime(static_cast<long long>(epochTime), isNanoseconds);
    }

    /*
    * Returns true if this DateTime falls within US Daylight Saving Time.
    * Logic: 2nd Sunday of March to 1st Sunday of November.
    */
    bool DateTime::isInsideUSDST() const {
        if (month < 3 || month > 11) return false;
        if (month > 3 && month < 11) return true;

        // March: Starts 2nd Sunday
        if (month == 3) {
            int dowMar1 = Date::dayOfWeek(year, 3, 1);
            // Days to first Sunday. If Sun(0), 0 days. If Mon(1), 6 days.
            int daysToFirstSun = (7 - dowMar1) % 7;
            int firstSun = 1 + daysToFirstSun;
            int secondSun = firstSun + 7;
            return day >= secondSun;
        }

        // November: Ends 1st Sunday
        if (month == 11) {
            int dowNov1 = Date::dayOfWeek(year, 11, 1);
            int daysToFirstSun = (7 - dowNov1) % 7;
            int firstSun = 1 + daysToFirstSun;
            return day < firstSun;
        }
        return false;
    }

    // Helper to get time as decimal hours for trading window checks
    double DateTime::timeAsDecimal() const {
        // nanoseconds is nanos since start of day
        return static_cast<double>(nanoseconds) / 3'600'000'000'000.0;
    }

    int DateTime::dayOfWeek() const {
        return getDayOfWeek(year, month, day);
    }

    int DateTime::dayOfWeek(const int& y, int m, int d) {
        return getDayOfWeek(y, m, d);
    }

    bool DateTime::isWeekend() const {
        int dow = dayOfWeek();
        return (dow == 0 || dow == 6);
    }

}; // namespace datetime
