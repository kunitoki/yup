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
/** Owns the post effects and runs the enabled ones in contract order, ending with tone mapping.

    TemporalAA runs first when enabled, then Bloom when enabled, then ToneMapping. Bloom renders
    only the glow, which ToneMapping adds while it maps the scene color. Each pass reading a
    reversibly tone mapped color inverts it per texel, un-premultiplied by its coverage first.

    @internal
*/
class YUP_API PostChain
{
public:
    //==============================================================================
    /** The textures the shading passes produced. */
    struct Inputs
    {
        GpuTexture::Ptr color;          ///< HDR scene color, premultiplied by its coverage alpha.
        GpuTexture::Ptr depth;          ///< Scene depth, or nullptr.
        GpuTexture::Ptr velocity;       ///< Screen-space motion, or nullptr.
        bool colorIsReversible = false; ///< True when color is reversibly tone mapped (raster HDR path).
        Color background;               ///< Display-referred, composited under the uncovered pixels.
        int temporalSampleIndex = 0;    ///< Frames accumulated in the TAA history, see TemporalAA::render().
    };

    //==============================================================================
    /** Creates the chain. GPU objects are created on the first render. */
    PostChain();

    /** Destructor. */
    ~PostChain();

    //==============================================================================
    /** Runs the enabled effects in contract order and writes the final image into @p output.
        The effects skipped this frame release the render targets they kept, and the others
        release the ones they did not draw into. On failure, call releaseTargets().

        @returns False if an effect could not be prepared or recorded.
    */
    bool render (const GpuDevice::Ptr& device, GpuFrame& frame, const RenderCapabilities& caps,
                 const RenderSettings& settings, const ViewData& view, FrameResources& resources,
                 const Inputs& inputs, const GpuTexture::Ptr& output);

    /** Releases the render targets every effect keeps between frames, for frames without the chain. */
    void releaseTargets() noexcept;

    /** The HDR color the last render() passed to the effects after temporal anti-aliasing: the
        resolved color when TAA ran, the input color otherwise, before bloom. Valid until the next
        render().
    */
    const GpuTexture::Ptr& getResolvedColor() const noexcept { return resolvedColor; }

private:
    TemporalAA temporalAA;
    Bloom bloom;
    ToneMapping toneMapping;
    GpuTexture::Ptr resolvedColor;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PostChain)
};

} // namespace yup
