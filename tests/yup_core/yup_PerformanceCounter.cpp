/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include <yup_core/yup_core.h>

using namespace yup;

namespace
{

class PerfTestLogger : public Logger
{
public:
    void logMessage (const String& message) override
    {
        const ScopedLock sl (lock);
        messages.add (message);
    }

    StringArray getMessages() const
    {
        const ScopedLock sl (lock);
        return messages;
    }

private:
    mutable CriticalSection lock;
    StringArray messages;
};

} // namespace

// =============================================================================
// PerformanceCounter::Statistics Tests
// =============================================================================

TEST (PerformanceCounterStatisticsTests, DefaultConstructionHasZeroValues)
{
    PerformanceCounter::Statistics stats;
    EXPECT_EQ (stats.numRuns, 0);
    EXPECT_EQ (stats.averageSeconds, 0.0);
    EXPECT_EQ (stats.minimumSeconds, 0.0);
    EXPECT_EQ (stats.maximumSeconds, 0.0);
    EXPECT_EQ (stats.totalSeconds, 0.0);
    EXPECT_TRUE (stats.name.isEmpty());
}

TEST (PerformanceCounterStatisticsTests, ClearResetsAllNumericValues)
{
    PerformanceCounter::Statistics stats;
    stats.name = "keep";
    stats.addResult (0.1);
    stats.addResult (0.2);

    stats.clear();

    EXPECT_EQ (stats.numRuns, 0);
    EXPECT_EQ (stats.averageSeconds, 0.0);
    EXPECT_EQ (stats.minimumSeconds, 0.0);
    EXPECT_EQ (stats.maximumSeconds, 0.0);
    EXPECT_EQ (stats.totalSeconds, 0.0);
}

TEST (PerformanceCounterStatisticsTests, AddFirstResultSetsBothMinAndMax)
{
    PerformanceCounter::Statistics stats;
    stats.addResult (0.05);

    EXPECT_EQ (stats.numRuns, 1);
    EXPECT_DOUBLE_EQ (stats.totalSeconds, 0.05);
    EXPECT_DOUBLE_EQ (stats.minimumSeconds, 0.05);
    EXPECT_DOUBLE_EQ (stats.maximumSeconds, 0.05);
}

TEST (PerformanceCounterStatisticsTests, AddMultipleResultsTracksMinMaxAndTotal)
{
    PerformanceCounter::Statistics stats;
    stats.addResult (0.3);
    stats.addResult (0.1);
    stats.addResult (0.5);
    stats.addResult (0.2);

    EXPECT_EQ (stats.numRuns, 4);
    EXPECT_DOUBLE_EQ (stats.minimumSeconds, 0.1);
    EXPECT_DOUBLE_EQ (stats.maximumSeconds, 0.5);
    EXPECT_DOUBLE_EQ (stats.totalSeconds, 1.1);
}

TEST (PerformanceCounterStatisticsTests, AddResultIncrementsRunCount)
{
    PerformanceCounter::Statistics stats;
    for (int i = 1; i <= 10; ++i)
    {
        stats.addResult (0.001 * i);
        EXPECT_EQ (stats.numRuns, i);
    }
}

TEST (PerformanceCounterStatisticsTests, ToStringContainsNameAndRunCount)
{
    PerformanceCounter::Statistics stats;
    stats.name = "MyBenchmark";
    stats.addResult (0.001);

    auto str = stats.toString();
    EXPECT_TRUE (str.contains ("MyBenchmark"));
    EXPECT_TRUE (str.contains ("1"));
}

TEST (PerformanceCounterStatisticsTests, ToStringContainsTimingKeywords)
{
    PerformanceCounter::Statistics stats;
    stats.name = "bench";
    stats.addResult (0.001);

    auto str = stats.toString();
    EXPECT_TRUE (str.contains ("Average") || str.contains ("average"));
    EXPECT_TRUE (str.contains ("minimum") || str.contains ("Minimum"));
    EXPECT_TRUE (str.contains ("maximum") || str.contains ("Maximum"));
}

// =============================================================================
// PerformanceCounter Tests
// =============================================================================

class PerformanceCounterTests : public ::testing::Test
{
protected:
    void TearDown() override
    {
        Logger::setCurrentLogger (nullptr);
    }
};

TEST_F (PerformanceCounterTests, StopReturnsFalseBeforeReachingRunsPerPrint)
{
    PerformanceCounter pc ("test", 5);

    for (int i = 0; i < 4; ++i)
    {
        pc.start();
        EXPECT_FALSE (pc.stop());
    }

    pc.getStatisticsAndReset();
}

TEST_F (PerformanceCounterTests, StopReturnsTrueWhenRunsPerPrintIsReached)
{
    PerformanceCounter pc ("test", 3);

    pc.start();
    EXPECT_FALSE (pc.stop());
    pc.start();
    EXPECT_FALSE (pc.stop());
    pc.start();
    EXPECT_TRUE (pc.stop());
}

TEST_F (PerformanceCounterTests, StopResetsCountAfterTriggeringPrint)
{
    PerformanceCounter pc ("test", 2);

    pc.start();
    pc.stop();
    pc.start();
    EXPECT_TRUE (pc.stop());

    pc.start();
    EXPECT_FALSE (pc.stop());

    pc.getStatisticsAndReset();
}

TEST_F (PerformanceCounterTests, GetStatisticsAndResetReturnsAccumulatedData)
{
    PerformanceCounter pc ("test", 100);

    for (int i = 0; i < 5; ++i)
    {
        pc.start();
        pc.stop();
    }

    auto stats = pc.getStatisticsAndReset();
    EXPECT_EQ (stats.numRuns, 5);
    EXPECT_GE (stats.totalSeconds, 0.0);
    EXPECT_GE (stats.minimumSeconds, 0.0);
    EXPECT_GE (stats.maximumSeconds, stats.minimumSeconds);
}

TEST_F (PerformanceCounterTests, GetStatisticsAndResetClearsInternalState)
{
    PerformanceCounter pc ("test", 100);

    pc.start();
    pc.stop();

    pc.getStatisticsAndReset();

    auto afterReset = pc.getStatisticsAndReset();
    EXPECT_EQ (afterReset.numRuns, 0);
}

TEST_F (PerformanceCounterTests, AverageEqualsToTotalDividedByNumRuns)
{
    PerformanceCounter pc ("test", 100);

    for (int i = 0; i < 4; ++i)
    {
        pc.start();
        pc.stop();
    }

    auto stats = pc.getStatisticsAndReset();
    EXPECT_EQ (stats.numRuns, 4);
    EXPECT_NEAR (stats.averageSeconds, stats.totalSeconds / 4.0, 1e-12);
}

TEST_F (PerformanceCounterTests, MeasuredTimesAreNonNegative)
{
    PerformanceCounter pc ("test", 100);

    for (int i = 0; i < 10; ++i)
    {
        pc.start();
        pc.stop();
    }

    auto stats = pc.getStatisticsAndReset();
    EXPECT_GE (stats.minimumSeconds, 0.0);
    EXPECT_GE (stats.maximumSeconds, 0.0);
    EXPECT_GE (stats.totalSeconds, 0.0);
    EXPECT_GE (stats.maximumSeconds, stats.minimumSeconds);
    EXPECT_GE (stats.totalSeconds, stats.maximumSeconds);
}

TEST_F (PerformanceCounterTests, PrintStatisticsLogsToCurrentLogger)
{
    PerfTestLogger logger;
    Logger::setCurrentLogger (&logger);

    {
        PerformanceCounter pc ("BenchmarkName", 100);
        pc.start();
        pc.stop();
        pc.printStatistics();
    }

    Logger::setCurrentLogger (nullptr);

    auto messages = logger.getMessages();
    bool found = false;
    for (int i = 0; i < messages.size(); ++i)
        found = found || messages[i].contains ("BenchmarkName");

    EXPECT_TRUE (found);
}

TEST_F (PerformanceCounterTests, FileLoggingWritesStatsToFile)
{
    auto tempFile = File::getSpecialLocation (File::tempDirectory)
                        .getChildFile ("YUP_PerfTest_" + String::toHexString (Random::getSystemRandom().nextInt()) + ".log");

    {
        PerformanceCounter pc ("FileTarget", 2, tempFile);
        pc.start();
        pc.stop();
        pc.start();
        pc.stop();
    }

    auto content = tempFile.loadFileAsString();
    EXPECT_TRUE (content.contains ("FileTarget"));

    tempFile.deleteFile();
}

// =============================================================================
// ScopedTimeMeasurement Tests
// =============================================================================

TEST (ScopedTimeMeasurementTests, ConstructorInitializesResultToZero)
{
    double result = 99.0;
    {
        ScopedTimeMeasurement m (result);
        EXPECT_EQ (result, 0.0);
    }
}

TEST (ScopedTimeMeasurementTests, DestructorPopulatesResultWithElapsedTime)
{
    double result = 0.0;
    {
        ScopedTimeMeasurement m (result);
    }

    EXPECT_GE (result, 0.0);
}

TEST (ScopedTimeMeasurementTests, ResultIsInSecondsRange)
{
    double result = 0.0;
    {
        ScopedTimeMeasurement m (result);
    }

    // An empty scope should complete in well under 1 second
    EXPECT_GE (result, 0.0);
    EXPECT_LT (result, 1.0);
}

TEST (ScopedTimeMeasurementTests, SequentialScopesDontInterfere)
{
    double result1 = -1.0;
    double result2 = -1.0;

    {
        ScopedTimeMeasurement m (result1);
    }

    {
        ScopedTimeMeasurement m (result2);
    }

    EXPECT_GE (result1, 0.0);
    EXPECT_GE (result2, 0.0);
}

TEST (ScopedTimeMeasurementTests, SleepingScopeYieldsLargerResult)
{
    double shortResult = 0.0;
    double longResult = 0.0;

    {
        ScopedTimeMeasurement m (shortResult);
    }

    {
        ScopedTimeMeasurement m (longResult);
        Thread::sleep (5);
    }

    EXPECT_GT (longResult, shortResult);
    EXPECT_GE (longResult, 0.003);
}
