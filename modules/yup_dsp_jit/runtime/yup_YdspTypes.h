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

#pragma once

namespace yup
{

//==============================================================================
/** The element type of a compiled endpoint buffer (streams, params, meters).

    Mirrors the YDSP primitive types on the host side; kernels address their
    buffers with the matching element size.
*/
enum class YdspElementType
{
    float32, // 4-byte IEEE float (the default for streams and params)
    float64, // 8-byte IEEE double
    int32,   // 4-byte signed integer
    int64,   // 8-byte signed integer
    boolean  // 4-byte integer boolean (0 or 1)
};

//==============================================================================
/** The outcome of a YdspAudioGraph::process() call.

    process() never asserts, throws or allocates: caller-side problems with the
    supplied stream buffers are reported through this value and the call is
    otherwise ignored (the graph is left untouched), so hosts may validate and
    log at their leisure.
*/
enum class YdspProcessResult
{
    ok,                 // the block was processed
    invalidGraph,       // the graph is null or failed to compile
    invalidBufferCount, // inputs/outputs span size != declared stream counts
    bufferTypeMismatch, // a buffer's variant alternative != declared stream type
    bufferTooShort      // a buffer holds fewer than numSamples elements
};

//==============================================================================
/** A typed, non-owning view of one input stream buffer.

    The buffer is a variant of fixed-element spans: the *active alternative*
    is the buffer's type, so there is no `void*` reinterpretation at the call
    site and no way to silently pass a buffer of the wrong element type.

    The active alternative must match the graph's declared stream type (see
    YdspAudioGraph::getInputStreamType()) and hold at least `numSamples`
    elements; mismatches are silently ignored rather than touching the buffer. Boolean streams (stored as
    int32 0/1) use the int32 alternative.
*/
using YdspInputBuffer = std::variant<
    yup::Span<const float>,
    yup::Span<const double>,
    yup::Span<const int32_t>,
    yup::Span<const int64_t>>;

/** A typed, non-owning view of one output stream buffer (the mutable
    counterpart of YdspInputBuffer). */
using YdspOutputBuffer = std::variant<
    yup::Span<float>,
    yup::Span<double>,
    yup::Span<int32_t>,
    yup::Span<int64_t>>;

//==============================================================================
/** A sample-accurate, stepped parameter change for the audio thread.

    Resolve the target slot once, on the control thread, via
    YdspAudioGraph::getParameterSlot() and then deliver the (slot, sampleOffset,
    value) triple to YdspAudioGraph::process(). The runtime applies the value as
    an exact-sample step change: every sub-block after `sampleOffset` sees the
    new value. Automation is deliberately *not* interpolated: de-zippering is
    the patch's job, via the `smooth (param, tau)` intrinsic or the
    `[[ smoothing: tau ]]` endpoint annotation.
*/
struct YdspAutomationEvent
{
    int parameterSlot = -1; // resolved via YdspAudioGraph::getParameterSlot()
    int sampleOffset = 0;   // sample index within the block (0 <= offset < numSamples)
    float value = 0.0f;     // the new float32 value
};

//==============================================================================
/** Metadata of one patch parameter, for building host UIs.

    Populated from the patch's `input value` endpoints: the qualified name
    (graph-level name, or "node.param"), the annotation style `[[ name: ... ]]`
    display name (falling back to the endpoint name), the value type, the
    declared default value (`input value float x = default`), and the
    `[[ min: ... ]]` / `[[ max: ... ]]` annotation bounds (falling back to
    `0` and `1` when not annotated).

    A parameter annotated `[[ values: { "a", "b", ... } ]]` is discrete: the
    annotation's labels are exposed in `discreteValues`, evenly spaced over
    [minValue, maxValue] (label i sits at minValue + i * step, with
    step = (maxValue - minValue) / (count - 1)). Hosts should snap the control
    to those steps and show the matching label instead of the raw number.

    `unit`, `stepSize` and `style` surface the optional `[[ unit: ... ]]`,
    `[[ step: ... ]]` and `[[ style: ... ]]` annotations verbatim, for a host UI
    to use as it sees fit (e.g. a unit suffix, a slider increment, or a hint
    about which control to render). All three are empty/zero when absent.

    `midValue` carries the optional `[[ mid: ... ]]` annotation: the value that
    should sit at the middle of a host slider's travel, so a UI can derive a
    logarithmic skew factor for the control. It is `std::nullopt` when absent.
    `bipolar` carries the optional `[[ bipolar: true ]]` annotation (default
    `false`), marking a parameter whose range is centered on zero.
*/
struct YdspParameterInfo
{
    String name;        // qualified name ("node.param" or graph-level "param")
    String displayName; // [[ name: "..." ]] annotation, else the endpoint name
    YdspElementType type = YdspElementType::float32;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 1.0;
    StringArray discreteValues; // [[ values: "a", "b", ... ]] labels; empty for continuous parameters

    String unit;           // [[ unit: "..." ]] annotation (e.g. "Hz", "dB"), else empty
    double stepSize = 0.0; // [[ step: ... ]] annotation (UI increment), else 0 (continuous)
    String style;          // [[ style: "..." ]] annotation (host-defined control hint), else empty

    std::optional<double> midValue; // [[ mid: ... ]] annotation (slider midpoint for skew), else std::nullopt
    bool bipolar = false;           // [[ bipolar: true ]] annotation (range centered on zero), else false

    /** Returns true if this parameter carries a [[ values ]] annotation. */
    bool isDiscrete() const noexcept { return discreteValues.size() >= 2; }

    /** Returns the label for the discrete position closest to `value`, or an
        empty string when this parameter is not discrete. */
    String labelForValue (double value) const
    {
        if (! isDiscrete())
            return {};

        const auto count = discreteValues.size();
        const auto step = (maxValue - minValue) / static_cast<double> (count - 1);
        auto index = static_cast<int> (std::lround ((value - minValue) / step));

        return discreteValues[std::clamp (index, 0, static_cast<int> (count) - 1)];
    }
};

} // namespace yup
