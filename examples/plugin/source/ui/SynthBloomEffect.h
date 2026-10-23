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

//==============================================================================
/** A soft bloom over the whole interface: the bright parts of what the component and its
    children painted glow into their surroundings.

    The interface is rendered to a texture by the component effect machinery. The effect
    keeps its bright parts at half the size, colored light far more than white, blurs
    them both ways and adds them back over the interface. Without a GPU, or if a shader fails to compile, the
    interface is drawn as it is.

    @see yup::ComponentEffect
*/
class SynthBloomEffect : public yup::ComponentEffect
{
public:
    void apply (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds) override;

private:
    static constexpr int downsample = 2;
    static constexpr float threshold = 0.5f;
    static constexpr float knee = 0.3f;
    static constexpr float whiteGlow = 0.1f;
    static constexpr float sigma = 8.0f;
    static constexpr float intensity = 0.6f;

    bool ensurePipelines (yup::GraphicsContext& context);
    bool ensureTargets (yup::GpuDevice::Ptr device, int width, int height);

    yup::GpuPipeline::Ptr prefilter;
    yup::GpuPipeline::Ptr blur;
    yup::GpuTarget::Ptr targetA;
    yup::GpuTarget::Ptr targetB;
    bool compileAttempted = false;
};
