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
/** Maps the HDR scene color to display sRGB, with the tone mapping operator of the view.

    The source holds exposed radiance: the shading passes apply the exposure, so this pass only
    maps. It is premultiplied by its coverage alpha over a transparent clear, and may be reversibly
    tone mapped (c / (1 + luma)).

    Each pixel tone maps the color of its covered part, un-premultiplied and inverted, then blends
    it over the background by the coverage in display space, the way the raster tier resolves tone
    mapped samples: silhouettes and transparent surfaces over the background match the inline
    raster output. The background color is display-referred, shown exactly as picked, never tone
    mapped. HDR effects running before this pass must therefore carry the alpha through.

    An optional glow (see Bloom) is added to the covered color before tone mapping, and onto the
    background in display space. NaN and infinite inputs are sanitized: NaN and -infinity count as
    0, +infinity as the largest half float.

    @internal
*/
class YUP_API ToneMapping
{
public:
    //==============================================================================
    /** Creates the pass. GPU objects are created on the first render. */
    ToneMapping();

    /** Destructor. */
    ~ToneMapping();

    //==============================================================================
    /** Maps HDR @p source (exposed radiance, or reversibly tone-mapped when @p sourceIsReversible)
        to display sRGB into the rgba8 @p destination, over the display-encoded @p background.

        @param resources  The pool the shared samplers come from.
        @param glow       A linear rgba16float glow of the size of @p source, or nullptr for none.

        @returns False if the pipeline could not be prepared or the pass could not be recorded.
    */
    bool render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, FrameResources& resources,
                 const GpuTexture::Ptr& source, bool sourceIsReversible, const GpuTexture::Ptr& glow,
                 const GpuTexture::Ptr& destination, Color background);

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
    Result prepare (const GpuDevice::Ptr& device, GpuTextureFormat format, const FrameResources::Samplers* samplers);

    FullscreenPass pass;
    std::optional<Result> prepared;
    GpuTextureFormat preparedFormat = GpuTextureFormat::rgba8unorm;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToneMapping)
};

} // namespace yup
