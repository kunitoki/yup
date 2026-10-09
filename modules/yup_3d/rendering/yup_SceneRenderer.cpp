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

namespace
{

//==============================================================================
/** FNV-1a over the values a frame depends on. Floats are canonicalized first, so 0 and -0, and
    NaNs of any payload, hash the same.
*/
class FrameSignature
{
public:
    void addBits (uint64 value) noexcept
    {
        addBytes (value, 8);
    }

    void addFloat (float value) noexcept
    {
        auto bits = std::bit_cast<uint32> (value);

        if ((bits & 0x7f800000u) == 0x7f800000u && (bits & 0x007fffffu) != 0)
            bits = 0x7fc00000u;
        else if (bits == 0x80000000u)
            bits = 0;

        addBytes (bits, 4);
    }

    void addFloats (Span<const float> values) noexcept
    {
        for (const auto value : values)
            addFloat (value);
    }

    void addPointer (const void* pointer) noexcept
    {
        addBits (static_cast<uint64> (reinterpret_cast<uintptr_t> (pointer)));
    }

    void addVector (const Vector3<float>& vector) noexcept
    {
        addFloat (vector.getX());
        addFloat (vector.getY());
        addFloat (vector.getZ());
    }

    void addMatrix (const Matrix4& matrix) noexcept
    {
        addFloats (Span<const float> (matrix.getData(), 16));
    }

    uint64 get() const noexcept { return hash; }

    /** The hash through the splitmix64 finalizer. Sums of plain FNV-1a hashes of similar items
        cancel out (two primitives of one mesh changing together leave the sum unchanged), sums of
        finalized ones don't.
    */
    uint64 getFinalized() const noexcept
    {
        auto z = hash;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        return z ^ (z >> 31);
    }

private:
    void addBytes (uint64 value, int count) noexcept
    {
        for (int i = 0; i < count; ++i)
        {
            hash ^= (value >> (i * 8)) & 0xff;
            hash *= 1099511628211ull;
        }
    }

    uint64 hash = 14695981039346656037ull;
};

std::array<const Texture::Ptr*, 5> getMaterialTextures (const Material& material) noexcept
{
    return { &material.baseColorTexture, &material.metallicRoughnessTexture, &material.normalTexture,
             &material.occlusionTexture, &material.emissiveTexture };
}

void addMaterialSignature (FrameSignature& signature, const Material& material)
{
    signature.addPointer (&material);
    signature.addFloats (material.baseColorFactor);
    signature.addFloats (material.emissiveFactor);
    signature.addFloat (material.metallicFactor);
    signature.addFloat (material.roughnessFactor);
    signature.addFloat (material.normalScale);
    signature.addFloat (material.occlusionStrength);
    signature.addFloat (material.alphaCutoff);
    signature.addBits (static_cast<uint64> (material.alphaMode));
    signature.addBits (material.doubleSided ? 1u : 0u);
    signature.addFloat (material.emissiveStrength);
    signature.addFloat (material.ior);
    signature.addFloat (material.specularFactor);
    signature.addFloats (material.specularColorFactor);

    for (const auto* texture : getMaterialTextures (material))
        signature.addPointer (texture->get());
}

} // namespace

//==============================================================================
struct SceneRenderer::Impl
{
    Impl (GpuDevice::Ptr deviceToUse, uint32_t samples)
        : device (std::move (deviceToUse))
        , sampleCount (samples)
        , deviceFacts (RenderCapabilities::queryDevice (*device))
        , frameResources (device)
    {
    }

    /** Creates the pipelines once per target formats and area light variant, remembering a failure instead of retrying every frame. */
    Result prepareResources (const ForwardPass::Targets& targets, uint32_t forwardSamples, bool full, bool areaLights, bool failAreaLightTables)
    {
        if (auto result = shadowRenderer.prepare (device); result.failed())
            return result;

        if (! ltcTablesAttempted)
        {
            ltcTablesAttempted = true;
            ltcMatrix = failAreaLightTables ? nullptr : LtcTables::createMatrixTexture (device);
            ltcAmplitude = failAreaLightTables ? nullptr : LtcTables::createAmplitudeTexture (device);

            // The passes bind black tables instead, which leave area lights their diffuse light only
            if (ltcMatrix == nullptr || ltcAmplitude == nullptr)
            {
                YUP_DBG ("SceneRenderer: unable to create the area light tables, area lights render without specular light");
                ltcMatrix = nullptr;
                ltcAmplitude = nullptr;
            }
        }

        if (full)
        {
            if (auto result = gbufferPass.prepare (device); result.failed())
                return result;

            if (auto result = lightingPass.prepare (device, areaLights); result.failed())
                return result;
        }

        return forwardPass.prepare (device, targets, forwardSamples, areaLights);
    }

    /** The canvas-backed colorTarget of both tiers, plus the depth of the raster tier. */
    bool prepareTargets (int width, int height, bool raster)
    {
        if (colorTarget != nullptr && colorTarget->getWidth() == width && colorTarget->getHeight() == height
            && (depthTexture != nullptr) == raster)
            return true;

        const auto w = static_cast<uint32_t> (width);
        const auto h = static_cast<uint32_t> (height);

        colorTarget = GpuTarget::create (device, width, height);
        depthTexture = nullptr;

        if (! raster)
            return colorTarget != nullptr;

        // With MSAA the pass draws into multisampled depth, and color from getForwardTargets()
        GpuTextureDesc depthDesc (w, h, GpuTextureFormat::depth24plusStencil8, true);
        depthDesc.sampleCount = sampleCount;
        depthDesc.label = "SceneRenderer depth";

        depthTexture = GpuTexture::create (device, depthDesc);

        return colorTarget != nullptr && depthTexture != nullptr;
    }

    /** Raster: the MSAA rgba8 color from the frame pool, released after a frame without it (raster
        HDR), resolved into the canvas-backed colorTarget. Without MSAA, colorTarget itself.
    */
    ForwardPass::Targets getForwardTargets()
    {
        ForwardPass::Targets targets;
        targets.color = colorTarget->asTexture();
        targets.depth = depthTexture;

        if (sampleCount > 1)
        {
            GpuTextureDesc desc (static_cast<uint32_t> (colorTarget->getWidth()), static_cast<uint32_t> (colorTarget->getHeight()), GpuTextureFormat::rgba8unorm, true);
            desc.sampleCount = sampleCount;
            desc.label = "SceneRenderer multisampled color";

            targets.resolve = targets.color;
            targets.color = frameResources.acquire (desc);
        }

        return targets;
    }

    /** Raster HDR opt-in: rgba16float color with the raster sample count, resolved into a
        single-sampled one when multisampled, from the frame pool. The depth stays the raster one.
    */
    ForwardPass::Targets acquireRasterHdrTargets (int width, int height)
    {
        GpuTextureDesc desc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), GpuTextureFormat::rgba16float, true);
        desc.label = "SceneRenderer HDR scene color";

        ForwardPass::Targets targets;
        targets.color = frameResources.acquire (desc);
        targets.depth = depthTexture;

        if (sampleCount > 1 && targets.color != nullptr)
        {
            desc.sampleCount = sampleCount;
            desc.label = "SceneRenderer multisampled HDR scene color";

            targets.resolve = targets.color;
            targets.color = frameResources.acquire (desc);
        }

        return targets;
    }

    /** Full: single-sampled HDR scene color, sampled depth and velocity, from the frame pool. */
    ForwardPass::Targets acquireFullTargets (int width, int height)
    {
        const auto acquire = [&] (GpuTextureFormat format, const char* label)
        {
            GpuTextureDesc desc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), format, true);
            desc.label = label;
            return frameResources.acquire (desc);
        };

        ForwardPass::Targets targets;
        targets.color = acquire (GpuTextureFormat::rgba16float, "SceneRenderer scene color");
        targets.depth = acquire (GpuTextureFormat::depth32float, "SceneRenderer depth");
        targets.velocity = acquire (GpuTextureFormat::rg16float, "SceneRenderer velocity");
        return targets;
    }

    /** Copies @p items into this frame's storage buffer of @p slot, once per frame and slot. The
        buffer has room for one item even without any, as the shader arrays it is bound to need.
    */
    template <class Item>
    GpuBuffer::Ptr uploadStorageBuffer (FrameResources::StorageSlot slot, Span<const Item> items)
    {
        auto buffer = frameResources.acquireStorageBuffer (slot, jmax (items.size(), size_t (1)) * sizeof (Item));
        if (buffer == nullptr || (! items.empty() && ! device->updateBuffer (buffer, items.data(), items.size() * sizeof (Item))))
            return nullptr;

        return buffer;
    }

    /** Full: uploads the packed lights and lists the ones reaching each cluster of the view, for
        the shading passes to read through @p lighting.
    */
    bool cullLights (GpuFrame& frame, LightingInputs& lighting)
    {
        const auto& packed = lighting.packedLights;
        const auto numLights = static_cast<int> (packed.size());
        const auto firstLocalLight = static_cast<int> (std::count_if (packed.begin(), packed.end(), [] (const GpuLight& light)
        {
            return light.positionType[3] < 0.5f;
        }));

        const auto grid = LightClusters::getGrid (viewData);
        LightClusters::packGrid (grid, numLights, firstLocalLight, lighting.block.clusterGrid, lighting.block.clusterDepth);

        lighting.lights = uploadStorageBuffer (FrameResources::StorageSlot::lights, Span<const GpuLight> (packed));
        lighting.clusterLights = frameResources.acquireStorageBuffer (FrameResources::StorageSlot::clusterLights, LightClusters::getClusterBufferSize (grid));

        return lighting.lights != nullptr && lighting.clusterLights != nullptr
            && lightClusters.dispatch (frame, device, grid, lighting.lights, numLights, firstLocalLight,
                                       viewData.getView(), viewData.getJitteredProjection().inverted(), lighting.clusterLights);
    }

    /** Nodes this renderer did not draw in its previous frame have no motion: their previous world is the current one. */
    void resolvePreviousTransforms()
    {
        for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
        {
            for (const auto& item : items)
            {
                if (item.node != nullptr && (lastStamp == 0 || item.node->previousStamp != lastStamp))
                    item.node->previousWorld = item.world;
            }
        }
    }

    /** Stamps every drawn node with this frame, so the next frame of this renderer reads its transform as the previous one. */
    void commitPreviousTransforms (uint64 stamp)
    {
        for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
        {
            for (const auto& item : items)
            {
                if (item.node != nullptr)
                {
                    item.node->previousWorld = item.world;
                    item.node->previousStamp = stamp;
                }
            }
        }

        lastStamp = stamp;
    }

    /** Adds one signature per draw item, summed: the lists are sorted by state and view distance,
        so a camera move or a transform can reorder them without anything else changing.
    */
    template <class AddItem>
    void addItemSignatures (FrameSignature& signature, AddItem&& addItem) const
    {
        for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
        {
            uint64 sum = 0;

            for (const auto& item : items)
            {
                FrameSignature itemSignature;
                addItem (itemSignature, item);
                sum += itemSignature.getFinalized();
            }

            signature.addBits (items.size());
            signature.addBits (sum);
        }
    }

    /** The changes that discard the history, which no longer fits the frame. */
    uint64 getInvalidatingSignature (int width, int height, int taaSamples) const
    {
        FrameSignature signature;
        signature.addBits (static_cast<uint64> (width));
        signature.addBits (static_cast<uint64> (height));
        signature.addBits (static_cast<uint64> (capabilities->getTier()));
        signature.addBits (static_cast<uint64> (taaSamples));
        return signature.get();
    }

    /** Everything else shaping the scene color before TAA. A change restarts the convergence and
        keeps the history, which TAA reprojects and clips to the new frame. Tone mapping and bloom
        run after TAA, so their settings are left out.
    */
    uint64 getRestartingSignature (const Scene& scene) const
    {
        FrameSignature signature;
        signature.addMatrix (viewData.getUnjitteredViewProjection());
        signature.addFloat (scene.getExposure());
        signature.addBits (scene.getAmbientColor().getARGB());
        signature.addPointer (scene.getEnvironment().get());
        signature.addFloat (scene.getEnvironmentIntensity());
        signature.addFloat (scene.getEnvironmentBlur());
        signature.addBits (scene.isEnvironmentVisible() ? 1u : 0u);

        // RenderSettings fields shaping the scene color before TAA go here: none yet, bloom runs after it

        // Each item's transform is hashed with what it draws, so swapping transforms between items still counts
        addItemSignatures (signature, [] (FrameSignature& itemSignature, const SceneDrawList::DrawItem& item)
        {
            itemSignature.addMatrix (item.world);
            itemSignature.addPointer (item.mesh);
            itemSignature.addBits (static_cast<uint64> (item.primitiveIndex));
            addMaterialSignature (itemSignature, *item.material);

            for (const auto* texture : getMaterialTextures (*item.material))
                itemSignature.addBits (*texture != nullptr ? (*texture)->getChangeCount() : 0u);
        });

        signature.addBits (drawList.getLights().size());

        for (const auto& light : drawList.getLights())
        {
            const auto& node = *light.node;
            signature.addBits (static_cast<uint64> (node.type));
            signature.addFloats (node.color);
            signature.addFloat (node.intensity);
            signature.addFloat (node.range);
            signature.addFloat (node.innerConeAngle);
            signature.addFloat (node.outerConeAngle);
            signature.addFloat (node.radius);
            signature.addBits (node.castsShadows ? 1u : 0u);
            signature.addVector (light.position);
            signature.addVector (light.direction);
        }

        return signature.get();
    }

    /** Full tier: compares this frame with the previous one. A new size, tier or TAA sample count
        discards the history and starts over; any other change only restarts the convergence. The
        first unchanged frame after changes counts the history as a single sample, so the frames
        that follow average it out instead of keeping a share of the image before the changes.
    */
    void trackChanges (const Scene& scene, const RenderSettings& settings, int width, int height)
    {
        const auto invalidating = getInvalidatingSignature (width, height, settings.taaSamples);
        const auto restarting = getRestartingSignature (scene);

        if (! hasSignature || invalidating != invalidatingSignature)
        {
            accumulatedSamples = 0;
            stillSamples = 0;
            changing = false;
            frameResources.invalidateHistory();
        }
        else if (restarting != restartingSignature)
        {
            stillSamples = 0;
            changing = true;
        }
        else if (changing)
        {
            accumulatedSamples = 1;
            changing = false;
        }

        hasSignature = true;
        invalidatingSignature = invalidating;
        restartingSignature = restarting;
    }

    /** The TAA sample index: kept at the end of the ramp while things change, so the history still counts. */
    int getTemporalSampleIndex() const noexcept
    {
        if (stillSamples >= samplesToConverge)
            return samplesToConverge;

        return jmin (accumulatedSamples, samplesToConverge - 1);
    }

    /** Frame stamps shared by every renderer, so two renderers never take each other's frames as their own. */
    static inline std::atomic<uint64> nextStamp { 0 };

    GpuDevice::Ptr device;
    uint32_t sampleCount = 1;
    RenderCapabilities::DeviceFacts deviceFacts;
    std::optional<RenderCapabilities> capabilities;
    FrameResources frameResources;
    ViewData viewData;
    SceneDrawList drawList;
    ShadowRenderer shadowRenderer;
    LightClusters lightClusters;
    GBufferPass gbufferPass;
    MaterialTable materialTable;
    LightingPass lightingPass;
    ForwardPass forwardPass;
    PostChain postChain;
    GpuTarget::Ptr colorTarget;
    GpuTexture::Ptr depthTexture;
    GpuTexture::Ptr ltcMatrix;    // The area light tables of this renderer and device, created once, or nullptr if that failed
    GpuTexture::Ptr ltcAmplitude;
    bool ltcTablesAttempted = false;
    GpuTexture::Ptr sceneColorOutput;
    GpuTexture::Ptr velocityOutput;
    GpuTexture::Ptr gbufferNormalOutput;
    uint64 frameIndex = 0;
    uint64 lastStamp = 0;
    uint64 invalidatingSignature = 0;
    uint64 restartingSignature = 0;
    bool hasSignature = false;
    bool changing = false;      // the previous frames restarted the convergence, see trackChanges()
    int samplesToConverge = 0;  // full: the TAA samples (1 without TAA), raster: 0
    int accumulatedSamples = 0; // frames in the history since its last reset
    int stillSamples = 0;       // frames since anything last changed

    /** The GPU outcomes of this renderer's submitted frames not yet known to have succeeded. */
    std::vector<GpuFrameStatus> pendingFrames;

    /** Forgets the frames the GPU finished, waiting for all when @p wait is set. On a failure the
        targets and history were left unwritten, so the history is discarded. */
    Result takeFinishedFrames (bool wait)
    {
        auto result = Result::ok();

        for (auto it = pendingFrames.begin(); it != pendingFrames.end();)
        {
            const auto outcome = wait ? std::optional<Result> (it->wait()) : it->poll();
            if (! outcome.has_value())
            {
                ++it;
                continue;
            }

            if (outcome->failed() && result.wasOk())
                result = *outcome;

            it = pendingFrames.erase (it);
        }

        if (result.failed())
        {
            frameResources.invalidateHistory();
            hasSignature = false;
            accumulatedSamples = 0;
            stillSamples = 0;
        }

        return result;
    }
};

//==============================================================================

SceneRenderer::SceneRenderer() = default;

SceneRenderer::~SceneRenderer() = default;

//==============================================================================
const RenderCapabilities* SceneRenderer::getCapabilities() const noexcept
{
    return impl != nullptr && impl->capabilities.has_value() ? &*impl->capabilities : nullptr;
}

bool SceneRenderer::isConverged() const noexcept
{
    return impl == nullptr || lastError.isNotEmpty() || impl->stillSamples >= impl->samplesToConverge;
}

Result SceneRenderer::waitForGPU()
{
    if (impl == nullptr)
        return Result::ok();

    return impl->takeFinishedFrames (true);
}

void SceneRenderer::restartAccumulation() noexcept
{
    if (impl == nullptr)
        return;

    // The next render sees no previous signature, so it discards the history and starts over
    impl->hasSignature = false;
    impl->accumulatedSamples = 0;
    impl->stillSamples = 0;
}

GpuTexture::Ptr SceneRenderer::getPassOutput (PassOutput output) const
{
    if (impl == nullptr)
        return nullptr;

    switch (output)
    {
        case PassOutput::sceneColor: return impl->sceneColorOutput;
        case PassOutput::velocity: return impl->velocityOutput;
        case PassOutput::gbufferNormal: return impl->gbufferNormalOutput;
    }

    return nullptr;
}

void SceneRenderer::setDeviceFactsOverride (std::function<void (RenderCapabilities::DeviceFacts&)> newOverride)
{
    deviceFactsOverride = std::move (newOverride);
}

//==============================================================================

GpuTexture::Ptr SceneRenderer::render (const GpuDevice::Ptr& device, Scene& scene, int widthPx, int heightPx)
{
    const auto fail = [this] (const String& error) -> GpuTexture::Ptr
    {
        // A failed frame leaves no pass outputs behind
        if (impl != nullptr)
        {
            impl->sceneColorOutput = nullptr;
            impl->velocityOutput = nullptr;
            impl->gbufferNormalOutput = nullptr;
        }

        lastError = error;
        return nullptr;
    };

    // A frame of this renderer the GPU failed to execute left its history unwritten: it was
    // discarded, and this frame renders anew rather than leaving the caller without an image
    lastGpuFailure = {};
    if (impl != nullptr)
        if (const auto gpuResult = impl->takeFinishedFrames (false); gpuResult.failed())
            lastGpuFailure = gpuResult.getErrorMessage();

    if (device == nullptr || widthPx <= 0 || heightPx <= 0)
        return fail ("No device, or an empty size");

    // The largest power of two within both the request and what the device supports
    uint32_t samples = 1;
    while (samples * 2 <= static_cast<uint32_t> (sampleCount) && samples * 2 <= device->getMaximumSampleCount())
        samples *= 2;

    if (impl == nullptr || device != impl->device)
    {
        impl = std::make_unique<Impl> (device, samples);
    }
    else if (samples != impl->sampleCount)
    {
        // Only the multisampled raster depth depends on the sample count, and prepareTargets()
        // recreates the raster targets without it. The full tier keeps its targets and accumulation
        impl->sampleCount = samples;
        impl->depthTexture = nullptr;
    }

    auto deviceFacts = impl->deviceFacts;
    if (deviceFactsOverride)
        deviceFactsOverride (deviceFacts);

    const auto& settings = scene.getRenderSettings();
    impl->capabilities.emplace (deviceFacts, settings, samples);

    // The full tier shades one sample per pixel into HDR targets, then runs the post chain. The
    // raster tier does the same with its samples only for effects needing HDR, like bloom
    const auto full = impl->capabilities->getTier() == RenderCapabilities::Tier::full;
    const auto hdr = impl->capabilities->isEnabled (RenderCapabilities::Feature::hdrSceneColor, settings);
    const auto forwardSamples = full ? 1u : samples;
    const auto temporalAntiAliasing = impl->capabilities->isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, settings);

    if (! impl->prepareTargets (widthPx, heightPx, ! full))
        return fail ("Unable to create the render targets");

    // A new size releases the pool's textures, which the fullscreen passes' render targets would keep alive
    if (widthPx != impl->frameResources.getWidth() || heightPx != impl->frameResources.getHeight())
    {
        impl->postChain.releaseTargets();
        impl->gbufferPass.releaseTargets();
        impl->lightingPass.releaseTargets();
    }

    impl->frameResources.beginFrame (widthPx, heightPx);

    // Every exit from here on ends the frame of the resource pool
    const auto failFrame = [&] (const String& error)
    {
        // The history may not have been written: the next frame starts over
        impl->frameResources.invalidateHistory();
        impl->postChain.releaseTargets();
        impl->gbufferPass.releaseTargets();
        impl->lightingPass.releaseTargets();
        impl->frameResources.endFrame();
        impl->hasSignature = false;
        return fail (error);
    };

    const auto targets = full ? impl->acquireFullTargets (widthPx, heightPx)
                              : (hdr ? impl->acquireRasterHdrTargets (widthPx, heightPx) : impl->getForwardTargets());
    if (targets.color == nullptr || targets.depth == nullptr || (full && targets.velocity == nullptr))
        return failFrame ("Unable to create the scene targets");

    // Full: the G-buffer shares the depth and velocity the blended items are drawn over
    const auto gbuffer = full ? GBufferPass::acquireTargets (impl->frameResources, targets.velocity, targets.depth) : GBufferPass::Targets();
    if (full && (gbuffer.albedoOcclusion == nullptr || gbuffer.normalRoughness == nullptr || gbuffer.emissionMaterial == nullptr))
        return failFrame ("Unable to create the G-buffer targets");

    const auto stamp = ++Impl::nextStamp;

    // The jitter cycles with the frame index: any taaSamples consecutive frames cover every point
    impl->viewData.update (scene, widthPx, heightPx, temporalAntiAliasing, settings.taaSamples, impl->frameIndex++);

    auto& drawList = impl->drawList;
    // The raster tier lights each draw with its own selection, the full tier through the clusters
    drawList.build (*scene.getRoot(), impl->viewData.getView(), { scene.isUsingDefaultLight(), ! full });

    // A light with a radius selects the area light variants of the shading passes. Only a change
    // of the lights switches variants, and the restarting signature hashes every light radius
    const auto areaLights = LightingInputs::hasAreaLights (drawList.getLights());
    if (auto result = impl->prepareResources (targets, forwardSamples, full, areaLights, failAreaLightTables); result.failed())
        return failFrame (result.getErrorMessage());

    impl->resolvePreviousTransforms();

    // Before TAA fetches its history, so a change invalidates it for this very frame
    impl->samplesToConverge = full ? (temporalAntiAliasing ? settings.taaSamples : 1) : 0;
    impl->hasSignature = impl->hasSignature && full;

    if (full)
        impl->trackChanges (scene, settings, widthPx, heightPx);

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return failFrame ("Unable to begin a GPU frame");

    impl->pendingFrames.push_back (frame.getStatus());

    const auto shadow = impl->shadowRenderer.render (device, frame, drawList);
    if (shadow.error.isNotEmpty())
        return failFrame (shadow.error);

    auto lighting = LightingInputs::create (device, scene, drawList.getLights(), shadow);
    lighting.ltcMatrix = impl->ltcMatrix;
    lighting.ltcAmplitude = impl->ltcAmplitude;
    const auto output = full ? ForwardPass::Output::hdrLinear : (hdr ? ForwardPass::Output::hdrReversible : ForwardPass::Output::ldrInline);

    if (full)
    {
        // Lights culled into clusters, the opaque items into the G-buffer, shaded once per pixel
        if (! impl->cullLights (frame, lighting))
            return failFrame ("Unable to cull the lights");

        if (! impl->gbufferPass.render (frame, impl->viewData, drawList.getOpaqueItems(), impl->materialTable, gbuffer))
            return failFrame ("Unable to render the G-buffer");

        // Uploaded once the G-buffer draws have numbered the materials
        lighting.materials = impl->uploadStorageBuffer (FrameResources::StorageSlot::materials, impl->materialTable.getEntries());
        if (lighting.materials == nullptr)
            return failFrame ("Unable to upload the material table");

        if (! impl->lightingPass.render (frame, impl->viewData, gbuffer, lighting, targets.color))
            return failFrame ("Unable to render the lighting pass");

        impl->lightingPass.trimUnusedTargets();
    }
    else
    {
        impl->gbufferPass.releaseTargets();
        impl->lightingPass.releaseTargets();
    }

    // Raster: every item over the cleared targets, HDR scene color starting transparent for tone
    // mapping to composite the display-referred background under it. Full: the blended items over
    // the shaded G-buffer, depth tested against it, writing their motion over its velocity
    const auto clear = full ? std::optional<GpuColor>()
                            : std::optional<GpuColor> (output == ForwardPass::Output::ldrInline ? GpuColor (scene.getBackgroundColor()) : GpuColor::transparentBlack());
    const auto opaque = full ? Span<const SceneDrawList::DrawItem>() : drawList.getOpaqueItems();

    if (! impl->forwardPass.render (device, frame, impl->viewData, lighting, opaque, drawList.getBlendItems(), targets, output, forwardSamples, clear))
        return failFrame ("Unable to render the forward pass");

    if (! hdr)
        impl->postChain.releaseTargets();

    if (hdr)
    {
        // Raster: the resolved color, reversibly tone mapped, and no depth or motion for the post effects
        const auto sceneColor = targets.resolve != nullptr ? targets.resolve : targets.color;
        const PostChain::Inputs inputs { sceneColor, full ? targets.depth : nullptr, targets.velocity, ! full, scene.getBackgroundColor(), impl->getTemporalSampleIndex() };
        if (! impl->postChain.render (device, frame, *impl->capabilities, settings, impl->viewData, impl->frameResources, inputs, impl->colorTarget->asTexture()))
            return failFrame ("Unable to render the post effects");
    }

    frame.submit();
    impl->commitPreviousTransforms (stamp);
    impl->frameResources.endFrame();

    impl->accumulatedSamples = jmin (impl->accumulatedSamples + 1, impl->samplesToConverge);
    impl->stillSamples = jmin (impl->stillSamples + 1, impl->samplesToConverge);

    impl->sceneColorOutput = hdr ? impl->postChain.getResolvedColor() : impl->colorTarget->asTexture();
    impl->velocityOutput = targets.velocity;
    impl->gbufferNormalOutput = gbuffer.normalRoughness;

    lastError = {};
    return impl->colorTarget->asTexture();
}

} // namespace yup
