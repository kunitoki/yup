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

namespace yup
{

//==============================================================================

namespace
{

double extractTime (const PaintProfileSample& sample, PaintProfileTimeKind kind)
{
    switch (kind)
    {
        case PaintProfileTimeKind::self:
            return sample.selfMicros;
        case PaintProfileTimeKind::children:
            return sample.childrenMicros;
        case PaintProfileTimeKind::framework:
            return sample.frameworkMicros;
        case PaintProfileTimeKind::total:
            return sample.totalMicros;
    }

    return sample.totalMicros;
}

double percentileFromSorted (const std::vector<double>& sorted, double p)
{
    if (sorted.empty())
        return 0.0;

    const int n = static_cast<int> (sorted.size());
    const int index = std::max (0, static_cast<int> (std::ceil (p / 100.0 * n)) - 1);
    return sorted[static_cast<std::size_t> (index)];
}

} // namespace

//==============================================================================

PaintProfileStats::PaintProfileStats (PaintProfileOptions options)
    : options (options)
{
    samples.resize (static_cast<std::size_t> (options.sampleCapacity));
    nextSample = 0;
    sampleCount = 0;
}

//==============================================================================

void PaintProfileStats::recordSample (const PaintProfileSample& sample)
{
    if (options.minimumSampleMicros > 0.0 && sample.totalMicros < options.minimumSampleMicros)
        return;

    samples[static_cast<std::size_t> (nextSample)] = sample;
    nextSample = (nextSample + 1) % options.sampleCapacity;
    sampleCount = std::min (sampleCount + 1, options.sampleCapacity);
}

void PaintProfileStats::reset()
{
    nextSample = 0;
    sampleCount = 0;
}

//==============================================================================

int PaintProfileStats::getSampleCount() const
{
    return sampleCount;
}

int PaintProfileStats::getCapacity() const
{
    return options.sampleCapacity;
}

PaintProfileOptions PaintProfileStats::getOptions() const
{
    return options;
}

//==============================================================================

std::vector<PaintProfileSample> PaintProfileStats::copySamples() const
{
    std::vector<PaintProfileSample> result;
    result.reserve (static_cast<std::size_t> (sampleCount));

    if (sampleCount < options.sampleCapacity)
    {
        for (int i = 0; i < sampleCount; ++i)
            result.push_back (samples[static_cast<std::size_t> (i)]);
    }
    else
    {
        for (int i = 0; i < options.sampleCapacity; ++i)
        {
            const int index = (nextSample + i) % options.sampleCapacity;
            result.push_back (samples[static_cast<std::size_t> (index)]);
        }
    }

    return result;
}

PaintProfileSample PaintProfileStats::getLastSample() const
{
    if (sampleCount == 0)
        return {};

    const int index = (nextSample - 1 + options.sampleCapacity) % options.sampleCapacity;
    return samples[static_cast<std::size_t> (index)];
}

//==============================================================================

PaintProfileSummary PaintProfileStats::summarize (PaintProfileTimeKind kind) const
{
    const auto all = copySamples();

    PaintProfileSummary summary;
    summary.sampleCount = static_cast<int> (all.size());

    if (all.empty())
        return summary;

    std::vector<double> values;
    values.reserve (all.size());

    double sum = 0.0;
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();

    for (const auto& s : all)
    {
        const double v = extractTime (s, kind);
        values.push_back (v);
        sum += v;
        minVal = std::min (minVal, v);
        maxVal = std::max (maxVal, v);
    }

    summary.lastMicros = values.back();
    summary.minMicros = minVal;
    summary.maxMicros = maxVal;
    summary.meanMicros = sum / static_cast<double> (values.size());

    std::vector<double> sorted = values;
    std::sort (sorted.begin(), sorted.end());

    summary.p50Micros = percentileFromSorted (sorted, 50.0);
    summary.p95Micros = percentileFromSorted (sorted, 95.0);
    summary.p99Micros = percentileFromSorted (sorted, 99.0);

    return summary;
}

PaintProfileHistogram PaintProfileStats::createHistogram (PaintProfileTimeKind kind, int numBuckets) const
{
    jassert (numBuckets >= 1);
    numBuckets = std::max (1, numBuckets);

    const auto all = copySamples();

    PaintProfileHistogram histogram;
    histogram.rangeMinMicros = 0.0;
    histogram.buckets.assign (static_cast<std::size_t> (numBuckets), 0);

    if (all.empty())
    {
        histogram.rangeMaxMicros = 100.0;
        return histogram;
    }

    std::vector<double> values;
    values.reserve (all.size());

    double maxObserved = 0.0;
    for (const auto& s : all)
    {
        const double v = extractTime (s, kind);
        values.push_back (v);
        maxObserved = std::max (maxObserved, v);
    }

    std::vector<double> sorted = values;
    std::sort (sorted.begin(), sorted.end());
    const double p99 = percentileFromSorted (sorted, 99.0);

    histogram.rangeMaxMicros = std::max ({ p99 * 1.25, maxObserved, 100.0 });

    const double bucketWidth = histogram.rangeMaxMicros / static_cast<double> (numBuckets);

    for (const double v : values)
    {
        int bucket = static_cast<int> (v / bucketWidth);
        bucket = std::min (bucket, numBuckets - 1);
        ++histogram.buckets[static_cast<std::size_t> (bucket)];
    }

    return histogram;
}

} // namespace yup
