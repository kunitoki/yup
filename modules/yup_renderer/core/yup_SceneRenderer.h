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

namespace yup
{

//==============================================================================
/** Renders a Scene into an offscreen texture through the RHI.

    Shading follows the glTF metallic-roughness model (GGX distribution, Smith visibility,
    Schlick Fresnel) with KHR_lights_punctual lights, a flat ambient term, normal mapping,
    alpha masking and blending, and double-sided materials. The result is exposed, tone mapped
    with Reinhard, ACES or AgX (see ToneMappingNode and ExposureNode) and sRGB encoded into an rgba8unorm texture.
    The scene features come from the feature nodes of the tree, resolved once per frame by SceneSettings.

    With an EnvironmentNode, surfaces reflect the environment, blurred by their roughness,
    and receive its diffuse light; EnvironmentNode::visible also draws it behind the scene.
    Surfaces inside the influence of a ReflectionProbeNode reflect the scene the probe captured
    instead, and surfaces inside an IrradianceVolumeNode take their diffuse light from the volume,
    see bakeProbes().

    The first directional LightNode with LightNode::castsShadows set casts shadows of the opaque
    items, from cascaded shadow maps: the view is cut into RenderSettings::sunCascades depth ranges,
    each with its own map of RenderSettings::shadowResolution texels, so shadows stay sharp near the
    camera and still reach far away. Point and spot lights with LightNode::castsShadows set cast
    them from perspective views packed into one shadow atlas, six for a point light and one for a
    spot light, sized by how large the light's reach looks on screen; the most important lights get
    views while they fit the shadow budget of the tier (16 views on raster), the others light
    unshadowed. The maps are filtered softly and only drawn again when the casters, the shadow
    casting lights, the shadow settings or the camera change.

    The tier comes from RenderSettings::tier and the device, see RenderCapabilities. The raster
    tier antialiases geometry edges with MSAA, 4 samples by default, resolved into the returned
    texture, and tone maps inline. The full tier shades one sample per pixel into a half-float
    scene color with screen-space motion, antialiases it by accumulating jittered frames (temporal
    anti-aliasing, see RenderSettings::taaSamples and isConverged()), then tone maps it in a post pass.
    It shades opaque surfaces once per pixel from a G-buffer (deferred shading), with the same
    lighting model as blended surfaces, and lights every surface with the lights reaching it, up
    to 63 point and spot lights per screen-space cluster.
    RenderSettings::bloom adds a glow around highlights on both tiers. On the raster tier it needs
    multisampled half-float targets (see RenderCapabilities): the multisampled scene color is then
    rendered in half float and tone mapped after the resolve, as on the full tier, instead of
    inline. The background color is display-referred on both tiers: it shows exactly as picked,
    never tone mapped. Silhouettes and transparent surfaces over the background blend with it after
    tone mapping on every path, so they look the same on both tiers and with or without bloom.
    RenderSettings::depthOfField blurs what lies away from the focus of the active camera, after
    the temporal accumulation and before bloom, on both tiers: on the raster tier where the device
    allows half-float targets and the depth and normal prepass (RenderCapabilities::Feature::depthOfField).
    The frames it blurs composite over the background color in radiance, so blurred highlights keep
    their brightness there.
    RenderSettings::motionBlur smears what moves on the full tier, after the depth of field, on the
    frames where the camera or an item moved, composited over the background color the same way.
    The scene's WorldFog (WorldFogNode) is applied from the camera to each opaque surface, or
    to the far plane where nothing was drawn, before the temporal accumulation: in place on the full
    tier, and on the raster tier where the device allows half-float targets and the depth and
    normal prepass, as the first post effect.
    Highlights of very smooth surfaces are widened where normals change quickly across a pixel, to
    keep them from sparkling.

    The color and depth targets are kept between frames and created again when the size, the
    tier, the raster sample count or the device changes, so render() can be called every frame.

    @see SceneComponent, SceneDrawList
*/
class YUP_API SceneRenderer
{
public:
    //==============================================================================
    /** Creates a renderer. GPU objects are created on the first render. */
    SceneRenderer();

    /** Destructor. */
    ~SceneRenderer();

    //==============================================================================
    /** Renders a scene.

        @param device    The device to render with.
        @param scene     The scene to render.
        @param widthPx   The width of the result in pixels.
        @param heightPx  The height of the result in pixels.

        Frames are not waited on, so a frame of this renderer the GPU failed to execute is only
        found by its next render, which restarts the accumulation, still renders, and reports the
        failure through getLastGpuFailure(); see also waitForGPU().

        @return The rendered texture, valid until the next render, or nullptr on failure; see getLastError().
    */
    GpuTexture::Ptr render (const GpuDevice::Ptr& device, Scene& scene, int widthPx, int heightPx);

    //==============================================================================
    /** Returns the settings that choose how scenes are rendered. */
    const RenderSettings& getRenderSettings() const noexcept { return renderSettings; }

    /** Changes the settings that choose how scenes are rendered, from the next render(). */
    void setRenderSettings (const RenderSettings& newSettings) noexcept { renderSettings = newSettings; }

    /** Bakes every reflection probe and irradiance volume of the scene that was never baked, was
        asked to bake again (ReflectionProbeNode::requestBake(), IrradianceVolumeNode::requestBake()),
        or whose own fields or entity world matrix changed since its bake, and returns how many
        probes and volumes it captured.

        render() bakes such probes and volumes at the start of its frames anyway: calling this only
        moves the cost, for example to a loading screen. The bakes record into a GpuFrame of their
        own, submitted without waiting. Each call captures at most ProbeBaker::maxVolumeProbesPerBake
        volume probe positions, so a large volume, or many volumes, take several calls: call it until
        it returns 0. A volume lights nothing until all of its positions are captured. The next
        render() or bakeProbes() that finds a frame failed on the GPU bakes the probes and volumes
        again; bakeProbes() reports such a failure through getLastGpuFailure() itself, until the
        next render() clears it.

        Bakes that cannot be prepared or recorded never fail a frame: the probes or volumes shade
        with weight 0 meanwhile, and after two failures in a row of one of them the renderer stops
        baking that one, logged once in debug builds; the other keeps baking. Calling bakeProbes()
        lets both try again.

        @param device  The device to bake with, the one render() is called with.
        @param scene   The scene whose reflection probes and irradiance volumes to bake.

        @returns The number of probes baked plus the number of volumes with positions captured,
                 complete or not; 0 when there was nothing left to bake or the bakes failed.
    */
    int bakeProbes (const GpuDevice::Ptr& device, Scene& scene);

    /** Blocks until the GPU finished every frame this renderer submitted.

        @returns Result::ok(), or the first failure of those frames, which then restarts the
                 accumulation and is not reported again by the next render. Only this renderer's
                 frames count.
    */
    Result waitForGPU();

    /** Returns the failure of an earlier frame of this renderer that the last render found and
        recovered from by starting the accumulation over, or an empty string.
    */
    const String& getLastGpuFailure() const noexcept { return lastGpuFailure; }

    /** Returns why the last render failed, or an empty string if it succeeded. */
    const String& getLastError() const noexcept { return lastError; }

    /** Returns the capabilities the last render ran with, or nullptr before the first render.

        The pointer stays valid until the next render.
    */
    const RenderCapabilities* getCapabilities() const noexcept;

    /** Returns false while rendering the same scene again would still refine the image.

        The full tier accumulates RenderSettings::taaSamples jittered frames, then keeps the result
        until something changes. Changes come in three kinds:
        - most changes restart the convergence: the camera, entity transforms, new content in a
          Texture (see Texture::setGpuTexture()), lights, the ambient light, the default light,
          materials, the meshes drawn, exposure and the environment. The accumulated history is
          kept, reprojected and clipped to the new frames, so moving parts blend in smoothly and
          still parts stay antialiased. Once nothing changes any more, the history counts as one
          frame of the accumulation that follows;
        - the size, the tier, RenderSettings::taaSamples and restartAccumulation() discard the
          history, and the accumulation starts over from the next frame alone;
        - tone mapping, the bloom, depth of field and motion blur settings, the depth of field
          fields of the camera and RenderSettings::debugView apply after the accumulation, so
          changing them neither restarts nor discards it: repaint after changing them.

        A frame that motion blur smeared is not final either, with or without temporal
        anti-aliasing: the still frame after it shows the scene sharp.

        Repaint until this returns true, see SceneComponent. Always true on the raster tier, before
        the first render and after a failed one.

        @see restartAccumulation
    */
    bool isConverged() const noexcept;

    /** Discards the accumulated history and the shadow maps, so the next renders start over from
        the next frame alone and isConverged() is false.

        Call it after changes the renderer cannot see: vertices edited in place in a Mesh, or
        a GpuTexture redrawn without calling Texture::setGpuTexture(). Also call it after a large
        change, such as a camera cut: changes that only restart the convergence keep the history,
        and silhouettes keep a faint trace of the image before them.
    */
    void restartAccumulation() noexcept;

    //==============================================================================
    /** What one frame of this renderer cost on the GPU and what it drew, see setFrameStatsEnabled(). */
    struct FrameStats
    {
        uint64 frameIndex = 0;       ///< The render() these numbers belong to, counting this renderer's calls from 1; 0 until the stats of a frame arrived.
        GpuFrameTimings gpu;         ///< The GPU time of the frame and of each pass it recorded; unavailable on devices that cannot measure it.
        int drawItems = 0;           ///< The opaque and blended draws of the main view.
        int shadowCasterDraws = 0;   ///< The shadow caster draws the frame recorded.
        bool shadowsRedrawn = false; ///< True when the frame drew shadow maps, false when it kept the previous ones.
        int internalWidth = 0;       ///< The width the scene was shaded at: the width render() was called with, times RenderCapabilities::getRenderScale() when upscaling.
        int internalHeight = 0;      ///< The height the scene was shaded at: the height render() was called with, times RenderCapabilities::getRenderScale() when upscaling.
    };

    /** Collects the FrameStats of the frames rendered from now on, see getLastFrameStats().

        Off by default, so frames pay nothing for it: they neither measure their GPU time nor open
        GPU scopes. Once a frame of the device measured, though, the device's later frames pay a
        GPU timestamp pair each when they complete, stats on or off. While on, each frame measures its GPU time (see GpuFrame::setTimingEnabled()) and
        opens a scope (see GpuFrame::beginScope()) for each pass it records, in frame order:
        probeBake, shadows, lightCulling, gbuffer, ambientOcclusion, hiZ, screenSpaceReflections,
        lighting, indirectDiffuse, depthNormalPrepass, ambientOcclusion (on the raster tier, after
        the prepass), forward, fog, temporalAA, depthOfField, motionBlur, bloom, toneMapping and debugView. A pass the
        frame does not run is not listed, and one that fails reads 0. shadows is listed in every
        frame: it reads 0 when the frame kept the previous shadow maps (FrameStats::shadowsRedrawn
        is then false). With screen-space GI on, ambientOcclusion includes the GI gather, which
        traces with it, and indirectDiffuse is only adding the gathered light to the scene.

        A scope's time is the time its pass added to the GPU's critical path, not its cost on its
        own (see GpuScopeTiming::milliseconds); only devices measuring scopes report them (see
        GpuDevice::isScopeTimingAvailable(), Metal today). Measured with at most 8 frames in flight,
        the scopes are reliable; the frame total (GpuFrameTimings::frameMilliseconds) also includes
        the time the frame waited behind earlier ones, even with 2 in flight, so it is best read
        waiting for each frame.

        @param shouldCollect  True to collect the stats of the next frames.
    */
    void setFrameStatsEnabled (bool shouldCollect) noexcept { frameStatsEnabled = shouldCollect; }

    /** Returns true while the frames collect their stats, see setFrameStatsEnabled(). */
    bool isFrameStatsEnabled() const noexcept { return frameStatsEnabled; }

    /** Returns the stats of the latest frame rendered with stats on that the GPU finished
        successfully, a few frames behind the last render().

        The stats of a frame arrive, without waiting for it, when a later render() or bakeProbes()
        finds it finished, or with waitForGPU(). A failed frame reports none. All the numbers of
        one FrameStats belong to the same frame, FrameStats::frameIndex.

        @returns The stats, frameIndex 0 until the stats of a frame arrived. Valid until the next
                 render(), bakeProbes() or waitForGPU().
    */
    const FrameStats& getLastFrameStats() const noexcept { return lastFrameStats; }

    //==============================================================================
    /** @internal Intermediate textures of the last frame, for tests and debug views. With upscaling
        (RenderSettings::upscaling), the ones before the temporal anti-aliasing are at the internal
        size (FrameStats::internalWidth), sceneColor and the ones after it at the frame's.
    */
    enum class PassOutput
    {
        sceneColor, ///< Full: the rgba16float scene color after the world fog and temporal anti-aliasing, before the other post effects. Raster: the resolved forward output, rgba16float reversibly tone mapped when bloom is on or the depth of field blurs the frame; on frames drawing the world fog, the fogged rgba16float linear color instead.
        velocity,     ///< Full: the rg16float screen-space motion, current uv minus previous uv, both unjittered, uv from the top-left. Raster: none.
        gbufferNormal, ///< Full: the rgba16float G-buffer RT1, the octahedral shading normal in xy, the filtered roughness in z and metallic in w, 0 where nothing was drawn. Raster: none.
        prepassNormal, ///< Raster with ambient occlusion, a frame the depth of field blurs, or one drawing the world fog: the rgba16float depth and normal prepass target, encoded as the G-buffer RT1, single sampled. Otherwise none.
        ambientOcclusion, ///< With ambient occlusion: the rgba8 AmbientOcclusionPass output, the view-space bent normal * 0.5 + 0.5 in rgb and the visibility in a, at half resolution. Otherwise none.
        reflections, ///< Full, with screen-space reflections and a history to read them from: the rgba16float ReflectionPass output, the reflected radiance (unexposed) in rgb and its confidence in a. Otherwise none, the first frame included.
        indirectDiffuse, ///< Full, with screen-space diffuse GI added to the frame: the rgba16float AmbientOcclusionPass::getIndirectDiffuse(), the gathered irradiance (unexposed) in rgb and the share of the hemisphere it came from in a, at half resolution. Otherwise none, the first frame included.
        circleOfConfusion, ///< With depth of field blurring the frame: the r16float signed circle of confusion radius of DepthOfField, in texels of its half resolution, negative in front of the focus. Otherwise none.
        depthOfField,      ///< With depth of field blurring the frame: the rgba16float DepthOfField result, the scene color after the blur and before bloom. Otherwise none.
        motionBlur         ///< Full, with motion blur smearing the frame: the rgba16float MotionBlur result, the scene color after the depth of field and the smear, before bloom. Otherwise none, still frames included.
    };

    /** @internal Returns an intermediate texture of the last frame, valid until the next render,
        or nullptr when the tier has none or the last render failed.
    */
    GpuTexture::Ptr getPassOutput (PassOutput output) const;

    /** @internal The shadow renderer of the main view, for tests and debug views, or nullptr before the first render. */
    const ShadowRenderer* getShadowRenderer() const noexcept;

    /** @internal The reflection probe and irradiance volume baker, for tests, or nullptr before the first frame or bake with a probe or a volume. */
    const ProbeBaker* getProbeBaker() const noexcept;

    /** @internal The ambient occlusion pass, for tests, or nullptr before the first frame with ambient occlusion. */
    const AmbientOcclusionPass* getAmbientOcclusionPass() const noexcept;

    /** @internal The screen-space reflection pass, for tests, or nullptr before the first frame tracing them. */
    const ReflectionPass* getReflectionPass() const noexcept;

    /** @internal The Hi-Z pyramid of the screen-space reflections, for tests, created with getReflectionPass(). */
    const HiZPyramid* getHiZPyramid() const noexcept;

    /** @internal The depth of field, for tests, or nullptr before the first frame blurring with it. */
    const DepthOfField* getDepthOfField() const noexcept;

    /** @internal The motion blur, for tests, or nullptr before the first moving frame smeared with it. */
    const MotionBlur* getMotionBlur() const noexcept;

    /** @internal The temporal anti-aliasing, for tests, or nullptr before the first render. */
    const TemporalAA* getTemporalAA() const noexcept;

    /** @internal The world fog pass, for tests, or nullptr before the first frame drawing the scene's WorldFog. */
    const FogApply* getFogApply() const noexcept;

    /** @internal The pass drawing RenderSettings::debugView, for tests, or nullptr before the first full tier frame with one. */
    const FullscreenPass* getDebugViewPass() const noexcept;

    /** @internal The deferred lighting pass of the full tier, for tests, or nullptr before the first render. */
    const LightingPass* getLightingPass() const noexcept;

    /** @internal The forward pass, for tests, or nullptr before the first render. */
    const ForwardPass* getForwardPass() const noexcept;

    /** @internal Test hook, not for production use: edits the facts read from the device before
        every render decides its tier and features, so tests can simulate weaker devices.

        @param newOverride  Called with a copy of the device facts at each render, or empty to use them as read.
    */
    void setDeviceFactsOverride (std::function<void (RenderCapabilities::DeviceFacts&)> newOverride);

    /** @internal Test hook, not for production use: from the next device this renderer sets up
        on, its lookup tables fail to create, as if the device could not create them. Call it
        before the first render. Frames still render, with area lights and the environment
        lighting only diffusely.
    */
    void failLookupTablesForTesting() noexcept { failLookupTables = true; }

    /** @internal Test hook, not for production use: the shadow comparison sampler of this
        renderer fails to create, as if the device could not create it. Call it before the first
        render. Frames still render.
    */
    void failShadowCompareSamplerForTesting() noexcept { failShadowCompareSampler = true; }

    /** @internal Test hook, not for production use: the shadow pipelines of this renderer fail to
        compile, as if the device rejected them. Call it before the first render. Frames still
        render, without shadow maps and without contact shadows.
    */
    void failShadowPipelinesForTesting() noexcept { failShadowPipelines = true; }

    /** @internal Test hook, not for production use: the shadow maps the next render creates (the
        sun cascades and the point and spot light atlas) fail to create, as if the device ran out of
        memory. That frame and the next ones render without those shadow maps, the lights keeping
        their contact shadows, until the shadow settings change their size.
    */
    void failShadowMapsForTesting() noexcept { failShadowMaps = true; }

    /** @internal Test hook, not for production use: the depth and normal prepass of this renderer
        fails to compile, as if the device rejected it. Call it before the first raster render
        drawing it (ambient occlusion, the depth of field blurring, or the world fog). Frames still render, without the
        prepass and the effects reading it.
    */
    void failDepthNormalPrepassForTesting() noexcept { failDepthNormalPrepass = true; }

    /** @internal Test hook, not for production use: the targets of the depth and normal prepass of
        the next @p count raster frames drawing it fail to create, as if the device ran out of memory.
        Those frames still render, without the prepass, and the next ones draw it again.
    */
    void failDepthNormalPrepassTargetsForTesting (int count) noexcept { depthNormalPrepassTargetFailures = count; }

    /** @internal Test hook, not for production use: every reflection probe capture of this
        renderer fails to record, as if the device rejected it. Frames still render, without the
        probes, and the irradiance volumes keep baking.
    */
    void failProbeCapturesForTesting() noexcept { failProbeCaptures = true; }

    /** @internal Test hook, not for production use: every irradiance volume capture of this renderer
        fails to record, as if the device rejected it. Frames still render, without the volumes, and
        the reflection probes keep baking.
    */
    void failVolumeCapturesForTesting() noexcept { failVolumeCaptures = true; }

    /** @internal Test hook, not for production use: the local indirect variants of the full tier's
        LightingPass fail to compile, as if the device rejected them. Call it before the first
        render. Frames still render, their opaque surfaces without reflection probes and irradiance
        volumes.
    */
    void failLocalIndirectLightingForTesting() noexcept { failLocalIndirectLighting = true; }

    /** @internal Test hook, not for production use: the material layers variants of the raster
        ForwardPass and of the full tier's LightingPass fail to compile, as if the device rejected
        them. Call it before the first render. Frames still render, their materials without the
        clearcoat and the sheen (the full ForwardPass, which always shades them, is unaffected).
    */
    void failMaterialLayersForTesting() noexcept { failMaterialLayers = true; }

    /** @internal Test hook, not for production use: the full tier's LightingPass shades every pixel
        of a frame with one variant, as without its classification, while @p disabled is set.
    */
    void disableLightingClassificationForTesting (bool disabled) noexcept { disableLightingClassification = disabled; }

    /** @internal Test hook, not for production use: the classification of the full tier's
        LightingPass fails to compile, as if the device rejected it. Call it before the first render.
        Frames still render, every pixel with one variant.
    */
    void failLightingClassificationForTesting() noexcept { failLightingClassification = true; }

    /** @internal Test hook, not for production use: from the next render on, every screen-space
        reflection pass of this renderer fails, as if the device could not record it. Frames still
        render, without screen-space reflections; the renderer stops trying after the second failure
        in a row, which restarts the accumulation.
    */
    void failScreenSpaceReflectionsForTesting() noexcept { failScreenSpaceReflections = true; }

    /** @internal Test hook, not for production use: from the next render on, every screen-space
        diffuse GI pass of this renderer fails, as if the device could not record it. Frames still
        render, with the ambient occlusion and without the GI; the renderer stops trying after the
        second failure in a row, which restarts the accumulation.
    */
    void failScreenSpaceGIForTesting() noexcept { failScreenSpaceGI = true; }

    /** @internal Test hook, not for production use: the next @p count depth of field passes of this
        renderer fail, every one from now on by default, as if the device could not record them.
        Frames still render, without the blur; the renderer stops trying after the second failure in
        a row.
    */
    void failDepthOfFieldForTesting (int count = std::numeric_limits<int>::max()) noexcept { depthOfFieldFailures = count; }

    /** @internal Test hook, not for production use: the next @p count motion blur passes of this
        renderer fail, every one from now on by default, as if the device could not record them.
        Frames still render, without the smear; the renderer stops trying after the second failure
        in a row.
    */
    void failMotionBlurForTesting (int count = std::numeric_limits<int>::max()) noexcept { motionBlurFailures = count; }

    /** @internal Test hook, not for production use: the next @p count world fog passes of this
        renderer fail, every one from now on by default, as if the device could not record them.
        Frames still render, without the fog; the renderer stops trying after the second failure in
        a row, and the raster tier then leaves the route the fog took (the prepass and half float).
    */
    void failWorldFogForTesting (int count = std::numeric_limits<int>::max()) noexcept { worldFogFailures = count; }

    /** @internal Test hook, not for production use: the environment cube and the other radiance
        textures of this renderer hold RGBM in rgba8unorm, as below the float floor, even when the
        device renders half floats. Call it before the first render.
    */
    void forceRgbmRadianceForTesting() noexcept { forceRgbmRadiance = true; }

    /** @internal Test hook: how many times the frames of this renderer, on its current device,
        turned GPU timing on or opened a GPU scope: 0 while setFrameStatsEnabled() was off.
    */
    int getNumTimingCallsForTesting() const noexcept;

    //==============================================================================
    /** Changes the number of MSAA samples per pixel of the raster tier.

        The count is rounded down to a power of two and limited to what the device supports,
        see GpuDevice::getMaximumSampleCount(). 1 disables multisampling. The full tier always
        renders one sample per pixel, and a change keeps its temporal accumulation.

        @param newSampleCount The requested sample count, 1 or above.
    */
    void setSampleCount (int newSampleCount) noexcept { sampleCount = jmax (1, newSampleCount); }

    /** Returns the requested number of MSAA samples per pixel. */
    int getSampleCount() const noexcept { return sampleCount; }

private:
    struct Impl;

    std::unique_ptr<Impl> impl;
    std::function<void (RenderCapabilities::DeviceFacts&)> deviceFactsOverride;
    RenderSettings renderSettings;
    String lastError;
    String lastGpuFailure;
    FrameStats lastFrameStats;
    uint64 numFrames = 0; // render() calls, numbering the frames of FrameStats
    bool frameStatsEnabled = false;
    int sampleCount = 4;
    bool failLookupTables = false;
    bool failShadowCompareSampler = false;
    bool failShadowPipelines = false;
    bool failShadowMaps = false;
    bool failDepthNormalPrepass = false;
    int depthNormalPrepassTargetFailures = 0;
    bool forceRgbmRadiance = false;
    bool failProbeCaptures = false;
    bool failVolumeCaptures = false;
    bool failLocalIndirectLighting = false;
    bool failMaterialLayers = false;
    bool disableLightingClassification = false;
    bool failLightingClassification = false;
    bool failScreenSpaceReflections = false;
    bool failScreenSpaceGI = false;
    int depthOfFieldFailures = 0;
    int motionBlurFailures = 0;
    int worldFogFailures = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneRenderer)
};

} // namespace yup
