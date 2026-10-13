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
/** Bakes the reflection probes of a scene into the reflection atlas, and packs them for the
    shading passes.

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

    The capture shadow renderer, its pipelines and the resolve pipeline live as long as the baker,
    since frames still in flight may use them. A call that bakes nothing releases their textures:
    the capture shadow maps, the scratch targets and the capture draw list. The atlas is kept while
    a probe uses it, created again with every probe baked again when it grows, and released when no
    probe is left.

    A bake that cannot be prepared or recorded never fails the frame: every probe bakes again
    (forgetBakes()) and the frame shades without them meanwhile. After two failures in a row the
    baker gives up, logged once, until clearFailures().

    The bookkeeping holds one entry per probe, by bake id: the signature it baked with and its tile.
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

    //==============================================================================
    /** What a bake reads besides the ForwardPass drawing the captures. */
    struct Inputs
    {
        const Scene& scene;                              ///< The scene to capture: its tree, lights, ambient light and environment.
        const SceneDrawList& drawList;                   ///< The probes of the frame (SceneDrawList::getReflectionProbes()).
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

    /** The number of probes of @p drawList the next bake() bakes: 0 once the baker gave up. */
    int getNumPending (const SceneDrawList& drawList) const;

    /** Bakes the pending probes of Inputs::drawList into @p frame, and gives back the tiles of the
        probes gone from it.

        A bake that cannot be prepared or recorded leaves the bakes recorded before it in @p frame
        untrustworthy: every probe bakes again (forgetBakes()), and they weigh nothing until then.
        A second failure in a row gives up, see hasGivenUp().

        @returns The number of probes baked.
    */
    int bake (GpuFrame& frame, ForwardPass& forward, const Inputs& inputs);

    /** Lets a baker that gave up try again, as an explicit SceneRenderer::bakeProbes() does. */
    void clearFailures() noexcept;

    /** True after two bakes in a row failed: no probe bakes or shades until clearFailures(). */
    bool hasGivenUp() const noexcept { return givenUp; }

    /** Makes every probe bake again at the next bake(), into the tile it holds, and the capture
        shadows draw again: for frames the GPU failed to execute, or whose bakes failed while being
        recorded. Every probe bakes again, not only the ones of the failed frame. */
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

    /** Changes with every call that bakes, and so with every atlas reallocation. */
    uint64 getGeneration() const noexcept { return generation; }

    //==============================================================================
    /** @internal The number of probe bakes since the baker was created, for tests. */
    int getNumBakes() const noexcept { return numBakes; }

    /** @internal The number of probes holding an entry, for tests. */
    int getNumBakedProbes() const noexcept { return static_cast<int> (baked.size()); }

    /** @internal The shadow renderer of the captures, or nullptr before the first call that bakes,
        for tests. Between calls that bake it holds no texture. */
    const ShadowRenderer* getCaptureShadowRenderer() const noexcept { return captureShadows.get(); }

    /** @internal True while a capture draw list and scratch targets are held, for tests. */
    bool holdsCaptureTargets() const noexcept { return scratch != nullptr || captureList != nullptr; }

    /** @internal Test hook: from the next bake() on, every capture fails to record. */
    void failCapturesForTesting (bool shouldFail) noexcept { capturesFailForTesting = shouldFail; }

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

    const BakedProbe* findEntry (uint64 bakeId) const noexcept;
    bool needsBake (const SceneDrawList::ReflectionProbe& probe) const noexcept;
    void releaseCaptureTargets() noexcept;
    bool prepareAtlas();
    FullscreenPass* prepareResolve();
    ResultValue<int> capture (GpuFrame& frame, ForwardPass& forward, const Inputs& inputs, Span<const SceneDrawList::ReflectionProbe* const> pending);

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
    int consecutiveFailures = 0;          // Bakes that failed in a row
    bool givenUp = false;                 // Two failures in a row: nothing bakes until clearFailures()
    bool capturesFailForTesting = false;
    mutable bool capReported = false;     // More than maxProbesPerFrame probes were packed, and logged once
    bool atlasFullReported = false;       // A probe found no tile, and was logged once

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProbeBaker)
};

} // namespace yup
