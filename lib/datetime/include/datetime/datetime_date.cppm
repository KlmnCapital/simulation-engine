// datetime_date.cppm
export module datetime:date;

import std;
import :duration;
import :types;

export namespace datetime {
    class DateTime;

    class Date {
    public:
        // Constructors
        /*
        * Constructor from string of the form "yyyy-mm-dd"
        */
        Date(const string& date);

        /*
        * Constructor from system time point with nanosecond precision (UTC)
        */
        Date(TimePointNanoSeconds timePointNanoSeconds);

        /*
        * Constructor from system time point with second precision
        */
        Date(TimePointSeconds timePointSeconds);

        /*
        * Constructor from system time point with minute precision
        */
        Date(TimePointMinutes timePointMinutes);

        /*
        * Copy constructor
        */
        Date(const Date& other);

        /*
        * Get today's date (UTC)
        */
        static Date today();


        Date firstOfMonth() const;

        // Get date X days ago
        Date daysAgo(int days) const;

        // Operators
        /*
        * Add two dates - adds the day values together
        */
        Date operator+(Date otherDate) const;

        /*
        * Add a duration to this date, creating a DateTime
        */
        DateTime operator+(Duration duration) const;

        /*
        * Subtract a duration from this date, creating a DateTime
        */
        DateTime operator-(Duration duration) const;

        /*
        * Convert date to string format "yyyy-mm-dd"
        */
        string toString() const;

        /*
        * Returns the day of the week (0 = Sunday, 6 = Saturday)
        */
        int dayOfWeek() const;

        bool isWeekend() const;

        static int dayOfWeek(const int& y, int m, int d);

        /*
        * Converts date to milliseconds since Unix epoch (UTC)
        */
        long long toMillisecondsSinceEpoch() const;

        // Comparison operators
        bool operator==(const Date& other) const;
        bool operator!=(const Date& other) const;
        bool operator<(const Date& other) const;
        bool operator<=(const Date& other) const;
        bool operator>(const Date& other) const;
        bool operator>=(const Date& other) const;

        // Getters
        int getYear() const;   // Get year value
        int getMonth() const;  // Get month value (1-12)
        int getDay() const;    // Get day value (1-31)

    private:
        int year;
        int month;
        int day;

        /*
         * Helper for day of week calculation (Sakamoto's algorithm)
         * Returns 0 = Sunday, 1 = Monday, ..., 6 = Saturday
         */
        static int getDayOfWeek(int y, int m, int d) {
            static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
            y -= m < 3;
            return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
        }
    };

}; // namespace sim::datetime

