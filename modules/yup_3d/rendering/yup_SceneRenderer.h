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
    with Reinhard, ACES or AgX (see Scene::setToneMapping) and sRGB encoded into an rgba8unorm texture.

    With a Scene::setEnvironment(), surfaces reflect the environment, blurred by their roughness,
    and receive its diffuse light; Scene::setEnvironmentVisible() also draws it behind the scene.
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
        - tone mapping and the bloom settings apply after the accumulation, so changing them
          neither restarts nor discards it.

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
    /** @internal Intermediate textures of the last frame, for tests and debug views. */
    enum class PassOutput
    {
        sceneColor, ///< Full: the rgba16float scene color after temporal anti-aliasing, before the other post effects. Raster: the resolved forward output, rgba16float reversibly tone mapped when bloom is on.
        velocity,     ///< Full: the rg16float screen-space motion, current uv minus previous uv, both unjittered, uv from the top-left. Raster: none.
        gbufferNormal, ///< Full: the rgba16float G-buffer RT1, the octahedral shading normal in xy, the filtered roughness in z and metallic in w, 0 where nothing was drawn. Raster: none.
        prepassNormal, ///< Raster with ambient occlusion: the rgba16float depth and normal prepass target, encoded as the G-buffer RT1, single sampled. Otherwise none.
        ambientOcclusion ///< With ambient occlusion: the rgba8 AmbientOcclusionPass output, the view-space bent normal * 0.5 + 0.5 in rgb and the visibility in a, at half resolution. Otherwise none.
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
        fails to compile, as if the device rejected it. Call it before the first render with ambient
        occlusion on the raster tier. Frames still render, without the prepass.
    */
    void failDepthNormalPrepassForTesting() noexcept { failDepthNormalPrepass = true; }

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

    /** @internal Test hook, not for production use: the environment cube and the other radiance
        textures of this renderer hold RGBM in rgba8unorm, as below the float floor, even when the
        device renders half floats. Call it before the first render.
    */
    void forceRgbmRadianceForTesting() noexcept { forceRgbmRadiance = true; }

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
    String lastError;
    String lastGpuFailure;
    int sampleCount = 4;
    bool failLookupTables = false;
    bool failShadowCompareSampler = false;
    bool failShadowPipelines = false;
    bool failShadowMaps = false;
    bool failDepthNormalPrepass = false;
    bool forceRgbmRadiance = false;
    bool failProbeCaptures = false;
    bool failVolumeCaptures = false;
    bool failLocalIndirectLighting = false;
    bool failMaterialLayers = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneRenderer)
};

} // namespace yup
