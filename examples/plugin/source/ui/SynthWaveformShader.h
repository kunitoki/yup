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
/** Renders a raymarched ridge landscape shaped by one period of a waveform.

    The dominant displacement of the landscape is the waveform itself, so the ridges
    follow the shape the oscillator plays, with a few faint octaves on top as shimmer.

    Compiling the pipeline isn't free, so one instance is shared by every waveform
    display and it compiles once, from the shader bundle embedded in the plugin. A
    failed compile is remembered rather than retried, and render() then returns
    nullptr so the caller can draw without it.
*/
class SynthWaveformShader
{
public:
    /** The number of waveform samples render() expects. */
    static constexpr int sampleCount = 256;

    /** Renders the landscape into a target owned by the caller.

        @param context  The context the display paints with, providing the GPU device
        @param target   The caller's target, recreated here whenever the size changes
        @param width    The width of the landscape, in logical units
        @param height   The height of the landscape, in logical units
        @param samples  One period of the waveform, sampleCount values in the range -1 to 1
        @param time     The animation time, in seconds
        @param accent   The color of the ridges
        @param ground   The color the landscape fades into
        @param isLight  True when the landscape sits on a light palette

        @returns The rendered landscape, or nullptr when no GPU path is available.
    */
    yup::GpuTexture::Ptr render (yup::GraphicsContext& context,
                                 yup::GpuTarget::Ptr& target,
                                 int width,
                                 int height,
                                 const std::vector<float>& samples,
                                 float time,
                                 yup::Color accent,
                                 yup::Color ground,
                                 bool isLight);

private:
    struct alignas (16) Params
    {
        float time;
        float width;
        float height;
        float lightMode;
        float accent[4];
        float background[4];
    };

    bool ensurePipeline (yup::GraphicsContext& context);

    yup::GpuDevice::Ptr device;
    yup::GpuPipeline::Ptr pipeline;
    bool compileAttempted = false;
};
