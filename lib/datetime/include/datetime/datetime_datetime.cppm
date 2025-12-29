// datetime_datetime.cppm
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

        // Static methods
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
    };
}; // namespace datetime

