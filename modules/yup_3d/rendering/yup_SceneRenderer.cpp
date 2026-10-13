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

/** The largest power of two within both @p requested and what @p device supports. */
uint32_t getSupportedSampleCount (int requested, const GpuDevice& device) noexcept
{
    uint32_t samples = 1;
    while (samples * 2 <= static_cast<uint32_t> (requested) && samples * 2 <= device.getMaximumSampleCount())
        samples *= 2;

    return samples;
}

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

    /** Creates the shadow and G-buffer pipelines and the lookup tables once, remembering a failure instead of retrying every frame.
        Only a G-buffer failure fails the frame: without shadow pipelines or lookup tables the frames render without them.
    */
    Result prepareResources (bool full, bool failLookupTables)
    {
        // A failure, logged by the shadow renderer, leaves the frames unshadowed
        shadowRenderer.prepare (device);

        if (! lookupTablesAttempted)
        {
            lookupTablesAttempted = true;
            lookupTables = failLookupTables ? nullptr : LookupTables::createTexture (device);

            // Black tables instead leave area lights and the environment their diffuse light only
            if (lookupTables == nullptr)
            {
                YUP_DBG ("SceneRenderer: unable to create the lookup tables, area lights and the environment render without specular light");
                lookupTables = LookupTables::createBlackTexture (device);
            }
        }

        if (full)
        {
            if (auto result = gbufferPass.prepare (device); result.failed())
                return result;
        }

        return Result::ok();
    }

    /** Compiles the default variants of the shading passes, for a frame shading with them: LightingPass
        on full, and ForwardPass when @p forwardDraws (raster, or full with blended items). Once per
        target formats and area light variant, remembering a failure instead of retrying every frame.
    */
    Result prepareDefaultShading (const ForwardPass::Targets& targets, uint32_t forwardSamples, bool full, bool areaLights, bool forwardDraws)
    {
        if (full)
        {
            if (auto result = lightingPass.prepare (device, areaLights, false); result.failed())
                return result;
        }

        return forwardDraws ? forwardPass.prepare (device, targets, forwardSamples, areaLights) : Result::ok();
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

    /** Raster: draws the depth and normal prepass while ambient occlusion is enabled. It is created
        by the first frame needing it, and only releases its targets on the frames without it, so
        turning ambient occlusion back on compiles nothing. A prepass that cannot compile is logged
        once and leaves the frames without it. Returns false only when the prepass could not be
        recorded. @p failForTesting makes a new prepass fail to compile.
    */
    bool renderDepthNormalPrepass (GpuFrame& frame, const RenderSettings& settings, int width, int height, bool failForTesting)
    {
        if (capabilities->getTier() == RenderCapabilities::Tier::full
            || ! capabilities->isEnabled (RenderCapabilities::Feature::ambientOcclusion, settings))
        {
            if (depthNormalPrepass != nullptr)
                depthNormalPrepass->releaseTargets();

            return true;
        }

        if (depthNormalPrepass == nullptr)
        {
            depthNormalPrepass = std::make_unique<DepthNormalPrepass> (device);
            if (failForTesting)
                depthNormalPrepass->failPipelinesForTesting();
        }

        if (depthNormalPrepass->prepare().failed())
            return true;

        return depthNormalPrepass->render (frame, viewData, drawList, frameResources, width, height);
    }

    /** Traces the ambient occlusion of @p depth and @p normalRoughness into lighting.ambientOcclusion
        while it is enabled and both exist: the G-buffer's on full, the prepass's on raster. The pass
        is created by the first frame needing it, and only releases its targets on the frames without
        it, so turning ambient occlusion back on compiles nothing. A pass that cannot compile, or
        targets that cannot be created, are logged once and leave the frames without it, the shading
        passes binding white. Block::probeInfo.w is the frame's ambient occlusion flag, which LightingPass reads. Returns false
        only when the pass could not be recorded.
    */
    bool renderAmbientOcclusion (GpuFrame& frame, const RenderSettings& settings, bool temporal,
                                 const GpuTexture::Ptr& depth, const GpuTexture::Ptr& normalRoughness, LightingInputs& lighting)
    {
        lighting.ambientOcclusion = nullptr;
        lighting.block.probeInfo[3] = 0.0f;

        if (! capabilities->isEnabled (RenderCapabilities::Feature::ambientOcclusion, settings) || depth == nullptr || normalRoughness == nullptr)
        {
            if (ambientOcclusionPass != nullptr)
                ambientOcclusionPass->releaseTargets();

            return true;
        }

        if (ambientOcclusionPass == nullptr)
            ambientOcclusionPass = std::make_unique<AmbientOcclusionPass> (device);

        if (ambientOcclusionPass->prepare().failed())
            return true;

        if (! ambientOcclusionPass->render (frame, viewData, capabilities->getTier(), temporal, depth, normalRoughness, settings.aoDistance, frameResources))
            return false;

        ambientOcclusionPass->trimUnusedTargets();
        lighting.ambientOcclusion = ambientOcclusionPass->getOutput();
        lighting.block.probeInfo[3] = lighting.ambientOcclusion != nullptr ? 1.0f : 0.0f;
        return true;
    }

    /** Bakes the pending reflection probes of @p probeList into @p frame, through the forward pass
        (see ProbeBaker), and returns how many were baked. The baker is created by the first frame or
        bake with a probe, then kept, so a scene without probes allocates nothing. A bake that fails
        never fails the frame: the probes shade with weight 0 until they bake again.
        @p failCapturesForTesting makes every capture fail.
    */
    int bakeReflectionProbes (GpuFrame& frame, const Scene& scene, const SceneDrawList& probeList, const RenderCapabilities::DeviceFacts& facts,
                              const GpuTexture::Ptr& environmentCube, const FrameResources::Samplers& samplers, bool failCapturesForTesting)
    {
        if (probeBaker == nullptr)
        {
            if (probeList.getReflectionProbes().empty())
                return 0;

            probeBaker = std::make_unique<ProbeBaker> (probeRenderer);
        }

        probeBaker->failCapturesForTesting (failCapturesForTesting);

        const ProbeBaker::Inputs inputs { scene, probeList, facts, environmentCube, lookupTables, samplers };
        return probeBaker->bake (frame, forwardPass, inputs);
    }

    /** The reflection probes of the frame for the shading passes, Block::probeInfo y counting them:
        on raster every probe of the draw list, which each draw picks from; on full the baked ones by
        priority in a storage buffer, uploaded every full frame (sized for one probe without any).
        Returns false only when that buffer could not be uploaded.
    */
    bool prepareReflectionProbes (LightingInputs& lighting, bool full)
    {
        // A baker that gave up shades no probe
        const auto hasProbes = probeBaker != nullptr && ! probeBaker->hasGivenUp() && ! drawList.getReflectionProbes().empty();
        lighting.reflectionAtlas = hasProbes ? probeBaker->getReflectionAtlas() : nullptr;

        if (full)
        {
            const auto probes = hasProbes ? probeBaker->packReflectionProbesByPriority (drawList) : std::vector<GpuReflectionProbe>();
            lighting.block.probeInfo[1] = static_cast<float> (probes.size());
            lighting.reflectionProbeBuffer = uploadStorageBuffer (FrameResources::StorageSlot::reflectionProbes, Span<const GpuReflectionProbe> (probes));
            return lighting.reflectionProbeBuffer != nullptr;
        }

        if (hasProbes)
            lighting.reflectionProbes = probeBaker->packReflectionProbes (drawList);

        lighting.block.probeInfo[1] = static_cast<float> (lighting.reflectionProbes.size());
        return true;
    }

    /** Makes the reflection probes bake again: for frames the GPU did not execute. */
    void forgetProbeBakes() noexcept
    {
        if (probeBaker != nullptr)
            probeBaker->forgetBakes();
    }

    /** Releases the render targets the passes keep for the next frames. */
    void releasePassTargets() noexcept
    {
        postChain.releaseTargets();
        gbufferPass.releaseTargets();
        lightingPass.releaseTargets();

        if (depthNormalPrepass != nullptr)
            depthNormalPrepass->releaseTargets();

        if (ambientOcclusionPass != nullptr)
            ambientOcclusionPass->releaseTargets();
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

    /** Adds one signature per draw item of @p items, summed: the lists are sorted by state and view
        distance, so a camera move or a transform can reorder them without anything else changing.
    */
    template <class AddItem>
    static void addItemSignatures (FrameSignature& signature, Span<const SceneDrawList::DrawItem> items, AddItem&& addItem)
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
    uint64 getRestartingSignature (const Scene& scene, const RenderSettings& settings, const GpuTexture* environmentCube, int pendingBakes) const
    {
        FrameSignature signature;
        signature.addMatrix (viewData.getUnjitteredViewProjection());
        signature.addFloat (scene.getExposure());
        signature.addBits (scene.getAmbientColor().getARGB());
        signature.addPointer (scene.getEnvironment().get());
        signature.addPointer (environmentCube); // Rebuilt after the GPU failed to build it
        signature.addFloat (scene.getEnvironmentIntensity());
        signature.addFloat (scene.getEnvironmentBlur());
        signature.addBits (scene.isEnvironmentVisible() ? 1u : 0u);

        // The RenderSettings shaping the scene color before TAA, as the tier resolves them; bloom runs after it
        signature.addBits (static_cast<uint64> (capabilities->getCascadeCount (settings)));
        signature.addBits (static_cast<uint64> (RenderCapabilities::getShadowResolution (settings)));
        signature.addBits (capabilities->isEnabled (RenderCapabilities::Feature::softShadows, settings) ? 1u : 0u);
        signature.addBits (capabilities->isEnabled (RenderCapabilities::Feature::contactShadows, settings) ? 1u : 0u);

        const auto ambientOcclusion = capabilities->isEnabled (RenderCapabilities::Feature::ambientOcclusion, settings);
        signature.addBits (ambientOcclusion ? 1u : 0u);
        if (ambientOcclusion)
            signature.addFloat (settings.aoDistance);

        // Each item's transform is hashed with what it draws, so swapping transforms between items still counts
        const auto addItem = [] (FrameSignature& itemSignature, const SceneDrawList::DrawItem& item)
        {
            itemSignature.addMatrix (item.world);
            itemSignature.addPointer (item.mesh);
            itemSignature.addBits (static_cast<uint64> (item.primitiveIndex));
            addMaterialSignature (itemSignature, *item.material);

            for (const auto* texture : getMaterialTextures (*item.material))
                itemSignature.addBits (*texture != nullptr ? (*texture)->getChangeCount() : 0u);
        };

        for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
            addItemSignatures (signature, items, addItem);

        // The reflection probes: their fields and placement, and their bakes, the ones this frame records included
        const auto bakes = probeBaker != nullptr ? probeBaker->getGeneration() : 0;
        signature.addBits (bakes + (pendingBakes > 0 ? 1u : 0u));
        signature.addBits (drawList.getReflectionProbes().size());

        for (const auto& probe : drawList.getReflectionProbes())
            signature.addBits (ProbeBaker::getSignature (probe));

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
            signature.addFloat (node.shadowBias);
            signature.addBits (node.contactShadows ? 1u : 0u);
            signature.addFloat (node.contactShadowDistance);
            signature.addFloat (node.contactShadowThickness);
            signature.addVector (light.position);
            signature.addVector (light.direction);
        }

        return signature.get();
    }

    /** What every sun cascade depends on besides its fit and casters, which ShadowRenderer compares
        itself: the shadow settings of the tier and the shadow casting directional lights. Fills
        shadowCasterSignatures with the signature of each caster (transform, mesh and what alpha
        masking reads), in the order of the opaque items. The point and spot lights reach the atlas
        through the matrices of their views.
    */
    uint64 getShadowSignature (const RenderSettings& settings)
    {
        FrameSignature global;
        global.addBits (static_cast<uint64> (capabilities->getTier()));
        global.addBits (static_cast<uint64> (capabilities->getCascadeCount (settings)));
        global.addBits (static_cast<uint64> (RenderCapabilities::getShadowResolution (settings)));

        for (const auto& light : drawList.getLights())
        {
            const auto& node = *light.node;
            if (! node.castsShadows || node.type != LightNode::Type::directional)
                continue;

            global.addBits (static_cast<uint64> (node.type));
            global.addBits (GpuSceneData::isDark (node) ? 1u : 0u);
            global.addVector (light.position);
            global.addVector (light.direction);
        }

        // The casters: the opaque and masked items
        shadowCasterSignatures.clear();

        for (const auto& item : drawList.getOpaqueItems())
        {
            FrameSignature itemSignature;
            itemSignature.addMatrix (item.world);
            itemSignature.addPointer (item.mesh);
            itemSignature.addBits (static_cast<uint64> (item.primitiveIndex));

            const auto& material = *item.material;
            itemSignature.addBits (static_cast<uint64> (material.alphaMode));

            if (material.alphaMode == Material::AlphaMode::mask)
            {
                itemSignature.addFloat (material.alphaCutoff);
                itemSignature.addFloat (material.baseColorFactor[3]);
                itemSignature.addPointer (material.baseColorTexture.get());
                itemSignature.addBits (material.baseColorTexture != nullptr ? material.baseColorTexture->getChangeCount() : 0u);
            }

            shadowCasterSignatures.push_back (itemSignature.getFinalized());
        }

        return global.get();
    }

    /** Full tier: compares this frame with the previous one. A new size, tier or TAA sample count
        discards the history and starts over; any other change only restarts the convergence. The
        first unchanged frame after changes counts the history as a single sample, so the frames
        that follow average it out instead of keeping a share of the image before the changes.
    */
    void trackChanges (const Scene& scene, const RenderSettings& settings, int width, int height, const GpuTexture* environmentCube, int pendingBakes)
    {
        const auto invalidating = getInvalidatingSignature (width, height, settings.taaSamples);
        const auto restarting = getRestartingSignature (scene, settings, environmentCube, pendingBakes);

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
    std::vector<uint64> shadowCasterSignatures; // One per opaque item of drawList, see getShadowSignature()
    LightClusters lightClusters;
    GBufferPass gbufferPass;
    std::unique_ptr<DepthNormalPrepass> depthNormalPrepass; // Created by the first raster frame with ambient occlusion, see renderDepthNormalPrepass()
    std::unique_ptr<AmbientOcclusionPass> ambientOcclusionPass; // Created by the first frame with ambient occlusion, see renderAmbientOcclusion()
    MaterialTable materialTable;
    LightingPass lightingPass;
    ForwardPass forwardPass;
    PostChain postChain;
    GpuTarget::Ptr colorTarget;
    GpuTexture::Ptr depthTexture;
    GpuTexture::Ptr lookupTables; // The lookup tables of this renderer and device, created once, black if that failed
    bool lookupTablesAttempted = false;
    ProbeRenderer probeRenderer { device };      // Builds the environment cube; its radiance format is the device's
    LightingInputs::EnvironmentCube environment; // The scene's environment cube on this device
    std::unique_ptr<ProbeBaker> probeBaker;      // Created by the first frame or bake with a reflection probe, see bakeReflectionProbes()
    GpuTexture::Ptr sceneColorOutput;
    GpuTexture::Ptr velocityOutput;
    GpuTexture::Ptr gbufferNormalOutput;
    GpuTexture::Ptr prepassNormalOutput;
    GpuTexture::Ptr ambientOcclusionOutput;
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
            shadowRenderer.invalidate();
            forgetProbeBakes();
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
    impl->shadowRenderer.invalidate();
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
        case PassOutput::prepassNormal: return impl->prepassNormalOutput;
        case PassOutput::ambientOcclusion: return impl->ambientOcclusionOutput;
    }

    return nullptr;
}

const ShadowRenderer* SceneRenderer::getShadowRenderer() const noexcept
{
    return impl != nullptr ? &impl->shadowRenderer : nullptr;
}

const ProbeBaker* SceneRenderer::getProbeBaker() const noexcept
{
    return impl != nullptr ? impl->probeBaker.get() : nullptr;
}

int SceneRenderer::bakeProbes (const GpuDevice::Ptr& device, Scene& scene)
{
    if (device == nullptr)
        return 0;

    // An earlier frame the GPU failed to execute makes its bakes bake again now
    if (impl != nullptr)
        if (const auto gpuResult = impl->takeFinishedFrames (false); gpuResult.failed())
            lastGpuFailure = gpuResult.getErrorMessage();

    if (impl == nullptr || device != impl->device)
        impl = std::make_unique<Impl> (device, getSupportedSampleCount (sampleCount, *device));

    SceneDrawList probeList;
    probeList.build (*scene.getRoot(), Matrix4());
    if (probeList.getReflectionProbes().empty() && impl->probeBaker == nullptr)
        return 0;

    auto deviceFacts = impl->deviceFacts;
    if (deviceFactsOverride)
        deviceFactsOverride (deviceFacts);

    // The lookup tables, and the environment cube, built in a frame of its own, never inside this one
    if (impl->prepareResources (false, failLookupTables).failed())
        return 0;

    impl->probeRenderer.setForceRgbmForTesting (forceRgbmRadiance);
    const auto environmentCube = LightingInputs::prepareEnvironment (impl->probeRenderer, scene, impl->environment);

    const auto* samplers = impl->frameResources.getSamplers();
    if (samplers == nullptr)
        return 0;

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return 0;

    impl->pendingFrames.push_back (frame.getStatus());

    // An explicit bake tries again even after the baker gave up
    if (impl->probeBaker != nullptr)
        impl->probeBaker->clearFailures();

    const auto baked = impl->bakeReflectionProbes (frame, scene, probeList, deviceFacts, environmentCube, *samplers, failProbeCaptures);
    frame.submit();
    return baked;
}

const AmbientOcclusionPass* SceneRenderer::getAmbientOcclusionPass() const noexcept
{
    return impl != nullptr ? impl->ambientOcclusionPass.get() : nullptr;
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
            impl->prepassNormalOutput = nullptr;
            impl->ambientOcclusionOutput = nullptr;
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

    const auto samples = getSupportedSampleCount (sampleCount, *device);

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
        impl->releasePassTargets();

    impl->frameResources.beginFrame (widthPx, heightPx);

    // Every exit from here on ends the frame of the resource pool
    const auto failFrame = [&] (const String& error)
    {
        // The history, the shadow maps and the probe bakes may not have been written: the next frame starts over
        impl->frameResources.invalidateHistory();
        impl->shadowRenderer.invalidate();
        impl->forgetProbeBakes();
        impl->releasePassTargets();
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
    if (failShadowPipelines)
        impl->shadowRenderer.failPipelinesForTesting();

    if (auto result = impl->prepareResources (full, failLookupTables); result.failed())
        return failFrame (result.getErrorMessage());

    impl->resolvePreviousTransforms();

    // Before TAA fetches its history, so a change invalidates it for this very frame
    impl->samplesToConverge = full ? (temporalAntiAliasing ? settings.taaSamples : 1) : 0;
    impl->hasSignature = impl->hasSignature && full;

    // Built in a frame of its own the first time, never inside this one
    impl->probeRenderer.setForceRgbmForTesting (forceRgbmRadiance);
    const auto environmentCube = LightingInputs::prepareEnvironment (impl->probeRenderer, scene, impl->environment);

    // The probes this frame bakes, counted before it records them so the frame baking them restarts the convergence
    const auto pendingBakes = impl->probeBaker != nullptr ? impl->probeBaker->getNumPending (drawList)
                                                          : static_cast<int> (drawList.getReflectionProbes().size());

    if (full)
        impl->trackChanges (scene, settings, widthPx, heightPx, environmentCube.get(), pendingBakes);

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return failFrame ("Unable to begin a GPU frame");

    impl->pendingFrames.push_back (frame.getStatus());

    if (failShadowCompareSampler)
        impl->frameResources.failShadowCompareSamplerForTesting();

    if (std::exchange (failShadowMaps, false))
        impl->shadowRenderer.failTexturesForTesting();

    const auto* samplers = impl->frameResources.getSamplers();
    if (samplers == nullptr)
        return failFrame ("Unable to create the shared samplers");

    // The pending reflection probes bake first, in this frame, so failFrame() and a GPU failure bake
    // them again. A bake that fails leaves the frame without them
    impl->bakeReflectionProbes (frame, scene, drawList, deviceFacts, environmentCube, *samplers, failProbeCaptures);

    // Without the comparison sampler nothing can read a shadow map: the frame renders unshadowed
    ShadowRenderer::Inputs shadowInputs { drawList, impl->viewData, *impl->capabilities, settings };
    shadowInputs.globalSignature = impl->getShadowSignature (settings);
    shadowInputs.casterSignatures = impl->shadowCasterSignatures;
    shadowInputs.enabled = samplers->shadowCompare != nullptr;

    if (auto result = impl->shadowRenderer.render (device, frame, shadowInputs); result.failed())
        return failFrame (result.getErrorMessage());

    auto lighting = LightingInputs::create (scene, environmentCube, drawList.getLights(), impl->shadowRenderer.getShadowRecordIndices());
    lighting.shadowCascades = impl->shadowRenderer.getCascadeTexture();
    lighting.shadowAtlas = impl->shadowRenderer.getAtlasTexture();
    lighting.shadows = impl->shadowRenderer.getShadowData();
    lighting.shadowFilters = impl->shadowRenderer.needsShadowFilters();
    lighting.lookupTables = impl->lookupTables;

    // One flag for every radiance texture of the device, whether or not the scene has an environment
    lighting.block.probeInfo[0] = impl->probeRenderer.getRadianceFormat() == GpuTextureFormat::rgba8unorm ? 1.0f : 0.0f;
    lighting.samplers = *samplers;

    if (! impl->prepareReflectionProbes (lighting, full))
        return failFrame ("Unable to upload the reflection probes");

    // The shading passes compile only the variants the frame shades with: the defaults here, so a
    // failure tells why, the shadow filter variants in the passes, which fall back to the defaults
    if (! lighting.shadowFilters)
    {
        if (auto result = impl->prepareDefaultShading (targets, forwardSamples, full, areaLights, ! full || ! drawList.getBlendItems().empty()); result.failed())
            return failFrame (result.getErrorMessage());
    }

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

        if (! impl->renderAmbientOcclusion (frame, settings, temporalAntiAliasing, gbuffer.depth, gbuffer.normalRoughness, lighting))
            return failFrame ("Unable to render the ambient occlusion");

        if (! impl->lightingPass.render (device, frame, impl->viewData, gbuffer, lighting, targets.color))
            return failFrame ("Unable to render the lighting pass");

        impl->lightingPass.trimUnusedTargets();
    }
    else
    {
        impl->gbufferPass.releaseTargets();
        impl->lightingPass.releaseTargets();
    }

    // Raster screen-space effects read the depth and normals of the prepass, full ones the G-buffer
    if (! impl->renderDepthNormalPrepass (frame, settings, widthPx, heightPx, failDepthNormalPrepass))
        return failFrame ("Unable to render the depth and normal prepass");

    if (! full)
    {
        const auto* prepass = impl->depthNormalPrepass.get();
        if (! impl->renderAmbientOcclusion (frame, settings, false, prepass != nullptr ? prepass->getDepth() : nullptr,
                                            prepass != nullptr ? prepass->getNormalRoughness() : nullptr, lighting))
            return failFrame ("Unable to render the ambient occlusion");
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
        const PostChain::Inputs inputs { sceneColor, full ? targets.depth : nullptr, targets.velocity, ! full, scene.getBackgroundColor(), impl->getTemporalSampleIndex(), impl->changing };
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
    impl->prepassNormalOutput = impl->depthNormalPrepass != nullptr ? impl->depthNormalPrepass->getNormalRoughness() : nullptr;
    impl->ambientOcclusionOutput = lighting.ambientOcclusion;

    lastError = {};
    return impl->colorTarget->asTexture();
}

} // namespace yup
