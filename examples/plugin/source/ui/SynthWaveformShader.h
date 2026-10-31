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

#include <yup_gui/yup_gui.h>

#include <vector>

//==============================================================================
/** Renders a raymarched ridge landscape shaped by a profile of samples.

    The dominant displacement of the landscape is the profile itself, so the ridges follow
    the shape it traces, with a few faint octaves on top as shimmer. A waveform scrolls by
    as a repeating period; the other displays hold a profile spanning their width once, the
    ridge on the far wall lining up with the curve they draw (see sampleForY()), and may
    light it around a playhead.

    Compiling the pipeline isn't free, so one instance is shared by every display and it
    compiles once, from the shader bundle embedded in the plugin. A failed compile is
    remembered rather than retried, and render() then returns nullptr so the caller can
    draw without it.
*/
class SynthWaveformShader
{
public:
    /** The number of profile samples render() expects. */
    static constexpr int sampleCount = 256;

    /** How a landscape is colored, moves and is lit. */
    struct Look
    {
        yup::Color accent;      /**< The color of the ridges. */
        yup::Color ground;      /**< The color the landscape fades into. */
        bool isLight = false;   /**< True when the landscape sits on a light palette. */
        bool scrolls = true;    /**< Repeats the profile as a period scrolling sideways. */
        float playhead = -1.0f; /**< Where a playhead lights the ridge, 0 to 1 across the width, or negative for none. */
        float trail = 0.1f;     /**< How far the playhead's trail reaches behind it, as a fraction of the width. */
    };

    /** Returns the sample that puts the ridge on the far wall at a vertical position.

        @param y       The position, from the top of the landscape
        @param height  The height of the landscape
    */
    static float sampleForY (float y, float height) noexcept;

    /** Renders the landscape into a target owned by the caller.

        @param context  The context the display paints with, providing the GPU device
        @param target   The caller's target, recreated here whenever the size changes
        @param width    The width of the landscape, in logical units
        @param height   The height of the landscape, in logical units
        @param samples  The profile, sampleCount values: -1 to 1 spans most of the height
        @param time     The animation time, in seconds
        @param look     The colors, motion and playhead of the landscape

        @returns The rendered landscape, or nullptr when no GPU path is available.
    */
    yup::GpuTexture::Ptr render (yup::GraphicsContext& context,
                                 yup::GpuTarget::Ptr& target,
                                 int width,
                                 int height,
                                 const std::vector<float>& samples,
                                 float time,
                                 const Look& look);

private:
    struct alignas (16) Params
    {
        float time;
        float width;
        float height;
        float lightMode;
        float accent[4];
        float background[4];
        float scrolling;
        float playhead;
        float trail;
        float padding;
    };

    // The ridge on the far wall rises by focal / 8 of the height per unit of sample.
    static constexpr float ridgeScale = 2.8f / 8.0f;

    bool ensurePipeline (yup::GraphicsContext& context);

    yup::GpuDevice::Ptr device;
    yup::GpuPipeline::Ptr pipeline;
    bool compileAttempted = false;
};
