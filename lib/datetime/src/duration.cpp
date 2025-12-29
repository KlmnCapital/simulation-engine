// duration.cpp
module datetime;

import std;

namespace datetime {
    Duration::Duration(DurationNanoSeconds duration)
    : durationNanoSeconds(duration) {}

    Duration::Duration(DurationSeconds duration)
    : durationNanoSeconds(std::chrono::duration_cast<DurationNanoSeconds>(duration)) {}

    Duration::Duration(DurationMinutes duration)
    : durationNanoSeconds(std::chrono::duration_cast<DurationNanoSeconds>(duration)) {}

    Duration::Duration(DurationHours duration)
    : durationNanoSeconds(std::chrono::duration_cast<DurationNanoSeconds>(duration)) {}

    Duration::Duration(DurationDays duration)
    : durationNanoSeconds(std::chrono::duration_cast<DurationNanoSeconds>(duration)) {}

    Duration::Duration(int days)
    : durationNanoSeconds(std::chrono::duration_cast<DurationNanoSeconds>(DurationDays(days))) {}

    Duration::Duration(long long nanoseconds)
    : durationNanoSeconds(DurationNanoSeconds(nanoseconds)) {}

    Duration::Duration(const Duration& other)
    : durationNanoSeconds(other.durationNanoSeconds) {}

    Duration Duration::operator+(Duration otherDuration) const {
        return Duration(durationNanoSeconds + otherDuration.durationNanoSeconds);
    }

    Duration Duration::operator+(DurationNanoSeconds duration) const {
        return Duration(durationNanoSeconds + duration);
    }

    Duration Duration::operator+(DurationSeconds duration) const {
        return Duration(durationNanoSeconds + std::chrono::duration_cast<DurationNanoSeconds>(duration));
    }
    Duration Duration::operator+(DurationMinutes duration) const {
        return Duration(durationNanoSeconds + std::chrono::duration_cast<DurationNanoSeconds>(duration));
    }

    Duration Duration::operator+(DurationHours duration) const {
        return Duration(durationNanoSeconds + std::chrono::duration_cast<DurationNanoSeconds>(duration));
    }

    Duration Duration::operator+(DurationDays duration) const {
        return Duration(durationNanoSeconds + std::chrono::duration_cast<DurationNanoSeconds>(duration));
    }

    // Subtraction operators
    Duration Duration::operator-(Duration otherDuration) const {
        return Duration(durationNanoSeconds - otherDuration.durationNanoSeconds);
    }

    Duration Duration::operator-(DurationNanoSeconds duration) const {
        return Duration(durationNanoSeconds - duration);
    }

    Duration Duration::operator-(DurationSeconds duration) const {
        return Duration(durationNanoSeconds - std::chrono::duration_cast<DurationNanoSeconds>(duration));
    }

    Duration Duration::operator-(DurationMinutes duration) const {
        return Duration(durationNanoSeconds - std::chrono::duration_cast<DurationNanoSeconds>(duration));
    }

    Duration Duration::operator-(DurationHours duration) const {
        return Duration(durationNanoSeconds - std::chrono::duration_cast<DurationNanoSeconds>(duration));
    }

    Duration Duration::operator-(DurationDays duration) const {
        return Duration(durationNanoSeconds - std::chrono::duration_cast<DurationNanoSeconds>(duration));
    }

    // Comparison operators
    bool Duration::operator==(const Duration& other) const {
        return durationNanoSeconds == other.durationNanoSeconds;
    }

    bool Duration::operator!=(const Duration& other) const {
        return durationNanoSeconds != other.durationNanoSeconds;
    }

    bool Duration::operator<(const Duration& other) const {
        return durationNanoSeconds < other.durationNanoSeconds;
    }

    bool Duration::operator>(const Duration& other) const {
        return durationNanoSeconds > other.durationNanoSeconds;
    }

    bool Duration::operator<=(const Duration& other) const {
        return durationNanoSeconds <= other.durationNanoSeconds;
    }

    bool Duration::operator>=(const Duration& other) const {
        return durationNanoSeconds >= other.durationNanoSeconds;
    }

    // Boolean conversion operator
    Duration::operator bool() const {
        return durationNanoSeconds.count() > 0;
    }

    string Duration::toString() const {
        // Convert nanoseconds to total seconds and remaining nanoseconds
        auto totalNanos = durationNanoSeconds.count();
        auto totalSeconds = totalNanos / 1'000'000'000;
        auto remainingNanos = totalNanos % 1'000'000'000;

        // Calculate hours, minutes, seconds from total seconds
        auto hours = totalSeconds / 3600;
        auto minutes = (totalSeconds % 3600) / 60;
        auto seconds = totalSeconds % 60;

        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << hours << ":"
            << std::setw(2) << minutes << ":"
            << std::setw(2) << seconds;

        // Add nanoseconds if they exist
        if (remainingNanos > 0) {
            oss << "." << std::setw(9) << remainingNanos;
        }

        return oss.str();
    }

    DurationNanoSeconds Duration::getNanoseconds() const {
        return durationNanoSeconds;
    }

    double Duration::getSeconds() const {
        return durationNanoSeconds.count() / 1e9;
    }

    double Duration::getMinutes() const {
        return durationNanoSeconds.count() / (60 * 1e9);
    }

    double Duration::getHours() const {
        return durationNanoSeconds.count() / (60 * 60 * 1e9);
    }

    double Duration::getDays() const {
        return durationNanoSeconds.count() / (24 * 60 * 60 * 1e9);
    }

    // Stream output operator implementation
    std::ostream& operator<<(std::ostream& os, const Duration& duration) {
        return os << duration.toString();
    }
}; // namespace datetime
