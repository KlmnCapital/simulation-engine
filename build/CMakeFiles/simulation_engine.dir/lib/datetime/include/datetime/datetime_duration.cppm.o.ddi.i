# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_duration.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_duration.cppm"

export module datetime:duration;

import std;
import :types;

export namespace datetime {

    class Duration {
        public:

            Duration() = default;
            Duration(DurationNanoSeconds duration);
            Duration(DurationSeconds duration);
            Duration(DurationMinutes duration);
            Duration(DurationHours duration);
            Duration(DurationDays duration);
            Duration(int days);
            Duration(long long nanoseconds);
            Duration(const Duration& other);


            Duration operator+(Duration otherDuration) const;
            Duration operator+(DurationNanoSeconds duration) const;
            Duration operator+(DurationSeconds duration) const;
            Duration operator+(DurationMinutes duration) const;
            Duration operator+(DurationHours duration) const;
            Duration operator+(DurationDays duration) const;


            Duration operator-(Duration otherDuration) const;
            Duration operator-(DurationNanoSeconds duration) const;
            Duration operator-(DurationSeconds duration) const;
            Duration operator-(DurationMinutes duration) const;
            Duration operator-(DurationHours duration) const;
            Duration operator-(DurationDays duration) const;


            bool operator==(const Duration& other) const;
            bool operator!=(const Duration& other) const;
            bool operator<(const Duration& other) const;
            bool operator>(const Duration& other) const;
            bool operator<=(const Duration& other) const;
            bool operator>=(const Duration& other) const;


            explicit operator bool() const;


            string toString() const;


            DurationNanoSeconds getNanoseconds() const;


            double getSeconds() const;
            double getMinutes() const;
            double getHours() const;
            double getDays() const;
        private:
            DurationNanoSeconds durationNanoSeconds{0};
    };


    std::ostream& operator<<(std::ostream& os, const Duration& duration);
};
