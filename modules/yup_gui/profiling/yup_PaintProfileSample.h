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

/** Identifies which time contribution to extract from a PaintProfileSample. */
enum class PaintProfileTimeKind
{
    /** Time spent executing the component's own paint callback. */
    self,

    /** Time spent painting all direct and indirect children. */
    children,

    /** Time spent in framework bookkeeping around the paint call. */
    framework,

    /** Total elapsed time for the full paint pass of this component. */
    total
};

//==============================================================================

/** Controls the behaviour and storage policy of a PaintProfileStats instance. */
struct PaintProfileOptions
{
    /** Maximum number of samples retained in the ring buffer. */
    int sampleCapacity = 300;

    /** Samples whose totalMicros is below this threshold are discarded.
        Set to 0.0 to record every sample. */
    double minimumSampleMicros = 0.0;

    /** When true, each sample stores the component bounds at paint time. */
    bool includeBounds = true;

    /** When true, each sample stores the dirty region that triggered the repaint. */
    bool includeRepaintArea = true;

    /** When true, samples are recorded even for components whose isVisible() returns false. */
    bool includeInvisibleComponents = false;

    /** When true, a sample is recorded even when the component's own paint was skipped
        (e.g. because it has no paint implementation), so that child-only costs are tracked. */
    bool recordSkippedSelfPaint = true;
};

//==============================================================================

/** A single timing snapshot captured during one paint pass of a component. */
struct PaintProfileSample
{
    /** Monotonically increasing index of the display frame in which this sample was taken. */
    uint64 frameIndex = 0;

    /** Monotonically increasing index of the paint call within the current frame. */
    uint64 paintIndex = 0;

    /** Microseconds spent inside the component's own paint callback. */
    double selfMicros = 0;

    /** Microseconds spent painting all children of this component. */
    double childrenMicros = 0;

    /** Microseconds consumed by framework bookkeeping (transform setup, clip, etc.). */
    double frameworkMicros = 0;

    /** Total microseconds from the start to the end of the full paint pass. */
    double totalMicros = 0;

    /** Axis-aligned bounding rectangle of the component in its parent's coordinate space. */
    Rectangle<float> componentBounds;

    /** The dirty region that triggered this repaint, in the component's local coordinate space. */
    Rectangle<float> repaintArea;

    /** True when the component requested a continuous repaint (e.g. animation loop). */
    bool renderContinuous = false;

    /** True when the component's own paint callback was skipped for this sample. */
    bool selfPaintSkipped = false;
};

//==============================================================================

/** Statistical summary computed over a collection of PaintProfileSample values
    for a single PaintProfileTimeKind field. */
struct PaintProfileSummary
{
    /** Number of samples included in this summary. */
    int sampleCount = 0;

    /** Value from the most recently recorded sample. */
    double lastMicros = 0.0;

    /** Minimum observed value across all samples. */
    double minMicros = 0.0;

    /** Maximum observed value across all samples. */
    double maxMicros = 0.0;

    /** Arithmetic mean of all sample values. */
    double meanMicros = 0.0;

    /** 50th-percentile (median) value. */
    double p50Micros = 0.0;

    /** 95th-percentile value. */
    double p95Micros = 0.0;

    /** 99th-percentile value. */
    double p99Micros = 0.0;
};

//==============================================================================

/** A linear histogram of sample values for a single PaintProfileTimeKind field.

    The range [rangeMinMicros, rangeMaxMicros] is divided into buckets.size() equal-width
    buckets. Each bucket stores the count of samples whose value falls within that interval.
    Values above rangeMaxMicros are clamped into the last bucket.
*/
struct PaintProfileHistogram
{
    /** Lower bound of the histogram range in microseconds (always 0.0). */
    double rangeMinMicros = 0.0;

    /** Upper bound of the histogram range in microseconds. */
    double rangeMaxMicros = 0.0;

    /** Per-bucket sample counts. buckets[i] covers the sub-range
        [rangeMinMicros + i*width, rangeMinMicros + (i+1)*width). */
    std::vector<int> buckets;
};

} // namespace yup
