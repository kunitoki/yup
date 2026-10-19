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

// Shader bundles built with the module, see shaderBundles in yup_renderer.h
constexpr uint8_t debugViewShaderBundle[] = {
#include "yup_renderer_shaders/yup_DebugView.inc"
};

// std140 mirror of DebugViewBlock in yup_DebugView.frag
struct DebugViewBlock
{
    float options[4]; // x what to show: 0 black, else the RenderSettings::DebugView of the texture bound; y the scale of a circle of confusion
};

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
    signature.addFloat (material.clearcoatFactor);
    signature.addFloat (material.clearcoatRoughnessFactor);
    signature.addFloats (material.sheenColorFactor);
    signature.addFloat (material.sheenRoughnessFactor);

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
        YUP_PROFILE_NAMED_INTERNAL_TRACE (lightCulling);
        beginScope (frame, "lightCulling");

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

    /** Returns true on the raster tier when an effect reading the depth and normal prepass is
        enabled: ambient occlusion, the depth of field when @p depthOfField (PostChain::willBlur()),
        or the world fog when @p worldFog (willDrawWorldFog()). The full tier reads its G-buffer instead.
    */
    bool needsDepthNormalPrepass (const RenderSettings& settings, bool depthOfField, bool worldFog) const
    {
        return capabilities->getTier() == RenderCapabilities::Tier::raster
            && (capabilities->isEnabled (RenderCapabilities::Feature::ambientOcclusion, settings) || depthOfField || worldFog);
    }

    /** True when the frame draws the scene's @p fog: it is visible (FogApply::isVisible()), the tier
        can draw it (RenderCapabilities::Feature::worldFog) and it has not given up. The one predicate
        deciding whether the frame pays for it, with a raster prepass that compiles
        (prepareDepthNormalPrepass()): the raster tier routes the scene color through half float and
        draws the depth and normal prepass only then.
    */
    bool willDrawWorldFog (const WorldFog& fog) const
    {
        return FogApply::isVisible (fog) && capabilities->isAvailable (RenderCapabilities::Feature::worldFog)
            && (fogApply == nullptr || ! fogApply->hasGivenUp());
    }

    /** Applies the scene's @p fog to @p inputs.color while @p drawn (willDrawWorldFog()): on full in
        place, after the forward pass and before TAA; on raster into a new target, the first post
        pass. The pass is created by the first frame drawing the fog, then kept; frames without it
        release its targets only. A failure, or a missing input (no prepass depth), leaves the frame
        without the fog. Returns the fogged color, or nullptr when the frame has none.
    */
    GpuTexture::Ptr renderWorldFog (GpuFrame& frame, const WorldFog& fog, bool drawn, const LightingInputs& lighting, const FogApply::Inputs& inputs)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (fog);

        if (! drawn)
        {
            if (fogApply != nullptr)
                fogApply->releaseTargets();

            return nullptr;
        }

        if (fogApply == nullptr)
            fogApply = std::make_unique<FogApply> (device);

        if (worldFogFailures > 0)
            fogApply->failRendersForTesting (std::exchange (worldFogFailures, 0));

        // A frame missing an input records nothing, so it opens no scope
        if (inputs.color != nullptr && inputs.depth != nullptr)
            beginScope (frame, "fog");

        auto fogged = fogApply->render (frame, viewData, fog, lighting, drawList.getLights(), frameResources, inputs);
        if (fogged == nullptr)
        {
            fogApply->releaseTargets();
            return nullptr;
        }

        fogApply->trimUnusedTargets();
        return fogged;
    }

    /** Raster: true when the depth and normal prepass compiles. It is created by the first frame
        needing it, @p failForTesting making it fail to compile, and its compile result is kept: a
        prepass that cannot compile is logged once, and takes the depth of field and the world fog
        off the raster route, so their frames pay nothing for them.
    */
    bool prepareDepthNormalPrepass (bool failForTesting)
    {
        if (depthNormalPrepass == nullptr)
        {
            depthNormalPrepass = std::make_unique<DepthNormalPrepass> (device);
            if (failForTesting)
                depthNormalPrepass->failPipelinesForTesting();
        }

        return depthNormalPrepass->prepare().wasOk();
    }

    /** Raster: draws the depth and normal prepass while needsDepthNormalPrepass(). It is created by
        the first frame needing it, and only releases its targets on the frames without it, so
        turning its effects back on compiles nothing. A prepass that cannot compile leaves the frames
        without it (prepareDepthNormalPrepass()). Returns false only when the prepass could not be
        recorded. depthNormalPrepassTargetFailures is handed to the prepass once it exists.
    */
    bool renderDepthNormalPrepass (GpuFrame& frame, const RenderSettings& settings, bool depthOfField, bool worldFog, int width, int height, bool failForTesting)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (depthNormalPrepass);

        if (! needsDepthNormalPrepass (settings, depthOfField, worldFog))
        {
            if (depthNormalPrepass != nullptr)
                depthNormalPrepass->releaseTargets();

            return true;
        }

        if (! prepareDepthNormalPrepass (failForTesting))
            return true;

        if (depthNormalPrepassTargetFailures > 0)
            depthNormalPrepass->failTargetsForTesting (std::exchange (depthNormalPrepassTargetFailures, 0));

        // Targets that cannot be created record nothing, so they open no scope
        return depthNormalPrepass->render (frame, viewData, drawList, frameResources, width, height, [this, &frame]
        {
            beginScope (frame, "depthNormalPrepass");
        });
    }

    /** Traces the ambient occlusion of @p depth and @p normalRoughness into lighting.ambientOcclusion
        while it is enabled and both exist: the G-buffer's on full, the prepass's on raster. The pass
        is created by the first frame needing it, and only releases its targets on the frames without
        it, so turning ambient occlusion back on compiles nothing. A pass that cannot compile, or
        targets that cannot be created, are logged once and leave the frames without it, the shading
        passes binding white. Block::probeInfo.w is the frame's ambient occlusion flag, which LightingPass reads. Returns false
        only when the pass could not be recorded. On full, @p radiance (getIndirectDiffuseRadiance()) makes the pass gather
        the screen-space diffuse GI too, which addIndirectDiffuse() adds after LightingPass; @p failIndirectDiffuseForTesting
        makes every GI pass fail from then on.
    */
    bool renderAmbientOcclusion (GpuFrame& frame, const RenderSettings& settings, bool temporal,
                                 const GpuTexture::Ptr& depth, const GpuTexture::Ptr& normalRoughness, LightingInputs& lighting,
                                 const AmbientOcclusionPass::Radiance& radiance = {}, bool failIndirectDiffuseForTesting = false)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (ambientOcclusion);

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

        if (failIndirectDiffuseForTesting)
            ambientOcclusionPass->failIndirectDiffuseForTesting();

        if (ambientOcclusionPass->prepare().failed())
            return true;

        beginScope (frame, "ambientOcclusion");
        if (! ambientOcclusionPass->render (frame, viewData, capabilities->getTier(), temporal, depth, normalRoughness, settings.aoDistance, frameResources, radiance))
            return false;

        ambientOcclusionPass->trimUnusedTargets();
        lighting.ambientOcclusion = ambientOcclusionPass->getOutput();
        lighting.block.probeInfo[3] = lighting.ambientOcclusion != nullptr ? 1.0f : 0.0f;
        return true;
    }

    /** Full tier: the radiance the screen-space diffuse GI gathers, the history of temporal
        anti-aliasing and the exposure it holds, while the GI is enabled and has not given up.
        Empty otherwise, and on the frames without a valid history to read (the first one, after a
        resize, restartAccumulation(), a taaSamples change, a failed frame or once released), which
        then trace the ambient occlusion alone, exactly as without GI.
    */
    AmbientOcclusionPass::Radiance getIndirectDiffuseRadiance (const RenderSettings& settings, const GpuTexture::Ptr& velocity) const
    {
        // An exposure of 0 left nothing to divide the history by
        if (! isGatheringIndirectDiffuse (settings) || ! (previousExposure > 0.0f))
            return {};

        const auto history = frameResources.findHistory (TemporalAA::historyName);
        if (! history.valid || history.read == nullptr)
            return {};

        return { history.read, velocity, previousExposure };
    }

    /** True while the frames gather screen-space diffuse GI: enabled, and not given up after failures. */
    bool isGatheringIndirectDiffuse (const RenderSettings& settings) const
    {
        return capabilities->isEnabled (RenderCapabilities::Feature::screenSpaceGI, settings)
            && (ambientOcclusionPass == nullptr || ! ambientOcclusionPass->hasGivenUpIndirectDiffuse());
    }

    /** Full tier: traces the screen-space reflections of @p gbuffer into lighting.reflections while
        they are enabled and the previous frame left a history of temporal anti-aliasing to read
        their radiance from; otherwise the lighting pass binds its stand-in, a confidence of 0, and
        the frame shades exactly as without them. The Hi-Z pyramid and the pass are created by the
        first frame tracing them, then kept for the renderer's life (frames do not retain
        pipelines); frames without them release their targets only. A failure, the Hi-Z pyramid
        included, never fails the frame: it shades without the reflections, and the second failure
        in a row gives up (ReflectionPass::hasGivenUp()). @p failForTesting makes the pass fail.
    */
    void renderScreenSpaceReflections (GpuFrame& frame, const RenderSettings& settings, const GBufferPass::Targets& gbuffer,
                                       LightingInputs& lighting, bool failForTesting)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (screenSpaceReflections);

        lighting.reflections = nullptr;

        // An exposure of 0 left nothing to divide the history by
        if (! isTracingReflections (settings) || ! (previousExposure > 0.0f))
        {
            releaseReflectionTargets();
            return;
        }

        // The radiance comes from the history of the previous frame, which TemporalAA alone creates
        // and sizes: none on the first frame, after a resize or once released, which leave the frame
        // without the reflections
        const auto history = frameResources.findHistory (TemporalAA::historyName);
        if (! history.valid || history.read == nullptr)
        {
            releaseReflectionTargets();
            return;
        }

        if (reflectionPass == nullptr)
        {
            hiZPyramid = std::make_unique<HiZPyramid> (device);
            reflectionPass = std::make_unique<ReflectionPass> (device);
        }

        if (failForTesting)
            reflectionPass->failRendersForTesting();

        // A pyramid that cannot be built counts as a failure of the pass
        GpuTexture::Ptr pyramid;
        if (reflectionPass->prepare().wasOk())
        {
            beginScope (frame, "hiZ");
            pyramid = hiZPyramid->build (frame, gbuffer.depth, frameResources);
        }

        beginScope (frame, "screenSpaceReflections");
        lighting.reflections = reflectionPass->render (frame, viewData, pyramid, gbuffer.normalRoughness, gbuffer.velocity,
                                                       history.read, previousExposure, frameResources);

        if (lighting.reflections == nullptr)
        {
            releaseReflectionTargets();
            return;
        }

        hiZPyramid->trimUnusedTargets();
        reflectionPass->trimUnusedTargets();
    }

    /** True while the frames trace screen-space reflections: enabled, and not given up after failures. */
    bool isTracingReflections (const RenderSettings& settings) const
    {
        return capabilities->isEnabled (RenderCapabilities::Feature::screenSpaceReflections, settings)
            && (reflectionPass == nullptr || ! reflectionPass->hasGivenUp());
    }

    /** Releases the targets of the screen-space reflections, keeping their pipelines. */
    void releaseReflectionTargets() noexcept
    {
        if (reflectionPass != nullptr)
        {
            hiZPyramid->releaseTargets();
            reflectionPass->releaseTargets();
        }
    }

    /** Bakes the pending reflection probes and irradiance volumes of @p probeList into @p frame,
        through the forward pass (see ProbeBaker), and returns how many were baked. The baker is
        created by the first frame or bake with a probe or a volume, then kept, so a scene without
        either allocates nothing. A bake that fails never fails the frame: the probes and volumes
        shade with weight 0 until they bake again, each feature failing on its own.
        @p failCapturesForTesting makes every probe capture fail, @p failVolumeCapturesForTesting every volume one.
    */
    int bakeReflectionProbes (GpuFrame& frame, const Scene& scene, const SceneSettings& sceneSettings, const RenderSettings& settings, const SceneDrawList& probeList, const RenderCapabilities::DeviceFacts& facts,
                              const GpuTexture::Ptr& environmentCube, const FrameResources::Samplers& samplers, bool failCapturesForTesting,
                              bool failVolumeCapturesForTesting)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (probeBake);

        if (probeBaker == nullptr)
        {
            if (probeList.getReflectionProbes().empty() && probeList.getIrradianceVolumes().empty())
                return 0;

            probeBaker = std::make_unique<ProbeBaker> (probeRenderer);
        }

        probeBaker->failCapturesForTesting (failCapturesForTesting);
        probeBaker->failVolumeCapturesForTesting (failVolumeCapturesForTesting);

        const ProbeBaker::Inputs inputs { scene, sceneSettings, settings, probeList, facts, environmentCube, lookupTables, samplers };
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
        const auto hasProbes = probeBaker != nullptr && ! probeBaker->hasGivenUpProbes() && ! drawList.getReflectionProbes().empty();
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

    /** The irradiance volumes of the frame for the shading passes, Block::probeInfo z counting them:
        on raster every volume of the draw list, which each draw picks from; on full the baked ones by
        priority in a storage buffer, uploaded every full frame (sized for one volume without any).
        Returns false only when that buffer could not be uploaded.
    */
    bool prepareIrradianceVolumes (LightingInputs& lighting, bool full)
    {
        // A baker that gave up shades no volume
        const auto hasVolumes = probeBaker != nullptr && ! probeBaker->hasGivenUpVolumes() && ! drawList.getIrradianceVolumes().empty();
        lighting.irradianceAtlas = hasVolumes ? probeBaker->getIrradianceAtlas() : nullptr;
        lighting.visibilityAtlas = hasVolumes ? probeBaker->getVisibilityAtlas() : nullptr;

        if (full)
        {
            const auto volumes = hasVolumes ? probeBaker->packIrradianceVolumesByPriority (drawList) : std::vector<GpuIrradianceVolume>();
            lighting.block.probeInfo[2] = static_cast<float> (volumes.size());
            lighting.irradianceVolumeBuffer = uploadStorageBuffer (FrameResources::StorageSlot::irradianceVolumes, Span<const GpuIrradianceVolume> (volumes));
            return lighting.irradianceVolumeBuffer != nullptr;
        }

        if (hasVolumes)
            lighting.irradianceVolumes = probeBaker->packIrradianceVolumes (drawList);

        lighting.block.probeInfo[2] = static_cast<float> (lighting.irradianceVolumes.size());
        return true;
    }

    /** Makes the reflection probes and irradiance volumes bake again: for frames the GPU did not execute. */
    void forgetProbeBakes() noexcept
    {
        if (probeBaker != nullptr)
            probeBaker->forgetBakes();
    }

    /** Full tier: draws RenderSettings::debugView into @p output in place of the final image, from
        this frame's @p lighting, @p indirectDiffuse and depth of field, or black when the frame has none of them,
        binding @p standIn instead. The pass is created by the first frame with a debug view, then
        kept; the frames without one release its targets. A pass that cannot compile or record is
        logged once and leaves the final image.
    */
    void renderDebugView (GpuFrame& frame, const RenderSettings& settings, bool full, const LightingInputs& lighting,
                          const GpuTexture::Ptr& indirectDiffuse, const GpuTexture::Ptr& standIn, const GpuTexture::Ptr& output)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (debugView);
        using DebugView = RenderSettings::DebugView;

        if (! full || settings.debugView == DebugView::none)
        {
            if (debugViewPass != nullptr)
                debugViewPass->releaseTargets();

            return;
        }

        if (debugViewPass == nullptr)
        {
            debugViewPass = std::make_unique<FullscreenPass>();
            if (debugViewPass->prepare (device, debugViewShaderBundle, sizeof (debugViewShaderBundle), output->getFormat()).failed())
                YUP_DBG ("SceneRenderer: unable to compile the debug view, the frames show the final image");
        }

        if (! debugViewPass->isPrepared())
            return;

        const auto* depthOfField = postChain.getDepthOfField();
        const auto source = settings.debugView == DebugView::ambientOcclusion  ? lighting.ambientOcclusion
                          : settings.debugView == DebugView::reflections       ? lighting.reflections
                          : settings.debugView == DebugView::circleOfConfusion ? (depthOfField != nullptr ? depthOfField->getCircleOfConfusionTexture() : nullptr)
                                                                               : indirectDiffuse;

        DebugViewBlock block {};
        block.options[0] = source != nullptr ? static_cast<float> (settings.debugView) : 0.0f;
        block.options[1] = depthOfField != nullptr && depthOfField->getMaxCircleOfConfusion() > 0.0f ? 1.0f / depthOfField->getMaxCircleOfConfusion() : 0.0f;

        // The shader scales the output pixels to the source through the viewport: the frame's size, also when upscaling
        auto outputView = viewData.getBlock();
        outputView.viewport[0] = static_cast<float> (output->getWidth());
        outputView.viewport[1] = static_cast<float> (output->getHeight());
        outputView.viewport[2] = 1.0f / outputView.viewport[0];
        outputView.viewport[3] = 1.0f / outputView.viewport[1];

        beginScope (frame, "debugView");
        const auto drawn = debugViewPass->draw (frame, output, {}, [&] (GpuRenderPass& renderPass)
        {
            renderPass.setUniformBuffer (0, 0, &outputView, sizeof (ViewData::Block));
            renderPass.setSampler (0, 3, lighting.samplers.pointClamp);
            renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
            renderPass.setTexture (1, 1, source != nullptr ? source : standIn);
        });

        if (! drawn && ! std::exchange (debugViewFailed, true))
            YUP_DBG ("SceneRenderer: unable to record the debug view, the frames show the final image");

        debugViewPass->trimUnusedTargets();
    }

    /** Releases the render targets the passes keep for the next frames. */
    void releasePassTargets() noexcept
    {
        postChain.releaseTargets();
        gbufferPass.releaseTargets();
        lightingPass.releaseTargets();

        if (debugViewPass != nullptr)
            debugViewPass->releaseTargets();

        if (depthNormalPrepass != nullptr)
            depthNormalPrepass->releaseTargets();

        if (ambientOcclusionPass != nullptr)
            ambientOcclusionPass->releaseTargets();

        if (fogApply != nullptr)
            fogApply->releaseTargets();

        releaseReflectionTargets();
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

    /** True when the unjittered camera or the world matrix of a drawn item changed since the
        previous frame, after resolvePreviousTransforms(): the first frame, and a node that was not
        drawn in the previous one, do not move. Floats compare as the restarting signature hashes
        them (FrameSignature::getCanonicalBits()), so a non-finite matrix that stays the same, or a
        0 turning into -0, is no motion either.
    */
    bool hasMotion() const
    {
        const auto same = [] (const float* a, const float* b)
        {
            return std::equal (a, a + 16, b, [] (float x, float y)
            {
                return FrameSignature::getCanonicalBits (x) == FrameSignature::getCanonicalBits (y);
            });
        };

        const auto& block = viewData.getBlock();
        if (! same (block.unjitteredViewProjection, block.previousViewProjection))
            return true;

        for (const auto items : { drawList.getOpaqueItems(), drawList.getBlendItems() })
        {
            for (const auto& item : items)
            {
                if (item.node != nullptr && ! same (item.node->previousWorld.getData(), item.world.getData()))
                    return true;
            }
        }

        return false;
    }

    /** True when the history of the previous frame carries on into a frame of @p width x @p height,
        shaded at @p internalWidth x @p internalHeight, with @p settings: there was a previous frame
        that did not fail, no restartAccumulation() since, and the sizes, the tier and the TAA
        samples are unchanged. Call it before
        trackChanges(). The motion of a frame is measured against that previous one, so without it
        the frame does not move (preflight R23).
    */
    bool historyContinues (const RenderSettings& settings, int width, int height, int internalWidth, int internalHeight) const
    {
        return hasSignature && getInvalidatingSignature (width, height, internalWidth, internalHeight, settings.taaSamples) == invalidatingSignature;
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

    /** The changes that discard the history, which no longer fits the frame: the upscaling changes
        the size the scene is shaded at, and the frames to converge with it. */
    uint64 getInvalidatingSignature (int width, int height, int internalWidth, int internalHeight, int taaSamples) const
    {
        FrameSignature signature;
        signature.addBits (static_cast<uint64> (width));
        signature.addBits (static_cast<uint64> (height));
        signature.addBits (static_cast<uint64> (internalWidth));
        signature.addBits (static_cast<uint64> (internalHeight));
        signature.addBits (static_cast<uint64> (capabilities->getTier()));
        signature.addBits (static_cast<uint64> (taaSamples));
        return signature.get();
    }

    /** Everything else shaping the scene color before TAA. A change restarts the convergence and
        keeps the history, which TAA reprojects and clips to the new frame. Tone mapping and bloom
        run after TAA, so their settings are left out.
    */
    uint64 getRestartingSignature (const SceneSettings& sceneSettings, const RenderSettings& settings, const GpuTexture* environmentCube, int pendingBakes) const
    {
        FrameSignature signature;
        signature.addMatrix (viewData.getUnjitteredViewProjection());
        signature.addFloat (sceneSettings.exposure);
        signature.addBits (sceneSettings.ambientColor.getARGB());
        signature.addPointer (sceneSettings.environment.get());
        signature.addPointer (environmentCube); // Rebuilt after the GPU failed to build it
        signature.addFloat (sceneSettings.environmentIntensity);
        signature.addFloat (sceneSettings.environmentBlur);
        signature.addBits (sceneSettings.environmentVisible ? 1u : 0u);

        // The RenderSettings shaping the scene color before TAA, as the tier resolves them; bloom runs after it
        signature.addBits (static_cast<uint64> (capabilities->getCascadeCount (settings)));
        signature.addBits (static_cast<uint64> (RenderCapabilities::getShadowResolution (settings)));
        signature.addBits (capabilities->isEnabled (RenderCapabilities::Feature::softShadows, settings) ? 1u : 0u);
        signature.addBits (capabilities->isEnabled (RenderCapabilities::Feature::contactShadows, settings) ? 1u : 0u);

        const auto ambientOcclusion = capabilities->isEnabled (RenderCapabilities::Feature::ambientOcclusion, settings);
        signature.addBits (ambientOcclusion ? 1u : 0u);
        if (ambientOcclusion)
            signature.addFloat (settings.aoDistance);

        // Giving up on them after failures changes the image as much as turning them off
        signature.addBits (isTracingReflections (settings) ? 1u : 0u);
        signature.addBits (isGatheringIndirectDiffuse (settings) ? 1u : 0u);

        // The world fog is applied before TAA: every field WorldFog::operator== compares, while it is drawn
        const auto& fog = sceneSettings.fog;
        const auto fogged = willDrawWorldFog (fog);
        signature.addBits (fogged ? 1u : 0u);

        if (fogged)
        {
            signature.addFloats (fog.color);
            signature.addFloat (fog.density);
            signature.addFloat (fog.heightFalloff);
            signature.addFloat (fog.baseHeight);
            signature.addFloat (fog.anisotropy);
        }

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

        // The reflection probes: their fields and placement, and the bakes of the probes and volumes, the ones this frame records included
        const auto bakes = probeBaker != nullptr ? probeBaker->getGeneration() : 0;
        signature.addBits (bakes + (pendingBakes > 0 ? 1u : 0u));
        signature.addBits (drawList.getReflectionProbes().size());

        for (const auto& probe : drawList.getReflectionProbes())
            signature.addBits (ProbeBaker::getSignature (probe));

        // The irradiance volumes: their fields and placement
        signature.addBits (drawList.getIrradianceVolumes().size());

        for (const auto& volume : drawList.getIrradianceVolumes())
            signature.addBits (ProbeBaker::getSignature (volume));

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

    /** Full tier: compares this frame with the previous one. A new size, internal size, tier or TAA
        sample count discards the history and starts over; any other change only restarts the convergence. The
        first unchanged frame after changes counts the history as a single sample, so the frames
        that follow average it out instead of keeping a share of the image before the changes.
    */
    void trackChanges (const SceneSettings& sceneSettings, const RenderSettings& settings, int width, int height, int internalWidth, int internalHeight,
                       const GpuTexture* environmentCube, int pendingBakes)
    {
        const auto invalidating = getInvalidatingSignature (width, height, internalWidth, internalHeight, settings.taaSamples);
        const auto restarting = getRestartingSignature (sceneSettings, settings, environmentCube, pendingBakes);

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
    std::unique_ptr<DepthNormalPrepass> depthNormalPrepass; // Created by the first raster frame with ambient occlusion, depth of field or world fog, see renderDepthNormalPrepass()
    std::unique_ptr<AmbientOcclusionPass> ambientOcclusionPass; // Created by the first frame with ambient occlusion, see renderAmbientOcclusion()
    std::unique_ptr<HiZPyramid> hiZPyramid;         // Created by the first frame tracing screen-space reflections, see renderScreenSpaceReflections()
    std::unique_ptr<ReflectionPass> reflectionPass; // Created with hiZPyramid
    MaterialTable materialTable;
    LightingPass lightingPass;
    ForwardPass forwardPass;
    PostChain postChain;
    std::unique_ptr<FogApply> fogApply;            // Created by the first frame drawing the scene's fog, see renderWorldFog()
    std::unique_ptr<FullscreenPass> debugViewPass; // Created by the first full frame with a debug view, see renderDebugView(), for the format of colorTarget, which never changes
    bool debugViewFailed = false;                  // A debug view draw failed, logged once
    bool depthOfFieldUnavailableLogged = false;    // Depth of field was asked for where the tier cannot run it, logged once
    bool worldFogUnavailableLogged = false;        // A scene's world fog was asked for where the tier cannot draw it, logged once
    int worldFogFailures = 0;                      // Handed to the fog by the next frame drawing it, see SceneRenderer::failWorldFogForTesting()
    int depthNormalPrepassTargetFailures = 0;      // Handed to the prepass by the next frame drawing it, see SceneRenderer::failDepthNormalPrepassTargetsForTesting()
    GpuTarget::Ptr colorTarget;
    GpuTexture::Ptr depthTexture;
    GpuTexture::Ptr lookupTables; // The lookup tables of this renderer and device, created once, black if that failed
    bool lookupTablesAttempted = false;
    ProbeRenderer probeRenderer { device };      // Builds the environment cube; its radiance format is the device's
    LightingInputs::EnvironmentCube environment; // The scene's environment cube on this device
    std::unique_ptr<ProbeBaker> probeBaker;      // Created by the first frame or bake with a reflection probe or an irradiance volume, see bakeReflectionProbes()
    GpuTexture::Ptr sceneColorOutput;
    GpuTexture::Ptr velocityOutput;
    GpuTexture::Ptr gbufferNormalOutput;
    GpuTexture::Ptr prepassNormalOutput;
    GpuTexture::Ptr ambientOcclusionOutput;
    GpuTexture::Ptr reflectionsOutput;
    GpuTexture::Ptr indirectDiffuseOutput;
    GpuTexture::Ptr circleOfConfusionOutput;
    GpuTexture::Ptr depthOfFieldOutput;
    GpuTexture::Ptr motionBlurOutput;
    float previousExposure = 1.0f; // The exposure of the last frame, which its history holds, see renderScreenSpaceReflections() and getIndirectDiffuseRadiance()
    uint64 frameIndex = 0;
    uint64 lastStamp = 0;
    uint64 invalidatingSignature = 0;
    uint64 restartingSignature = 0;
    bool hasSignature = false;
    bool changing = false;      // the previous frames restarted the convergence, see trackChanges()
    int samplesToConverge = 0;  // full: the TAA samples (1 without TAA), raster: 0
    int accumulatedSamples = 0; // frames in the history since its last reset
    int stillSamples = 0;       // frames since anything last changed
    bool measuring = false;     // the frame being rendered collects its stats, see beginScope()
    int numTimingCalls = 0;     // see SceneRenderer::getNumTimingCallsForTesting()

    /** Opens the GPU scope @p name in @p frame, right before the first pass the frame records for
        it, while the frame collects its stats (see SceneRenderer::setFrameStatsEnabled()). The only
        caller of GpuFrame::beginScope() in yup_3d, so getNumTimingCallsForTesting() sees them all.
    */
    void beginScope (GpuFrame& frame, const char* name)
    {
        if (! measuring)
            return;

        frame.beginScope (name);
        ++numTimingCalls;
    }

    /** A submitted frame of this renderer, with its stats when it collected them, all but the GPU times. */
    struct PendingFrame
    {
        GpuFrameStatus status;
        std::optional<FrameStats> stats;
    };

    /** The GPU outcomes of this renderer's submitted frames not yet known to have succeeded. */
    std::vector<PendingFrame> pendingFrames;

    /** Forgets the frames the GPU finished, waiting for all when @p wait is set, and moves the stats
        of the latest one that succeeded into @p latest when they are newer. On a failure the targets
        and history were left unwritten, so the history is discarded. */
    Result takeFinishedFrames (bool wait, FrameStats& latest)
    {
        auto result = Result::ok();
        std::optional<PendingFrame> newest; // the newest finished frame with stats, its timings copied once

        for (auto it = pendingFrames.begin(); it != pendingFrames.end();)
        {
            const auto outcome = wait ? std::optional<Result> (it->status.wait()) : it->status.poll();
            if (! outcome.has_value())
            {
                ++it;
                continue;
            }

            if (outcome->failed())
            {
                if (result.wasOk())
                    result = *outcome;
            }
            else if (it->stats.has_value() && it->stats->frameIndex > (newest.has_value() ? newest->stats->frameIndex : latest.frameIndex))
            {
                newest = std::move (*it);
            }

            it = pendingFrames.erase (it);
        }

        if (newest.has_value())
        {
            latest = std::move (*newest->stats);
            latest.gpu = newest->status.getTimings();
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
    // A smeared frame is never final: the still frame after it shows the scene sharp (preflight R23)
    return impl == nullptr || lastError.isNotEmpty()
        || (impl->stillSamples >= impl->samplesToConverge && impl->motionBlurOutput == nullptr);
}

Result SceneRenderer::waitForGPU()
{
    if (impl == nullptr)
        return Result::ok();

    return impl->takeFinishedFrames (true, lastFrameStats);
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
        case PassOutput::reflections: return impl->reflectionsOutput;
        case PassOutput::indirectDiffuse: return impl->indirectDiffuseOutput;
        case PassOutput::circleOfConfusion: return impl->circleOfConfusionOutput;
        case PassOutput::depthOfField: return impl->depthOfFieldOutput;
        case PassOutput::motionBlur: return impl->motionBlurOutput;
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
        if (const auto gpuResult = impl->takeFinishedFrames (false, lastFrameStats); gpuResult.failed())
            lastGpuFailure = gpuResult.getErrorMessage();

    if (impl == nullptr || device != impl->device)
        impl = std::make_unique<Impl> (device, getSupportedSampleCount (sampleCount, *device));

    SceneDrawList probeList;
    probeList.build (scene.getRoot(), Matrix4());
    if (probeList.getReflectionProbes().empty() && probeList.getIrradianceVolumes().empty() && impl->probeBaker == nullptr)
        return 0;

    const auto sceneSettings = SceneSettings::resolve (scene);

    auto deviceFacts = impl->deviceFacts;
    if (deviceFactsOverride)
        deviceFactsOverride (deviceFacts);

    // The lookup tables, and the environment cube, built in a frame of its own, never inside this one
    if (impl->prepareResources (false, failLookupTables).failed())
        return 0;

    impl->probeRenderer.setForceRgbmForTesting (forceRgbmRadiance);
    const auto environmentCube = LightingInputs::prepareEnvironment (impl->probeRenderer, sceneSettings, impl->environment);

    const auto* samplers = impl->frameResources.getSamplers();
    if (samplers == nullptr)
        return 0;

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return 0;

    impl->pendingFrames.push_back ({ frame.getStatus(), {} });

    // An explicit bake tries again even after the baker gave up
    if (impl->probeBaker != nullptr)
        impl->probeBaker->clearFailures();

    const auto baked = impl->bakeReflectionProbes (frame, scene, sceneSettings, renderSettings, probeList, deviceFacts, environmentCube, *samplers, failProbeCaptures, failVolumeCaptures);
    frame.submit();
    return baked;
}

const AmbientOcclusionPass* SceneRenderer::getAmbientOcclusionPass() const noexcept
{
    return impl != nullptr ? impl->ambientOcclusionPass.get() : nullptr;
}

const ReflectionPass* SceneRenderer::getReflectionPass() const noexcept
{
    return impl != nullptr ? impl->reflectionPass.get() : nullptr;
}

const HiZPyramid* SceneRenderer::getHiZPyramid() const noexcept
{
    return impl != nullptr ? impl->hiZPyramid.get() : nullptr;
}

const DepthOfField* SceneRenderer::getDepthOfField() const noexcept
{
    return impl != nullptr ? impl->postChain.getDepthOfField() : nullptr;
}

const MotionBlur* SceneRenderer::getMotionBlur() const noexcept
{
    return impl != nullptr ? impl->postChain.getMotionBlur() : nullptr;
}

const TemporalAA* SceneRenderer::getTemporalAA() const noexcept
{
    return impl != nullptr ? &impl->postChain.getTemporalAA() : nullptr;
}

const FogApply* SceneRenderer::getFogApply() const noexcept
{
    return impl != nullptr ? impl->fogApply.get() : nullptr;
}

const FullscreenPass* SceneRenderer::getDebugViewPass() const noexcept
{
    return impl != nullptr ? impl->debugViewPass.get() : nullptr;
}

const LightingPass* SceneRenderer::getLightingPass() const noexcept
{
    return impl != nullptr ? &impl->lightingPass : nullptr;
}

const ForwardPass* SceneRenderer::getForwardPass() const noexcept
{
    return impl != nullptr ? &impl->forwardPass : nullptr;
}

int SceneRenderer::getNumTimingCallsForTesting() const noexcept
{
    return impl != nullptr ? impl->numTimingCalls : 0;
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
            impl->reflectionsOutput = nullptr;
            impl->indirectDiffuseOutput = nullptr;
            impl->circleOfConfusionOutput = nullptr;
            impl->depthOfFieldOutput = nullptr;
            impl->motionBlurOutput = nullptr;
        }

        lastError = error;
        return nullptr;
    };

    const auto frameIndex = ++numFrames;

    // A frame of this renderer the GPU failed to execute left its history unwritten: it was
    // discarded, and this frame renders anew rather than leaving the caller without an image
    lastGpuFailure = {};
    if (impl != nullptr)
        if (const auto gpuResult = impl->takeFinishedFrames (false, lastFrameStats); gpuResult.failed())
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

    const auto& settings = renderSettings;
    impl->capabilities.emplace (deviceFacts, settings, samples);

    // The scene features of this frame, resolved once from the feature nodes of the tree
    const auto sceneSettings = SceneSettings::resolve (scene);

    // The full tier shades one sample per pixel into HDR targets, then runs the post chain. The
    // raster tier does the same with its samples only for effects needing HDR: bloom, the depth of
    // field on the frames it blurs, and the scene's world fog. Raster depth of field and fog read
    // the depth of the prepass, so they take the route only while it compiles
    const auto full = impl->capabilities->getTier() == RenderCapabilities::Tier::full;
    const auto prepassCompiles = [&] { return full || impl->prepareDepthNormalPrepass (failDepthNormalPrepass); };
    const auto blurs = impl->postChain.willBlur (*impl->capabilities, settings, scene.getActiveCamera()) && prepassCompiles();
    const auto& worldFog = sceneSettings.fog;
    const auto fogs = impl->willDrawWorldFog (worldFog) && prepassCompiles();
    const auto hdr = full || impl->capabilities->isEnabled (RenderCapabilities::Feature::bloom, settings) || blurs || fogs;

    if (settings.depthOfField && ! impl->capabilities->isAvailable (RenderCapabilities::Feature::depthOfField)
        && ! std::exchange (impl->depthOfFieldUnavailableLogged, true))
        YUP_DBG ("SceneRenderer: raster depth of field needs multisampled half-float targets and the depth and normal prepass, the frames render without it");

    if (FogApply::isVisible (worldFog) && ! impl->capabilities->isAvailable (RenderCapabilities::Feature::worldFog)
        && ! std::exchange (impl->worldFogUnavailableLogged, true))
        YUP_DBG ("SceneRenderer: raster world fog needs multisampled half-float targets and the depth and normal prepass, the frames render without it");

    const auto forwardSamples = full ? 1u : samples;
    const auto temporalAntiAliasing = impl->capabilities->isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, settings);

    // Upscaling shades the scene at a share of the frame's size, which TAA reconstructs over more
    // frames, as many more as the share of the pixels it skips. A variant that cannot compile shades at full size
    auto renderScale = impl->capabilities->getRenderScale (settings);
    if (renderScale < 1.0f && ! impl->postChain.prepareUpscaling (device))
        renderScale = 1.0f;

    const auto internalWidth = jmax (1, roundToInt (static_cast<float> (widthPx) * renderScale));
    const auto internalHeight = jmax (1, roundToInt (static_cast<float> (heightPx) * renderScale));
    const auto shadedShare = (static_cast<float> (internalWidth) * static_cast<float> (internalHeight)) / (static_cast<float> (widthPx) * static_cast<float> (heightPx));
    const auto temporalSamples = internalWidth == widthPx && internalHeight == heightPx ? settings.taaSamples
                                                                                        : static_cast<int> (std::ceil (static_cast<float> (settings.taaSamples) / shadedShare));

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

    const auto targets = full ? impl->acquireFullTargets (internalWidth, internalHeight)
                              : (hdr ? impl->acquireRasterHdrTargets (widthPx, heightPx) : impl->getForwardTargets());
    if (targets.color == nullptr || targets.depth == nullptr || (full && targets.velocity == nullptr))
        return failFrame ("Unable to create the scene targets");

    // Full: the G-buffer shares the depth and velocity the blended items are drawn over
    const auto gbuffer = full ? GBufferPass::acquireTargets (impl->frameResources, targets.velocity, targets.depth) : GBufferPass::Targets();
    if (full && (gbuffer.albedoOcclusion == nullptr || gbuffer.normalRoughness == nullptr || gbuffer.emissionMaterial == nullptr))
        return failFrame ("Unable to create the G-buffer targets");

    const auto stamp = ++Impl::nextStamp;

    // The jitter cycles with the frame index: any temporalSamples consecutive frames cover every point
    impl->viewData.update (scene, sceneSettings, internalWidth, internalHeight, temporalAntiAliasing, temporalSamples, impl->frameIndex++);

    auto& drawList = impl->drawList;
    // The raster tier lights each draw with its own selection, the full tier through the clusters
    drawList.build (scene.getRoot(), impl->viewData.getView(), { sceneSettings.useDefaultLight, ! full });

    // A light with a radius selects the area light variants of the shading passes. Only a change
    // of the lights switches variants, and the restarting signature hashes every light radius
    const auto areaLights = LightingInputs::hasAreaLights (drawList.getLights());
    if (failShadowPipelines)
        impl->shadowRenderer.failPipelinesForTesting();

    if (auto result = impl->prepareResources (full, failLookupTables); result.failed())
        return failFrame (result.getErrorMessage());

    impl->resolvePreviousTransforms();

    // Before TAA fetches its history, so a change invalidates it for this very frame
    impl->samplesToConverge = full ? (temporalAntiAliasing ? temporalSamples : 1) : 0;
    impl->hasSignature = impl->hasSignature && full;

    // Only a frame where something moved since a previous frame whose history carries on gives the
    // motion blur anything to smear (preflight R4, R23)
    const auto moving = impl->postChain.willMotionBlur (*impl->capabilities, settings)
                     && impl->historyContinues (settings, widthPx, heightPx, internalWidth, internalHeight)
                     && impl->hasMotion();

    // Built in a frame of its own the first time, never inside this one
    impl->probeRenderer.setForceRgbmForTesting (forceRgbmRadiance);
    const auto environmentCube = LightingInputs::prepareEnvironment (impl->probeRenderer, sceneSettings, impl->environment);

    // The probes and volumes this frame bakes, counted before it records them so the frame baking them restarts the convergence
    const auto pendingBakes = impl->probeBaker != nullptr ? impl->probeBaker->getNumPending (drawList)
                                                          : static_cast<int> (drawList.getReflectionProbes().size() + drawList.getIrradianceVolumes().size());

    if (full)
        impl->trackChanges (sceneSettings, settings, widthPx, heightPx, internalWidth, internalHeight, environmentCube.get(), pendingBakes);

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return failFrame ("Unable to begin a GPU frame");

    impl->pendingFrames.push_back ({ frame.getStatus(), {} });

    // Before the first pass, so every scope is measured
    impl->measuring = frameStatsEnabled;
    if (impl->measuring)
    {
        frame.setTimingEnabled (true);
        ++impl->numTimingCalls;
    }

    if (failShadowCompareSampler)
        impl->frameResources.failShadowCompareSamplerForTesting();

    if (std::exchange (failShadowMaps, false))
        impl->shadowRenderer.failTexturesForTesting();

    const auto* samplers = impl->frameResources.getSamplers();
    if (samplers == nullptr)
        return failFrame ("Unable to create the shared samplers");

    // The pending reflection probes and irradiance volumes bake first, in this frame, so failFrame()
    // and a GPU failure bake them again. A bake that fails leaves the frame without them
    if (pendingBakes > 0)
        impl->beginScope (frame, "probeBake");

    impl->bakeReflectionProbes (frame, scene, sceneSettings, settings, drawList, deviceFacts, environmentCube, *samplers, failProbeCaptures, failVolumeCaptures);

    // Without the comparison sampler nothing can read a shadow map: the frame renders unshadowed
    ShadowRenderer::Inputs shadowInputs { drawList, impl->viewData, *impl->capabilities, settings };
    shadowInputs.globalSignature = impl->getShadowSignature (settings);
    shadowInputs.casterSignatures = impl->shadowCasterSignatures;
    shadowInputs.enabled = samplers->shadowCompare != nullptr;

    const auto casterDrawsBefore = impl->shadowRenderer.getNumCasterDraws();
    const auto shadowFramesBefore = impl->shadowRenderer.getNumRenderedFrames();

    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (shadows);
        impl->beginScope (frame, "shadows");

        if (auto result = impl->shadowRenderer.render (device, frame, shadowInputs); result.failed())
            return failFrame (result.getErrorMessage());
    }

    auto lighting = LightingInputs::create (sceneSettings, environmentCube, drawList.getLights(), impl->shadowRenderer.getShadowRecordIndices());
    lighting.shadowCascades = impl->shadowRenderer.getCascadeTexture();
    lighting.shadowAtlas = impl->shadowRenderer.getAtlasTexture();
    lighting.shadows = impl->shadowRenderer.getShadowData();
    lighting.shadowFilters = impl->shadowRenderer.needsShadowFilters();
    lighting.lookupTables = impl->lookupTables;

    // A clearcoat or a sheen selects the variants compiling the layers: raster for any item, full
    // for the opaque items of the lighting pass (its forward pass always compiles them)
    lighting.materialLayers = LightingInputs::hasMaterialLayers (drawList.getOpaqueItems())
                           || (! full && LightingInputs::hasMaterialLayers (drawList.getBlendItems()));

    // One flag for every radiance texture of the device, whether or not the scene has an environment
    lighting.block.probeInfo[0] = impl->probeRenderer.getRadianceFormat() == GpuTextureFormat::rgba8unorm ? 1.0f : 0.0f;
    lighting.samplers = *samplers;

    if (! impl->prepareReflectionProbes (lighting, full))
        return failFrame ("Unable to upload the reflection probes");

    if (! impl->prepareIrradianceVolumes (lighting, full))
        return failFrame ("Unable to upload the irradiance volumes");

    // The shading passes compile only the variants the frame shades with: the defaults here, so a
    // failure tells why, the shadow filter variants in the passes, which fall back to the defaults
    if (! lighting.shadowFilters)
    {
        if (auto result = impl->prepareDefaultShading (targets, forwardSamples, full, areaLights, ! full || ! drawList.getBlendItems().empty()); result.failed())
            return failFrame (result.getErrorMessage());
    }

    const auto output = full ? ForwardPass::Output::hdrLinear : (hdr ? ForwardPass::Output::hdrReversible : ForwardPass::Output::ldrInline);
    GpuTexture::Ptr indirectDiffuse; // The screen-space diffuse GI this frame added, if any

    if (full)
    {
        // Lights culled into clusters, the opaque items into the G-buffer, shaded once per pixel
        if (! impl->cullLights (frame, lighting))
            return failFrame ("Unable to cull the lights");

        {
            YUP_PROFILE_NAMED_INTERNAL_TRACE (gbuffer);
            impl->beginScope (frame, "gbuffer");

            if (! impl->gbufferPass.render (frame, impl->viewData, drawList.getOpaqueItems(), impl->materialTable, gbuffer))
                return failFrame ("Unable to render the G-buffer");
        }

        // Uploaded once the G-buffer draws have numbered the materials
        lighting.materials = impl->uploadStorageBuffer (FrameResources::StorageSlot::materials, impl->materialTable.getEntries());
        if (lighting.materials == nullptr)
            return failFrame ("Unable to upload the material table");

        if (! impl->renderAmbientOcclusion (frame, settings, temporalAntiAliasing, gbuffer.depth, gbuffer.normalRoughness, lighting,
                                            impl->getIndirectDiffuseRadiance (settings, gbuffer.velocity), failScreenSpaceGI))
            return failFrame ("Unable to render the ambient occlusion");

        impl->renderScreenSpaceReflections (frame, settings, gbuffer, lighting, failScreenSpaceReflections);

        if (failLocalIndirectLighting)
            impl->lightingPass.failLocalIndirectForTesting();

        if (failMaterialLayers)
            impl->lightingPass.failMaterialLayersForTesting();

        if (failLightingClassification)
            impl->lightingPass.failClassificationForTesting();

        impl->lightingPass.disableClassificationForTesting (disableLightingClassification);

        {
            YUP_PROFILE_NAMED_INTERNAL_TRACE (lighting);
            impl->beginScope (frame, "lighting");

            if (! impl->lightingPass.render (device, frame, impl->viewData, gbuffer, lighting, targets.color))
                return failFrame ("Unable to render the lighting pass");
        }

        // The screen-space diffuse GI the ambient occlusion gathered, if any: a failure leaves the frame without it
        if (impl->ambientOcclusionPass != nullptr && impl->ambientOcclusionPass->getIndirectDiffuse() != nullptr)
        {
            YUP_PROFILE_NAMED_INTERNAL_TRACE (indirectDiffuse);
            impl->beginScope (frame, "indirectDiffuse");

            if (impl->ambientOcclusionPass->addIndirectDiffuse (frame, impl->viewData, gbuffer, lighting, targets.color))
                indirectDiffuse = impl->ambientOcclusionPass->getIndirectDiffuse();
        }

        impl->lightingPass.trimUnusedTargets();
    }
    else
    {
        impl->gbufferPass.releaseTargets();
        impl->lightingPass.releaseTargets();
        impl->releaseReflectionTargets();
    }

    // Raster screen-space effects read the depth and normals of the prepass, full ones the G-buffer
    if (depthNormalPrepassTargetFailures > 0)
        impl->depthNormalPrepassTargetFailures = std::exchange (depthNormalPrepassTargetFailures, 0);

    if (! impl->renderDepthNormalPrepass (frame, settings, blurs, fogs, widthPx, heightPx, failDepthNormalPrepass))
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
                            : std::optional<GpuColor> (output == ForwardPass::Output::ldrInline ? GpuColor (sceneSettings.backgroundColor) : GpuColor::transparentBlack());
    const auto opaque = full ? Span<const SceneDrawList::DrawItem>() : drawList.getOpaqueItems();

    if (failMaterialLayers)
        impl->forwardPass.failMaterialLayersForTesting();

    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (forward);

        // Full: only the blended items, when there are any
        if (! full || ! drawList.getBlendItems().empty())
            impl->beginScope (frame, "forward");

        if (! impl->forwardPass.render (device, frame, impl->viewData, lighting, opaque, drawList.getBlendItems(), targets, output, forwardSamples, clear))
            return failFrame ("Unable to render the forward pass");
    }

    if (! hdr)
        impl->postChain.releaseTargets();

    // Raster: the resolved color, reversibly tone mapped, the single sampled depth of the prepass
    // when it ran, and no motion for the post effects
    auto sceneColor = targets.resolve != nullptr ? targets.resolve : targets.color;
    auto colorIsReversible = ! full;
    const auto depth = full ? targets.depth : (impl->depthNormalPrepass != nullptr ? impl->depthNormalPrepass->getDepth() : nullptr);

    if (worldFogFailures > 0)
        impl->worldFogFailures = std::exchange (worldFogFailures, 0);

    // The world fog, before TAA: in place on full, the first post pass on raster, which writes linear color
    if (auto fogged = impl->renderWorldFog (frame, worldFog, fogs, lighting, { sceneColor, colorIsReversible, depth }))
    {
        sceneColor = std::move (fogged);
        colorIsReversible = false;
    }

    if (hdr)
    {
        const PostChain::Inputs inputs { sceneColor, depth, targets.velocity, colorIsReversible, sceneSettings.backgroundColor,
                                         impl->getTemporalSampleIndex(), temporalSamples, impl->changing, scene.getActiveCamera(), blurs, moving,
                                         impl->measuring ? std::function<void (const char*)> ([this, &frame] (const char* name) { impl->beginScope (frame, name); })
                                                         : nullptr };

        if (depthOfFieldFailures > 0)
            impl->postChain.failDepthOfFieldForTesting (std::exchange (depthOfFieldFailures, 0));

        if (motionBlurFailures > 0)
            impl->postChain.failMotionBlurForTesting (std::exchange (motionBlurFailures, 0));

        if (! impl->postChain.render (device, frame, *impl->capabilities, settings, impl->viewData, impl->frameResources, inputs, impl->colorTarget->asTexture()))
            return failFrame ("Unable to render the post effects");
    }

    impl->renderDebugView (frame, settings, full, lighting, indirectDiffuse, impl->postChain.getResolvedColor(), impl->colorTarget->asTexture());

    // The counts travel with the frame's status, which adds its GPU times once it completed
    if (impl->measuring)
    {
        FrameStats stats;
        stats.frameIndex = frameIndex;
        stats.drawItems = static_cast<int> (drawList.getOpaqueItems().size() + drawList.getBlendItems().size());
        stats.shadowCasterDraws = impl->shadowRenderer.getNumCasterDraws() - casterDrawsBefore;
        stats.shadowsRedrawn = impl->shadowRenderer.getNumRenderedFrames() != shadowFramesBefore;
        stats.internalWidth = internalWidth;
        stats.internalHeight = internalHeight;
        impl->pendingFrames.back().stats = std::move (stats);
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
    impl->reflectionsOutput = lighting.reflections;
    impl->indirectDiffuseOutput = indirectDiffuse;

    const auto* depthOfField = hdr ? impl->postChain.getDepthOfField() : nullptr;
    impl->circleOfConfusionOutput = depthOfField != nullptr ? depthOfField->getCircleOfConfusionTexture() : nullptr;
    impl->depthOfFieldOutput = depthOfField != nullptr ? depthOfField->getOutput() : nullptr;

    const auto* motionBlur = hdr ? impl->postChain.getMotionBlur() : nullptr;
    impl->motionBlurOutput = motionBlur != nullptr ? motionBlur->getOutput() : nullptr;
    impl->previousExposure = impl->viewData.getBlock().cameraPosition[3];

    lastError = {};
    return impl->colorTarget->asTexture();
}

} // namespace yup
