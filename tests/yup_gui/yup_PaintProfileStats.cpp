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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

PaintProfileSample makeSample (double totalMicros,
                               double selfMicros = 0.0,
                               double childrenMicros = 0.0)
{
    PaintProfileSample s;
    s.totalMicros = totalMicros;
    s.selfMicros = selfMicros;
    s.childrenMicros = childrenMicros;
    s.frameworkMicros = totalMicros - selfMicros - childrenMicros;
    return s;
}

} // namespace

TEST (PaintProfileStatsTests, InitialStateIsEmpty)
{
    PaintProfileStats stats;
    EXPECT_EQ (0, stats.getSampleCount());
    EXPECT_EQ (300, stats.getCapacity());
    EXPECT_EQ (0, stats.getLastSample().totalMicros);
}

TEST (PaintProfileStatsTests, CapacityFromOptions)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 50;
    PaintProfileStats stats (opts);
    EXPECT_EQ (50, stats.getCapacity());
}

TEST (PaintProfileStatsTests, OptionsPreserved)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 100;
    opts.minimumSampleMicros = 5.0;
    opts.includeBounds = false;
    PaintProfileStats stats (opts);
    auto stored = stats.getOptions();
    EXPECT_EQ (100, stored.sampleCapacity);
    EXPECT_DOUBLE_EQ (5.0, stored.minimumSampleMicros);
    EXPECT_FALSE (stored.includeBounds);
}

TEST (PaintProfileStatsTests, RecordingBelowCapacity)
{
    PaintProfileStats stats;
    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));

    EXPECT_EQ (3, stats.getSampleCount());

    auto samples = stats.copySamples();
    ASSERT_EQ (3u, samples.size());
    EXPECT_DOUBLE_EQ (10.0, samples[0].totalMicros);
    EXPECT_DOUBLE_EQ (20.0, samples[1].totalMicros);
    EXPECT_DOUBLE_EQ (30.0, samples[2].totalMicros);
}

TEST (PaintProfileStatsTests, LastSampleReturnsNewest)
{
    PaintProfileStats stats;
    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));

    EXPECT_DOUBLE_EQ (30.0, stats.getLastSample().totalMicros);
}

TEST (PaintProfileStatsTests, RingBufferWrapsAtCapacity)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 5;
    PaintProfileStats stats (opts);

    for (int i = 1; i <= 10; ++i)
        stats.recordSample (makeSample (static_cast<double> (i) * 10.0));

    EXPECT_EQ (5, stats.getSampleCount());

    auto samples = stats.copySamples();
    ASSERT_EQ (5u, samples.size());
    EXPECT_DOUBLE_EQ (60.0, samples[0].totalMicros);
    EXPECT_DOUBLE_EQ (70.0, samples[1].totalMicros);
    EXPECT_DOUBLE_EQ (80.0, samples[2].totalMicros);
    EXPECT_DOUBLE_EQ (90.0, samples[3].totalMicros);
    EXPECT_DOUBLE_EQ (100.0, samples[4].totalMicros);
}

TEST (PaintProfileStatsTests, LastSampleAfterWrapping)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 3;
    PaintProfileStats stats (opts);

    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));
    stats.recordSample (makeSample (40.0));
    stats.recordSample (makeSample (50.0));

    EXPECT_DOUBLE_EQ (50.0, stats.getLastSample().totalMicros);
}

TEST (PaintProfileStatsTests, ResetClearsSamples)
{
    PaintProfileStats stats;
    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));

    stats.reset();

    EXPECT_EQ (0, stats.getSampleCount());
    EXPECT_EQ (300, stats.getCapacity());
    EXPECT_EQ (0, stats.getLastSample().totalMicros);
}

TEST (PaintProfileStatsTests, ResetDoesNotChangeCapacity)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 50;
    PaintProfileStats stats (opts);
    stats.recordSample (makeSample (10.0));

    stats.reset();

    EXPECT_EQ (50, stats.getCapacity());
}

TEST (PaintProfileStatsTests, MinimumSampleThresholdFilters)
{
    PaintProfileOptions opts;
    opts.minimumSampleMicros = 100.0;
    PaintProfileStats stats (opts);

    stats.recordSample (makeSample (50.0));
    EXPECT_EQ (0, stats.getSampleCount());

    stats.recordSample (makeSample (100.0));
    EXPECT_EQ (1, stats.getSampleCount());

    stats.recordSample (makeSample (200.0));
    EXPECT_EQ (2, stats.getSampleCount());

    stats.recordSample (makeSample (99.9));
    EXPECT_EQ (2, stats.getSampleCount());
}

TEST (PaintProfileStatsTests, SummarizeEmptyStatsReturnsZeros)
{
    PaintProfileStats stats;

    auto summary = stats.summarize (PaintProfileTimeKind::total);

    EXPECT_EQ (0, summary.sampleCount);
    EXPECT_DOUBLE_EQ (0.0, summary.lastMicros);
    EXPECT_DOUBLE_EQ (0.0, summary.minMicros);
    EXPECT_DOUBLE_EQ (0.0, summary.maxMicros);
    EXPECT_DOUBLE_EQ (0.0, summary.meanMicros);
    EXPECT_DOUBLE_EQ (0.0, summary.p50Micros);
    EXPECT_DOUBLE_EQ (0.0, summary.p95Micros);
    EXPECT_DOUBLE_EQ (0.0, summary.p99Micros);
}

TEST (PaintProfileStatsTests, SummarizeTotalMicros)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));
    stats.recordSample (makeSample (40.0));
    stats.recordSample (makeSample (50.0));

    auto summary = stats.summarize (PaintProfileTimeKind::total);

    EXPECT_EQ (5, summary.sampleCount);
    EXPECT_DOUBLE_EQ (50.0, summary.lastMicros);
    EXPECT_DOUBLE_EQ (10.0, summary.minMicros);
    EXPECT_DOUBLE_EQ (50.0, summary.maxMicros);
    EXPECT_DOUBLE_EQ (30.0, summary.meanMicros);
    EXPECT_DOUBLE_EQ (30.0, summary.p50Micros);
    EXPECT_DOUBLE_EQ (50.0, summary.p95Micros);
    EXPECT_DOUBLE_EQ (50.0, summary.p99Micros);
}

TEST (PaintProfileStatsTests, SummarizeSelfMicros)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (100.0, 10.0, 50.0));
    stats.recordSample (makeSample (100.0, 20.0, 50.0));
    stats.recordSample (makeSample (100.0, 30.0, 50.0));
    stats.recordSample (makeSample (100.0, 40.0, 50.0));
    stats.recordSample (makeSample (100.0, 50.0, 50.0));

    auto summary = stats.summarize (PaintProfileTimeKind::self);

    EXPECT_EQ (5, summary.sampleCount);
    EXPECT_DOUBLE_EQ (50.0, summary.lastMicros);
    EXPECT_DOUBLE_EQ (10.0, summary.minMicros);
    EXPECT_DOUBLE_EQ (50.0, summary.maxMicros);
    EXPECT_DOUBLE_EQ (30.0, summary.meanMicros);
}

TEST (PaintProfileStatsTests, SummarizeChildrenMicros)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (100.0, 40.0, 10.0));
    stats.recordSample (makeSample (100.0, 40.0, 20.0));
    stats.recordSample (makeSample (100.0, 40.0, 30.0));
    stats.recordSample (makeSample (100.0, 40.0, 40.0));
    stats.recordSample (makeSample (100.0, 40.0, 50.0));

    auto summary = stats.summarize (PaintProfileTimeKind::children);

    EXPECT_EQ (5, summary.sampleCount);
    EXPECT_DOUBLE_EQ (50.0, summary.lastMicros);
    EXPECT_DOUBLE_EQ (10.0, summary.minMicros);
    EXPECT_DOUBLE_EQ (50.0, summary.maxMicros);
    EXPECT_DOUBLE_EQ (30.0, summary.meanMicros);
}

TEST (PaintProfileStatsTests, SummarizeFrameworkMicros)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (100.0, 50.0, 50.0));
    stats.recordSample (makeSample (100.0, 50.0, 40.0));
    stats.recordSample (makeSample (100.0, 50.0, 30.0));
    stats.recordSample (makeSample (100.0, 50.0, 20.0));
    stats.recordSample (makeSample (100.0, 50.0, 10.0));

    auto summary = stats.summarize (PaintProfileTimeKind::framework);

    EXPECT_EQ (5, summary.sampleCount);
    EXPECT_DOUBLE_EQ (40.0, summary.lastMicros);
    EXPECT_DOUBLE_EQ (0.0, summary.minMicros);
    EXPECT_DOUBLE_EQ (40.0, summary.maxMicros);
}

TEST (PaintProfileStatsTests, CreateHistogramEmpty)
{
    PaintProfileStats stats;

    auto histogram = stats.createHistogram (PaintProfileTimeKind::total, 10);

    EXPECT_DOUBLE_EQ (0.0, histogram.rangeMinMicros);
    EXPECT_DOUBLE_EQ (100.0, histogram.rangeMaxMicros);
    ASSERT_EQ (10u, histogram.buckets.size());
    for (int count : histogram.buckets)
        EXPECT_EQ (0, count);
}

TEST (PaintProfileStatsTests, CreateHistogramCoversSamples)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));
    stats.recordSample (makeSample (40.0));
    stats.recordSample (makeSample (50.0));
    stats.recordSample (makeSample (60.0));
    stats.recordSample (makeSample (70.0));
    stats.recordSample (makeSample (80.0));
    stats.recordSample (makeSample (90.0));
    stats.recordSample (makeSample (100.0));

    auto histogram = stats.createHistogram (PaintProfileTimeKind::total, 10);

    EXPECT_DOUBLE_EQ (0.0, histogram.rangeMinMicros);
    EXPECT_GT (histogram.rangeMaxMicros, 0.0);
    ASSERT_EQ (10u, histogram.buckets.size());

    int totalCount = 0;
    for (int count : histogram.buckets)
        totalCount += count;
    EXPECT_EQ (10, totalCount);
}

TEST (PaintProfileStatsTests, CreateHistogramWithSelfMicros)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (100.0, 10.0, 40.0));
    stats.recordSample (makeSample (100.0, 20.0, 40.0));
    stats.recordSample (makeSample (100.0, 30.0, 40.0));
    stats.recordSample (makeSample (100.0, 40.0, 40.0));
    stats.recordSample (makeSample (100.0, 50.0, 40.0));

    auto histogram = stats.createHistogram (PaintProfileTimeKind::self, 5);

    ASSERT_EQ (5u, histogram.buckets.size());
    int totalCount = 0;
    for (int count : histogram.buckets)
        totalCount += count;
    EXPECT_EQ (5, totalCount);
}

TEST (PaintProfileStatsTests, CopySamplesOrderAfterWrapping)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 3;
    PaintProfileStats stats (opts);

    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));
    stats.recordSample (makeSample (40.0));
    stats.recordSample (makeSample (50.0));
    stats.recordSample (makeSample (60.0));

    auto samples = stats.copySamples();

    ASSERT_EQ (3u, samples.size());
    EXPECT_DOUBLE_EQ (40.0, samples[0].totalMicros);
    EXPECT_DOUBLE_EQ (50.0, samples[1].totalMicros);
    EXPECT_DOUBLE_EQ (60.0, samples[2].totalMicros);
}

TEST (PaintProfileStatsTests, SingleSampleStatistics)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (42.0));

    auto summary = stats.summarize (PaintProfileTimeKind::total);

    EXPECT_EQ (1, summary.sampleCount);
    EXPECT_DOUBLE_EQ (42.0, summary.lastMicros);
    EXPECT_DOUBLE_EQ (42.0, summary.minMicros);
    EXPECT_DOUBLE_EQ (42.0, summary.maxMicros);
    EXPECT_DOUBLE_EQ (42.0, summary.meanMicros);
    EXPECT_DOUBLE_EQ (42.0, summary.p50Micros);
    EXPECT_DOUBLE_EQ (42.0, summary.p95Micros);
    EXPECT_DOUBLE_EQ (42.0, summary.p99Micros);
}

TEST (PaintProfileStatsTests, ResetAfterWrapping)
{
    PaintProfileOptions opts;
    opts.sampleCapacity = 2;
    PaintProfileStats stats (opts);

    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));

    stats.reset();

    EXPECT_EQ (0, stats.getSampleCount());
    auto samples = stats.copySamples();
    EXPECT_TRUE (samples.empty());
}

TEST (PaintProfileStatsTests, MinimumThresholdZeroRecordsAll)
{
    PaintProfileOptions opts;
    opts.minimumSampleMicros = 0.0;
    PaintProfileStats stats (opts);

    stats.recordSample (makeSample (0.0));
    stats.recordSample (makeSample (0.001));
    stats.recordSample (makeSample (1000.0));

    EXPECT_EQ (3, stats.getSampleCount());
}

TEST (PaintProfileStatsTests, HistogramBucketCountMatches)
{
    PaintProfileStats stats;

    for (int i = 1; i <= 50; ++i)
        stats.recordSample (makeSample (static_cast<double> (i)));

    auto hist5 = stats.createHistogram (PaintProfileTimeKind::total, 5);
    auto hist20 = stats.createHistogram (PaintProfileTimeKind::total, 20);

    EXPECT_EQ (5u, hist5.buckets.size());
    EXPECT_EQ (20u, hist20.buckets.size());
}

TEST (PaintProfileStatsTests, SummarizePercentileOrdering)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));
    stats.recordSample (makeSample (30.0));
    stats.recordSample (makeSample (40.0));
    stats.recordSample (makeSample (50.0));

    auto summary = stats.summarize (PaintProfileTimeKind::total);

    EXPECT_LE (summary.minMicros, summary.p50Micros);
    EXPECT_LE (summary.p50Micros, summary.p95Micros);
    EXPECT_LE (summary.p95Micros, summary.p99Micros);
    EXPECT_LE (summary.p99Micros, summary.maxMicros);
}

TEST (PaintProfileStatsTests, CopySamplesDoesNotModifyState)
{
    PaintProfileStats stats;

    stats.recordSample (makeSample (10.0));
    stats.recordSample (makeSample (20.0));

    auto first = stats.copySamples();
    auto second = stats.copySamples();

    ASSERT_EQ (first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i)
        EXPECT_DOUBLE_EQ (first[i].totalMicros, second[i].totalMicros);
}
