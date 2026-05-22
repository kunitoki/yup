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

/** Collects and analyses PaintProfileSample records for a single component.

    Samples are stored in a fixed-capacity ring buffer whose size is determined by
    PaintProfileOptions::sampleCapacity. Once full, the oldest sample is silently
    overwritten by each new arrival.

    All query methods are const and operate on a snapshot of the ring buffer, so
    they are safe to call from any thread as long as no concurrent writes are in
    progress. If concurrent access is required, the caller is responsible for
    external synchronisation.
*/
class PaintProfileStats
{
public:
    //==============================================================================

    /** Constructs a PaintProfileStats with the given options.

        @param options  Controls capacity, filtering thresholds, and recording behaviour.
                        Defaults to a PaintProfileOptions with sampleCapacity = 300.
    */
    explicit PaintProfileStats (PaintProfileOptions options = {});

    //==============================================================================

    /** Records a new paint sample.

        If PaintProfileOptions::minimumSampleMicros is greater than zero and
        sample.totalMicros is below that threshold, the sample is silently discarded.
        Otherwise the sample overwrites the oldest entry in the ring buffer.

        @param sample  The sample to store.
    */
    void recordSample (const PaintProfileSample& sample);

    /** Clears all stored samples and resets internal counters to zero.

        The configured PaintProfileOptions are preserved; only the sample data is cleared.
    */
    void reset();

    //==============================================================================

    /** Returns the number of valid samples currently stored (at most getCapacity()). */
    int getSampleCount() const;

    /** Returns the maximum number of samples that can be stored concurrently. */
    int getCapacity() const;

    /** Returns the PaintProfileOptions that were supplied at construction time. */
    PaintProfileOptions getOptions() const;

    //==============================================================================

    /** Returns a copy of all valid samples in chronological order (oldest first).

        If fewer samples than the capacity have been recorded, only the recorded
        samples are returned. Once the buffer is full, iteration wraps correctly
        around the ring so that chronological order is preserved.

        @returns A vector containing getSampleCount() samples.
    */
    std::vector<PaintProfileSample> copySamples() const;

    /** Returns the most recently recorded sample.

        @returns The last sample, or a default-constructed PaintProfileSample if
                 no samples have been recorded yet.
    */
    PaintProfileSample getLastSample() const;

    //==============================================================================

    /** Computes a statistical summary for the specified time field across all stored samples.

        @param kind  Selects which time field (self, children, framework, or total)
                     is used when computing the statistics.
        @returns A PaintProfileSummary populated with min, max, mean, and percentile values.
    */
    PaintProfileSummary summarize (PaintProfileTimeKind kind) const;

    /** Builds a linear frequency histogram for the specified time field.

        The histogram range is derived automatically from the data: the upper bound is
        max(p99 * 1.25, maxObserved, 100.0) microseconds, and the range is divided into
        numBuckets equal-width buckets starting from 0. Values above the upper bound are
        clamped into the last bucket.

        @param kind        Selects which time field is histogrammed.
        @param numBuckets  Number of equal-width buckets to create. Must be >= 1.
        @returns A PaintProfileHistogram with buckets.size() == numBuckets.
    */
    PaintProfileHistogram createHistogram (PaintProfileTimeKind kind, int numBuckets) const;

private:
    std::vector<PaintProfileSample> samples;
    PaintProfileOptions options;
    int nextSample = 0;
    int sampleCount = 0;
};

} // namespace yup
