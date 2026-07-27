import pytest

import yup


#==================================================================================================

def test_construct_with_values():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 45)

    assert time_obj.getYear() == 2022
    assert time_obj.getMonth() == 1
    assert time_obj.getDayOfMonth() == 19
    assert time_obj.getHours() == 12
    assert time_obj.getMinutes() == 30
    assert time_obj.getSeconds() == 45

#==================================================================================================

def test_get_current_time():
    current_time = yup.Time.getCurrentTime()
    assert isinstance(current_time, yup.Time)

#==================================================================================================

def test_add_relative_time():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 0)
    relative_time = yup.RelativeTime.seconds(10)

    new_time = time_obj + relative_time

    assert new_time.getSeconds() == 10

#==================================================================================================

def test_subtract_relative_time():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 0)
    relative_time = yup.RelativeTime.seconds(5)

    new_time = time_obj - relative_time

    assert new_time.getSeconds() == 55

#==================================================================================================

def test_comparisons():
    today = yup.Time(2022, 1, 19, 12, 30, 0)
    tomorrow = yup.Time(2022, 1, 20, 11, 30, 0)

    assert today == today
    assert today <= today
    assert today >= today
    assert today != tomorrow
    assert tomorrow != today
    assert today < tomorrow
    assert today <= tomorrow
    assert tomorrow > today
    assert tomorrow >= today

#==================================================================================================

def test_to_milliseconds():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 45, 0, False)

    assert time_obj.toMilliseconds() == 1645273845000

#==================================================================================================

def test_get_month_name():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 45)

    month_name = time_obj.getMonthName(True)
    assert month_name == "Feb"

    month_name = time_obj.getMonthName(False)
    assert month_name == "February"

#==================================================================================================

def test_get_weekday_name():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 45)

    weekday_name = time_obj.getWeekdayName(True)
    assert weekday_name == "Sat"

    weekday_name = time_obj.getWeekdayName(False)
    assert weekday_name == "Saturday"

#==================================================================================================

def test_set_system_time_to_this_time():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 45)
    # time_obj.setSystemTimeToThisTime()

#==================================================================================================

def test_to_string():
    time_obj = yup.Time(2022, 1, 19, 14, 30, 45)

    time_str = time_obj.toString(includeDate=False, includeTime=False)
    assert time_str == ""

    time_str = time_obj.toString(includeDate=False, includeTime=False, includeSeconds=False)
    assert time_str == ""

    time_str = time_obj.toString(includeDate=True, includeTime=False)
    assert time_str == "19 Feb 2022"

    time_str = time_obj.toString(includeDate=True, includeTime=True)
    assert time_str == "19 Feb 2022 2:30:45pm"

    time_str = time_obj.toString(includeDate=False, includeTime=True)
    assert time_str == "2:30:45pm"

    time_str = time_obj.toString(includeDate=False, includeTime=True, includeSeconds=False)
    assert time_str == "2:30pm"

    time_str = time_obj.toString(includeDate=False, includeTime=True, use24HourClock=True)
    assert time_str == "14:30:45"

    time_str = time_obj.toString(includeDate=False, includeTime=True, includeSeconds=False, use24HourClock=True)
    assert time_str == "14:30"

#==================================================================================================

@pytest.mark.skip(reason="This produces wrong results because of the timezone")
def test_from_iso8601():
    time_obj = yup.Time.fromISO8601("2022-02-19T12:30:45.000+00:00")
    assert isinstance(time_obj, yup.Time)
    assert yup.Time(2022, 1, 19, 12, 30, 45, False) == time_obj
    assert time_obj.toISO8601(False) == "20220219T123045.000+0000"
    assert time_obj.toISO8601(True) == "2022-02-19T12:30:45.000+00:00"

    a = yup.Time.fromISO8601("2022-02-19T12:30:45.000+00:00")
    b = yup.Time.fromISO8601(a.toISO8601(True))
    assert a == b

#==================================================================================================

def test_compilation_date():
    compilation_date = yup.Time.getCompilationDate()
    assert isinstance(compilation_date, yup.Time)
    assert compilation_date > yup.Time()

#==================================================================================================

def test_utc_offset_seconds():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 45)

    offset_seconds = time_obj.getUTCOffsetSeconds()
    assert isinstance(offset_seconds, int)

#==================================================================================================

def test_utc_offset_string():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 45)

    offset_str = time_obj.getUTCOffsetString(True)
    assert offset_str is not None
    assert isinstance(offset_str, str)

#==================================================================================================

def test_high_resolution_ticks():
    ticks = yup.Time.getHighResolutionTicks()
    assert isinstance(ticks, int)

#==================================================================================================

def test_high_resolution_ticks_per_second():
    ticks_per_second = yup.Time.getHighResolutionTicksPerSecond()
    assert isinstance(ticks_per_second, int)

#==================================================================================================

def test_high_resolution_ticks_to_seconds():
    ticks = yup.Time.getHighResolutionTicks()
    seconds = yup.Time.highResolutionTicksToSeconds(ticks)
    assert isinstance(seconds, float)

#==================================================================================================

def test_seconds_to_high_resolution_ticks():
    seconds = 10.5
    ticks = yup.Time.secondsToHighResolutionTicks(seconds)
    assert isinstance(ticks, int)

#==================================================================================================

def test_wait_for_millisecond_counter():
    initial_counter = yup.Time.getMillisecondCounter()
    target_counter = initial_counter + 100  # wait for 100ms

    yup.Time.waitForMillisecondCounter(target_counter)

    final_counter = yup.Time.getMillisecondCounter()
    assert final_counter >= target_counter

#==================================================================================================

def test_is_daylight_saving_time():
    time_obj = yup.Time(2022, 7, 1, 12, 30, 45)
    is_dst = time_obj.isDaylightSavingTime()
    assert isinstance(is_dst, bool)

#==================================================================================================

# Ensure __repr__ and __str__ are equivalent to toISO8601
def test_representation_methods():
    time_obj = yup.Time(2022, 1, 19, 12, 30, 45)
    assert repr(time_obj).startswith("yup.Time('20220219T123045")
    assert str(time_obj) == time_obj.toISO8601(includeDividerCharacters=False)

#==================================================================================================

def test_legacy_tests():
    t = yup.Time.getCurrentTime()
    assert t > yup.Time()

    yup.Thread.sleep (15)
    assert yup.Time.getCurrentTime() > t

    assert t.getTimeZone() != ""
    assert t.getUTCOffsetString(True)  == "Z" or len(t.getUTCOffsetString(True)) == 6
    assert t.getUTCOffsetString(False) == "Z" or len(t.getUTCOffsetString(False)) == 5

    assert yup.Time.fromISO8601(t.toISO8601(True)) == t
    assert yup.Time.fromISO8601(t.toISO8601(False)) == t

    assert yup.Time.fromISO8601("2016-02-16") == yup.Time(2016, 1, 16, 0, 0, 0, 0, False)
    assert yup.Time.fromISO8601("20160216Z")  == yup.Time(2016, 1, 16, 0, 0, 0, 0, False)

    assert yup.Time.fromISO8601("2016-02-16T15:03:57+00:00") == yup.Time(2016, 1, 16, 15, 3, 57, 0, False)
    assert yup.Time.fromISO8601("20160216T150357+0000")      == yup.Time(2016, 1, 16, 15, 3, 57, 0, False)

    assert yup.Time.fromISO8601("2016-02-16T15:03:57.999+00:00") == yup.Time(2016, 1, 16, 15, 3, 57, 999, False)
    assert yup.Time.fromISO8601("20160216T150357.999+0000")      == yup.Time(2016, 1, 16, 15, 3, 57, 999, False)
    assert yup.Time.fromISO8601("2016-02-16T15:03:57.999Z")      == yup.Time(2016, 1, 16, 15, 3, 57, 999, False)
    assert yup.Time.fromISO8601("2016-02-16T15:03:57,999Z")      == yup.Time(2016, 1, 16, 15, 3, 57, 999, False)
    assert yup.Time.fromISO8601("20160216T150357.999Z")          == yup.Time(2016, 1, 16, 15, 3, 57, 999, False)
    assert yup.Time.fromISO8601("20160216T150357,999Z")          == yup.Time(2016, 1, 16, 15, 3, 57, 999, False)

    assert yup.Time.fromISO8601("2016-02-16T15:03:57.999-02:30") == yup.Time(2016, 1, 16, 17, 33, 57, 999, False)
    assert yup.Time.fromISO8601("2016-02-16T15:03:57,999-02:30") == yup.Time(2016, 1, 16, 17, 33, 57, 999, False)
    assert yup.Time.fromISO8601("20160216T150357.999-0230")      == yup.Time(2016, 1, 16, 17, 33, 57, 999, False)
    assert yup.Time.fromISO8601("20160216T150357,999-0230")      == yup.Time(2016, 1, 16, 17, 33, 57, 999, False)

    assert yup.Time(1970,  0,  1,  0,  0,  0, 0, False) == yup.Time(0)
    assert yup.Time(2106,  1,  7,  6, 28, 15, 0, False) == yup.Time(4294967295000)
    assert yup.Time(2007, 10,  7,  1,  7, 20, 0, False) == yup.Time(1194397640000)
    assert yup.Time(2038,  0, 19,  3, 14,  7, 0, False) == yup.Time(2147483647000)
    assert yup.Time(2016,  2,  7, 11, 20,  8, 0, False) == yup.Time(1457349608000)
    assert yup.Time(1969, 11, 31, 23, 59, 59, 0, False) == yup.Time(-1000)
    assert yup.Time(1901, 11, 13, 20, 45, 53, 0, False) == yup.Time(-2147483647000)

    assert yup.Time(1982, 1, 1, 12, 0, 0, 0, True) + yup.RelativeTime.days(365) == yup.Time(1983, 1, 1, 12, 0, 0, 0, True)
    assert yup.Time(1970, 1, 1, 12, 0, 0, 0, True) + yup.RelativeTime.days(365) == yup.Time(1971, 1, 1, 12, 0, 0, 0, True)
    assert yup.Time(2038, 1, 1, 12, 0, 0, 0, True) + yup.RelativeTime.days(365) == yup.Time(2039, 1, 1, 12, 0, 0, 0, True)

    assert yup.Time(1982, 1, 1, 12, 0, 0, 0, False) + yup.RelativeTime.days(365) == yup.Time(1983, 1, 1, 12, 0, 0, 0, False)
    assert yup.Time(1970, 1, 1, 12, 0, 0, 0, False) + yup.RelativeTime.days(365) == yup.Time(1971, 1, 1, 12, 0, 0, 0, False)
    assert yup.Time(2038, 1, 1, 12, 0, 0, 0, False) + yup.RelativeTime.days(365) == yup.Time(2039, 1, 1, 12, 0, 0, 0, False)

