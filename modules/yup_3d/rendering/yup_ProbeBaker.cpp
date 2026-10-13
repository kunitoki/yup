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

// Precompiled bundle (do not edit the .inc by hand). Regenerate it after changing
// shaders/yup_ProbeCaptureResolve.frag or the lib files it includes with: just yup3d_shaders
constexpr uint8_t probeCaptureResolveShaderBundle[] = {
#include "../shaders/yup_ProbeCaptureResolve.inc"
};

// std140 mirror of ProbeCaptureBlock in yup_ProbeCaptureResolve.frag
struct ProbeCaptureBlock
{
    float params[4]; // x 1 when the capture is Reinhard and sRGB encoded and the cube holds RGBM
};

/** The axis each cube face looks along and the up of its image, in the order and orientation of
    lib/cube.glsl: +X, -X, +Y, -Y, +Z, -Z. */
const std::array<std::pair<Vector3<float>, Vector3<float>>, 6> captureFaces { {
    { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
    { { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    { { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f } },
} };

/** The capture of one cube face: a 90 degree square perspective along the face's axis. */
void setCaptureFace (ViewData& view, const Vector3<float>& position, size_t face, float clipNear, float clipFar)
{
    const auto& [axis, up] = captureFaces[face];
    view.setCapture (Matrix4::lookAt (position, position + axis, up),
                     Matrix4::perspective (MathConstants<float>::halfPi, 1.0f, clipNear, clipFar),
                     position,
                     ProbeBaker::tileSize);
}

/** The near and far planes of the captures of @p probe: the far one four times the influence's
    bounding radius, at least 100, and the near one within 0.001 and half the far one. */
std::pair<float, float> getCaptureClipPlanes (const SceneDrawList::ReflectionProbe& probe) noexcept
{
    const auto clipFar = jmax (100.0f, 4.0f * probe.bounds.getRadius());
    return { jmin (jmax (0.001f, probe.node->clipNear), 0.5f * clipFar), clipFar };
}

/** The influence size the raster selection ranks a probe by: the volume of its world bounds. */
float getInfluenceVolume (const SceneDrawList::ReflectionProbe& probe) noexcept
{
    const auto size = probe.bounds.getSize();
    return size.getX() * size.getY() * size.getZ();
}

} // namespace

//==============================================================================
ProbeBaker::ProbeBaker (ProbeRenderer& probesToUse)
    : probes (probesToUse)
{
}

ProbeBaker::~ProbeBaker() = default;

//==============================================================================
uint64 ProbeBaker::getSignature (const SceneDrawList::ReflectionProbe& probe) noexcept
{
    const auto& node = *probe.node;

    FrameSignature signature;
    signature.addBits (static_cast<uint64> (node.shape));
    signature.addFloat (node.influenceRadius);
    signature.addVector (node.influenceExtents);
    signature.addFloat (node.falloff);
    signature.addBits (node.parallaxCorrection ? 1u : 0u);
    signature.addFloat (node.intensity);
    signature.addFloat (node.clipNear);
    signature.addMatrix (probe.world);
    return signature.get();
}

const ProbeBaker::BakedProbe* ProbeBaker::findEntry (uint64 bakeId) const noexcept
{
    const auto entry = std::find_if (baked.begin(), baked.end(), [bakeId] (const BakedProbe& e) { return e.bakeId == bakeId; });
    return entry != baked.end() ? &*entry : nullptr;
}

bool ProbeBaker::needsBake (const SceneDrawList::ReflectionProbe& probe) const noexcept
{
    // An atlas in another format than the radiance textures is baked again whole
    if (atlasTexture != nullptr && atlasTexture->getFormat() != probes.getRadianceFormat())
        return true;

    const auto* entry = findEntry (probe.node->getBakeId());
    return entry == nullptr || entry->stale || entry->signature != getSignature (probe);
}

int ProbeBaker::getNumPending (const SceneDrawList& drawList) const
{
    if (givenUp)
        return 0;

    const auto probesOfFrame = drawList.getReflectionProbes();
    return static_cast<int> (std::count_if (probesOfFrame.begin(), probesOfFrame.end(), [this] (const auto& probe) { return needsBake (probe); }));
}

void ProbeBaker::forgetBakes() noexcept
{
    for (auto& entry : baked)
        entry.stale = true;

    if (captureShadows != nullptr)
        captureShadows->invalidate();
}

void ProbeBaker::clearFailures() noexcept
{
    consecutiveFailures = 0;
    givenUp = false;
}

void ProbeBaker::releaseCaptureTargets() noexcept
{
    // The pipelines stay: frames still in flight may use them, while they keep the textures they bind
    if (captureShadows != nullptr)
        captureShadows->releaseTextures();

    scratch.reset();
    captureList.reset();
}

bool ProbeBaker::prepareAtlas()
{
    const auto format = probes.getRadianceFormat();
    if (atlasTexture != nullptr && atlasGeneration == atlas.getGeneration() && atlasTexture->getFormat() == format)
        return true;

    GpuTextureDesc desc (static_cast<uint32> (atlasWidth), static_cast<uint32> (atlas.getRequiredHeight()), format, true);
    desc.mipLevels = static_cast<uint32> (numMips);
    desc.label = "ProbeBaker reflection atlas";

    atlasTexture = GpuTexture::create (probes.getDevice(), desc);
    atlasGeneration = atlas.getGeneration();

    // A new texture holds none of the tiles baked before
    for (auto& entry : baked)
        entry.stale = true;

    return atlasTexture != nullptr;
}

FullscreenPass* ProbeBaker::prepareResolve()
{
    const auto format = probes.getRadianceFormat();
    auto existing = std::find_if (resolvePasses.begin(), resolvePasses.end(), [format] (const auto& resolve) { return resolve->format == format; });

    if (existing == resolvePasses.end())
    {
        auto resolve = std::make_unique<ResolvePass>();
        resolve->format = format;
        resolve->prepared = resolve->pass.prepare (probes.getDevice(), probeCaptureResolveShaderBundle, sizeof (probeCaptureResolveShaderBundle), format);

        resolvePasses.push_back (std::move (resolve));
        existing = std::prev (resolvePasses.end());
    }

    return (*existing)->prepared.wasOk() ? &(*existing)->pass : nullptr;
}

//==============================================================================
int ProbeBaker::bake (GpuFrame& frame, ForwardPass& forward, const Inputs& inputs)
{
    const auto probesOfFrame = inputs.drawList.getReflectionProbes();

    const auto isInFrame = [&probesOfFrame] (uint64 bakeId)
    {
        return std::any_of (probesOfFrame.begin(), probesOfFrame.end(), [bakeId] (const auto& probe) { return probe.node->getBakeId() == bakeId; });
    };

    // The probes gone from the frame give their tiles back
    std::erase_if (baked, [&] (const BakedProbe& entry)
    {
        if (isInFrame (entry.bakeId))
            return false;

        if (! entry.tile.isEmpty())
            atlas.release (entry.tile);

        return true;
    });

    if (baked.empty() && probesOfFrame.empty())
    {
        atlas.clear();
        atlasTexture = nullptr;
        atlasFullReported = false;
    }

    std::vector<const SceneDrawList::ReflectionProbe*> pending;
    for (const auto& probe : probesOfFrame)
        if (needsBake (probe))
            pending.push_back (&probe);

    // The capture targets of the previous call that baked are retained by its frames
    if (pending.empty() || givenUp)
    {
        releaseCaptureTargets();
        return 0;
    }

    // Every tile is placed first, so the atlas is sized once. A probe left without one tries again
    // whenever it bakes
    for (const auto* probe : pending)
    {
        auto entry = std::find_if (baked.begin(), baked.end(), [probe] (const BakedProbe& e) { return e.bakeId == probe->node->getBakeId(); });
        if (entry == baked.end())
            entry = baked.insert (baked.end(), { probe->node->getBakeId(), 0, {}, true });

        if (! entry->tile.isEmpty())
            continue;

        const auto tile = atlas.allocate (tileSize, tileSize, tileSize);
        if (! tile.has_value() && ! std::exchange (atlasFullReported, true))
            YUP_DBG ("ProbeBaker: the reflection atlas is full, the probes left without a tile reflect nothing");

        entry->tile = tile.value_or (Rectangle<int>());
    }

    const auto result = [&]() -> ResultValue<int>
    {
        if (! prepareAtlas())
            return makeResultValueFail ("Unable to create the reflection atlas");

        // A new atlas bakes every probe again
        pending.clear();
        for (const auto& probe : probesOfFrame)
            if (needsBake (probe))
                pending.push_back (&probe);

        return capture (frame, forward, inputs, pending);
    }();

    ++generation;

    if (result.wasOk())
    {
        consecutiveFailures = 0;
        return result.getValue();
    }

    // What was recorded before the failure cannot be trusted: everything bakes again, and weighs nothing until then
    forgetBakes();

    if (++consecutiveFailures >= 2 && ! std::exchange (givenUp, true))
        YUP_DBG ("ProbeBaker: baking the reflection probes failed twice in a row, shading without them: " << result.getErrorMessage());

    return 0;
}

ResultValue<int> ProbeBaker::capture (GpuFrame& frame, ForwardPass& forward, const Inputs& inputs, Span<const SceneDrawList::ReflectionProbe* const> pending)
{
    const auto& device = probes.getDevice();
    const auto format = probes.getRadianceFormat();
    const auto rgbm = format == GpuTextureFormat::rgba8unorm;

    auto* resolvePass = prepareResolve();
    if (resolvePass == nullptr)
        return makeResultValueFail ("Unable to compile the reflection probe capture resolve");

    if (captureShadows == nullptr)
        captureShadows = std::make_unique<ShadowRenderer>();

    if (scratch == nullptr)
        scratch = std::make_unique<FrameResources> (device);

    if (captureList == nullptr)
        captureList = std::make_unique<SceneDrawList>();

    // One list for every capture of the call: per-draw light selection only depends on the bounds
    const auto& scene = inputs.scene;
    captureList->build (*scene.getRoot(), Matrix4::translation (-pending.front()->position), { scene.isUsingDefaultLight(), true });

    // The raster tier with hard shadows: no temporal noise, no contact shadows, no ambient occlusion
    auto settings = scene.getRenderSettings();
    settings.tier = RenderSettings::TierPreference::raster;
    settings.softShadows = false;
    settings.ambientOcclusion = false;
    const RenderCapabilities capabilities (inputs.facts, settings, 1);

    // One sun map over the whole scene serves every capture, drawn again by every call that bakes
    const auto [firstNear, firstFar] = getCaptureClipPlanes (*pending.front());
    setCaptureFace (captureView, pending.front()->position, 0, firstNear, firstFar);
    const std::vector<uint64> casterSignatures (captureList->getOpaqueItems().size(), generation + 1);

    ShadowRenderer::Inputs shadowInputs { *captureList, captureView, capabilities, settings };
    shadowInputs.globalSignature = generation + 1;
    shadowInputs.casterSignatures = casterSignatures;
    shadowInputs.enabled = inputs.samplers.shadowCompare != nullptr;
    shadowInputs.sceneFit = true;

    captureShadows->prepare (device);
    if (auto result = captureShadows->render (device, frame, shadowInputs); result.failed())
        return makeResultValueFail (result.getErrorMessage());

    // Exposure 1, the environment drawn sharp behind every capture, no probes and no ambient
    // occlusion. ViewData::setCapture() picks Reinhard, which the rgba8 captures below the float floor use
    auto lighting = LightingInputs::create (scene, inputs.environmentCube, captureList->getLights(), captureShadows->getShadowRecordIndices());
    lighting.shadowCascades = captureShadows->getCascadeTexture();
    lighting.shadowAtlas = captureShadows->getAtlasTexture();
    lighting.shadows = captureShadows->getShadowData();
    lighting.shadowFilters = captureShadows->needsShadowFilters();
    lighting.lookupTables = inputs.lookupTables;
    lighting.samplers = inputs.samplers;
    lighting.block.ambient[3] = lighting.environment != nullptr ? 1.0f : 0.0f;
    lighting.block.environmentInfo[2] = 0.0f;
    lighting.block.probeInfo[0] = rgbm ? 1.0f : 0.0f;

    scratch->beginFrame (tileSize, tileSize);

    const auto result = [&]() -> ResultValue<int>
    {
        const auto acquire = [this] (GpuTextureDesc desc, const char* label)
        {
            desc.label = label;
            return scratch->acquire (desc);
        };

        // Below the float floor the capture is tone mapped into rgba8unorm, which the resolve inverts into RGBM
        ForwardPass::Targets targets;
        targets.color = acquire (GpuTextureDesc (tileSize, tileSize, format, true), "ProbeBaker capture color");
        targets.depth = acquire (GpuTextureDesc (tileSize, tileSize, GpuTextureFormat::depth32float, true), "ProbeBaker capture depth");

        GpuTextureDesc cubeDesc (tileSize, tileSize, format, true);
        cubeDesc.type = GpuTextureType::cube;
        cubeDesc.depthOrArrayLayers = 6;
        cubeDesc.mipLevels = static_cast<uint32> (std::log2 (tileSize)) + 1;
        const auto cube = acquire (cubeDesc, "ProbeBaker capture cube");

        if (targets.color == nullptr || targets.depth == nullptr || cube == nullptr)
            return makeResultValueFail ("Unable to create the reflection probe capture targets");

        if (capturesFailForTesting)
            return makeResultValueFail ("Reflection probe captures failed for testing");

        const ProbeCaptureBlock block { { rgbm ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f } };
        const auto output = rgbm ? ForwardPass::Output::ldrInline : ForwardPass::Output::hdrLinear;
        auto numBaked = 0;

        for (const auto* probe : pending)
        {
            auto& entry = *std::find_if (baked.begin(), baked.end(), [probe] (const BakedProbe& e) { return e.bakeId == probe->node->getBakeId(); });
            entry.signature = getSignature (*probe);
            entry.stale = false;

            // Left without a tile, it reflects nothing
            if (entry.tile.isEmpty())
                continue;

            const auto [clipNear, clipFar] = getCaptureClipPlanes (*probe);

            for (size_t face = 0; face < captureFaces.size(); ++face)
            {
                setCaptureFace (captureView, probe->position, face, clipNear, clipFar);

                if (! forward.render (device, frame, captureView, lighting, captureList->getOpaqueItems(), captureList->getBlendItems(),
                                      targets, output, 1, GpuColor::transparentBlack()))
                    return makeResultValueFail ("Unable to render a reflection probe capture");

                const auto resolved = resolvePass->draw (frame, cube, GpuTextureViewDesc (0, static_cast<uint32> (face)), [&] (GpuRenderPass& pass)
                {
                    pass.setSampler (0, 3, inputs.samplers.pointClamp);
                    pass.setUniformBuffer (1, 0, &block, sizeof (block));
                    pass.setTexture (1, 1, targets.color);
                });

                if (! resolved)
                    return makeResultValueFail ("Unable to copy a reflection probe capture");
            }

            if (! probes.downsampleCube (frame, cube) || ! probes.prefilterToAtlas (frame, cube, atlasTexture, entry.tile, numMips))
                return makeResultValueFail ("Unable to filter a reflection probe");

            ++numBaked;
            ++numBakes;
        }

        return makeResultValueOk (numBaked);
    }();

    resolvePass->releaseTargets();
    scratch->endFrame();
    return result;
}

//==============================================================================
std::vector<GpuReflectionProbe> ProbeBaker::packReflectionProbes (const SceneDrawList& drawList) const
{
    std::vector<GpuReflectionProbe> packed;
    packed.reserve (drawList.getReflectionProbes().size());

    for (const auto& probe : drawList.getReflectionProbes())
    {
        const auto* entry = findEntry (probe.node->getBakeId());
        if (atlasTexture == nullptr || entry == nullptr || entry->stale || entry->tile.isEmpty())
        {
            packed.push_back ({});
            continue;
        }

        const auto width = static_cast<float> (atlasTexture->getWidth());
        const auto height = static_cast<float> (atlasTexture->getHeight());
        const auto& tile = entry->tile;
        const Rectangle<float> uv (static_cast<float> (tile.getX()) / width, static_cast<float> (tile.getY()) / height,
                                   static_cast<float> (tile.getWidth()) / width, static_cast<float> (tile.getHeight()) / height);

        packed.push_back (GpuSceneData::packReflectionProbe (probe, uv, numMips));
    }

    return packed;
}

std::vector<GpuReflectionProbe> ProbeBaker::packReflectionProbesByPriority (const SceneDrawList& drawList) const
{
    const auto probesOfFrame = drawList.getReflectionProbes();
    const auto packed = packReflectionProbes (drawList);

    std::vector<size_t> order;
    for (size_t i = 0; i < probesOfFrame.size(); ++i)
    {
        const auto* entry = findEntry (probesOfFrame[i].node->getBakeId());
        if (atlasTexture != nullptr && entry != nullptr && ! entry->stale && ! entry->tile.isEmpty())
            order.push_back (i);
    }

    // As the raster selection ranks them, without its distance to each draw: the smallest influence first, then the oldest bake id
    std::stable_sort (order.begin(), order.end(), [&] (size_t a, size_t b)
    {
        const auto volumeA = getInfluenceVolume (probesOfFrame[a]);
        const auto volumeB = getInfluenceVolume (probesOfFrame[b]);
        if (volumeA != volumeB)
            return volumeA < volumeB;

        return probesOfFrame[a].node->getBakeId() < probesOfFrame[b].node->getBakeId();
    });

    if (order.size() > static_cast<size_t> (maxProbesPerFrame))
    {
        if (! std::exchange (capReported, true))
            YUP_DBG ("ProbeBaker: more than " << maxProbesPerFrame << " reflection probes, the full tier leaves out the largest");

        order.resize (static_cast<size_t> (maxProbesPerFrame));
    }

    std::vector<GpuReflectionProbe> sorted;
    sorted.reserve (order.size());
    for (const auto index : order)
        sorted.push_back (packed[index]);

    return sorted;
}

} // namespace yup
