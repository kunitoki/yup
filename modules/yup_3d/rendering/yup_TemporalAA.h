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
/** Temporal anti-aliasing: accumulates jittered frames into a history reprojected along the
    screen-space motion, after Karis 2014, "High Quality Temporal Supersampling".

    Each pixel takes its motion from the closest depth around it, reads the history there with a
    Catmull-Rom filter and clips it to the variance box of its neighborhood in YCoCg, then blends
    in the current frame. Each color weighs 1 / (1 + luma) of its un-premultiplied color, so a bright
    surface can't dominate an edge with another one, times its coverage, so an edge against the
    background stays linear in the coverage. The coverage alpha is resolved too, clamped to its
    neighborhood, and blended linearly. Off-screen,
    invalid or non-finite history is rejected; non-finite current texels count as 0 (+infinity as
    the largest half float).

    Once sampleCount frames have accumulated the history is kept as it is, so an unchanged scene
    stops changing.

    @internal
*/
class YUP_API TemporalAA
{
public:
    /** The name of the FrameResources history the pass keeps its accumulation in. */
    static constexpr const char* historyName = "temporalAA";

    //==============================================================================
    /** Creates the pass. GPU objects are created on the first render. */
    TemporalAA();

    /** Destructor. */
    ~TemporalAA();

    //==============================================================================
    /** Resolves @p current (jittered) against the history into @p output and updates the history.
        @p sampleIndex counts frames since the last reset, starting at 0.

        @param device       The device to render with.
        @param frame        The frame to record into.
        @param view         The view of this frame, with its jitter and viewport.
        @param resources    The pool keeping the rgba16float history between frames, and the shared samplers.
        @param current      This frame's rgba16float scene color, premultiplied by its coverage alpha.
        @param depth        This frame's depth32float depth.
        @param velocity     This frame's rg16float motion, current uv minus previous uv.
        @param sampleIndex  Frames accumulated in the history, 0 to start over from @p current.
        @param sampleCount  Frames to accumulate: from this index on the history is kept unchanged.
        @param output       The rgba16float texture receiving the resolved color.
        @param changing     True while the scene changes, see getBlendWeight().

        @returns False if the pipeline could not be prepared or the pass could not be recorded.
    */
    bool render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, FrameResources& resources,
                 const GpuTexture::Ptr& current, const GpuTexture::Ptr& depth, const GpuTexture::Ptr& velocity,
                 int sampleIndex, int sampleCount, const GpuTexture::Ptr& output, bool changing = false);

    /** The weight of the current frame against the history, frame @p sampleIndex of @p sampleCount.

        While the scene holds still it is the true running mean, 1 / (sampleIndex + 1), so the
        converged history holds every frame in the same share and noise that rotates from frame to
        frame (the soft shadow taps) averages out. While it changes (@p changing) it stays at least
        0.1, so the history fades within a few frames behind moving content. 0 once @p sampleCount
        frames have accumulated: the history is then kept unchanged.
    */
    static float getBlendWeight (int sampleIndex, int sampleCount, bool changing) noexcept;

    /** Releases the render targets not drawn into since the previous call, once per frame. */
    void trimUnusedTargets()
    {
        pass.trimUnusedTargets();
    }

    /** Releases the render targets kept for the next renders, when the pass is skipped this frame. */
    void releaseTargets() noexcept
    {
        pass.releaseTargets();
    }

private:
    Result prepare (const GpuDevice::Ptr& device, const FrameResources::Samplers* samplers);

    FullscreenPass pass;
    std::optional<Result> prepared;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TemporalAA)
};

} // namespace yup
