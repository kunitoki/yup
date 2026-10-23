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
/** A synthwave finish over the whole interface, like an old tube screen: curvature in
    a bezel, neon glow, chromatic aberration, a violet color grade, scanlines, a
    phosphor mask, a vignette and grain.

    The interface is rendered to a texture by the component effect machinery. Its bright
    colors are taken at one texel per two points, colored light far more than white, and blurred
    both ways; a full size pass then shifts red and blue apart toward the corners,
    screens the glow back, grades, and draws the scanlines, phosphor mask, vignette and
    grain, all through the curve of the tube. The mouse is mapped through the same curve,
    so what is clicked is what is shown. Each part can be switched off. Sizes are in points,
    so every display scale looks alike. Without a GPU, or if a shader fails to compile, the
    interface is drawn as it is.

    @see yup::ComponentEffect
*/
class SynthNeonEffect : public yup::ComponentEffect
{
public:
    /** Which parts of the finish are drawn. */
    struct Options
    {
        bool glow = true;
        bool aberration = true;
        bool grade = true;
        bool scanlines = true;
        bool vignette = true;
        bool grain = true;
        bool curvature = true;
        bool phosphor = true;
    };

    explicit SynthNeonEffect (Options optionsToUse);

    void apply (yup::Graphics& g, yup::GpuTexture::Ptr input, yup::Rectangle<float> bounds) override;
    std::optional<yup::Point<float>> displayToContent (yup::Point<float> displayPoint, yup::Rectangle<float> bounds) const override;
    std::optional<yup::Point<float>> contentToDisplay (yup::Point<float> contentPoint, yup::Rectangle<float> bounds) const override;

private:
    static constexpr float downsamplePerPoint = 2.0f;
    static constexpr float glowThreshold = 0.35f;
    static constexpr float glowKnee = 0.3f;
    static constexpr float whiteGlow = 0.12f;
    static constexpr float glowSigma = 7.0f;
    static constexpr float glowAmount = 1.1f;
    static constexpr float aberrationAmount = 2.0f;
    static constexpr float gradeAmount = 1.0f;
    static constexpr float scanlineAmount = 0.2f;
    static constexpr float vignetteAmount = 0.4f;
    static constexpr float grainAmount = 0.02f;
    static constexpr float curvatureAmount = 0.020f;
    static constexpr float phosphorAmount = 0.25f;
    static constexpr float cornerRadius = 0.04f;

    float getCurvature() const noexcept { return options.curvature ? curvatureAmount : 0.0f; }

    bool ensurePipelines (yup::GraphicsContext& context);
    bool ensureTargets (yup::GpuDevice::Ptr device, int width, int height, int smallWidth, int smallHeight);

    const Options options;
    yup::GpuDevice::Ptr pipelineDevice;
    yup::GpuPipeline::Ptr prefilter;
    yup::GpuPipeline::Ptr blur;
    yup::GpuPipeline::Ptr composite;
    yup::GpuTarget::Ptr smallA;
    yup::GpuTarget::Ptr smallB;
    yup::GpuTarget::Ptr output;
    bool compileAttempted = false;
};
