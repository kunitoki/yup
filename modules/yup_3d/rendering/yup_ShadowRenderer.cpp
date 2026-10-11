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

// Regenerate after changing shaders/yup_ShadowPass.vert / .frag or the libraries they include with: just yup3d_shaders
constexpr uint8_t shadowPassShaderBundle[] = {
#include "../shaders/yup_ShadowPass.inc"
};
constexpr uint8_t shadowPassMaskedShaderBundle[] = {
#include "../shaders/yup_ShadowPass_masked.inc"
};

// Receivers compare against a depth offset towards the light by this many texels of the cascade,
// times LightNode::shadowBias, and are moved along their normal by as many where the light grazes them
constexpr float shadowBiasTexels = 1.5f;

// The share of each cascade's depth range over which it hands over to the next one
constexpr float cascadeBlendBand = 0.1f;

// filterInfo.w flags: the tier marches contact shadows; the shadow noise rotates with the frame, for
// temporal anti-aliasing to average
constexpr float contactShadowsFlag = 1.0f;
constexpr float temporalNoiseFlag = 2.0f;

// Soft shadow taps per lookup, each of the filter and the blocker search: fewer per frame where
// temporal anti-aliasing averages their rotation over many frames, or on raster
constexpr float softShadowTaps = 8.0f;
constexpr float stillSoftShadowTaps = 16.0f;

// The point and spot light atlas: its largest side, its smallest tile, and the shadow records the
// raster tier gives to point and spot lights
constexpr int maxAtlasSize = 4096;
constexpr int minAtlasTile = 64;
constexpr int maxRasterLocalRecords = 16;

// The nearest plane of a point or spot light view, for lights without a radius
constexpr float minLocalNear = 0.01f;

int findShadowedSun (Span<const SceneDrawList::Light> lights) noexcept
{
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const auto& node = *lights[i].node;
        if (node.castsShadows && node.type == LightNode::Type::directional && ! GpuSceneData::isDark (node))
            return static_cast<int> (i);
    }

    return -1;
}

/** The view of face @p face of a point light at @p position: +X, -X, +Y, -Y, +Z, -Z, with the up
    vectors of the cube map convention. */
Matrix4 getCubeFaceView (const Vector3<float>& position, int face) noexcept
{
    static const std::array<std::pair<Vector3<float>, Vector3<float>>, 6> faces { {
        { { 1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
        { { -1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } },
        { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
        { { 0.0f, 0.0f, 1.0f }, { 0.0f, -1.0f, 0.0f } },
        { { 0.0f, 0.0f, -1.0f }, { 0.0f, -1.0f, 0.0f } },
    } };

    const auto& [forward, up] = faces[static_cast<size_t> (face)];
    return Matrix4::lookAt (position, position + forward, up);
}

/** The side of the point and spot light atlas: the largest power of two within the shadow
    resolution (twice it on full), so the atlas never exceeds the setting, and at most maxAtlasSize. */
int getLocalAtlasSize (const RenderCapabilities& capabilities, const RenderSettings& settings) noexcept
{
    const auto resolution = RenderCapabilities::getShadowResolution (settings);
    const auto full = capabilities.getTier() == RenderCapabilities::Tier::full;
    return jmin (nextPowerOfTwo ((full ? 2 * resolution : resolution) + 1) / 2, maxAtlasSize);
}

/** The view of a spot light at @p position shining along @p direction. */
Matrix4 getSpotView (const Vector3<float>& position, const Vector3<float>& direction) noexcept
{
    const auto up = std::abs (direction.getY()) < 0.99f ? Vector3<float> (0.0f, 1.0f, 0.0f) : Vector3<float> (1.0f, 0.0f, 0.0f);
    return Matrix4::lookAt (position, position + direction, up);
}

/** True unless every corner of @p box lies beyond one plane of the clip volume of @p viewProjection
    (depth in [0, 1]). Tested in homogeneous clip space, so corners behind a perspective view stay
    on their side of each plane.
*/
bool boxMeetsView (const Matrix4& viewProjection, const BoundingBox& box) noexcept
{
    const auto* m = viewProjection.getData();
    auto outsideAll = 0x3f;

    for (int corner = 0; corner < 8 && outsideAll != 0; ++corner)
    {
        const auto p = box.getCorner (corner);
        const auto clip = [&] (int row)
        {
            return m[row] * p.getX() + m[row + 4] * p.getY() + m[row + 8] * p.getZ() + m[row + 12];
        };

        const auto x = clip (0), y = clip (1), z = clip (2), w = clip (3);
        outsideAll &= (x < -w ? 1 : 0) | (x > w ? 2 : 0) | (y < -w ? 4 : 0) | (y > w ? 8 : 0) | (z < 0.0f ? 16 : 0) | (z > w ? 32 : 0);
    }

    return outsideAll == 0;
}

/** The view block that maps the plane z = 0 of the unit quad to the far plane (clip z = w), for
    clearing a tile to depth 1 by drawing over it. */
ViewData::Block makeTileClearBlock() noexcept
{
    ViewData::Block block {};
    std::copy_n (Matrix4::identity().getData(), 16, block.view);
    std::copy_n (Matrix4::identity().getData(), 16, block.projection);
    block.projection[10] = 0.0f;
    block.projection[14] = 1.0f;
    std::copy_n (block.projection, 16, block.viewProjection);
    return block;
}

} // namespace

//==============================================================================
/** One tile of the atlas and the light view rendered into it, with the casters it draws. */
struct ShadowRenderer::LocalView
{
    int lightIndex = 0;
    int x = 0;
    int y = 0;
    int size = 0;
    Matrix4 viewProjection;
    std::vector<std::pair<size_t, uint64>> casters; // an index into the opaque items, and its Inputs::casterSignatures entry

    /** True when @p other drew the depth this view would draw, into the same tile. */
    bool holdsSameDepth (const LocalView& other) const noexcept
    {
        return x == other.x && y == other.y && size == other.size && viewProjection == other.viewProjection && casters == other.casters;
    }
};

/** What one sun cascade holds: its light transform, and the casters reaching it with their signatures. */
struct ShadowRenderer::CascadeState
{
    Matrix4 viewProjection;
    std::vector<std::pair<size_t, uint64>> casters; // an index into the opaque items, and its Inputs::casterSignatures entry

    bool operator== (const CascadeState&) const = default;
};

/** What the shadow textures hold: each map is drawn again only when its part changes. */
struct ShadowRenderer::RenderedState
{
    uint64 globalSignature = 0;
    int resolution = 0;
    int numCascades = 0;
    std::array<CascadeState, ShadowCascades::maxCascades> cascades;
    int atlasSize = 0;
    std::vector<LocalView> localViews;
};

//==============================================================================
ShadowRenderer::ShadowRenderer() = default;

ShadowRenderer::~ShadowRenderer() = default;

//==============================================================================
Result ShadowRenderer::prepare (const GpuDevice::Ptr& device)
{
    if (preparation)
        return *preparation;

    preparation = preparePipelines (device);

    // Remembered: the frames render unshadowed from now on, see render()
    if (preparation->failed())
    {
        opaquePipeline = nullptr;
        maskedPipeline = nullptr;
        tileClearPipeline = nullptr;
        YUP_DBG ("ShadowRenderer: " << preparation->getErrorMessage() << ", the frames render without shadows");
    }

    return *preparation;
}

Result ShadowRenderer::preparePipelines (const GpuDevice::Ptr& device)
{
    if (pipelinesFailForTesting)
        return Result::fail ("Unable to compile the shadow pipeline");

    if (auto result = geometry.prepare (device); result.failed())
        return result;

    auto opaqueBundle = ShaderBundle::loadFromData (shadowPassShaderBundle, sizeof (shadowPassShaderBundle));
    auto maskedBundle = ShaderBundle::loadFromData (shadowPassMaskedShaderBundle, sizeof (shadowPassMaskedShaderBundle));

    for (const auto* bundle : { &opaqueBundle, &maskedBundle })
        if (bundle->failed())
            return Result::fail ("Unable to load the shadow shader bundle: " + bundle->getErrorMessage());

    // Depth only, pushed away from the light by the slope of each triangle (the body of a model can
    // mix windings, so nothing is culled); the tile clears are written over the whole tile instead
    const auto compile = [&] (const ShaderBundle& bundle, bool tileClear, GpuPipeline::Ptr& pipeline)
    {
        GpuPipelineOptions options;
        GeometryBindings::setPipelineGeometry (options, 0);
        options.cullMode = GpuCullMode::none;
        options.depthStencil.enabled = true;
        options.depthStencil.format = GpuTextureFormat::depth32float;
        options.depthStencil.depthCompare = tileClear ? GpuCompareFunction::always : GpuCompareFunction::lessEqual;
        options.depthStencil.depthWriteEnabled = true;
        options.depthStencil.depthBias = tileClear ? 0 : 1;
        options.depthStencil.depthBiasSlopeScale = tileClear ? 0.0f : 1.5f;

        auto compiled = GpuPipeline::compileFromBundle (device, bundle, options);
        if (compiled.failed())
            return Result::fail ("Unable to compile the shadow pipeline: " + compiled.getErrorMessage());

        pipeline = compiled.getValue();
        return Result::ok();
    };

    if (auto result = compile (opaqueBundle.getReference(), false, opaquePipeline); result.failed())
        return result;

    if (auto result = compile (maskedBundle.getReference(), false, maskedPipeline); result.failed())
        return result;

    if (auto result = compile (opaqueBundle.getReference(), true, tileClearPipeline); result.failed())
        return result;

    // A unit quad in the plane z = 0, drawn over a tile through makeTileClearBlock()
    tileClearMaterial = new Material();
    tileClearQuad = new Mesh ("SceneRenderer shadow tile clear");

    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -1.0f, -1.0f, 0.0f };
    vertices[1].position = { 1.0f, -1.0f, 0.0f };
    vertices[2].position = { 1.0f, 1.0f, 0.0f };
    vertices[3].position = { -1.0f, 1.0f, 0.0f };

    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    tileClearQuad->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, tileClearMaterial);
    return Result::ok();
}

GpuTexture::Ptr ShadowRenderer::getCascadeTexture() const noexcept
{
    return cascades != nullptr ? cascades : cascadePlaceholder.texture;
}

GpuTexture::Ptr ShadowRenderer::getAtlasTexture() const noexcept
{
    return atlas != nullptr ? atlas : atlasPlaceholder.texture;
}

int ShadowRenderer::getTileSize (int lightIndex) const noexcept
{
    if (atlas == nullptr || atlasLayout == nullptr || ! isPositiveAndBelow (lightIndex, static_cast<int> (recordIndices.size())))
        return 0;

    // Contact shadow only records follow the views, and have no tile
    const auto record = recordIndices[static_cast<size_t> (lightIndex)];
    if (record < 0 || record >= firstContactOnlyRecord)
        return 0;

    const auto* allocation = atlasLayout->findAllocation (lightIndex);
    return allocation != nullptr ? allocation->size : 0;
}

int ShadowRenderer::getNumCascadeRenders (int cascade) const noexcept
{
    return isPositiveAndBelow (cascade, ShadowCascades::maxCascades) ? numCascadeRenders[static_cast<size_t> (cascade)] : 0;
}

void ShadowRenderer::invalidate() noexcept
{
    rendered.reset();
    cascadePlaceholder.cleared = false;
    atlasPlaceholder.cleared = false;
}

//==============================================================================
Result ShadowRenderer::preparePlaceholder (const GpuDevice::Ptr& device, GpuFrame& frame, Placeholder& placeholder, bool array)
{
    // Stands in for a shadow map no light needs: the shaders never read it, since no light then has
    // a record in it, but every declared texture must be bound with its type
    if (placeholder.texture == nullptr)
    {
        GpuTextureDesc desc (1, 1, GpuTextureFormat::depth32float, true);
        desc.type = array ? GpuTextureType::array2D : GpuTextureType::texture2D;
        desc.label = "SceneRenderer shadow placeholder";

        placeholder.texture = GpuTexture::create (device, desc);
        placeholder.cleared = false;

        if (placeholder.texture == nullptr)
            return Result::fail ("Unable to create the shadow placeholder");
    }

    if (placeholder.cleared)
        return Result::ok();

    auto target = GpuTarget::createFromTexture (device, placeholder.texture, GpuTextureViewDesc (0, 0));
    if (target == nullptr)
        return Result::fail ("Unable to clear the shadow placeholder");

    auto pass = target->beginRenderPass (frame, GpuDepthStencilOptions (1.0f));
    if (! pass.isValid() || ! pass.finish())
        return Result::fail ("Unable to clear the shadow placeholder");

    placeholder.cleared = true;
    return Result::ok();
}

bool ShadowRenderer::prepareCascades (const GpuDevice::Ptr& device, int resolution, int numCascades)
{
    const auto size = static_cast<uint32_t> (resolution);
    const auto layers = static_cast<uint32_t> (numCascades);

    // The scene fit draws into the first layer of the cascades it replaces, so a camera crossing
    // between the two fits allocates nothing
    const auto hasLayers = layerTargets.size() == layers || (sceneFitted && ! layerTargets.empty());
    if (cascades != nullptr && cascades->getWidth() == size && hasLayers)
        return true;

    // A resolution the device could not create is not asked for again until the settings change,
    // whatever the layers (the scene fit's one too, so the sun's shadow does not come and go), and
    // what the other maps hold is kept: render() releases a texture of another size
    if (resolution == failedCascadeResolution)
        return false;

    rendered.reset();
    cascades = nullptr;
    layerTargets.clear();

    GpuTextureDesc desc (size, size, GpuTextureFormat::depth32float, true);
    desc.type = GpuTextureType::array2D;
    desc.depthOrArrayLayers = layers;
    desc.label = "SceneRenderer sun cascades";

    cascades = failTextures ? nullptr : GpuTexture::create (device, desc);

    for (uint32_t layer = 0; cascades != nullptr && layer < layers; ++layer)
    {
        auto target = GpuTarget::createFromTexture (device, cascades, GpuTextureViewDesc (0, layer));
        if (target == nullptr)
            cascades = nullptr;
        else
            layerTargets.push_back (std::move (target));
    }

    if (cascades != nullptr)
        return true;

    layerTargets.clear();
    failedCascadeResolution = resolution;
    YUP_DBG ("ShadowRenderer: unable to create " << numCascades << " sun cascades of " << resolution << " texels, the sun casts no shadow");
    return false;
}

bool ShadowRenderer::prepareAtlas (const GpuDevice::Ptr& device, int size)
{
    if (atlas != nullptr && atlas->getWidth() == static_cast<uint32_t> (size))
        return true;

    // A size the device could not create is not asked for again until the settings change, and
    // what the cascades hold is kept: render() releases an atlas of another size
    if (size == failedAtlasSize)
        return false;

    rendered.reset();
    atlas = nullptr;
    atlasTarget = nullptr;

    GpuTextureDesc desc (static_cast<uint32_t> (size), static_cast<uint32_t> (size), GpuTextureFormat::depth32float, true);
    desc.label = "SceneRenderer shadow atlas";

    atlas = failTextures ? nullptr : GpuTexture::create (device, desc);
    if (atlas != nullptr)
        atlasTarget = GpuTarget::createFromTexture (device, atlas, GpuTextureViewDesc (0, 0));

    if (atlasTarget != nullptr)
        return true;

    atlas = nullptr;
    failedAtlasSize = size;
    YUP_DBG ("ShadowRenderer: unable to create a shadow atlas of " << size << " texels, point and spot lights cast no shadow");
    return false;
}

//==============================================================================
int ShadowRenderer::planLocalShadows (const Inputs& inputs, bool withAtlas, int firstRecord, GpuShadowData& out, std::vector<LocalView>& views)
{
    const auto full = inputs.capabilities.getTier() == RenderCapabilities::Tier::full;
    const auto atlasSize = getLocalAtlasSize (inputs.capabilities, inputs.settings);
    const auto budget = jmin (GpuShadowData::maxRecords - firstRecord, full ? GpuShadowData::maxRecords : maxRasterLocalRecords);

    if (halfTileLayout == nullptr || halfTileLayout->getAtlasSize() != atlasSize)
    {
        halfTileLayout = std::make_unique<ShadowAtlas> (atlasSize, minAtlasTile, atlasSize / 2);
        quarterTileLayout = std::make_unique<ShadowAtlas> (atlasSize, minAtlasTile, atlasSize / 4);
    }

    // The tile sizes follow the screen size of each light's influence: an orthographic camera
    // (w = 1) sees every distance at the same scale
    const auto* projection = inputs.view.getProjection().getData();
    const auto orthographic = projection[15] == 1.0f;
    const auto yFov = 2.0f * std::atan (1.0f / projection[5]);
    const auto halfHeight = orthographic ? 1.0f / projection[5] : 0.0f;
    const auto viewportHeight = static_cast<int> (inputs.view.getBlock().viewport[1]);
    const auto camera = inputs.view.getCameraPosition();

    const auto lights = inputs.drawList.getLights();

    receivers.clear();
    requests.clear();

    for (size_t i = 0; withAtlas && i < lights.size(); ++i)
    {
        const auto& light = lights[i];
        const auto request = halfTileLayout->makeRequest (*light.node, static_cast<int> (i), (light.position - camera).length(), yFov, viewportHeight, halfHeight);
        if (! request)
            continue;

        // Gathered for the first light asking for tiles: frames without one never pay for them
        if (receivers.empty())
            for (const auto items : { inputs.drawList.getOpaqueItems(), inputs.drawList.getBlendItems() })
                for (const auto& item : items)
                    receivers.push_back (item.primitive->bounds.transformedBy (item.world));

        // A light lighting no receiver shadows nothing
        const auto range = GpuSceneData::getEffectiveRange (*light.node);
        const auto isSpot = light.node->type == LightNode::Type::spot;
        const auto cosOuter = std::cos (light.node->outerConeAngle);

        const auto reachesReceiver = std::any_of (receivers.begin(), receivers.end(), [&] (const BoundingBox& box)
        {
            return GpuSceneData::lightReachesBox (box, light.position, light.direction, range, isSpot, cosOuter);
        });

        if (reachesReceiver)
            requests.push_back (*request);
    }

    // Tiles up to half the atlas side, unless a light would get a smaller tile than with tiles up
    // to a quarter of it (which fit more lights): then those
    halfTileLayout->allocate (requests);
    quarterTileLayout->allocate (requests);

    const auto quarterAllocations = quarterTileLayout->getAllocations();
    const auto halfTilesFit = std::all_of (quarterAllocations.begin(), quarterAllocations.end(), [this] (const ShadowAtlas::Allocation& quarter)
    {
        const auto* half = halfTileLayout->findAllocation (quarter.lightIndex);
        return half != nullptr && half->size >= quarter.size;
    });

    atlasLayout = halfTilesFit ? halfTileLayout.get() : quarterTileLayout.get();

    const auto tiles = atlasLayout->getTiles();
    const auto texel = 1.0f / static_cast<float> (atlasSize);
    auto used = 0;
    size_t numViews = 0;

    for (const auto& allocation : atlasLayout->getAllocations())
    {
        // Whole lights only, from the most important: the first that does not fit ends the list
        if (used + allocation.faces > budget)
            break;

        const auto& light = lights[static_cast<size_t> (allocation.lightIndex)];
        const auto& node = *light.node;
        const auto nearPlane = jmax (minLocalNear, node.radius);
        const auto farPlane = GpuSceneData::getEffectiveRange (node);

        // A light inside its own emitter has nothing to shadow
        if (! (farPlane > nearPlane))
            continue;

        const auto cube = allocation.faces == 6;
        const auto guard = ShadowAtlas::getGuardAngle (allocation.size);
        const auto fov = (cube ? MathConstants<float>::halfPi : 2.0f * node.outerConeAngle) + guard;
        const auto lightProjection = Matrix4::perspective (fov, 1.0f, nearPlane, farPlane);
        const auto isSpot = node.type == LightNode::Type::spot;
        const auto cosOuter = std::cos (node.outerConeAngle);

        // The world size of a texel one unit of depth away from the light
        const auto texelPerDepth = 2.0f * std::tan (fov * 0.5f) / static_cast<float> (allocation.size);

        recordIndices[static_cast<size_t> (allocation.lightIndex)] = firstRecord + used;

        for (int face = 0; face < allocation.faces; ++face)
        {
            const auto& tile = tiles[static_cast<size_t> (allocation.firstTile + face)];
            const auto lightView = cube ? getCubeFaceView (light.position, face) : getSpotView (light.position, light.direction);
            const auto viewProjection = lightView.followedBy (lightProjection);

            auto& record = out.records[firstRecord + used + face];
            std::copy_n (viewProjection.getData(), 16, record.viewProjection);
            record.atlasRect[0] = static_cast<float> (tile.x) * texel;
            record.atlasRect[1] = static_cast<float> (tile.y) * texel;
            record.atlasRect[2] = static_cast<float> (tile.size) * texel;
            record.atlasRect[3] = texel;
            record.params[0] = node.shadowBias * shadowBiasTexels * texelPerDepth;
            record.params[1] = shadowBiasTexels * texelPerDepth;
            record.params[2] = nearPlane;
            record.params[3] = farPlane;
            record.contact[2] = static_cast<float> (allocation.faces);

            // Reused in place, so the caster lists keep their storage from frame to frame
            if (numViews == views.size())
                views.emplace_back();

            auto& view = views[numViews++];
            view.lightIndex = allocation.lightIndex;
            view.x = tile.x;
            view.y = tile.y;
            view.size = tile.size;
            view.viewProjection = viewProjection;
            view.casters.clear();

            // A view draws the casters within the light's reach and its own frustum
            for (size_t c = 0; c < casterBounds.size(); ++c)
            {
                const auto& bounds = casterBounds[c];
                if (GpuSceneData::lightReachesBox (bounds, light.position, light.direction, farPlane, isSpot, cosOuter) && boxMeetsView (viewProjection, bounds))
                    view.casters.emplace_back (c, c < inputs.casterSignatures.size() ? inputs.casterSignatures[c] : 0);
            }
        }

        used += allocation.faces;
    }

    views.resize (numViews);
    firstContactOnlyRecord = firstRecord + used;

    // Contact shadows (full tier): each light asking for them carries their settings in its first
    // record, the sun's being its first cascade. A light without a shadow map takes one record
    // without a map (atlasRect size 0) from the same budget, from the most important as the atlas
    // serves them: directional lights, then larger tiles, then lower light indices. The tile sizes
    // rank with the quarter cap whichever layout the frame uses, so a switch between the layouts,
    // or an atlas planned without (no requests), does not reorder the lights
    if (! inputs.capabilities.isEnabled (RenderCapabilities::Feature::contactShadows, inputs.settings))
        return used;

    contactLights.clear();
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const auto& node = *lights[i].node;
        if (! node.contactShadows || GpuSceneData::isDark (node))
            continue;

        const auto rank = node.type == LightNode::Type::directional
                            ? std::numeric_limits<int>::max()
                            : quarterTileLayout->getLightTileSize (node, (lights[i].position - camera).length(), yFov, viewportHeight, halfHeight);

        contactLights.emplace_back (rank, i);
    }

    std::sort (contactLights.begin(), contactLights.end(), [] (const auto& a, const auto& b)
    {
        return a.first != b.first ? a.first > b.first : a.second < b.second;
    });

    for (const auto& [rank, i] : contactLights)
    {
        const auto& node = *lights[i].node;
        auto& index = recordIndices[i];

        if (index < 0)
        {
            if (used >= budget)
                break;

            index = firstRecord + used++;
        }

        auto& record = out.records[index];
        record.contact[0] = jmax (0.0f, node.contactShadowDistance);
        record.contact[1] = jmax (0.0f, node.contactShadowThickness);
    }

    return used;
}

//==============================================================================
void ShadowRenderer::drawCasters (GpuRenderPass& pass, const GpuDevice::Ptr& device, const SceneDrawList& drawList, const Matrix4& viewProjection, Span<const std::pair<size_t, uint64>> casters)
{
    // lib/geometry.glsl transforms by projection * view: the whole light transform goes in the projection
    ViewData::Block block {};
    std::copy_n (Matrix4::identity().getData(), 16, block.view);
    std::copy_n (viewProjection.getData(), 16, block.projection);
    std::copy_n (viewProjection.getData(), 16, block.viewProjection);
    pass.setUniformBuffer (0, 0, &block, sizeof (block));

    GpuPipeline* currentPipeline = nullptr;
    const auto items = drawList.getOpaqueItems();

    for (const auto& caster : casters)
    {
        const auto& item = items[caster.first];
        const auto& pipeline = item.material->alphaMode == Material::AlphaMode::mask ? maskedPipeline : opaquePipeline;
        if (pipeline.get() != currentPipeline)
        {
            pass.setPipeline (pipeline);
            currentPipeline = pipeline.get();
        }

        geometry.draw (pass, device, item, 0);
        ++numCasterDraws;
    }
}

bool ShadowRenderer::clearTile (GpuRenderPass& pass, const GpuDevice::Ptr& device)
{
    // The quad covers the viewport of the tile, and the scissor keeps it inside
    const auto block = makeTileClearBlock();
    pass.setUniformBuffer (0, 0, &block, sizeof (block));
    pass.setPipeline (tileClearPipeline);

    SceneDrawList::DrawItem item;
    item.mesh = tileClearQuad.get();
    item.primitive = &tileClearQuad->getPrimitive (0);
    item.material = tileClearMaterial.get();
    return geometry.draw (pass, device, item, 0);
}

Result ShadowRenderer::renderCascade (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList, int layer, const CascadeState& cascade)
{
    auto pass = layerTargets[static_cast<size_t> (layer)]->beginRenderPass (frame, GpuDepthStencilOptions (1.0f));
    if (! pass.isValid())
        return Result::fail ("Unable to begin the shadow pass");

    drawCasters (pass, device, drawList, cascade.viewProjection, cascade.casters);

    if (! pass.finish())
        return Result::fail ("Unable to record the shadow pass");

    return Result::ok();
}

Result ShadowRenderer::renderAtlas (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList, Span<const LocalView> views, const std::vector<bool>& redraw, bool keepOtherTiles)
{
    // One pass draws every tile through its viewport and scissor. A load clear covers the whole
    // attachment, not a tile: when the other tiles are kept, each tile drawn again is cleared alone
    GpuDepthStencilOptions options (1.0f);
    if (keepOtherTiles)
        options.depthLoadOp = GpuLoadOp::load;

    auto pass = atlasTarget->beginRenderPass (frame, options);
    if (! pass.isValid())
        return Result::fail ("Unable to begin the shadow atlas pass");

    for (size_t i = 0; i < views.size(); ++i)
    {
        if (! redraw[i])
            continue;

        const auto& view = views[i];
        pass.setViewport (static_cast<float> (view.x), static_cast<float> (view.y), static_cast<float> (view.size), static_cast<float> (view.size));
        pass.setScissorRect (view.x, view.y, view.size, view.size);

        // A tile left uncleared would keep the nearest of its old and new depths
        if (keepOtherTiles && ! clearTile (pass, device))
            return Result::fail ("Unable to clear a shadow atlas tile");

        drawCasters (pass, device, drawList, view.viewProjection, view.casters);
    }

    if (! pass.finish())
        return Result::fail ("Unable to record the shadow atlas pass");

    return Result::ok();
}

//==============================================================================
Result ShadowRenderer::render (const GpuDevice::Ptr& device, GpuFrame& frame, const Inputs& inputs)
{
    auto& out = shadowData;
    out = {};
    numRecords = 0;
    firstContactOnlyRecord = GpuShadowData::maxRecords;
    shadowFilters = false;
    failTextures = std::exchange (texturesFailForTesting, false);

    const auto lights = inputs.drawList.getLights();
    recordIndices.assign (lights.size(), -1);

    // Without the comparison sampler, or the pipelines prepare() could not create, nothing is shadowed
    const auto enabled = inputs.enabled && opaquePipeline != nullptr && maskedPipeline != nullptr && tileClearPipeline != nullptr;
    const auto sun = enabled ? findShadowedSun (lights) : -1;
    const auto resolution = RenderCapabilities::getShadowResolution (inputs.settings);

    // The world bounds of the casters, which every map culls: frames without a shadow casting light never pay for them
    casterBounds.clear();
    if (enabled && std::any_of (lights.begin(), lights.end(), [] (const SceneDrawList::Light& light) { return light.node->castsShadows; }))
        for (const auto& item : inputs.drawList.getOpaqueItems())
            casterBounds.push_back (item.primitive->bounds.transformedBy (item.world));

    ShadowCascades::Inputs cascadeInputs;
    std::array<ShadowCascades::Cascade, ShadowCascades::maxCascades> fitted;
    auto numCascades = 0;

    if (sun >= 0)
    {
        cascadeInputs.setView (inputs.view);
        cascadeInputs.lightDirection = lights[static_cast<size_t> (sun)].direction;
        cascadeInputs.resolution = resolution;

        // An orthographic camera sees every depth at the same scale: one cascade fits it
        const auto orthographic = cascadeInputs.projection.getData()[15] == 1.0f;
        cascadeInputs.count = orthographic ? 1 : inputs.capabilities.getCascadeCount (inputs.settings);

        for (const auto& bounds : casterBounds)
        {
            cascadeInputs.casterBounds.expand (bounds);
            cascadeInputs.receiverBounds.expand (bounds);
        }

        for (const auto& item : inputs.drawList.getBlendItems())
            cascadeInputs.receiverBounds.expand (item.primitive->bounds.transformedBy (item.world));

        numCascades = ShadowCascades::build (cascadeInputs, fitted);

        // A compact scene gets one map over all of it, which does not follow the camera
        ShadowCascades::Cascade sceneFit;
        sceneFitted = numCascades > 0
                   && ShadowCascades::fitScene (cascadeInputs, sceneFit)
                   && ShadowCascades::prefersSceneFit (sceneFit, fitted[0], sceneFitted);

        if (sceneFitted)
        {
            fitted[0] = sceneFit;
            numCascades = 1;
        }
    }
    else
    {
        sceneFitted = false;
    }

    // Cascades the device cannot create leave the sun unshadowed. The textures no light needs are
    // released, and placeholders stand in for them
    if (numCascades > 0 && ! prepareCascades (device, resolution, numCascades))
    {
        numCascades = 0;
        sceneFitted = false;
    }

    if (numCascades > 0)
    {
        recordIndices[static_cast<size_t> (sun)] = 0;
    }
    else
    {
        cascades = nullptr;
        layerTargets.clear();

        if (auto result = preparePlaceholder (device, frame, cascadePlaceholder, true); result.failed())
            return result;
    }

    // Built in place, so the caster lists keep their storage from frame to frame
    if (pending == nullptr)
        pending = std::make_unique<RenderedState>();

    auto& state = *pending;
    state.globalSignature = inputs.globalSignature;
    state.resolution = resolution;
    state.numCascades = numCascades;

    if (! enabled)
        state.localViews.clear();

    // An atlas size the device could not create is planned without from the start
    const auto withAtlas = getLocalAtlasSize (inputs.capabilities, inputs.settings) != failedAtlasSize;
    auto numLocalRecords = enabled ? planLocalShadows (inputs, withAtlas, numCascades, out, state.localViews) : 0;

    // An atlas the device cannot create leaves its lights unshadowed: planned again without it,
    // they keep their contact shadows
    if (! state.localViews.empty() && ! prepareAtlas (device, atlasLayout->getAtlasSize()))
    {
        for (size_t i = 0; i < recordIndices.size(); ++i)
            if (numCascades == 0 || static_cast<int> (i) != sun)
                recordIndices[i] = -1;

        std::fill (out.records + numCascades, out.records + numCascades + numLocalRecords, GpuShadowRecord {});
        numLocalRecords = planLocalShadows (inputs, false, numCascades, out, state.localViews);
    }

    if (state.localViews.empty())
    {
        atlas = nullptr;
        atlasTarget = nullptr;

        if (auto result = preparePlaceholder (device, frame, atlasPlaceholder, false); result.failed())
            return result;
    }

    state.atlasSize = getAtlasSize();
    numRecords = numCascades + numLocalRecords;

    if (numCascades > 0)
    {
        const auto& node = *lights[static_cast<size_t> (sun)].node;

        for (int c = 0; c < numCascades; ++c)
        {
            const auto& cascade = fitted[static_cast<size_t> (c)];
            auto& record = out.records[c];

            // A constant world distance becomes a fraction of the cascade's depth range, which changes with the fit
            const auto texel = cascade.texelWorldSize;
            std::copy_n (cascade.viewProjection.getData(), 16, record.viewProjection);
            record.atlasRect[0] = static_cast<float> (c);
            record.atlasRect[2] = 1.0f;
            record.atlasRect[3] = 1.0f / static_cast<float> (resolution);
            record.params[0] = node.shadowBias * shadowBiasTexels * texel / cascade.depthRange;
            record.params[1] = shadowBiasTexels * texel;
            record.params[3] = cascade.depthRange;

            out.cascadeSplits[c] = cascade.splitFar;
            out.cascadeTexel[c] = texel;

            auto& cascadeState = state.cascades[static_cast<size_t> (c)];
            cascadeState.viewProjection = cascade.viewProjection;
            cascadeState.casters.clear();

            for (size_t i = 0; i < casterBounds.size(); ++i)
                if (ShadowCascades::castsInto (cascade, casterBounds[i]))
                    cascadeState.casters.emplace_back (i, i < inputs.casterSignatures.size() ? inputs.casterSignatures[i] : 0);
        }

        out.sunInfo[0] = static_cast<float> (numCascades);
        out.sunInfo[2] = cascadeBlendBand;
        out.sunInfo[3] = ! sceneFitted && ShadowCascades::cutsOffReceivers (cascadeInputs) ? 1.0f : 0.0f;
    }

    if (numRecords == 0)
    {
        rendered.reset();
        return Result::ok();
    }

    // Soft filtering only for a light with a shadow map and a size to soften it with: a contact
    // shadow only record has no map
    auto soft = false;
    for (size_t i = 0; i < lights.size(); ++i)
        soft = soft || (recordIndices[i] >= 0 && out.records[recordIndices[i]].atlasRect[2] > 0.0f && lights[i].node->radius > 0.0f);

    soft = soft && inputs.capabilities.isEnabled (RenderCapabilities::Feature::softShadows, inputs.settings);

    // The shading passes take their shadow filter variants for soft filtering, or for contact shadows to march
    const auto contact = inputs.capabilities.isEnabled (RenderCapabilities::Feature::contactShadows, inputs.settings);
    shadowFilters = soft || (contact && std::any_of (out.records, out.records + numRecords, [] (const GpuShadowRecord& record) { return record.contact[0] > 0.0f; }));

    const auto still = inputs.capabilities.getTier() == RenderCapabilities::Tier::full
                    && ! inputs.capabilities.isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, inputs.settings);
    out.filterInfo[0] = soft ? 1.0f : 0.0f;
    out.filterInfo[1] = soft ? (still ? stillSoftShadowTaps : softShadowTaps) : 0.0f;
    out.filterInfo[2] = out.filterInfo[1];
    out.filterInfo[3] = (inputs.capabilities.isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, inputs.settings) ? temporalNoiseFlag : 0.0f)
                      + (contact ? contactShadowsFlag : 0.0f);

    // Without a signature per caster nothing tells what a map holds: every map is drawn
    if (inputs.casterSignatures.size() != inputs.drawList.getOpaqueItems().size())
    {
        jassertfalse;
        rendered.reset();
    }

    const auto keepsCascades = rendered != nullptr
                            && rendered->globalSignature == state.globalSignature
                            && rendered->resolution == state.resolution
                            && rendered->numCascades == state.numCascades;

    std::array<bool, ShadowCascades::maxCascades> redrawCascade {};
    for (size_t c = 0; c < static_cast<size_t> (numCascades); ++c)
        redrawCascade[c] = ! keepsCascades || rendered->cascades[c] != state.cascades[c];

    // Each atlas view is drawn again unless its tile already holds the depth it would draw
    const auto keepsAtlas = rendered != nullptr && rendered->atlasSize == state.atlasSize;
    redrawViews.assign (state.localViews.size(), true);
    size_t numRedrawnViews = 0;

    for (size_t i = 0; i < state.localViews.size(); ++i)
    {
        const auto& view = state.localViews[i];
        redrawViews[i] = ! keepsAtlas || std::none_of (rendered->localViews.begin(), rendered->localViews.end(), [&] (const LocalView& kept)
        {
            return kept.holdsSameDepth (view);
        });

        numRedrawnViews += redrawViews[i] ? 1 : 0;
    }

    if (numRedrawnViews == 0 && std::none_of (redrawCascade.begin(), redrawCascade.end(), [] (bool redraw) { return redraw; }))
        return Result::ok();

    // A failure leaves no rendered state: the next frame draws every map
    for (size_t c = 0; c < static_cast<size_t> (numCascades); ++c)
    {
        if (! redrawCascade[c])
            continue;

        if (auto result = renderCascade (device, frame, inputs.drawList, static_cast<int> (c), state.cascades[c]); result.failed())
        {
            rendered.reset();
            return result;
        }

        ++numCascadeRenders[c];
    }

    if (numRedrawnViews > 0)
    {
        if (auto result = renderAtlas (device, frame, inputs.drawList, state.localViews, redrawViews, numRedrawnViews < state.localViews.size()); result.failed())
        {
            rendered.reset();
            return result;
        }
    }

    if (rendered == nullptr)
        rendered = std::make_unique<RenderedState>();

    std::swap (rendered, pending);
    ++numRenderedFrames;
    return Result::ok();
}

} // namespace yup
