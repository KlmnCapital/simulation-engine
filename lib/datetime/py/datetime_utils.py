"""
Python wrapper for DateTime functionality.

This module provides Python classes that wrap the C++ DateTime, Date, and Duration classes
for better IDE support, linting suggestions, and AI assistance.
"""

from typing import Union, Optional
from .datetime_utils_ext import DateTime as _DateTime, Date as _Date, Duration as _Duration, TimeOfDay as _TimeOfDay, nanosecondsToTimeOfDay as _nanosecondsToTimeOfDay, millisecondsToTimeOfDay as _millisecondsToTimeOfDay


class DateTime(_DateTime):
    """
    A high-precision DateTime class for handling timestamps with nanosecond precision.
    
    This class wraps the C++ DateTime implementation and provides methods for
    creating, manipulating, and comparing DateTime objects.
    """
    
    def __init__(self, date_time: Optional[Union[str, 'DateTime']] = None):
        """
        Initialize a DateTime object.
        
        Args:
            date_time: Can be:
                - None: creates DateTime for current time
                - str: date string in format "yyyy-mm-dd HH:MM:SS" or "yyyy-mm-dd HH:MM:SS.nnnnnnnnn"
                - DateTime: copy constructor
        
        Examples:
            >>> dt = DateTime()  # Current time
            >>> dt = DateTime("2023-12-25 14:30:00")  # Specific datetime
            >>> dt = DateTime("2023-12-25 14:30:00.123456789")  # With nanoseconds
            >>> dt2 = DateTime(dt)  # Copy constructor
        """
        super().__init__(date_time)
            
    def to_epoch_ms(self) -> int:
        """
        Convert this DateTime to milliseconds since Unix epoch (UTC).
        
        Returns:
            Milliseconds since Unix epoch as a long integer.
        
        Example:
            >>> dt = DateTime("2023-12-25 14:30:00")
            >>> ms = dt.to_epoch_ms()
        """
        return self._to_epoch_ms()

    @classmethod
    def now(cls) -> 'DateTime':
        """
        Get current date and time.
        
        Returns:
            DateTime object representing the current date and time.
        
        Example:
            >>> now = DateTime.now()
            >>> print(now)  # "2025-08-13 14:30:25.123456789" (current datetime)
        """
        # Get the C++ DateTime object and convert it to string, then create a new Python wrapper
        cpp_datetime = cls._now()
        datetime_str = str(cpp_datetime)
        return cls(datetime_str)


class Duration(_Duration):
    """
    A Duration class for representing time intervals with high precision.
    
    This class wraps the C++ Duration implementation and provides methods for
    creating and manipulating time durations.
    """
    
    def __init__(self, duration: Union[int, float, 'Duration'] = None, **kwargs):
        """
        Initialize a Duration object.
        
        Args:
            duration: Can be:
                - int: interpreted as days if >= 1, otherwise nanoseconds
                - float: interpreted as seconds
                - Duration: copy constructor
            **kwargs: Keyword arguments for specific time units:
                - days: number of days
                - hours: number of hours
                - minutes: number of minutes
                - seconds: number of seconds
                - nanoseconds: number of nanoseconds
        
        Examples:
            >>> d = Duration(1)  # 1 day
            >>> d = Duration(1000000000)  # 1 second in nanoseconds
            >>> d = Duration(1.5)  # 1.5 seconds
            >>> d = Duration(Duration(60))  # Copy constructor
            >>> d = Duration(days=1)  # 1 day
            >>> d = Duration(hours=2)  # 2 hours
            >>> d = Duration(minutes=30)  # 30 minutes
            >>> d = Duration(seconds=45)  # 45 seconds
        """
        if duration is not None:
            super().__init__(duration)
        elif 'days' in kwargs:
            super().__init__(kwargs['days'])
        elif 'hours' in kwargs:
            super().__init__(int(kwargs['hours'] * 3600 * 1_000_000_000))
        elif 'minutes' in kwargs:
            super().__init__(int(kwargs['minutes'] * 60 * 1_000_000_000))
        elif 'seconds' in kwargs:
            super().__init__(int(kwargs['seconds'] * 1_000_000_000))
        elif 'nanoseconds' in kwargs:
            super().__init__(kwargs['nanoseconds'])
        else:
            super().__init__(0)
    
class Date(_Date):
    """
    A Date class for representing calendar dates.
    
    This class wraps the C++ Date implementation and provides methods for
    creating and manipulating date objects.
    """
    
    def __init__(self, date: Union[str, int, 'Date']):
        """
        Initialize a Date object.
        
        Args:
            date: Can be:
                - str: date string in format "yyyy-mm-dd"
                - int: timestamp in nanoseconds (will be converted to TimePointNanoSeconds)
                - Date: copy constructor
                - datetime_ext.Date: C++ Date object
        
        Examples:
            >>> d = Date("2023-12-25")  # Specific date
            >>> d = Date(1703520000000000000)  # Timestamp in nanoseconds
            >>> d = Date(Date("2023-12-25"))  # Copy constructor
        """
        super().__init__(date)

    def to_epoch_ms(self) -> int:
        """
        Convert this Date to milliseconds since Unix epoch (UTC).
        
        Returns:
            Milliseconds since Unix epoch as a long integer.
        
        Example:
            >>> d = Date("2023-12-25")
            >>> ms = d.to_epoch_ms()
        """
        return self._to_epoch_ms()

    def first_of_month(self) -> 'Date':
        """
        Get the first day of the month.
        """
        return self._first_of_month()

    def __sub__(self, duration: 'Duration') -> 'DateTime':
        """
        Subtract a duration from this date, returning a wrapped DateTime.
        
        Args:
            duration: Duration to subtract
            
        Returns:
            DateTime object representing the result
        """
        # Call the C++ operation
        cpp_result = super().__sub__(duration)
        # Wrap the result in our Python DateTime class
        return DateTime(str(cpp_result))
    
    def __add__(self, duration: 'Duration') -> 'DateTime':
        """
        Add a duration to this date, returning a wrapped DateTime.
        
        Args:
            duration: Duration to add
            
        Returns:
            DateTime object representing the result
        """
        # Call the C++ operation
        cpp_result = super().__add__(duration)
        # Wrap the result in our Python DateTime class
        return DateTime(str(cpp_result))

    @classmethod
    def today(cls) -> 'Date':
        """
        Get today's date.
        
        Returns:
            Date object representing today's date.
        
        Example:
            >>> today = Date.today()
            >>> print(today)  # "2025-08-13" (current date)
        """
        # Get the C++ Date object and convert it to string, then create a new Python wrapper
        cpp_date = cls._today()
        date_str = str(cpp_date)
        return cls(date_str)
    
    def year(self) -> int:
        """
        Get the year component of the date.
        
        Returns:
            Year as an integer (e.g., 2023).
        
        Example:
            >>> d = Date("2023-12-25")
            >>> y = d.year()  # 2023
        """
        return self._year
    
    def month(self) -> int:
        """
        Get the month component of the date.
        
        Returns:
            Month as an integer (1-12).
        
        Example:
            >>> d = Date("2023-12-25")
            >>> m = d.month()  # 12
        """
        return self._month
    
    def day(self) -> int:
        """
        Get the day component of the date.
        
        Returns:
            Day as an integer (1-31).
        
        Example:
            >>> d = Date("2023-25")
            >>> day = d.day()  # 25
        """
        return self._day
    
    def to_epoch_ms(self) -> int:
        """
        Convert this Date to milliseconds since Unix epoch (UTC).
        
        Returns:
            Milliseconds since Unix epoch as a long integer.
        
        Example:
            >>> d = Date("2023-12-25")
            >>> ms = d.to_epoch_ms()
        """
        return self._to_epoch_ms()
    
    def getYear(self) -> int:
        """
        Get the year component of the date.
        
        Returns:
            Year as an integer (e.g., 2023).
        """
        return self._year
    
    def getMonth(self) -> int:
        """
        Get the month component of the date.
        
        Returns:
            Month as an integer (1-12).
        """
        return self._month
    
    def getDay(self) -> int:
        """
        Get the day component of the date.
        
        Returns:
            Day as an integer (1-31).
        """
        return self._day


# Convenience functions for creating durations
def nanoseconds(ns: int) -> Duration:
    """Create a Duration from nanoseconds."""
    return Duration(ns)


def seconds(sec: float) -> Duration:
    """Create a Duration from seconds."""
    return Duration(int(sec * 1_000_000_000))


def minutes(min: float) -> Duration:
    """Create a Duration from minutes."""
    return Duration(int(min * 60 * 1_000_000_000))


def hours(hrs: float) -> Duration:
    """Create a Duration from hours."""
    return Duration(int(hrs * 3600 * 1_000_000_000))


def days(days: float) -> Duration:
    """Create a Duration from days."""
    return Duration(int(days * 24 * 3600 * 1_000_000_000))


class TimeOfDay(_TimeOfDay):
    """
    A TimeOfDay class for representing time components since midnight.
    
    This class wraps the C++ TimeOfDay implementation and provides methods for
    accessing time components with high precision.
    """
    
    def __init__(self, hour: int = 0, minute: int = 0, second: int = 0, 
                 nanosecond: int = 0, millisecond: int = 0):
        """
        Initialize a TimeOfDay object.
        
        Args:
            hour: Hour of day (0-23)
            minute: Minute of hour (0-59)
            second: Second of minute (0-59)
            nanosecond: Nanosecond of second (0-999999999)
            millisecond: Millisecond of second (0-999)
        
        Example:
            >>> tod = TimeOfDay(14, 30, 45, 123456789, 123)
        """
        super().__init__(hour, minute, second, nanosecond, millisecond)
    
    def __str__(self) -> str:
        """Return string representation of time of day."""
        return f"{self.hour:02d}:{self.minute:02d}:{self.second:02d}.{self.millisecond:03d}.{self.nanosecond:09d}"


def nanosecondsToTimeOfDay(nanosecondsSinceEpoch: int) -> TimeOfDay:
    """
    Convert nanoseconds since epoch to time of day.
    
    Args:
        nanosecondsSinceEpoch: Nanoseconds since Unix epoch (January 1, 1970, 00:00:00 UTC)
    
    Returns:
        TimeOfDay object with hour, minute, second, nanosecond, and millisecond components
    
    Example:
        >>> tod = nanosecondsToTimeOfDay(1703512200000000000)
    """
    cpp_result = _nanosecondsToTimeOfDay(nanosecondsSinceEpoch)
    return TimeOfDay(cpp_result.hour, cpp_result.minute, cpp_result.second, 
                    cpp_result.nanosecond, cpp_result.millisecond)


def millisecondsToTimeOfDay(millisecondsSinceEpoch: int) -> TimeOfDay:
    """
    Convert milliseconds since epoch to time of day.
    
    Args:
        millisecondsSinceEpoch: Milliseconds since Unix epoch (January 1, 1970, 00:00:00 UTC)
    
    Returns:
        TimeOfDay object with hour, minute, second, nanosecond, and millisecond components
    
    Example:
        >>> tod = millisecondsToTimeOfDay(1703512200000)
    """
    cpp_result = _millisecondsToTimeOfDay(millisecondsSinceEpoch)
    return TimeOfDay(cpp_result.hour, cpp_result.minute, cpp_result.second, 
                    cpp_result.nanosecond, cpp_result.millisecond)
