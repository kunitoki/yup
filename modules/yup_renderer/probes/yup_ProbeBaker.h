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

class ForwardPass;

//==============================================================================
/** Bakes the reflection probes of a scene into the reflection atlas, and its irradiance volumes into
    the irradiance and visibility atlases, and packs them for the shading passes.

    A probe bakes when it has no bake yet: when its part was attached, or given a new bake id by
    ReflectionProbeNode::requestBake(). It also bakes again when one of its own fields or the world
    matrix of its entity changed since its bake (getSignature()). Changes to the rest of the scene
    never bake it.

    Each bake captures the scene around the probe into the six faces of a 128 texel cube, with the
    raster variant of the ForwardPass on both tiers: exposure 1, no tone mapping (below the float
    floor, Reinhard tone mapped and sRGB encoded into rgba8, then inverted into RGBM), the
    environment always drawn behind, no probes and no ambient occlusion. The capture is lit by the
    scene's lights and shadowed by a ShadowRenderer of its own (hard filtering, no temporal noise,
    no contact shadows), drawn once per call that bakes: the sun from one map fitted to the whole
    scene, point and spot lights from atlas tiles sized for the first probe's capture, so at low
    resolution. The cube is then GGX-prefiltered (ProbeRenderer::prefilterToAtlas()) into a 128
    texel octahedral tile of the atlas, ProbeAtlas (1024, 4096), with 6 mips, one roughness per mip.

    The capture shadow renderer, its pipelines and the resolve and distance pipelines live as long
    as the baker, since frames still in flight may use them. A call that bakes nothing releases
    their textures: the capture shadow maps, the scratch targets and the capture draw list. Each
    atlas is kept while a probe or volume uses it, created again with every probe or volume in it
    baked again when it grows, and released when none is left.

    A bake that cannot be prepared or recorded never fails the frame: the probes, or the volumes,
    whichever failed, bake again and the frame shades without them meanwhile. After two failures
    in a row of one of them the baker gives that one up, logged once, until clearFailures(); the
    other keeps baking.

    An irradiance volume bakes under the same rules. Each of its probes, at the cell centers of the
    grid spanning its box (GpuSceneData::getVolumeResolution()), captures the scene the same way
    into a 32 texel cube, with the radial distance of every texel taken from the capture depth
    (shaders/yup_ProbeCaptureDistance.frag), lit as the reflection probes are: one bounce, the
    volumes and probes do not light their captures. ProbeRenderer::projectIrradiance() then writes
    its two bands of spherical harmonics into four texels of the irradiance atlas, ProbeAtlas (256,
    1024) in the radiance format, and ProbeRenderer::renderVisibility() its visibility moments into
    an 18 texel bordered octahedral tile of the visibility atlas, ProbeAtlas (1024, 4096), rg16float.
    A volume's blocks hold probe (i, j, k) at column i + k nx and row j: its SH block is (nx nz) x
    (4 ny) texels, its visibility block (18 nx nz) x (18 ny). Below the float floor (RGBM radiance)
    there is no visibility atlas: volumes shade without visibility. Every volume fits the atlases
    (IrradianceVolumeNode::maxResolution); a volume left without a block because others fill them
    lights nothing, logged once. A call captures at most maxVolumeProbesPerBake probe positions, a
    volume going on from where the previous call stopped and weighing nothing until all of them
    are captured, so the frames recording a bake keep the device's uniform buffer pool bounded.

    The bookkeeping holds one entry per probe, by bake id: the signature it baked with and its tile.
    Volumes hold their entries apart, with their blocks, as their bake ids come from another counter.
    An entry whose probe is gone from the draw list gives its tile back. forgetBakes() makes every
    entry bake again into its tile, for frames the GPU did not execute.

    @internal
*/
class YUP_API ProbeBaker
{
public:
    //==============================================================================
    /** The width and height of a probe's tile in the atlas, and of its capture faces. */
    static constexpr int tileSize = 128;

    /** The mips of each tile: mip m is filtered for the perceptual roughness m / (numMips - 1). */
    static constexpr int numMips = 6;

    /** The width of the reflection atlas, and the height it may grow to. */
    static constexpr int atlasWidth = 1024;
    static constexpr int maxAtlasHeight = 4096;

    /** The most probes the full tier shades a frame with, see packReflectionProbesByPriority(). */
    static constexpr int maxProbesPerFrame = 64;

    /** The width and height of the capture faces of an irradiance volume's probes. */
    static constexpr int volumeFaceSize = 32;

    /** The width and height of each probe's octahedral visibility tile, without its one texel border. */
    static constexpr int visibilityTileSize = 16;

    /** The width of the irradiance atlas, and the height it may grow to. */
    static constexpr int irradianceAtlasWidth = 256;
    static constexpr int maxIrradianceAtlasHeight = 1024;

    /** The width of the visibility atlas, and the height it may grow to. */
    static constexpr int visibilityAtlasWidth = 1024;
    static constexpr int maxVisibilityAtlasHeight = 4096;

    /** The most irradiance volumes the full tier shades a frame with, see packIrradianceVolumesByPriority(). */
    static constexpr int maxVolumesPerFrame = 8;

    /** The most irradiance volume probe positions one bake() captures: a volume with more bakes over
        several calls, and weighs nothing until all of its positions are captured (preflight R28). */
    static constexpr int maxVolumeProbesPerBake = 48;

    //==============================================================================
    /** What a bake reads besides the ForwardPass drawing the captures. */
    struct Inputs
    {
        const Scene& scene;                              ///< The scene to capture: its tree and lights.
        const SceneSettings& sceneSettings;              ///< The resolved scene features: the ambient light, the environment and the default light.
        const RenderSettings& settings;                  ///< The settings of the renderer, the captures render on the raster tier with hard shadows.
        const SceneDrawList& drawList;                   ///< The probes and volumes of the frame (SceneDrawList::getReflectionProbes(), getIrradianceVolumes()).
        const RenderCapabilities::DeviceFacts& facts;    ///< The device, for the raster capabilities the captures render with.
        const GpuTexture::Ptr& environmentCube;          ///< The environment cube of the scene on this device, or nullptr.
        const GpuTexture::Ptr& lookupTables;             ///< The lookup tables (LookupTables).
        const FrameResources::Samplers& samplers;        ///< The shared set 0 samplers.
    };

    //==============================================================================
    /** Creates a baker filtering with @p probes, the renderer's toolkit of the same device. */
    explicit ProbeBaker (ProbeRenderer& probes);

    /** Destructor. */
    ~ProbeBaker();

    //==============================================================================
    /** What a bake of @p probe depends on besides its bake id: every field of its part and the
        world matrix of its entity. */
    static uint64 getSignature (const SceneDrawList::ReflectionProbe& probe) noexcept;

    /** What a bake of @p volume depends on besides its bake id: every field of its part and the
        world matrix of its entity. */
    static uint64 getSignature (const SceneDrawList::IrradianceVolume& volume) noexcept;

    /** The number of probes and volumes of @p drawList still to bake, a volume counting until all of
        its positions are captured. A feature the baker gave up counts 0. */
    int getNumPending (const SceneDrawList& drawList) const;

    /** Bakes the pending probes and volumes of Inputs::drawList into @p frame, and gives back the
        tiles and blocks of the probes and volumes gone from it.

        A bake that cannot be prepared or recorded leaves the bakes of that feature recorded before it
        in @p frame untrustworthy: those probes, or volumes, bake again, and weigh nothing until
        then. A second failure in a row of a feature gives it up, see hasGivenUpProbes() and
        hasGivenUpVolumes().

        @returns The number of probes baked plus the number of volumes with positions captured,
                 complete or not.
    */
    int bake (GpuFrame& frame, ForwardPass& forward, const Inputs& inputs);

    /** Lets a baker that gave up try again, as an explicit SceneRenderer::bakeProbes() does. */
    void clearFailures() noexcept;

    /** True after two bakes in a row of the probes or of the volumes failed: see hasGivenUpProbes() and hasGivenUpVolumes(). */
    bool hasGivenUp() const noexcept { return probesGivenUp || volumesGivenUp; }

    /** True after two bakes of the reflection probes in a row failed: they no longer bake or shade until clearFailures(). */
    bool hasGivenUpProbes() const noexcept { return probesGivenUp; }

    /** True after two bakes of the irradiance volumes in a row failed: they no longer bake or shade until clearFailures(). */
    bool hasGivenUpVolumes() const noexcept { return volumesGivenUp; }

    /** Makes every probe and volume bake again at the next bake(), into the tile or blocks it holds,
        and the capture shadows draw again: for frames the GPU failed to execute, or whose bakes
        failed while being recorded. Every probe and volume bakes again, not only the ones of the
        failed frame. */
    void forgetBakes() noexcept;

    //==============================================================================
    /** The reflection atlas, in ProbeRenderer::getRadianceFormat() with numMips mips, or nullptr
        while no probe is baked. */
    GpuTexture::Ptr getReflectionAtlas() const noexcept { return atlasTexture; }

    /** Packs each probe of @p drawList, in its order, for the raster probe lists: a probe without a
        baked tile, or whose bake is stale, packs an empty influence, which weighs 0 everywhere. */
    std::vector<GpuReflectionProbe> packReflectionProbes (const SceneDrawList& drawList) const;

    /** Packs the baked probes of @p drawList for the full tier, in the order the raster selection
        ranks them (SceneDrawList::selectReflectionProbes()): the smallest influence first, then the
        oldest bake id. At most maxProbesPerFrame are packed: the rest are left out, logged once.
    */
    std::vector<GpuReflectionProbe> packReflectionProbesByPriority (const SceneDrawList& drawList) const;

    //==============================================================================
    /** The irradiance atlas, in ProbeRenderer::getRadianceFormat(), or nullptr while no volume is baked. */
    GpuTexture::Ptr getIrradianceAtlas() const noexcept { return irradianceTexture; }

    /** The visibility atlas, rg16float, or nullptr while no volume is baked with visibility. */
    GpuTexture::Ptr getVisibilityAtlas() const noexcept { return visibilityTexture; }

    /** Packs each volume of @p drawList, in its order, for the raster probe lists: a volume without
        a baked block, or whose bake is stale, packs all zeros, which weighs 0 everywhere. */
    std::vector<GpuIrradianceVolume> packIrradianceVolumes (const SceneDrawList& drawList) const;

    /** Packs the baked volumes of @p drawList for the full tier, the smallest box first, then the
        oldest bake id. At most maxVolumesPerFrame are packed: the rest are left out, logged once. */
    std::vector<GpuIrradianceVolume> packIrradianceVolumesByPriority (const SceneDrawList& drawList) const;

    //==============================================================================
    /** Changes with every call that bakes, and so with every atlas reallocation. */
    uint64 getGeneration() const noexcept { return generation; }

    //==============================================================================
    /** @internal The number of probe bakes since the baker was created, for tests. */
    int getNumBakes() const noexcept { return numBakes; }

    /** @internal The number of probes holding an entry, for tests. */
    int getNumBakedProbes() const noexcept { return static_cast<int> (baked.size()); }

    /** @internal The number of irradiance volume bakes since the baker was created, for tests. */
    int getNumVolumeBakes() const noexcept { return numVolumeBakes; }

    /** @internal The number of irradiance volumes holding an entry, for tests. */
    int getNumBakedVolumes() const noexcept { return static_cast<int> (bakedVolumes.size()); }

    /** @internal The shadow renderer of the captures, or nullptr before the first call that bakes,
        for tests. Between calls that bake it holds no texture. */
    const ShadowRenderer* getCaptureShadowRenderer() const noexcept { return captureShadows.get(); }

    /** @internal True while a capture draw list and scratch targets are held, for tests. */
    bool holdsCaptureTargets() const noexcept { return scratch != nullptr || volumeScratch != nullptr || captureList != nullptr; }

    /** @internal Test hook: from the next bake() on, every reflection probe capture fails to record. */
    void failCapturesForTesting (bool shouldFail) noexcept { capturesFailForTesting = shouldFail; }

    /** @internal Test hook: from the next bake() on, every irradiance volume capture fails to record. */
    void failVolumeCapturesForTesting (bool shouldFail) noexcept { volumeCapturesFailForTesting = shouldFail; }

private:
    struct BakedProbe
    {
        uint64 bakeId = 0;        // The probe's ReflectionProbeNode::getBakeId()
        uint64 signature = 0;     // getSignature() when it baked
        Rectangle<int> tile;      // Its tile in the atlas, empty when the atlas is full
        bool stale = true;        // Bakes again at the next bake()
    };

    /** The capture resolve of one radiance format, kept once prepared: frames in flight may use its pipeline. */
    struct ResolvePass
    {
        GpuTextureFormat format;
        FullscreenPass pass;
        Result prepared = Result::ok();
    };

    struct BakedVolume
    {
        uint64 bakeId = 0;          // The volume's IrradianceVolumeNode::getBakeId()
        uint64 signature = 0;       // getSignature() when it baked
        Rectangle<int> shBlock;     // Its block in the irradiance atlas, empty when it did not fit
        Rectangle<int> visibilityBlock; // Its block in the visibility atlas, empty without visibility
        bool stale = true;          // Bakes at the next bake(), and weighs nothing until all its positions are captured
        int nextPosition = 0;       // The positions of the current bake captured so far, i + nx (j + ny k)
    };

    /** What capture() recorded, and how each feature fared. */
    struct CaptureResult
    {
        Result probes = Result::ok();
        Result volumes = Result::ok();
        int recordedProbes = 0;  // Probes captured
        int recordedVolumes = 0; // Volumes with positions captured, complete or not
    };

    const BakedProbe* findEntry (uint64 bakeId) const noexcept;
    const BakedVolume* findVolumeEntry (uint64 bakeId) const noexcept;
    bool needsBake (const SceneDrawList::ReflectionProbe& probe) const noexcept;
    bool needsBake (const SceneDrawList::IrradianceVolume& volume) const noexcept;
    void releaseVolumeBlocks (const BakedVolume& entry);
    void releaseCaptureTargets() noexcept;
    bool prepareAtlas();
    bool prepareVolumeAtlases();
    FullscreenPass* prepareResolve();
    ResolvePass* prepareDistance();
    void forgetProbeBakes() noexcept;
    void forgetVolumeBakes() noexcept;
    CaptureResult capture (GpuFrame& frame, ForwardPass& forward, const Inputs& inputs,
                           Span<const SceneDrawList::ReflectionProbe* const> pending,
                           Span<const SceneDrawList::IrradianceVolume* const> pendingVolumes);

    ProbeRenderer& probes;
    ProbeAtlas atlas { atlasWidth, maxAtlasHeight };
    GpuTexture::Ptr atlasTexture;
    uint32 atlasGeneration = 0;
    std::vector<BakedProbe> baked;
    std::unique_ptr<ShadowRenderer> captureShadows;
    std::unique_ptr<FrameResources> scratch;
    std::unique_ptr<SceneDrawList> captureList;
    std::vector<std::unique_ptr<ResolvePass>> resolvePasses;
    ViewData captureView;
    uint64 generation = 0;
    int numBakes = 0;
    ProbeAtlas irradianceAtlas { irradianceAtlasWidth, maxIrradianceAtlasHeight };
    ProbeAtlas visibilityAtlas { visibilityAtlasWidth, maxVisibilityAtlasHeight };
    GpuTexture::Ptr irradianceTexture;
    GpuTexture::Ptr visibilityTexture;
    uint32 irradianceGeneration = 0;
    uint32 visibilityGeneration = 0;
    std::vector<BakedVolume> bakedVolumes;
    std::unique_ptr<FrameResources> volumeScratch;  // The volume captures' targets, of another size than scratch's
    std::unique_ptr<ResolvePass> distancePass;      // The radial distance of the volume captures, kept once prepared
    int numVolumeBakes = 0;
    mutable bool volumeCapReported = false; // More than maxVolumesPerFrame volumes were packed, and logged once
    bool volumeAtlasFullReported = false;   // A volume found no block, and was logged once
    int probeFailures = 0;                // Probe bakes that failed in a row
    int volumeFailures = 0;               // Volume bakes that failed in a row
    bool probesGivenUp = false;           // Two probe failures in a row: no probe bakes until clearFailures()
    bool volumesGivenUp = false;          // Two volume failures in a row: no volume bakes until clearFailures()
    bool capturesFailForTesting = false;
    bool volumeCapturesFailForTesting = false;
    mutable bool capReported = false;     // More than maxProbesPerFrame probes were packed, and logged once
    bool atlasFullReported = false;       // A probe found no tile, and was logged once

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProbeBaker)
};

} // namespace yup
