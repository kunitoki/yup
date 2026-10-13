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
/** A hierarchical depth (Hi-Z) pyramid of the closest depth, for screen-space ray marching.

    The module's depth is standard: drawn with the less comparison, 0 at the near plane and 1 at the
    far one, cleared to 1 where nothing is drawn. The closest depth is therefore the minimum.

    build() renders an r32float texture of the size of the depth with the full mip chain, one
    FullscreenPass draw per mip and no compute:
    - mip 0 is a copy of the depth, texel for texel;
    - every other mip n keeps the minimum of the texels of mip n - 1 it covers: 2x2, or 3 along an
      axis where mip n - 1 has an odd extent, since each texel then spans more than two of them.
      Mip n - 1 is read through a single-mip view while a single-mip view of mip n is rendered.
    - a reader finds the texel of mip n covering a pixel as floor (uv * size of mip n), or as the
      shifted pixel clamped to the last texel: at odd sizes a bare shift reads past the edge.

    The pyramid comes from the frame's FrameResources, so it lives as long as the pool keeps it.
    The pipelines live as long as this object, and frames do not retain them: keep it alive while a
    frame that built a pyramid can be in flight.

    Bindings: set 0 binding 3 pointClamp; set 1 binding 0 the depth (copy), or binding 0 the
    extent of the mip read and binding 1 that mip (downsample).

    @internal
*/
class YUP_API HiZPyramid
{
public:
    //==============================================================================
    /** Creates a pyramid builder for @p device. The pipelines are compiled by the first build(). */
    explicit HiZPyramid (GpuDevice::Ptr device);

    /** Destructor. */
    ~HiZPyramid();

    //==============================================================================
    /** Builds an r32float pyramid of the closest depth: mip 0 copies @p depth, every other mip
        keeps the minimum of the texels it covers (3 along an axis of odd extent, else 2).

        @param frame      The frame to record into, after @p depth was written.
        @param depth      A single-sampled depth32float texture.
        @param resources  The frame's pool, for the pyramid.

        @returns The pyramid, with the size of @p depth and its full mip chain, or nullptr when the
                 depth is missing or of another kind, the pipelines did not compile, r32float is not
                 renderable, the pool has no samplers or cannot create the texture, or a draw could
                 not be recorded. The first failure after a success is logged.
    */
    GpuTexture::Ptr build (GpuFrame& frame, const GpuTexture::Ptr& depth, FrameResources& resources);

    /** Releases the render targets not drawn into since the previous call: call it every frame the pyramid is built. */
    void trimUnusedTargets();

    /** Releases every render target, for a frame without the pyramid, a resize or a failed frame. */
    void releaseTargets() noexcept;

    /** @internal For tests: the number of render targets kept for the next builds. */
    size_t getNumCachedTargets() const noexcept { return copy.getNumCachedTargets() + downsample.getNumCachedTargets(); }

private:
    Result prepare();
    GpuTexture::Ptr fail (const String& reason);

    GpuDevice::Ptr device;
    std::optional<Result> prepared;
    FullscreenPass copy;
    FullscreenPass downsample;
    bool failureLogged = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HiZPyramid)
};

} // namespace yup
