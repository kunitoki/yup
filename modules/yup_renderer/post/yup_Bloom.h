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
/** Glow around highlights, after Jimenez 2014, "Next Generation Post Processing in Call of Duty:
    Advanced Warfare".

    The light above RenderSettings::bloomThreshold is downsampled into a chain of half-float
    textures, one per level, halving down to 8 pixels or 6 levels, whichever comes first. The
    chain is then upsampled back with a tent filter whose footprint follows
    RenderSettings::bloomRadius, each level added onto the one above, and the first level is
    upsampled to full resolution scaled by RenderSettings::bloomIntensity.

    The result is the glow alone: ToneMapping adds it to the scene color, and onto the background
    where the scene doesn't cover it.

    @internal
*/
class YUP_API Bloom
{
public:
    //==============================================================================
    /** Creates the pass. GPU objects are created on the first render. */
    Bloom();

    /** Destructor. */
    ~Bloom();

    //==============================================================================
    /** Renders the glow of @p color into a new texture.

        @param device             The device to render with.
        @param frame              The frame to record into.
        @param resources          The pool the chain, the result and the shared samplers are acquired from.
        @param settings           The bloom threshold, intensity and radius.
        @param color              The rgba16float exposed scene color, premultiplied by its coverage alpha.
        @param colorIsReversible  True when @p color is reversibly tone mapped (c / (1 + luma)).

        @returns An rgba16float texture of the size of @p color holding the linear glow, scaled by
                 the intensity, or nullptr if the pass could not be prepared or recorded.
    */
    GpuTexture::Ptr render (const GpuDevice::Ptr& device, GpuFrame& frame, FrameResources& resources,
                            const RenderSettings& settings, const GpuTexture::Ptr& color, bool colorIsReversible);

    /** Releases the render targets not drawn into since the previous call, once per frame. */
    void trimUnusedTargets()
    {
        downsample.trimUnusedTargets();
        upsample.trimUnusedTargets();
    }

    /** Releases the render targets kept for the next renders, when the pass is skipped this frame. */
    void releaseTargets() noexcept
    {
        downsample.releaseTargets();
        upsample.releaseTargets();
    }

private:
    Result prepare (const GpuDevice::Ptr& device, const FrameResources::Samplers* samplers);

    FullscreenPass downsample;
    FullscreenPass upsample;
    std::optional<Result> prepared;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Bloom)
};

} // namespace yup
