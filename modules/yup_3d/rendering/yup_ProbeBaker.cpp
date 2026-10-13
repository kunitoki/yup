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

constexpr uint8_t probeCaptureDistanceShaderBundle[] = {
#include "../shaders/yup_ProbeCaptureDistance.inc"
};

// std140 mirror of ProbeCaptureBlock in yup_ProbeCaptureResolve.frag
struct ProbeCaptureBlock
{
    float params[4]; // x 1 when the capture is Reinhard and sRGB encoded and the cube holds RGBM
};

// std140 mirror of ProbeDistanceBlock in yup_ProbeCaptureDistance.frag
struct ProbeDistanceBlock
{
    float projection[4]; // the capture projection's m22, m32, m23 and m33 (column, row)
};

/** The near plane of the captures of the irradiance volume probes. */
constexpr float volumeClipNear = 0.01f;

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

/** The capture of one cube face of @p size texels: a 90 degree square perspective along the face's axis. */
void setCaptureFace (ViewData& view, const Vector3<float>& position, size_t face, float clipNear, float clipFar, int size = ProbeBaker::tileSize)
{
    const auto& [axis, up] = captureFaces[face];
    view.setCapture (Matrix4::lookAt (position, position + axis, up),
                     Matrix4::perspective (MathConstants<float>::halfPi, 1.0f, clipNear, clipFar),
                     position,
                     size);
}

/** The near and far planes of the captures of @p probe: the far one four times the influence's
    bounding radius, at least 100, and the near one within 0.001 and half the far one. */
std::pair<float, float> getCaptureClipPlanes (const SceneDrawList::ReflectionProbe& probe) noexcept
{
    const auto clipFar = jmax (100.0f, 4.0f * probe.bounds.getRadius());
    return { jmin (jmax (0.001f, probe.node->clipNear), 0.5f * clipFar), clipFar };
}

/** The size the selections rank probes and volumes by: the volume of their world bounds. */
float getBoundsVolume (const BoundingBox& bounds) noexcept
{
    const auto size = bounds.getSize();
    return size.getX() * size.getY() * size.getZ();
}

/** The far plane of the captures of a volume's probes: four times the radius of its box, at least 100. */
float getVolumeClipFar (const SceneDrawList::IrradianceVolume& volume) noexcept
{
    return jmax (100.0f, 4.0f * volume.bounds.getRadius());
}

/** The world position of probe (i, j, k) of @p volume: the center of its cell of the box. */
Vector3<float> getVolumeProbePosition (const SceneDrawList::IrradianceVolume& volume, int i, int j, int k) noexcept
{
    const auto [nx, ny, nz] = GpuSceneData::getVolumeResolution (*volume.node);
    const auto& e = volume.node->extents;
    const Vector3<float> unit ((static_cast<float> (i) + 0.5f) / static_cast<float> (nx),
                               (static_cast<float> (j) + 0.5f) / static_cast<float> (ny),
                               (static_cast<float> (k) + 0.5f) / static_cast<float> (nz));

    return volume.world.transformPoint ({ e.getX() * (2.0f * unit.getX() - 1.0f), e.getY() * (2.0f * unit.getY() - 1.0f), e.getZ() * (2.0f * unit.getZ() - 1.0f) });
}

/** The world length of the diagonal of one cell of @p volume, the unit of its visibility distances. */
float getVolumeCellDiagonal (const SceneDrawList::IrradianceVolume& volume) noexcept
{
    const auto [nx, ny, nz] = GpuSceneData::getVolumeResolution (*volume.node);
    const auto& e = volume.node->extents;
    return volume.world.transformVector ({ 2.0f * e.getX() / static_cast<float> (nx), 2.0f * e.getY() / static_cast<float> (ny), 2.0f * e.getZ() / static_cast<float> (nz) }).length();
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

uint64 ProbeBaker::getSignature (const SceneDrawList::IrradianceVolume& volume) noexcept
{
    const auto& node = *volume.node;

    FrameSignature signature;
    signature.addVector (node.extents);
    signature.addBits (static_cast<uint64> (static_cast<uint32> (node.resolutionX)));
    signature.addBits (static_cast<uint64> (static_cast<uint32> (node.resolutionY)));
    signature.addBits (static_cast<uint64> (static_cast<uint32> (node.resolutionZ)));
    signature.addFloat (node.falloff);
    signature.addFloat (node.intensity);
    signature.addFloat (node.normalBias);
    signature.addMatrix (volume.world);
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

const ProbeBaker::BakedVolume* ProbeBaker::findVolumeEntry (uint64 bakeId) const noexcept
{
    const auto entry = std::find_if (bakedVolumes.begin(), bakedVolumes.end(), [bakeId] (const BakedVolume& e) { return e.bakeId == bakeId; });
    return entry != bakedVolumes.end() ? &*entry : nullptr;
}

bool ProbeBaker::needsBake (const SceneDrawList::IrradianceVolume& volume) const noexcept
{
    // An atlas in another format than the radiance textures is baked again whole
    if (irradianceTexture != nullptr && irradianceTexture->getFormat() != probes.getRadianceFormat())
        return true;

    const auto* entry = findVolumeEntry (volume.node->getBakeId());
    return entry == nullptr || entry->stale || entry->signature != getSignature (volume);
}

int ProbeBaker::getNumPending (const SceneDrawList& drawList) const
{
    const auto probesOfFrame = drawList.getReflectionProbes();
    const auto volumesOfFrame = drawList.getIrradianceVolumes();
    const auto pendingProbes = probesGivenUp ? 0 : std::count_if (probesOfFrame.begin(), probesOfFrame.end(), [this] (const auto& probe) { return needsBake (probe); });
    const auto pendingVolumes = volumesGivenUp ? 0 : std::count_if (volumesOfFrame.begin(), volumesOfFrame.end(), [this] (const auto& volume) { return needsBake (volume); });
    return static_cast<int> (pendingProbes + pendingVolumes);
}

void ProbeBaker::forgetBakes() noexcept
{
    forgetProbeBakes();
    forgetVolumeBakes();
}

void ProbeBaker::forgetProbeBakes() noexcept
{
    for (auto& entry : baked)
        entry.stale = true;

    if (captureShadows != nullptr)
        captureShadows->invalidate();
}

void ProbeBaker::forgetVolumeBakes() noexcept
{
    for (auto& entry : bakedVolumes)
    {
        entry.stale = true;
        entry.nextPosition = 0;
    }

    if (captureShadows != nullptr)
        captureShadows->invalidate();
}

void ProbeBaker::clearFailures() noexcept
{
    probeFailures = 0;
    volumeFailures = 0;
    probesGivenUp = false;
    volumesGivenUp = false;
}

void ProbeBaker::releaseCaptureTargets() noexcept
{
    // The pipelines stay: frames still in flight may use them, while they keep the textures they bind
    if (captureShadows != nullptr)
        captureShadows->releaseTextures();

    scratch.reset();
    volumeScratch.reset();
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

bool ProbeBaker::prepareVolumeAtlases()
{
    const auto format = probes.getRadianceFormat();
    const auto withVisibility = probes.canRenderVisibility();

    if (irradianceTexture == nullptr || irradianceGeneration != irradianceAtlas.getGeneration() || irradianceTexture->getFormat() != format)
    {
        GpuTextureDesc desc (static_cast<uint32> (irradianceAtlasWidth), static_cast<uint32> (irradianceAtlas.getRequiredHeight()), format, true);
        desc.label = "ProbeBaker irradiance atlas";

        irradianceTexture = GpuTexture::create (probes.getDevice(), desc);
        irradianceGeneration = irradianceAtlas.getGeneration();

        // A new texture holds none of the blocks baked before
        for (auto& entry : bakedVolumes)
        {
            entry.stale = true;
            entry.nextPosition = 0;
        }
    }

    if (! withVisibility)
    {
        visibilityTexture = nullptr;
    }
    else if (visibilityTexture == nullptr || visibilityGeneration != visibilityAtlas.getGeneration())
    {
        GpuTextureDesc desc (static_cast<uint32> (visibilityAtlasWidth), static_cast<uint32> (visibilityAtlas.getRequiredHeight()), ProbeRenderer::visibilityFormat, true);
        desc.label = "ProbeBaker visibility atlas";

        visibilityTexture = GpuTexture::create (probes.getDevice(), desc);
        visibilityGeneration = visibilityAtlas.getGeneration();

        for (auto& entry : bakedVolumes)
        {
            entry.stale = true;
            entry.nextPosition = 0;
        }

        if (visibilityTexture == nullptr)
            return false;
    }

    return irradianceTexture != nullptr;
}

ProbeBaker::ResolvePass* ProbeBaker::prepareDistance()
{
    if (distancePass == nullptr)
    {
        distancePass = std::make_unique<ResolvePass>();
        distancePass->format = ProbeRenderer::visibilityFormat;
        distancePass->prepared = distancePass->pass.prepare (probes.getDevice(), probeCaptureDistanceShaderBundle, sizeof (probeCaptureDistanceShaderBundle), ProbeRenderer::visibilityFormat);
    }

    return distancePass->prepared.wasOk() ? distancePass.get() : nullptr;
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
    const auto volumesOfFrame = inputs.drawList.getIrradianceVolumes();

    const auto isInFrame = [&probesOfFrame] (uint64 bakeId)
    {
        return std::any_of (probesOfFrame.begin(), probesOfFrame.end(), [bakeId] (const auto& probe) { return probe.node->getBakeId() == bakeId; });
    };

    const auto isVolumeInFrame = [&volumesOfFrame] (uint64 bakeId)
    {
        return std::any_of (volumesOfFrame.begin(), volumesOfFrame.end(), [bakeId] (const auto& volume) { return volume.node->getBakeId() == bakeId; });
    };

    // The probes and volumes gone from the frame give their tiles and blocks back
    std::erase_if (baked, [&] (const BakedProbe& entry)
    {
        if (isInFrame (entry.bakeId))
            return false;

        if (! entry.tile.isEmpty())
            atlas.release (entry.tile);

        return true;
    });

    std::erase_if (bakedVolumes, [&] (const BakedVolume& entry)
    {
        if (isVolumeInFrame (entry.bakeId))
            return false;

        releaseVolumeBlocks (entry);
        return true;
    });

    if (baked.empty() && probesOfFrame.empty())
    {
        atlas.clear();
        atlasTexture = nullptr;
        atlasFullReported = false;
    }

    if (bakedVolumes.empty() && volumesOfFrame.empty())
    {
        irradianceAtlas.clear();
        visibilityAtlas.clear();
        irradianceTexture = nullptr;
        visibilityTexture = nullptr;
        volumeAtlasFullReported = false;
    }

    const auto collectPending = [&] (std::vector<const SceneDrawList::ReflectionProbe*>& pendingProbes, std::vector<const SceneDrawList::IrradianceVolume*>& pendingVolumes)
    {
        pendingProbes.clear();
        for (const auto& probe : probesOfFrame)
            if (! probesGivenUp && needsBake (probe))
                pendingProbes.push_back (&probe);

        pendingVolumes.clear();
        for (const auto& volume : volumesOfFrame)
            if (! volumesGivenUp && needsBake (volume))
                pendingVolumes.push_back (&volume);
    };

    std::vector<const SceneDrawList::ReflectionProbe*> pending;
    std::vector<const SceneDrawList::IrradianceVolume*> pendingVolumes;
    collectPending (pending, pendingVolumes);

    // The capture targets of the previous call that baked are retained by its frames
    if (pending.empty() && pendingVolumes.empty())
    {
        releaseCaptureTargets();
        return 0;
    }

    // Every tile and block is placed first, so each atlas is sized once. A probe or volume left
    // without one tries again whenever it bakes
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

    const auto withVisibility = probes.canRenderVisibility();

    for (const auto* volume : pendingVolumes)
    {
        auto entry = std::find_if (bakedVolumes.begin(), bakedVolumes.end(), [volume] (const BakedVolume& e) { return e.bakeId == volume->node->getBakeId(); });
        if (entry == bakedVolumes.end())
            entry = bakedVolumes.insert (bakedVolumes.end(), { volume->node->getBakeId(), 0, {}, {}, true });

        // A volume that changed bakes again from its first position, and weighs nothing until that bake completes
        if (const auto signature = getSignature (*volume); entry->signature != signature)
        {
            entry->signature = signature;
            entry->stale = true;
            entry->nextPosition = 0;
        }

        // A new resolution, or visibility coming or going, needs blocks of another size
        const auto [nx, ny, nz] = GpuSceneData::getVolumeResolution (*volume->node);
        const Point<int> shSize (nx * nz, ny * 4);
        const Point<int> visibilitySize (nx * nz * (visibilityTileSize + 2), ny * (visibilityTileSize + 2));

        if (entry->shBlock.getWidth() != shSize.getX() || entry->shBlock.getHeight() != shSize.getY()
            || (withVisibility && (entry->visibilityBlock.getWidth() != visibilitySize.getX() || entry->visibilityBlock.getHeight() != visibilitySize.getY()))
            || (! withVisibility && ! entry->visibilityBlock.isEmpty()))
        {
            releaseVolumeBlocks (*entry);
            entry->shBlock = {};
            entry->visibilityBlock = {};
            entry->nextPosition = 0;

            const auto shBlock = irradianceAtlas.allocate (shSize.getX(), shSize.getY());
            const auto visibilityBlock = withVisibility && shBlock.has_value() ? visibilityAtlas.allocate (visibilitySize.getX(), visibilitySize.getY()) : std::optional<Rectangle<int>>();

            if (shBlock.has_value() && (visibilityBlock.has_value() || ! withVisibility))
            {
                entry->shBlock = *shBlock;
                entry->visibilityBlock = visibilityBlock.value_or (Rectangle<int>());
            }
            else
            {
                if (shBlock.has_value())
                    irradianceAtlas.release (*shBlock);

                if (! std::exchange (volumeAtlasFullReported, true))
                    YUP_DBG ("ProbeBaker: the irradiance volume atlases are full, the volumes left without a block light nothing");
            }
        }
    }

    // Each feature fails on its own: a failure of one leaves the other baking and shading
    const auto hadProbes = ! pending.empty();
    const auto hadVolumes = ! pendingVolumes.empty();
    auto atlasResult = Result::ok();
    auto volumeAtlasResult = Result::ok();

    if (hadProbes && ! prepareAtlas())
        atlasResult = Result::fail ("Unable to create the reflection atlas");

    if (hadVolumes && ! prepareVolumeAtlases())
        volumeAtlasResult = Result::fail ("Unable to create the irradiance volume atlases");

    // A new atlas bakes every probe or volume in it again
    collectPending (pending, pendingVolumes);
    if (atlasResult.failed())
        pending.clear();

    if (volumeAtlasResult.failed())
        pendingVolumes.clear();

    auto result = capture (frame, forward, inputs, pending, pendingVolumes);
    if (atlasResult.failed())
        result.probes = atlasResult;

    if (volumeAtlasResult.failed())
        result.volumes = volumeAtlasResult;

    ++generation;

    // What was recorded before a failure cannot be trusted: that feature bakes again, and weighs
    // nothing until then. Two failures in a row give it up
    const auto settle = [] (const Result& outcome, bool hadWork, int& failures, bool& givenUpFlag, const char* what, const std::function<void()>& forget)
    {
        if (! hadWork)
            return;

        if (outcome.wasOk())
        {
            failures = 0;
            return;
        }

        forget();

        if (++failures >= 2 && ! std::exchange (givenUpFlag, true))
            YUP_DBG ("ProbeBaker: baking the " << what << " failed twice in a row, shading without them: " << outcome.getErrorMessage());
    };

    settle (result.probes, hadProbes, probeFailures, probesGivenUp, "reflection probes", [this] { forgetProbeBakes(); });
    settle (result.volumes, hadVolumes, volumeFailures, volumesGivenUp, "irradiance volumes", [this] { forgetVolumeBakes(); });

    return (result.probes.wasOk() ? result.recordedProbes : 0) + (result.volumes.wasOk() ? result.recordedVolumes : 0);
}

void ProbeBaker::releaseVolumeBlocks (const BakedVolume& entry)
{
    if (! entry.shBlock.isEmpty())
        irradianceAtlas.release (entry.shBlock);

    if (! entry.visibilityBlock.isEmpty())
        visibilityAtlas.release (entry.visibilityBlock);
}

ProbeBaker::CaptureResult ProbeBaker::capture (GpuFrame& frame, ForwardPass& forward, const Inputs& inputs,
                                               Span<const SceneDrawList::ReflectionProbe* const> pending,
                                               Span<const SceneDrawList::IrradianceVolume* const> pendingVolumes)
{
    CaptureResult captured;
    if (pending.empty() && pendingVolumes.empty())
        return captured;

    // The preparation both features share fails them both
    const auto failBoth = [&captured] (const String& error)
    {
        captured.probes = captured.volumes = Result::fail (error);
        return captured;
    };

    const auto& device = probes.getDevice();
    const auto format = probes.getRadianceFormat();
    const auto rgbm = format == GpuTextureFormat::rgba8unorm;

    auto* resolvePass = prepareResolve();
    if (resolvePass == nullptr)
        return failBoth ("Unable to compile the reflection probe capture resolve");

    if (captureShadows == nullptr)
        captureShadows = std::make_unique<ShadowRenderer>();

    if (captureList == nullptr)
        captureList = std::make_unique<SceneDrawList>();

    // The probes first: the capture shadows are fitted from the first capture
    const auto firstPosition = ! pending.empty() ? pending.front()->position
                                                 : getVolumeProbePosition (*pendingVolumes.front(), 0, 0, 0);
    const auto [firstNear, firstFar] = ! pending.empty() ? getCaptureClipPlanes (*pending.front())
                                                         : std::pair { volumeClipNear, getVolumeClipFar (*pendingVolumes.front()) };

    // One list for every capture of the call: per-draw light selection only depends on the bounds
    const auto& scene = inputs.scene;
    captureList->build (*scene.getRoot(), Matrix4::translation (-firstPosition), { scene.isUsingDefaultLight(), true });

    // The raster tier with hard shadows: no temporal noise, no contact shadows, no ambient occlusion
    auto settings = scene.getRenderSettings();
    settings.tier = RenderSettings::TierPreference::raster;
    settings.softShadows = false;
    settings.ambientOcclusion = false;
    const RenderCapabilities capabilities (inputs.facts, settings, 1);

    // One sun map over the whole scene serves every capture, drawn again by every call that bakes
    setCaptureFace (captureView, firstPosition, 0, firstNear, firstFar);
    const std::vector<uint64> casterSignatures (captureList->getOpaqueItems().size(), generation + 1);

    ShadowRenderer::Inputs shadowInputs { *captureList, captureView, capabilities, settings };
    shadowInputs.globalSignature = generation + 1;
    shadowInputs.casterSignatures = casterSignatures;
    shadowInputs.enabled = inputs.samplers.shadowCompare != nullptr;
    shadowInputs.sceneFit = true;

    captureShadows->prepare (device);
    if (auto result = captureShadows->render (device, frame, shadowInputs); result.failed())
        return failBoth (result.getErrorMessage());

    // Exposure 1, the environment drawn sharp behind every capture, no probes, no volumes and no
    // ambient occlusion. ViewData::setCapture() picks Reinhard, which the rgba8 captures below the
    // float floor use
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

    const ProbeCaptureBlock block { { rgbm ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f } };
    const auto output = rgbm ? ForwardPass::Output::ldrInline : ForwardPass::Output::hdrLinear;

    // Below the float floor the captures are tone mapped into rgba8unorm, which the resolve inverts into RGBM
    const auto captureTargets = [&] (FrameResources& pool, int size, const char* colorLabel, const char* depthLabel)
    {
        GpuTextureDesc color (static_cast<uint32> (size), static_cast<uint32> (size), format, true);
        color.label = colorLabel;

        GpuTextureDesc depth (static_cast<uint32> (size), static_cast<uint32> (size), GpuTextureFormat::depth32float, true);
        depth.label = depthLabel;

        ForwardPass::Targets targets;
        targets.color = pool.acquire (color);
        targets.depth = pool.acquire (depth);
        return targets;
    };

    const auto cubeTarget = [] (FrameResources& pool, int size, GpuTextureFormat cubeFormat, uint32 mips, const char* label)
    {
        GpuTextureDesc desc (static_cast<uint32> (size), static_cast<uint32> (size), cubeFormat, true);
        desc.type = GpuTextureType::cube;
        desc.depthOrArrayLayers = 6;
        desc.mipLevels = mips;
        desc.label = label;
        return pool.acquire (desc);
    };

    // Renders the six faces of a capture from @p position into mip 0 of @p cube, and their radial distance into @p distanceCube when given
    const auto captureCube = [&] (const ForwardPass::Targets& targets, const Vector3<float>& position, float clipNear, float clipFar, int size,
                                  const GpuTexture::Ptr& cube, const GpuTexture::Ptr& distanceCube, ResolvePass* distance) -> bool
    {
        for (size_t face = 0; face < captureFaces.size(); ++face)
        {
            setCaptureFace (captureView, position, face, clipNear, clipFar, size);

            if (! forward.render (device, frame, captureView, lighting, captureList->getOpaqueItems(), captureList->getBlendItems(),
                                  targets, output, 1, GpuColor::transparentBlack()))
                return false;

            const auto resolved = resolvePass->draw (frame, cube, GpuTextureViewDesc (0, static_cast<uint32> (face)), [&] (GpuRenderPass& pass)
            {
                pass.setSampler (0, 3, inputs.samplers.pointClamp);
                pass.setUniformBuffer (1, 0, &block, sizeof (block));
                pass.setTexture (1, 1, targets.color);
            });

            if (! resolved)
                return false;

            if (distanceCube == nullptr)
                continue;

            const auto& projection = captureView.getProjection();
            const ProbeDistanceBlock distanceBlock { { projection (2, 2), projection (2, 3), projection (3, 2), projection (3, 3) } };

            const auto measured = distance->pass.draw (frame, distanceCube, GpuTextureViewDesc (0, static_cast<uint32> (face)), [&] (GpuRenderPass& pass)
            {
                pass.setSampler (0, 3, inputs.samplers.pointClamp);
                pass.setUniformBuffer (1, 0, &distanceBlock, sizeof (distanceBlock));
                pass.setTexture (1, 1, targets.depth);
            });

            if (! measured)
                return false;
        }

        return true;
    };

    captured.probes = [&]() -> Result
    {
        if (pending.empty())
            return Result::ok();

        if (scratch == nullptr)
            scratch = std::make_unique<FrameResources> (device);

        scratch->beginFrame (tileSize, tileSize);
        const auto targets = captureTargets (*scratch, tileSize, "ProbeBaker capture color", "ProbeBaker capture depth");
        const auto cube = cubeTarget (*scratch, tileSize, format, static_cast<uint32> (std::log2 (tileSize)) + 1, "ProbeBaker capture cube");
        scratch->endFrame();

        if (targets.color == nullptr || targets.depth == nullptr || cube == nullptr)
            return Result::fail ("Unable to create the reflection probe capture targets");

        if (capturesFailForTesting)
            return Result::fail ("Reflection probe captures failed for testing");

        for (const auto* probe : pending)
        {
            auto& entry = *std::find_if (baked.begin(), baked.end(), [probe] (const BakedProbe& e) { return e.bakeId == probe->node->getBakeId(); });
            entry.signature = getSignature (*probe);
            entry.stale = false;

            // Left without a tile, it reflects nothing
            if (entry.tile.isEmpty())
                continue;

            const auto [clipNear, clipFar] = getCaptureClipPlanes (*probe);

            if (! captureCube (targets, probe->position, clipNear, clipFar, tileSize, cube, nullptr, nullptr))
                return Result::fail ("Unable to render a reflection probe capture");

            if (! probes.downsampleCube (frame, cube) || ! probes.prefilterToAtlas (frame, cube, atlasTexture, entry.tile, numMips))
                return Result::fail ("Unable to filter a reflection probe");

            ++captured.recordedProbes;
            ++numBakes;
        }

        return Result::ok();
    }();

    captured.volumes = [&]() -> Result
    {
        if (pendingVolumes.empty())
            return Result::ok();

        auto* distance = visibilityTexture != nullptr ? prepareDistance() : nullptr;
        if (visibilityTexture != nullptr && distance == nullptr)
            return Result::fail ("Unable to compile the irradiance volume capture distance");

        if (volumeScratch == nullptr)
            volumeScratch = std::make_unique<FrameResources> (device);

        volumeScratch->beginFrame (volumeFaceSize, volumeFaceSize);
        const auto targets = captureTargets (*volumeScratch, volumeFaceSize, "ProbeBaker volume capture color", "ProbeBaker volume capture depth");
        const auto cube = cubeTarget (*volumeScratch, volumeFaceSize, format, static_cast<uint32> (std::log2 (volumeFaceSize)) + 1, "ProbeBaker volume capture cube");
        const auto distanceCube = distance != nullptr ? cubeTarget (*volumeScratch, volumeFaceSize, ProbeRenderer::visibilityFormat, 1, "ProbeBaker volume distance cube") : nullptr;
        volumeScratch->endFrame();

        if (targets.color == nullptr || targets.depth == nullptr || cube == nullptr || (distance != nullptr && distanceCube == nullptr))
            return Result::fail ("Unable to create the irradiance volume capture targets");

        if (volumeCapturesFailForTesting)
            return Result::fail ("Irradiance volume captures failed for testing");

        // At most maxVolumeProbesPerBake positions per call, so the frames recording them keep the
        // device's uniform buffer pool bounded; a volume goes on from where the previous call stopped
        auto budget = maxVolumeProbesPerBake;

        for (const auto* volume : pendingVolumes)
        {
            if (budget <= 0)
                break;

            auto& entry = *std::find_if (bakedVolumes.begin(), bakedVolumes.end(), [volume] (const BakedVolume& e) { return e.bakeId == volume->node->getBakeId(); });
            const auto [nx, ny, nz] = GpuSceneData::getVolumeResolution (*volume->node);
            const auto numPositions = nx * ny * nz;

            // Left without a block, it lights nothing
            if (! entry.shBlock.isEmpty())
            {
                const auto clipFar = getVolumeClipFar (*volume);
                const auto distanceScale = 1.0f / jmax (1.0e-6f, getVolumeCellDiagonal (*volume));

                for (; entry.nextPosition < numPositions && budget > 0; ++entry.nextPosition, --budget)
                {
                    const auto i = entry.nextPosition % nx;
                    const auto j = (entry.nextPosition / nx) % ny;
                    const auto k = entry.nextPosition / (nx * ny);

                    if (! captureCube (targets, getVolumeProbePosition (*volume, i, j, k), volumeClipNear, clipFar, volumeFaceSize, cube, distanceCube, distance))
                        return Result::fail ("Unable to render an irradiance volume capture");

                    // Probe (i, j, k) sits at column i + k nx of the blocks, row j
                    const auto column = i + k * nx;
                    const auto projected = probes.downsampleCube (frame, cube)
                                        && probes.projectIrradiance (frame, cube, irradianceTexture, entry.shBlock.getPosition() + Point<int> (column, j * 4));
                    if (! projected)
                        return Result::fail ("Unable to project an irradiance volume probe");

                    if (distance != nullptr)
                    {
                        const auto origin = entry.visibilityBlock.getPosition() + Point<int> (column, j) * (visibilityTileSize + 2);
                        if (! probes.renderVisibility (frame, distanceCube, visibilityTexture, origin, visibilityTileSize, distanceScale))
                            return Result::fail ("Unable to render the visibility of an irradiance volume probe");
                    }
                }

                ++captured.recordedVolumes;
            }

            // It weighs nothing until every position is captured
            if (entry.shBlock.isEmpty() || entry.nextPosition >= numPositions)
            {
                entry.stale = false;
                if (! entry.shBlock.isEmpty())
                    ++numVolumeBakes;
            }
        }

        return Result::ok();
    }();

    resolvePass->releaseTargets();
    if (distancePass != nullptr)
        distancePass->pass.releaseTargets();

    return captured;
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
        const auto volumeA = getBoundsVolume (probesOfFrame[a].bounds);
        const auto volumeB = getBoundsVolume (probesOfFrame[b].bounds);
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

//==============================================================================
std::vector<GpuIrradianceVolume> ProbeBaker::packIrradianceVolumes (const SceneDrawList& drawList) const
{
    std::vector<GpuIrradianceVolume> packed;
    packed.reserve (drawList.getIrradianceVolumes().size());

    for (const auto& volume : drawList.getIrradianceVolumes())
    {
        const auto* entry = findVolumeEntry (volume.node->getBakeId());
        if (irradianceTexture == nullptr || entry == nullptr || entry->stale || entry->shBlock.isEmpty())
        {
            packed.push_back ({});
            continue;
        }

        const auto withVisibility = visibilityTexture != nullptr && ! entry->visibilityBlock.isEmpty();
        packed.push_back (GpuSceneData::packIrradianceVolume (volume, entry->shBlock.getPosition(), entry->visibilityBlock.getPosition(), withVisibility ? visibilityTileSize : 0));
    }

    return packed;
}

std::vector<GpuIrradianceVolume> ProbeBaker::packIrradianceVolumesByPriority (const SceneDrawList& drawList) const
{
    const auto volumesOfFrame = drawList.getIrradianceVolumes();
    const auto packed = packIrradianceVolumes (drawList);

    std::vector<size_t> order;
    for (size_t i = 0; i < volumesOfFrame.size(); ++i)
    {
        const auto* entry = findVolumeEntry (volumesOfFrame[i].node->getBakeId());
        if (irradianceTexture != nullptr && entry != nullptr && ! entry->stale && ! entry->shBlock.isEmpty())
            order.push_back (i);
    }

    // As the raster selection ranks them: the smallest box first, then the oldest bake id
    std::stable_sort (order.begin(), order.end(), [&] (size_t a, size_t b)
    {
        const auto volumeA = getBoundsVolume (volumesOfFrame[a].bounds);
        const auto volumeB = getBoundsVolume (volumesOfFrame[b].bounds);
        if (volumeA != volumeB)
            return volumeA < volumeB;

        return volumesOfFrame[a].node->getBakeId() < volumesOfFrame[b].node->getBakeId();
    });

    if (order.size() > static_cast<size_t> (maxVolumesPerFrame))
    {
        if (! std::exchange (volumeCapReported, true))
            YUP_DBG ("ProbeBaker: more than " << maxVolumesPerFrame << " irradiance volumes, the full tier leaves out the largest");

        order.resize (static_cast<size_t> (maxVolumesPerFrame));
    }

    std::vector<GpuIrradianceVolume> sorted;
    sorted.reserve (order.size());
    for (const auto index : order)
        sorted.push_back (packed[index]);

    return sorted;
}

} // namespace yup
