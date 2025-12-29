// datetime_time.cppm
export module datetime:time;

import std;

export namespace datetime {

    struct TimeOfDay {
        std::int32_t hour;        // 0-23
        std::int32_t minute;      // 0-59
        std::int32_t second;      // 0-59
        std::int32_t nanosecond;  // 0-999999999
        std::int32_t millisecond; // 0-999
    };

    /**
    * Convert nanoseconds since epoch to time of day
    * @param nanosecondsSinceEpoch Nanoseconds since Unix epoch (January 1, 1970, 00:00:00 UTC)
    * @return TimeOfDay struct with hour, minute, second, nanosecond, and millisecond components
    */
    TimeOfDay nanosecondsToTimeOfDay(std::int64_t nanosecondsSinceEpoch);

    /**
    * Convert milliseconds since epoch to time of day
    * @param millisecondsSinceEpoch Milliseconds since Unix epoch (January 1, 1970, 00:00:00 UTC)
    * @return TimeOfDay struct with hour, minute, second, nanosecond, and millisecond components
    */
    TimeOfDay millisecondsToTimeOfDay(std::int64_t millisecondsSinceEpoch);

} // namespace datetime
