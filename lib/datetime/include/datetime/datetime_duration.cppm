// duration.hpp
export module datetime:duration;

import std;
import :types;

export namespace datetime {

    class Duration {
        public:
            // Constructors
            Duration() = default;                        // Default constructor
            Duration(DurationNanoSeconds duration);  // Create Duration from nanoseconds
            Duration(DurationSeconds duration);      // Create Duration from seconds
            Duration(DurationMinutes duration);      // Create Duration from minutes
            Duration(DurationHours duration);        // Create Duration from hours
            Duration(DurationDays duration);         // Create Duration from days
            Duration(int days);                      // Create Duration from integer number of days
            Duration(long long nanoseconds);         // Create Duration from raw nanoseconds
            Duration(const Duration& other);             // Copy constructor

            // Addition operators
            Duration operator+(Duration otherDuration) const;             // Add two Duration objects
            Duration operator+(DurationNanoSeconds duration) const;  // Add nanoseconds
            Duration operator+(DurationSeconds duration) const;      // Add seconds
            Duration operator+(DurationMinutes duration) const;      // Add minutes
            Duration operator+(DurationHours duration) const;        // Add hours
            Duration operator+(DurationDays duration) const;         // Add days

            // Subtraction operators
            Duration operator-(Duration otherDuration) const;             // Subtract two Duration objects
            Duration operator-(DurationNanoSeconds duration) const;  // Subtract nanoseconds
            Duration operator-(DurationSeconds duration) const;      // Subtract seconds
            Duration operator-(DurationMinutes duration) const;      // Subtract minutes
            Duration operator-(DurationHours duration) const;        // Subtract hours
            Duration operator-(DurationDays duration) const;         // Subtract days

            // Comparison operators
            bool operator==(const Duration& other) const;   // Equality comparison
            bool operator!=(const Duration& other) const;   // Inequality comparison
            bool operator<(const Duration& other) const;    // Less than comparison
            bool operator>(const Duration& other) const;    // Greater than comparison
            bool operator<=(const Duration& other) const;   // Less than or equal comparison
            bool operator>=(const Duration& other) const;   // Greater than or equal comparison

            // Boolean conversion operator (returns true if duration > 0)
            explicit operator bool() const;

            // Convert to string of form HH:MM:SS.NNNNNNNNN.
            string toString() const;
            
            // Getter for nanoseconds duration
            DurationNanoSeconds getNanoseconds() const;
            
            // Convenience getters for different time units
            double getSeconds() const;
            double getMinutes() const;
            double getHours() const;
            double getDays() const;
        private:
            DurationNanoSeconds durationNanoSeconds{0};
    };

    // Stream output operator for Duration
    std::ostream& operator<<(std::ostream& os, const Duration& duration);
}; // namespace datetime
