# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_types.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_types.cppm"
export module datetime:types;

import std;

export namespace datetime {
    using string = std::string;
    using SystemClock = std::chrono::system_clock;

    using NanoSeconds = std::chrono::nanoseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using hours = std::chrono::hours;
    using days = std::chrono::duration<int, std::ratio<86400>>;

    using TimePointNanoSeconds = std::chrono::time_point<SystemClock, NanoSeconds>;
    using TimePointSeconds = std::chrono::time_point<SystemClock, seconds>;
    using TimePointMinutes = std::chrono::time_point<SystemClock, minutes>;
    using TimePointDays = std::chrono::time_point<SystemClock, hours>;

    using DurationNanoSeconds = std::chrono::nanoseconds;
    using DurationSeconds = std::chrono::seconds;
    using DurationMinutes = std::chrono::minutes;
    using DurationHours = std::chrono::hours;
    using DurationDays = std::chrono::duration<int, std::ratio<86400>>;
}
