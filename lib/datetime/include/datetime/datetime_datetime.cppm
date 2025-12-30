// datetime_datetime.cppm export module datetime:datetime;
export module datetime:datetime;

import std;
import :date;
import :duration;
import :types;

export namespace datetime {

   class DateTime {
    public:
        // Constructors
        /*
        * Default constructor - creates DateTime for current time
        */
        DateTime();

        /*
        * Constructor from string of the form
        * "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd HH:MM:SS.nnnnnnnnn".
        */
        DateTime(const string& dateTime);

        /*
        * Copy constructor
        */
        DateTime(const DateTime& other);

        // Addition operators
        DateTime operator+(DateTime otherDateTime) const;        // Add two DateTime objects
        DateTime operator+(Date date) const;                     // Add a date (adds days)
        DateTime operator+(Duration duration) const;             // Add a duration
        DateTime operator+(DurationNanoSeconds duration) const;  // Add nanoseconds
        DateTime operator+(DurationSeconds duration) const;      // Add seconds
        DateTime operator+(DurationMinutes duration) const;      // Add minutes
        DateTime operator+(DurationHours duration) const;        // Add hours

        // Subtraction operators
        DateTime operator-(Duration duration) const;  // Subtract a duration

        /*
        * Converts the DateTime to string format
        * "yyyy-mm-dd HH:MM:SS.nnnnnnnnn"
        */
        string toString() const;

        // Comparison operators
        bool operator<(const DateTime& other) const;   // Less than comparison
        bool operator>(const DateTime& other) const;   // Greater than comparison
        bool operator<=(const DateTime& other) const;  // Less than or equal comparison
        bool operator>=(const DateTime& other) const;  // Greater than or equal comparison
        bool operator==(const DateTime& other) const;  // Equality comparison
        bool operator!=(const DateTime& other) const;  // Inequality comparison

        // Converts this DateTime to milliseconds since Unix epoch (UTC)
        long long toMillisecondsSinceEpoch() const;

        /*
         * Returns true if this DateTime falls within US Daylight Saving Time.
         * Logic: 2nd Sunday of March to 1st Sunday of November.
         */
        bool isInsideUSDST() const;

        /*
         * Returns the day of the week (0 = Sunday, 6 = Saturday)
         */
        int dayOfWeek() const;

        bool isWeekend() const;

        int dayOfWeek(const int& y, int m, int d);

        // Helper to get time as decimal hours for trading window checks
        double timeAsDecimal() const;

        /*
        * Get current date and time (UTC)
        */
        static DateTime now();

        /*
        * Create string representation from epoch time (nanoseconds or milliseconds since Unix epoch
        * UTC)
        */
        static string fromEpochTime(long long epochTime, bool isNanoseconds = false);
        static string fromEpochTime(std::uint64_t epochTime, bool isNanoseconds = false);
 
    private:
        int year;
        int month;
        int day;
        long long nanoseconds;

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
}; // namespace datetime

