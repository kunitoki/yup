/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

using namespace yup;

TEST (TimeTests, DefaultConstructor)
{
    Time time;
    EXPECT_EQ (time.toMilliseconds(), 0);
}

TEST (TimeTests, MillisecondsConstructor)
{
    int64 millis = 1625000000000;
    Time time (millis);
    EXPECT_EQ (time.toMilliseconds(), millis);
}

TEST (TimeTests, DISABLED_DateComponentsConstructorUTC)
{
    Time time (2022, 11, 1, 19, 50, 50, 111, false);
    EXPECT_EQ (time.getYear(), 2022);
    EXPECT_EQ (time.getMonth(), 11);
    EXPECT_EQ (time.getDayOfMonth(), 1);
    EXPECT_EQ (time.getHours(), 20);
    EXPECT_EQ (time.getMinutes(), 50);
    EXPECT_EQ (time.getSeconds(), 50);
    EXPECT_EQ (time.getMilliseconds(), 111);
}

TEST (TimeTests, DateComponentsConstructorLocalTime)
{
    Time time (2022, 11, 31, 23, 59, 59, 999, true);
    EXPECT_EQ (time.getYear(), 2022);
    EXPECT_EQ (time.getMonth(), 11);
    EXPECT_EQ (time.getDayOfMonth(), 31);
    EXPECT_EQ (time.getHours(), 23);
    EXPECT_EQ (time.getMinutes(), 59);
    EXPECT_EQ (time.getSeconds(), 59);
    EXPECT_EQ (time.getMilliseconds(), 999);
}

TEST (TimeTests, GetCurrentTime)
{
    Time now = Time::getCurrentTime();
    EXPECT_GT (now.toMilliseconds(), 0);
}

TEST (TimeTests, GetYear)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getYear(), 2021);
}

TEST (TimeTests, GetMonth)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getMonth(), 5); // Months are 0-based, so 5 is June
}

TEST (TimeTests, GetMonthName)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getMonthName (false), "June");
    EXPECT_EQ (time.getMonthName (true), "Jun");
}

TEST (TimeTests, GetDayOfMonth)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getDayOfMonth(), 29);
}

TEST (TimeTests, GetDayOfWeek)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getDayOfWeek(), 2); // 0 = Sunday, 2 = Tuesday
}

TEST (TimeTests, GetDayOfYear)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getDayOfYear(), 179); // June 29th is the 179th day of the year
}

TEST (TimeTests, GetWeekdayName)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getWeekdayName (false), "Tuesday");
    EXPECT_EQ (time.getWeekdayName (true), "Tue");
}

TEST (TimeTests, DISABLED_GetHours)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getHours(), 22); // 10 PM UTC
}

TEST (TimeTests, IsAfternoon)
{
    Time morning (1624970400000);   // 8:00 AM UTC
    Time afternoon (1625013600000); // 8:00 PM UTC
    EXPECT_TRUE (morning.isAfternoon());
    EXPECT_FALSE (afternoon.isAfternoon());
}

TEST (TimeTests, DISABLED_GetHoursInAmPmFormat)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getHoursInAmPmFormat(), 10); // 10 AM
}

TEST (TimeTests, GetMinutes)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getMinutes(), 53);
}

TEST (TimeTests, GetSeconds)
{
    Time time (1625000000000);
    EXPECT_EQ (time.getSeconds(), 20);
}

TEST (TimeTests, GetMilliseconds)
{
    Time time (1625000000123);
    EXPECT_EQ (time.getMilliseconds(), 123);
}

TEST (TimeTests, DISABLED_IsDaylightSavingTime)
{
    Time time (1625000000000);
    EXPECT_FALSE (time.isDaylightSavingTime());
}

TEST (TimeTests, GetTimeZone)
{
    Time time (1625000000000);
    EXPECT_FALSE (time.getTimeZone().isEmpty());
}

TEST (TimeTests, DISABLED_GetUTCOffsetSeconds)
{
    Time time (1625000000000);
    EXPECT_NE (time.getUTCOffsetSeconds(), 0);
}

TEST (TimeTests, GetUTCOffsetString)
{
    Time time (1625000000000);
    EXPECT_FALSE (time.getUTCOffsetString (true).isEmpty());
    EXPECT_FALSE (time.getUTCOffsetString (false).isEmpty());
}

TEST (TimeTests, ToString)
{
    Time time (1625000000000);
    EXPECT_FALSE (time.toString (true, true).isEmpty());
}

/*
TEST (TimeTests, DISABLED_Formatted) // WASM undefined symbol: wcsftime
{
    Time time(1625000000000);
    EXPECT_EQ(time.formatted("%Y-%m-%d %H:%M:%S"), "2021-06-29 22:53:20");
}
*/

TEST (TimeTests, ToISO8601)
{
    Time time (1625000000000);
    EXPECT_FALSE (time.toISO8601 (true).isEmpty());
}

TEST (TimeTests, FromISO8601)
{
    Time time = Time::fromISO8601 ("2021-06-29T10:00:00Z");
    EXPECT_EQ (time.getYear(), 2021);
    EXPECT_EQ (time.getMonth(), 5); // June
    EXPECT_EQ (time.getDayOfMonth(), 29);
}

TEST (TimeTests, AddRelativeTime)
{
    Time time (1625000000000);
    RelativeTime delta (60.0); // 1 minute
    time += delta;
    EXPECT_EQ (time.getMinutes(), 54);
    EXPECT_EQ (time.getSeconds(), 20);
}

TEST (TimeTests, SubtractRelativeTime)
{
    Time time (1625000000000);
    RelativeTime delta (60.0); // 1 minute
    time -= delta;
    EXPECT_EQ (time.getMinutes(), 52);
    EXPECT_EQ (time.getSeconds(), 20);
}

TEST (TimeTests, ComparisonOperators)
{
    Time time1 (1625000000000);
    Time time2 (1625000000000);
    Time time3 (1625000000001);

    EXPECT_EQ (time1, time2);
    EXPECT_NE (time1, time3);
    EXPECT_LT (time1, time3);
    EXPECT_LE (time1, time3);
    EXPECT_GT (time3, time1);
    EXPECT_GE (time3, time1);
}

TEST (TimeTests, GetMillisecondCounter)
{
    uint32 millis1 = Time::getMillisecondCounter();
    EXPECT_GE (millis1, 0);
    Time::waitForMillisecondCounter (millis1 + 100);
    uint32 millis2 = Time::getMillisecondCounter();
    EXPECT_GT (millis2, millis1);
}

TEST (TimeTests, GetMillisecondCounterHiRes)
{
    double hiResMillis1 = Time::getMillisecondCounterHiRes();
    EXPECT_GE (hiResMillis1, 0.0);
    Time::waitForMillisecondCounter (static_cast<uint32> (hiResMillis1) + 100);
    double hiResMillis2 = Time::getMillisecondCounterHiRes();
    EXPECT_GT (hiResMillis2, hiResMillis1);
}

TEST (TimeTests, GetApproximateMillisecondCounter)
{
    uint32 approxMillis1 = Time::getApproximateMillisecondCounter();
    EXPECT_GE (approxMillis1, 0);
    Time::waitForMillisecondCounter (approxMillis1 + 100);
    uint32 approxMillis2 = Time::getApproximateMillisecondCounter();
    EXPECT_GT (approxMillis2, approxMillis1);
}

TEST (TimeTests, GetHighResolutionTicks)
{
    int64 ticks1 = Time::getHighResolutionTicks();
    EXPECT_GT (ticks1, 0);
}

TEST (TimeTests, GetHighResolutionTicksPerSecond)
{
    int64 ticksPerSecond = Time::getHighResolutionTicksPerSecond();
    EXPECT_GT (ticksPerSecond, 0);
}

TEST (TimeTests, HighResolutionTicksToSeconds)
{
    int64 ticks = Time::getHighResolutionTicks();
    double seconds = Time::highResolutionTicksToSeconds (ticks);
    EXPECT_GT (seconds, 0.0);
}

TEST (TimeTests, SecondsToHighResolutionTicks)
{
    double seconds = 1.0;
    int64 ticks = Time::secondsToHighResolutionTicks (seconds);
    EXPECT_GT (ticks, 0);
}

TEST (TimeTests, GetCompilationDate)
{
    Time compilationDate = Time::getCompilationDate();
    EXPECT_GT (compilationDate.toMilliseconds(), 0);
}

TEST (TimeTests, DISABLED_SetSystemTimeToThisTime)
{
    Time now = Time::getCurrentTime();
    // This test may fail if the system does not have sufficient privileges
    EXPECT_TRUE (now.setSystemTimeToThisTime());
}

TEST (TimeTests, xxx)
{
    Time t = Time::getCurrentTime();
    EXPECT_TRUE (t > Time());

    Thread::sleep (15);
    EXPECT_TRUE (Time::getCurrentTime() > t);

    EXPECT_TRUE (t.getTimeZone().isNotEmpty());
    EXPECT_TRUE (t.getUTCOffsetString (true) == "Z" || t.getUTCOffsetString (true).length() == 6);
    EXPECT_TRUE (t.getUTCOffsetString (false) == "Z" || t.getUTCOffsetString (false).length() == 5);

    //EXPECT_TRUE (TimeHelpers::getUTCOffsetString (-(3 * 60 + 15) * 60, true) == "-03:15");
    //EXPECT_TRUE (TimeHelpers::getUTCOffsetString (-(3 * 60 + 30) * 60, true) == "-03:30");
    //EXPECT_TRUE (TimeHelpers::getUTCOffsetString (-(3 * 60 + 45) * 60, true) == "-03:45");
    //EXPECT_TRUE (TimeHelpers::getUTCOffsetString ((3 * 60 + 15) * 60, true) == "+03:15");

    EXPECT_TRUE (Time::fromISO8601 (t.toISO8601 (true)) == t);
    EXPECT_TRUE (Time::fromISO8601 (t.toISO8601 (false)) == t);

    EXPECT_TRUE (Time::fromISO8601 ("2016-02-16") == Time (2016, 1, 16, 0, 0, 0, 0, false));
    EXPECT_TRUE (Time::fromISO8601 ("20160216Z") == Time (2016, 1, 16, 0, 0, 0, 0, false));

    EXPECT_TRUE (Time::fromISO8601 ("2016-02-16T15:03:57+00:00") == Time (2016, 1, 16, 15, 3, 57, 0, false));
    EXPECT_TRUE (Time::fromISO8601 ("20160216T150357+0000") == Time (2016, 1, 16, 15, 3, 57, 0, false));

    EXPECT_TRUE (Time::fromISO8601 ("2016-02-16T15:03:57.999+00:00") == Time (2016, 1, 16, 15, 3, 57, 999, false));
    EXPECT_TRUE (Time::fromISO8601 ("20160216T150357.999+0000") == Time (2016, 1, 16, 15, 3, 57, 999, false));
    EXPECT_TRUE (Time::fromISO8601 ("2016-02-16T15:03:57.999Z") == Time (2016, 1, 16, 15, 3, 57, 999, false));
    EXPECT_TRUE (Time::fromISO8601 ("2016-02-16T15:03:57,999Z") == Time (2016, 1, 16, 15, 3, 57, 999, false));
    EXPECT_TRUE (Time::fromISO8601 ("20160216T150357.999Z") == Time (2016, 1, 16, 15, 3, 57, 999, false));
    EXPECT_TRUE (Time::fromISO8601 ("20160216T150357,999Z") == Time (2016, 1, 16, 15, 3, 57, 999, false));

    EXPECT_TRUE (Time::fromISO8601 ("2016-02-16T15:03:57.999-02:30") == Time (2016, 1, 16, 17, 33, 57, 999, false));
    EXPECT_TRUE (Time::fromISO8601 ("2016-02-16T15:03:57,999-02:30") == Time (2016, 1, 16, 17, 33, 57, 999, false));
    EXPECT_TRUE (Time::fromISO8601 ("20160216T150357.999-0230") == Time (2016, 1, 16, 17, 33, 57, 999, false));
    EXPECT_TRUE (Time::fromISO8601 ("20160216T150357,999-0230") == Time (2016, 1, 16, 17, 33, 57, 999, false));

    EXPECT_TRUE (Time (1970, 0, 1, 0, 0, 0, 0, false) == Time (0));
    EXPECT_TRUE (Time (2106, 1, 7, 6, 28, 15, 0, false) == Time (4294967295000));
    EXPECT_TRUE (Time (2007, 10, 7, 1, 7, 20, 0, false) == Time (1194397640000));
    EXPECT_TRUE (Time (2038, 0, 19, 3, 14, 7, 0, false) == Time (2147483647000));
    EXPECT_TRUE (Time (2016, 2, 7, 11, 20, 8, 0, false) == Time (1457349608000));
    EXPECT_TRUE (Time (1969, 11, 31, 23, 59, 59, 0, false) == Time (-1000));
    EXPECT_TRUE (Time (1901, 11, 13, 20, 45, 53, 0, false) == Time (-2147483647000));

    EXPECT_TRUE (Time (1982, 1, 1, 12, 0, 0, 0, true) + RelativeTime::days (365) == Time (1983, 1, 1, 12, 0, 0, 0, true));
    EXPECT_TRUE (Time (1970, 1, 1, 12, 0, 0, 0, true) + RelativeTime::days (365) == Time (1971, 1, 1, 12, 0, 0, 0, true));
    EXPECT_TRUE (Time (2038, 1, 1, 12, 0, 0, 0, true) + RelativeTime::days (365) == Time (2039, 1, 1, 12, 0, 0, 0, true));

    EXPECT_TRUE (Time (1982, 1, 1, 12, 0, 0, 0, false) + RelativeTime::days (365) == Time (1983, 1, 1, 12, 0, 0, 0, false));
    EXPECT_TRUE (Time (1970, 1, 1, 12, 0, 0, 0, false) + RelativeTime::days (365) == Time (1971, 1, 1, 12, 0, 0, 0, false));
    EXPECT_TRUE (Time (2038, 1, 1, 12, 0, 0, 0, false) + RelativeTime::days (365) == Time (2039, 1, 1, 12, 0, 0, 0, false));
}
