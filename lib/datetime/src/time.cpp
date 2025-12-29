// time.cpp
module datetime;

import std;

namespace datetime {

TimeOfDay nanosecondsToTimeOfDay(std::int64_t nanosecondsSinceEpoch) {
    // Convert nanoseconds to time_point
    auto epoch = std::chrono::time_point<std::chrono::system_clock>{};
    auto timePoint = epoch + std::chrono::nanoseconds(nanosecondsSinceEpoch);

    // Convert to time_t to get seconds since epoch
    auto timeT = std::chrono::system_clock::to_time_t(timePoint);

    // Get the time components using gmtime (UTC time)
    auto* tm = std::gmtime(&timeT);

    // Calculate nanoseconds within the day
    std::int64_t totalNanoseconds = nanosecondsSinceEpoch;
    std::int64_t secondsInDay = totalNanoseconds / 1000000000LL;
    std::int64_t nanosecondsInSecond = totalNanoseconds % 1000000000LL;

    // Get time of day components
    std::int32_t hour = static_cast<std::int32_t>(tm->tm_hour);
    std::int32_t minute = static_cast<std::int32_t>(tm->tm_min);
    std::int32_t second = static_cast<std::int32_t>(tm->tm_sec);
    std::int32_t nanosecond = static_cast<std::int32_t>(nanosecondsInSecond);
    std::int32_t millisecond = static_cast<std::int32_t>(nanosecondsInSecond / 1000000);

    return {hour, minute, second, nanosecond, millisecond};
}

TimeOfDay millisecondsToTimeOfDay(std::int64_t millisecondsSinceEpoch) {
    // Convert milliseconds to nanoseconds first
    std::int64_t nanosecondsSinceEpoch = millisecondsSinceEpoch * 1000000LL;
    return nanosecondsToTimeOfDay(nanosecondsSinceEpoch);
}

}  // namespace datetime
