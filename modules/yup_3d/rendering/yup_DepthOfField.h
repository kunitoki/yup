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
/** Scatter-as-gather depth of field with a near and a far field and a polygonal bokeh (Jimenez
    2014, "Next Generation Post Processing in Call of Duty: Advanced Warfare"; Abadie 2018, "A Life
    of a Bokeh"), four FullscreenPass draws after temporal anti-aliasing:

    - Setup, at half the size of the depth ((w + 1) / 2): the 2x2 footprint of the scene color,
      sanitized per texel, and the signed circle of confusion (CoC) of the closest of its four
      depths, the thin lens of CameraNode::getCircleOfConfusion() as a radius in texels of this
      half size, clamped to RenderSettings::depthOfFieldMaxSize.
    - Tiles, one texel per tileSize x tileSize CoC texels: the largest near CoC within reach of the
      tile, so the near field spreads over in-focus texels. One pipeline, three draws of small
      windows: two 4 x 4 reductions, then the dilation over the neighboring tiles.
    - Gather, at the half size: numBokehSamples offsets of makeBokehSamples() over the largest CoC
      reaching the texel, into the far field, the near field and its coverage. Texels with nothing
      blurring within reach skip it.
    - Composite, at the size of the scene color: the far field replaces the sharp color as the CoC
      of the pixel grows past a texel, then the near field covers the result by its coverage.

    Every pass carries the premultiplied rgba of the scene color, so silhouettes over the
    background keep their blend. The sample pattern is fixed: the passes are deterministic, as post
    passes after TAA must be. Sizes come from the textures read, so the depth may be smaller than
    the color (an upscaling renderer), the intermediates following the depth.

    Bindings: set 0 binding 0 the ViewBlock, 1 linearClamp and 3 pointClamp; set 1 binding 0 the
    pass block, then the inputs of each pass. Its targets come from the frame's FrameResources; its
    pipelines live as long as this object.

    @internal
*/
class YUP_API DepthOfField
{
public:
    /** The rings of the gather's bokeh pattern. */
    static constexpr int bokehRings = 4;

    /** The offsets the gather reads per texel: the center, then 8, 16, 24 and 32 on the rings.
        YUP_3D_DOF_SAMPLES in shaders/lib/dof.glsl is the same count.
    */
    static constexpr int numBokehSamples = 81;

    static_assert (numBokehSamples == 1 + 4 * bokehRings * (bokehRings + 1));

    /** CoC texels per side of a tile. */
    static constexpr int tileSize = 16;

    /** The largest RenderSettings::depthOfFieldMaxSize honored, in pixels of the scene color. */
    static constexpr float maxBlurSize = 64.0f;

    //==============================================================================
    /** The textures the blur reads. */
    struct Inputs
    {
        GpuTexture::Ptr color;          ///< The exposed rgba16float scene color, premultiplied by its coverage alpha.
        bool colorIsReversible = false; ///< True when color is reversibly tone mapped: the passes invert it per texel.
        GpuTexture::Ptr depth;          ///< The single-sampled depth of the scene, read by uv: color's size or smaller.
        const CameraNode* camera = nullptr; ///< The camera the scene was drawn with; nullptr (the default camera) has no depth of field.
    };

    /** The thin lens as the setup pass evaluates it: the signed CoC radius, in texels of a texture
        of the height given to getLens(), of a point at a view distance d is scale * (1 - focus / d).
    */
    struct Lens
    {
        float scale = 0.0f; ///< The CoC at an infinite distance, 0 for a camera that cannot blur.
        float focus = 0.0f; ///< The focus distance, at least just past the focal length.
    };

    //==============================================================================
    /** Returns sample offsets covering the unit aperture on concentric rings: the center, then ring
        r (1 to @p rings) with 8 * r samples at radius r / rings. A disk when @p blades is below 3,
        otherwise a regular polygon with that many sides inscribed in the unit circle, its first
        vertex at the angle @p rotation (radians), each disk sample moved along its ray onto the
        polygon (Abadie 2018).
    */
    static std::vector<Point<float>> makeBokehSamples (int rings, int blades, float rotation);

    /** Returns the lens of @p camera for a texture @p height texels tall: half the diameter
        CameraNode::getCircleOfConfusion() gives for that height. A scale of 0 for no camera, an
        orthographic one, an fStop or yFov of 0 or less, or a height of 0 or less.
    */
    static Lens getLens (const CameraNode* camera, float height) noexcept;

    /** True when @p camera blurs with @p settings: it has a lens and the largest blur is positive. */
    static bool canBlur (const CameraNode* camera, const RenderSettings& settings) noexcept;

    //==============================================================================
    /** Creates a pass for @p device. The pipelines are compiled by prepare(). */
    explicit DepthOfField (GpuDevice::Ptr device);

    /** Destructor. */
    ~DepthOfField();

    //==============================================================================
    /** Compiles the pipelines once, remembering a failure instead of retrying every frame. */
    Result prepare();

    /** Blurs @p inputs.color by the depth of field of @p inputs.camera.

        @returns An rgba16float texture of the size of the color holding the blurred, linear color,
                 or nullptr when nothing was drawn: the camera cannot blur (canBlur(), which records
                 and allocates nothing), an input is missing (a skip too, which never counts as a
                 failure), the pass gave up, or it failed to compile, to create its targets or to
                 record. A failure leaves the frame without the blur; the second in a
                 row makes the pass give up (hasGivenUp()), logged once with YUP_DBG.
    */
    GpuTexture::Ptr render (GpuFrame& frame, const ViewData& view, const RenderSettings& settings,
                            FrameResources& resources, const Inputs& inputs);

    /** The rgba16float result of the last render(), or nullptr when it drew nothing. */
    const GpuTexture::Ptr& getOutput() const noexcept { return output; }

    /** The r16float signed CoC of the last render(), in texels of its half size, or nullptr when it drew nothing. */
    const GpuTexture::Ptr& getCircleOfConfusionTexture() const noexcept { return circleOfConfusion; }

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
    FullscreenPass setup;
    FullscreenPass tiles;
    FullscreenPass gather;
    FullscreenPass composite;
    std::vector<Point<float>> samples;
    int sampleBlades = 0; // The apertureBlades samples was made for, when not empty
    GpuTexture::Ptr output;
    GpuTexture::Ptr circleOfConfusion;
    int consecutiveFailures = 0;
    bool givenUp = false;
    int failRenders = 0; // The renders still to fail, see failRendersForTesting()
    String giveUpReason;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DepthOfField)
};

} // namespace yup
