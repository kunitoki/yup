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

    TemporalAA runs first when enabled, then DepthOfField when enabled and the camera can blur, then
    MotionBlur when enabled and something moved, then Bloom when enabled, then ToneMapping. Bloom
    renders only the glow, which ToneMapping adds while it maps the scene color. Each pass reading a
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
        bool temporalChanging = false;  ///< True while the scene changes, see TemporalAA::getBlendWeight().
        const CameraNode* camera = nullptr; ///< The active camera, for the depth of field; nullptr has none.
        bool blurring = false;          ///< True when the depth of field blurs this frame: willBlur(), and on raster a depth and normal prepass that compiles.
        bool moving = false;            ///< True when the camera or a drawn item moved since the previous frame, whose history carries on, so the motion blur has something to smear.
        std::function<void (const char*)> beginScope; ///< Called with the name of each effect right before it records, to open its GPU scope (see SceneRenderer::setFrameStatsEnabled()); may be empty.
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

    /** Releases the render targets every effect keeps between frames, and the resolved color, for frames without the chain. */
    void releaseTargets() noexcept;

    /** The HDR color the last render() passed to the effects after temporal anti-aliasing: the
        resolved color when TAA ran, the input color otherwise, before the depth of field and bloom.
        Valid until the next render() or releaseTargets().
    */
    const GpuTexture::Ptr& getResolvedColor() const noexcept { return resolvedColor; }

    /** True when the depth of field can blur the next render(): it is enabled, @p camera can blur
        (DepthOfField::canBlur()) and it has not given up. The caller adds its own terms, like the
        raster prepass compiling, and passes the decision in Inputs::blurring: the raster tier routes
        the scene color through half float and draws the depth and normal prepass only then.
    */
    bool willBlur (const RenderCapabilities& caps, const RenderSettings& settings, const CameraNode* camera) const noexcept;

    /** The depth of field, for its outputs and for tests: created by the first render() blurring
        (enabled, and a camera that can blur), then kept; nullptr before.
    */
    const DepthOfField* getDepthOfField() const noexcept { return depthOfField.get(); }

    /** @internal Test hook: the next @p count depth of field renders fail, see DepthOfField::failRendersForTesting(). */
    void failDepthOfFieldForTesting (int count) noexcept { depthOfFieldFailures = count; }

    /** True when the motion blur will smear the next render() of a moving frame (Inputs::moving):
        it is enabled with a positive shutter (RenderCapabilities::Feature::motionBlur, full tier
        only) and it has not given up.
    */
    bool willMotionBlur (const RenderCapabilities& caps, const RenderSettings& settings) const noexcept;

    /** The motion blur, for its outputs and for tests: created by the first render() smearing (enabled,
        on a moving frame), then kept; nullptr before.
    */
    const MotionBlur* getMotionBlur() const noexcept { return motionBlur.get(); }

    /** @internal Test hook: the next @p count motion blur renders fail, see MotionBlur::failRendersForTesting(). */
    void failMotionBlurForTesting (int count) noexcept { motionBlurFailures = count; }

private:
    TemporalAA temporalAA;
    std::unique_ptr<DepthOfField> depthOfField;
    int depthOfFieldFailures = 0; // Handed to the depth of field by the next frame blurring
    std::unique_ptr<MotionBlur> motionBlur;
    int motionBlurFailures = 0; // Handed to the motion blur by the next frame smearing
    Bloom bloom;
    ToneMapping toneMapping;
    GpuTexture::Ptr resolvedColor;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PostChain)
};

} // namespace yup
