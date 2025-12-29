#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "datetime_utils/datetime.hpp"
#include "datetime_utils/date.hpp"
#include "datetime_utils/duration.hpp"
#include "datetime_utils/time.hpp"

namespace py = pybind11;

PYBIND11_MODULE(datetime_utils_ext, m) {
    py::class_<DateTime>(m, "DateTime")
        .def(py::init<>())
        .def(py::init<const std::string&>())
        .def(py::init<const DateTime&>())
        .def("_to_epoch_ms", &DateTime::toMillisecondsSinceEpoch)
        .def("__str__", &DateTime::toString)
        .def_static("_now", &DateTime::now)  // Static method
        .def("__add__", static_cast<DateTime (DateTime::*)(DateTime) const>(&DateTime::operator+))
        .def("__add__", static_cast<DateTime (DateTime::*)(Date) const>(&DateTime::operator+))
        .def("__add__", static_cast<DateTime (DateTime::*)(Duration) const>(&DateTime::operator+))
        .def("__add__", static_cast<DateTime (DateTime::*)(DurationNanoSeconds) const>(&DateTime::operator+))
        .def("__add__", static_cast<DateTime (DateTime::*)(DurationSeconds) const>(&DateTime::operator+))
        .def("__add__", static_cast<DateTime (DateTime::*)(DurationMinutes) const>(&DateTime::operator+))
        .def("__add__", static_cast<DateTime (DateTime::*)(DurationHours) const>(&DateTime::operator+))
        .def("__lt__", &DateTime::operator<)
        .def("__gt__", &DateTime::operator>)
        .def("__le__", &DateTime::operator<=)
        .def("__ge__", &DateTime::operator>=)
        .def("__eq__", &DateTime::operator==)
        .def("__ne__", &DateTime::operator!=);

    py::class_<Duration>(m, "Duration")
        .def(py::init<DurationNanoSeconds>())
        .def(py::init<DurationSeconds>())
        .def(py::init<DurationMinutes>())
        .def(py::init<DurationHours>())
        .def(py::init<DurationDays>())  // Constructor for days using DurationDays type
        .def(py::init<const Duration&>())
        .def(py::init<long long>())  // Constructor for nanoseconds using long long
        .def_property_readonly("_nanoseconds", &Duration::getNanoseconds)
        .def_property_readonly("_seconds", &Duration::getSeconds)
        .def_property_readonly("_minutes", &Duration::getMinutes)
        .def_property_readonly("_hours", &Duration::getHours)
        .def_property_readonly("_days", &Duration::getDays)
        .def("__str__", &Duration::toString)
        .def("__add__", static_cast<Duration (Duration::*)(Duration) const>(&Duration::operator+))
        .def("__add__", static_cast<Duration (Duration::*)(DurationNanoSeconds) const>(&Duration::operator+))
        .def("__add__", static_cast<Duration (Duration::*)(DurationSeconds) const>(&Duration::operator+))
        .def("__add__", static_cast<Duration (Duration::*)(DurationMinutes) const>(&Duration::operator+))
        .def("__add__", static_cast<Duration (Duration::*)(DurationHours) const>(&Duration::operator+));

    py::class_<Date>(m, "Date")
        .def(py::init<const std::string&>())
        .def(py::init<TimePointNanoSeconds>())
        .def(py::init<TimePointSeconds>())
        .def(py::init<TimePointMinutes>())
        .def(py::init<const Date&>())  // Copy constructor
        .def("_to_epoch_ms", &Date::toMillisecondsSinceEpoch)
        .def("_first_of_month", &Date::firstOfMonth)
        .def_static("_today", &Date::today)  // Static method
        .def_property_readonly("_year", &Date::getYear)
        .def_property_readonly("_month", &Date::getMonth)
        .def_property_readonly("_day", &Date::getDay)
        .def("__str__", &Date::toString)
        .def("__add__", static_cast<Date (Date::*)(Date) const>(&Date::operator+))
        .def("__add__", static_cast<DateTime (Date::*)(Duration) const>(&Date::operator+))
        .def("__sub__", static_cast<DateTime (Date::*)(Duration) const>(&Date::operator-));

    py::class_<datetime_utils::TimeOfDay>(m, "TimeOfDay")
        .def(py::init<>())
        .def(py::init<int32_t, int32_t, int32_t, int32_t, int32_t>())
        .def_readwrite("hour", &datetime_utils::TimeOfDay::hour)
        .def_readwrite("minute", &datetime_utils::TimeOfDay::minute)
        .def_readwrite("second", &datetime_utils::TimeOfDay::second)
        .def_readwrite("nanosecond", &datetime_utils::TimeOfDay::nanosecond)
        .def_readwrite("millisecond", &datetime_utils::TimeOfDay::millisecond);

    m.def("nanosecondsToTimeOfDay", &datetime_utils::nanosecondsToTimeOfDay);
    m.def("millisecondsToTimeOfDay", &datetime_utils::millisecondsToTimeOfDay);
}
