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
        gbufferNormal ///< Full: the rgba16float G-buffer RT1, the octahedral shading normal in xy, the filtered roughness in z and metallic in w, 0 where nothing was drawn. Raster: none.
    };

    /** @internal Returns an intermediate texture of the last frame, valid until the next render,
        or nullptr when the tier has none or the last render failed.
    */
    GpuTexture::Ptr getPassOutput (PassOutput output) const;

    /** @internal The number of frames that drew shadow maps since the renderer set up its device,
        for tests: still frames reuse the maps of the frame before.
    */
    int getShadowMapRenderCount() const noexcept;

    /** @internal The number of frames that drew sun cascade @p cascade since the renderer set up
        its device, for tests: a cascade is drawn again only when what it holds changed.
    */
    int getShadowCascadeRenderCount (int cascade) const noexcept;

    /** @internal The number of shadow casters drawn into shadow maps since the renderer set up its
        device, for tests and benchmarks: the draws the shadow maps cost.
    */
    int getShadowCasterDrawCount() const noexcept;

    /** @internal The shadow records the last frame wrote, the sun cascades and the views of the
        point and spot lights, plus one record per light with contact shadows but no shadow map, for
        tests: the tier's record budget bounds them.
    */
    int getShadowRecordCount() const noexcept;

    /** @internal The shadow records and sun parameters the last frame wrote, for tests. */
    GpuShadowData getShadowData() const noexcept;

    /** @internal True when the last frame asked its shading passes for their shadow filter
        variants (LightingInputs::shadowFilters), for tests.
    */
    bool usesShadowFilters() const noexcept;

    /** @internal The side in texels of the point and spot light shadow atlas the renderer holds,
        0 when no light needs one, for tests.
    */
    int getShadowAtlasSize() const noexcept;

    /** @internal The side in texels of the atlas tiles of light @p lightIndex (an index into the
        lights of the last frame, in scene order) in the last frame, 0 when it has none, for tests.
    */
    int getShadowTileSize (int lightIndex) const noexcept;

    /** @internal Test hook, not for production use: edits the facts read from the device before
        every render decides its tier and features, so tests can simulate weaker devices.

        @param newOverride  Called with a copy of the device facts at each render, or empty to use them as read.
    */
    void setDeviceFactsOverride (std::function<void (RenderCapabilities::DeviceFacts&)> newOverride);

    /** @internal Test hook, not for production use: from the next device this renderer sets up
        on, its area light tables fail to create, as if the device could not create them. Call it
        before the first render. Frames still render, with area lights lighting only diffusely.
    */
    void failAreaLightTablesForTesting() noexcept { failAreaLightTables = true; }

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
    bool failAreaLightTables = false;
    bool failShadowCompareSampler = false;
    bool failShadowPipelines = false;
    bool failShadowMaps = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneRenderer)
};

} // namespace yup
