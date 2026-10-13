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

//==============================================================================
/** Forward shading of draw items: the environment background, then opaque and blended surfaces.

    Bindings follow the set 0 (view) / set 1 (pass) / set 2 (draw and material) convention, so the
    per-frame sets are built once per pipeline change and only set 2 changes per draw. Set 0 holds
    the shared samplers and lookups of the LightingInputs: the environment at binding 6 with its
    sampler at 14, the lookup tables (LookupTables) at 7, read through linearClamp at binding 1, and
    shadowCompare at 4, and the reflection atlas at 8 in the variants reading the probes. Set 1 holds
    the LightingBlock at binding 0, and on the full tier the lights, the light lists of the clusters
    and the reflection probes at bindings 6, 7 and 8; raster frames with ambient occlusion the
    AmbientOcclusionPass output at 13 (LightingInputs::ambientOcclusion, at half resolution,
    upsampled bilinearly), which the full tier's blended surfaces skip. Raster draws read their own
    light list at set 2 binding 12, and in the local indirect variants their two reflection probes at
    13 (SceneDrawList::DrawItem::reflectionProbes). Set 3 holds the shadows: the sun cascades at binding 0, the point and spot
    light atlas at 2 and the ShadowBlock at 4, and for soft shadows the same two maps again at 1
    and 3, read raw by the blocker search.

    Pipelines are compiled lazily per color and depth format, sample count, velocity output, area
    lights, shadow filters, local indirect inputs and material layers, only for frames that draw: the full tier variant is the one with a velocity target, and draws
    blended items only. The area light variant (YUP_3D_AREA_LIGHTS) shades the frames where
    LightingInputs::areaLights is set; every variant declares the lookup tables, which the
    environment reads too. On both tiers the shadow filter variant (YUP_3D_SOFT_SHADOWS) shades the
    frames where LightingInputs::shadowFilters is set, and only it declares and binds the raw depth
    maps. It holds the soft shadows; on the full tier the frame also takes it when contact shadows
    are marched, which LightingPass alone does (blended items write no depth to march through).
    On raster, the local indirect variant (YUP_3D_LOCAL_INDIRECT) shades the frames with
    LightingInputs::ambientOcclusion, reflection probes (LightingInputs::hasReflectionProbes()),
    irradiance volumes (LightingInputs::hasIrradianceVolumes()) or materials with a clearcoat or a
    sheen (LightingInputs::materialLayers), and only it declares and binds them; its layers variant
    (YUP_3D_LAYERS) shades the frames drawing a layered material, and alone compiles the layers, so
    the default raster bundles carry no ambient occlusion, probe or layer code and the plain local
    indirect ones no layer code. Without
    ambient occlusion it binds white there, and without probes one empty probe list for the whole
    pass. Blended items skip the ambient occlusion (a white texture is bound before them), since it
    holds the opaque surfaces behind them, and keep their probes. If the variant fails to compile,
    the failure is logged once and the frames shade without ambient occlusion, probes and layers; if
    only the layers variant fails, they shade without the layers.
    The full tier variants always shade the layers. The clearcoat follows the interpolated normal.

    @internal
*/
class YUP_API ForwardPass
{
public:
    //==============================================================================
    /** What the color target receives. */
    enum class Output
    {
        ldrInline,     ///< Exposed, tone mapped and sRGB encoded, for rgba8unorm.
        hdrReversible, ///< Exposed and reversibly tone mapped (c / (1 + luma)), for rgba16float, over a transparent clear.
        hdrLinear      ///< Exposed radiance, for rgba16float, premultiplied by coverage alpha over a transparent clear.
    };

    /** The attachments of the pass. */
    struct Targets
    {
        GpuTexture::Ptr color;    ///< rgba8 / rgba16f, multisampled on raster.
        GpuTexture::Ptr resolve;  ///< Single-sampled resolve target, or nullptr. With one, color and depth are discarded once resolved.
        GpuTexture::Ptr depth;    ///< Depth, with the sample count of color.
        GpuTexture::Ptr velocity; ///< Full tier only (attachment 1), else nullptr.
    };

    //==============================================================================
    /** Creates a pass. GPU objects are created by prepare(). */
    ForwardPass();

    /** Destructor. */
    ~ForwardPass();

    //==============================================================================
    /** Creates the default textures, and compiles the pipelines for these targets,
        with the area light variant when @p areaLights is set (see LightingInputs::areaLights).

        Compiles once per target formats, sample count, velocity output and area lights,
        remembering a failure instead of retrying every frame.
    */
    Result prepare (const GpuDevice::Ptr& device, const Targets& targets, uint32_t sampleCount, bool areaLights);

    /** Draws @p opaque then @p blend items, after clearing the targets and drawing the visible
        environment when @p clear is set, or over the loaded targets otherwise.

        On the raster tier each item is lit by the lights its SceneDrawList::DrawItem::lightIndices
        pick from LightingInputs::packedLights, their intensities scaled by its
        SceneDrawList::DrawItem::lightWeights; on the full tier by the lights of the clusters, from
        LightingInputs::lights and LightingInputs::clusterLights. LightingInputs::areaLights picks
        the area light variant, and LightingInputs::shadowFilters the shadow filter variant, each
        compiled the first time it is needed. A shadow filter variant that fails to compile is
        logged once, and the frames shade hard shadows with the default variant.

        @param device       The device to draw with.
        @param frame        The frame to record into.
        @param view         The camera of the frame.
        @param lighting     What the surfaces are lit by.
        @param opaque       The opaque and masked items, writing depth. Raster only: must be empty with a velocity target.
        @param blend        The blended items, testing depth without writing it.
        @param targets      The attachments.
        @param output       What the color target receives.
        @param sampleCount  The sample count of the attachments.
        @param clear        The color to clear to, or empty to load the color, depth and velocity
                            drawn before (no environment is drawn then).

        @returns False if the pipelines for @p targets could not be prepared or the pass could not begin.
    */
    bool render (const GpuDevice::Ptr& device, GpuFrame& frame, const ViewData& view, const LightingInputs& lighting,
                 Span<const SceneDrawList::DrawItem> opaque, Span<const SceneDrawList::DrawItem> blend,
                 const Targets& targets, Output output, uint32_t sampleCount, std::optional<GpuColor> clear);

    /** True when the last render() shaded with a local indirect variant, for tests. */
    bool usesLocalIndirect() const noexcept { return usedLocalIndirect; }

    /** True when the last render() shaded with a layers variant, for tests. */
    bool usesMaterialLayers() const noexcept { return usedLayers; }

    /** @internal Test hook: the layers variants not compiled yet fail to compile, as if the device rejected them. */
    void failMaterialLayersForTesting() noexcept { layersFailForTesting = true; }

private:
    struct Variant;

    ResultValue<Variant*> prepareVariant (const GpuDevice::Ptr& device, const Targets& targets, uint32_t sampleCount, bool areaLights, bool shadowFilters, bool localIndirect, bool layers);
    Result createDefaults (const GpuDevice::Ptr& device);

    std::vector<std::unique_ptr<Variant>> variants;
    std::optional<Result> defaults;
    GeometryBindings geometry;
    GpuTexture::Ptr blackCube; // The environment without one
    GpuTexture::Ptr blackTexture; // The reflection atlas without probes, created by the first frame reading it
    GpuTarget::Ptr surface;
    bool shadowFiltersFailureReported = false; // A shadow filter variant failed to compile, and was logged once
    bool localIndirectFailureReported = false; // A local indirect variant failed to compile, and was logged once
    bool usedLocalIndirect = false;            // The last render() shaded with a local indirect variant
    bool layersFailureReported = false;        // A layers variant failed to compile, and was logged once
    bool layersFailForTesting = false;         // failMaterialLayersForTesting(): the layers variants not compiled yet fail
    bool usedLayers = false;                   // The last render() shaded with a layers variant

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ForwardPass)
};

} // namespace yup
