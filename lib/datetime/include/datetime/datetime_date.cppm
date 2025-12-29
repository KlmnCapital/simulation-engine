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

        // Static methods
        /*
        * Get today's date (UTC)
        */
        static Date today();

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

        // Comparison operators
        bool operator==(const Date& other) const;
        bool operator!=(const Date& other) const;
        bool operator<(const Date& other) const;
        bool operator<=(const Date& other) const;
        bool operator>(const Date& other) const;
        bool operator>=(const Date& other) const;

        /*
        * Converts date to milliseconds since Unix epoch (UTC)
        */
        long long toMillisecondsSinceEpoch() const;

        Date firstOfMonth() const;

        // Get date X days ago
        Date daysAgo(int days) const;

        // Getters
        int getYear() const;   // Get year value
        int getMonth() const;  // Get month value (1-12)
        int getDay() const;    // Get day value (1-31)

    private:
        int year;
        int month;
        int day;
    };

}; // namespace sim::datetime

