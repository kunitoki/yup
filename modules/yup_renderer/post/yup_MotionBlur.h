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
/** Tile-based motion blur of the scene color after temporal anti-aliasing and the depth of field
    (McGuire et al. 2012, "A Reconstruction Filter for Plausible Motion Blur"), three FullscreenPass
    pipelines:

    - TileMax, drawn twice: the longest motion of each 4 x 4 pixels of the color, and the length of
      the shortest, then of each 4 x 4 of those, so one texel per tileSize x tileSize pixels. The
      motion of a pixel is its velocity, in pixels of the color, scaled by
      RenderSettings::motionBlurShutter and halved, since the smear spans the motion centered on the
      pixel, then clamped to tileSize: a smear spans at most 2 * tileSize pixels.
    - NeighborMax: the longest motion of the 3 x 3 tiles around each tile, the diagonal ones only
      when they move towards it, and the shortest length of all 9. No pixel smears farther than
      tileSize, so this reaches every pixel that can smear into the tile.
    - Reconstruct, at the size of the color: numSamples taps along the longest motion around the
      tile, weighted by depth and by how far each pixel's own motion reaches. Tiles where nothing
      moves by half a pixel copy the color, and tiles where everything around moves alike, within a
      pixel, average their taps without reading depth or motion (Jimenez 2014).

    It carries the premultiplied rgba of the scene color, so a moving silhouette smears its coverage
    over the background, and writes how much the smear moved each pixel's coverage, for tone
    mapping to composite that share in radiance (preflight R22, R24): the change of coverage
    relative to the larger of the sharp and smeared coverage, in full from a tenth, so a uniform
    partial coverage the smear leaves as it was (fog over the background color) keeps the display
    blend. The share the depth of field already spread, when its mask is given, counts as spread too. Sizes come from
    the textures read, so the depth and velocity may be smaller than the color (an upscaling
    renderer). The sample pattern is fixed per pixel: the passes are deterministic, as post passes
    after TAA must be.

    Bindings: set 0 binding 0 the ViewBlock and 3 pointClamp; set 1 binding 0 the pass block, then
    the inputs of each pass. Its targets come from the frame's FrameResources; its pipelines live
    as long as this object.

    @internal
*/
class YUP_API MotionBlur
{
public:
    /** Pixels of the color per side of a velocity tile, and the farthest a pixel smears. */
    static constexpr int tileSize = 16;

    /** The taps of the reconstruction per pixel, besides the pixel itself. */
    static constexpr int numSamples = 12;

    //==============================================================================
    /** The textures the blur reads. */
    struct Inputs
    {
        GpuTexture::Ptr color;      ///< The linear rgba16float scene color, premultiplied by its coverage alpha.
        GpuTexture::Ptr depth;      ///< The single-sampled depth of the scene, read by uv: color's size or smaller.
        GpuTexture::Ptr velocity;   ///< The rg16float screen-space motion, current uv minus previous uv, read by uv.
        GpuTexture::Ptr blurAmount; ///< The r8unorm share of each pixel's coverage the depth of field moved (DepthOfField::getBlurAmount()), or nullptr.
    };

    //==============================================================================
    /** Creates a pass for @p device. The pipelines are compiled by prepare(). */
    explicit MotionBlur (GpuDevice::Ptr device);

    /** Destructor. */
    ~MotionBlur();

    //==============================================================================
    /** Compiles the pipelines once, remembering a failure instead of retrying every frame. */
    Result prepare();

    /** Smears @p inputs.color along its motion.

        @returns An rgba16float texture of the size of the color holding the blurred color, or
                 nullptr when nothing was drawn: RenderSettings::motionBlurShutter is not positive
                 or an input is missing (skips, which never count as failures), the pass gave up,
                 or it failed to compile, to create its targets or to record. A failure leaves the
                 frame without the blur; the second in a row makes the pass give up (hasGivenUp()),
                 logged once with YUP_DBG.
    */
    GpuTexture::Ptr render (GpuFrame& frame, const ViewData& view, const RenderSettings& settings,
                            FrameResources& resources, const Inputs& inputs);

    /** The rgba16float result of the last render(), or nullptr when it drew nothing. */
    const GpuTexture::Ptr& getOutput() const noexcept { return output; }

    /** The r8unorm blur amount of the last render(), of the size of the color, or nullptr when it
        drew nothing: per pixel, the share of the coverage the motion or the depth of field moved,
        0 where the pixel kept its coverage, 1 where it covers nothing. ToneMapping composites that
        share in radiance and the rest in display space (preflight R21, R22, R24).
    */
    const GpuTexture::Ptr& getBlurAmount() const noexcept { return blurAmount; }

    /** True after two failed renders in a row: the frames no longer blur. */
    bool hasGivenUp() const noexcept { return givenUp; }

    /** @internal Why the pass gave up, for tests: the message it logged. Empty while it has not given up. */
    const String& getGiveUpReason() const noexcept { return giveUpReason; }

    /** Releases the render targets not drawn into since the previous call: call it every frame the pass runs. */
    void trimUnusedTargets();

    /** Releases every render target and the last outputs, for a frame without the pass, a resize or a failed frame. */
    void releaseTargets() noexcept;

    /** @internal For tests: the number of render targets kept for the next draws. */
    size_t getNumCachedTargets() const noexcept;

    /** @internal Test hook: the next @p count render() calls that blur fail, as if the device could not record them. */
    void failRendersForTesting (int count = std::numeric_limits<int>::max()) noexcept { failRenders = count; }

private:
    GpuDevice::Ptr device;
    std::optional<Result> prepared;
    FullscreenPass tileMax;
    FullscreenPass neighborMaxPass;
    FullscreenPass reconstruct;
    GpuTexture::Ptr output;
    GpuTexture::Ptr blurAmount;
    int consecutiveFailures = 0;
    bool givenUp = false;
    int failRenders = 0; // The renders still to fail, see failRendersForTesting()
    String giveUpReason;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MotionBlur)
};

} // namespace yup
