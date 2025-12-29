export module datetime:types;

import std;

export namespace datetime {
    using string = std::string;
    using SystemClock = std::chrono::system_clock;

    using NanoSeconds = std::chrono::nanoseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using hours = std::chrono::hours;
    using days = std::chrono::duration<int, std::ratio<86400>>;  // 24*60*60 seconds

    using TimePointNanoSeconds = std::chrono::time_point<SystemClock, NanoSeconds>;
    using TimePointSeconds = std::chrono::time_point<SystemClock, seconds>;
    using TimePointMinutes = std::chrono::time_point<SystemClock, minutes>;
    using TimePointDays = std::chrono::time_point<SystemClock, hours>;

    using DurationNanoSeconds = std::chrono::nanoseconds;
    using DurationSeconds = std::chrono::seconds;
    using DurationMinutes = std::chrono::minutes;
    using DurationHours = std::chrono::hours;
    using DurationDays = std::chrono::duration<int, std::ratio<86400>>;  // 24*60*60 seconds
} // namespace datetime

