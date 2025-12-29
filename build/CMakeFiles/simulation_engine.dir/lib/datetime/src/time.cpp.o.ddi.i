# 0 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/src/time.cpp"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "/home/nemeleks/kelemen_capital/dev/simulation_engine/lib/datetime/src/time.cpp"

module datetime;

import std;

namespace datetime {

TimeOfDay nanosecondsToTimeOfDay(std::int64_t nanosecondsSinceEpoch) {

    auto epoch = std::chrono::time_point<std::chrono::system_clock>{};
    auto timePoint = epoch + std::chrono::nanoseconds(nanosecondsSinceEpoch);


    auto timeT = std::chrono::system_clock::to_time_t(timePoint);


    auto* tm = std::gmtime(&timeT);


    std::int64_t totalNanoseconds = nanosecondsSinceEpoch;
    std::int64_t secondsInDay = totalNanoseconds / 1000000000LL;
    std::int64_t nanosecondsInSecond = totalNanoseconds % 1000000000LL;


    std::int32_t hour = static_cast<std::int32_t>(tm->tm_hour);
    std::int32_t minute = static_cast<std::int32_t>(tm->tm_min);
    std::int32_t second = static_cast<std::int32_t>(tm->tm_sec);
    std::int32_t nanosecond = static_cast<std::int32_t>(nanosecondsInSecond);
    std::int32_t millisecond = static_cast<std::int32_t>(nanosecondsInSecond / 1000000);

    return {hour, minute, second, nanosecond, millisecond};
}

TimeOfDay millisecondsToTimeOfDay(std::int64_t millisecondsSinceEpoch) {

    std::int64_t nanosecondsSinceEpoch = millisecondsSinceEpoch * 1000000LL;
    return nanosecondsToTimeOfDay(nanosecondsSinceEpoch);
}

}
