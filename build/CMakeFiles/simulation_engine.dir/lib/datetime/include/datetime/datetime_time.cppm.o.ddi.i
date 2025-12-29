# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_time.cppm"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/include/datetime/datetime_time.cppm"

export module datetime:time;

import std;

export namespace datetime {

    struct TimeOfDay {
        std::int32_t hour;
        std::int32_t minute;
        std::int32_t second;
        std::int32_t nanosecond;
        std::int32_t millisecond;
    };






    TimeOfDay nanosecondsToTimeOfDay(std::int64_t nanosecondsSinceEpoch);






    TimeOfDay millisecondsToTimeOfDay(std::int64_t millisecondsSinceEpoch);

}
