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
/** Packs point (6 faces) and spot (1 face) shadow maps into one square atlas.

    A buddy quadtree of power-of-two tiles. Requests are served from the most to the least
    important: larger desired sizes first, then lower light indices. A request that does not fit
    is halved down to the minimum tile size, then dropped. All the faces of one light share one
    tile size.

    @code
    ShadowAtlas atlas (2048, 64, 512);

    std::vector<ShadowAtlas::Request> requests;
    for (int i = 0; i < (int) lights.size(); ++i)
        if (auto request = atlas.makeRequest (*lights[i].node, i, distanceToCamera[i], yFov, viewportHeight))
            requests.push_back (*request);

    atlas.allocate (requests);

    if (const auto* allocation = atlas.findAllocation (lightIndex))
        for (int face = 0; face < allocation->faces; ++face)
            renderFace (atlas.getTiles()[(size_t) (allocation->firstTile + face)]);
    @endcode

    @internal
*/
class YUP_API ShadowAtlas
{
public:
    //==============================================================================
    /** A light asking for shadow map tiles. */
    struct Request
    {
        int lightIndex = 0;  ///< The index in SceneDrawList::getLights().
        int faces = 1;       ///< 1 for spot lights, 6 for point lights and wide spot lights (see makeRequest()).
        int desiredSize = 0; ///< A power of two, see getDesiredTileSize().
    };

    /** A square region of the atlas, in texels, top-left origin. */
    struct Tile
    {
        int x = 0;
        int y = 0;
        int size = 0;
    };

    /** The tiles given to one light. */
    struct Allocation
    {
        int lightIndex = 0;
        int firstTile = 0; ///< The index in getTiles(): the faces tiles of the light follow each other.
        int faces = 0;
        int size = 0;
    };

    //==============================================================================
    /** Creates an empty atlas.

        @param atlasSize    The atlas side in texels, a power of two.
        @param minTileSize  The smallest tile side a request is halved to, a power of two.
        @param maxTileSize  The largest tile side, a power of two, clamped to atlasSize.
    */
    ShadowAtlas (int atlasSize, int minTileSize, int maxTileSize);

    /** Destructor. */
    ~ShadowAtlas();

    //==============================================================================
    /** Replaces every previous allocation. Deterministic for equal inputs.

        Desired sizes are first clamped to [minTileSize, maxTileSize], so requests resolving to the
        same size are ordered by light index. Requests are served by descending clamped size, then
        ascending light index. Each is tried at its desired size, then at half
        of it, down to minTileSize; when even that does not fit, the light gets no allocation.
    */
    void allocate (Span<const Request> requests);

    /** The allocations of the last allocate(), in serving order. */
    Span<const Allocation> getAllocations() const noexcept;

    /** The tiles of the last allocate(), indexed by Allocation::firstTile. */
    Span<const Tile> getTiles() const noexcept;

    /** The atlas side in texels. */
    int getAtlasSize() const noexcept;

    /** Returns the allocation of a light, or nullptr when it got none. */
    const Allocation* findAllocation (int lightIndex) const noexcept;

    //==============================================================================
    /** The request of a light seen by the camera, or nothing when it casts no shadow in the atlas:
        directional lights, lights without LightNode::castsShadows, and dark lights (a
        GpuSceneData::getEffectiveRange() of 0).

        The tile size follows the screen size of the light's influence sphere (its effective
        range), see getProjectedRadius(), getOrthographicProjectedRadius() and getDesiredTileSize(). Point lights get 6 faces; a spot
        light whose cone plus the guard band (2 * outerConeAngle + getGuardAngle (minTileSize))
        exceeds 120 degrees is shadowed as a point light too, with 6 faces. Degenerate camera
        inputs (a yFov or viewportHeight that is not positive, a NaN distance) give minTileSize.

        @param light             The light node.
        @param lightIndex        The index in SceneDrawList::getLights().
        @param distanceToCamera  The distance from the camera to the light's position.
        @param yFov              The vertical field of view of a perspective camera in radians.
        @param viewportHeight    The viewport height in pixels.
        @param orthographicHalfHeight  For an orthographic camera, half the height of its view in
                                       world units; @p distanceToCamera and @p yFov are then ignored.
                                       0 for a perspective camera.
    */
    std::optional<Request> makeRequest (const LightNode& light, int lightIndex, float distanceToCamera, float yFov, int viewportHeight, float orthographicHalfHeight = 0.0f) const noexcept;

    /** The tile size makeRequest() asks for a point or spot light, which also ranks it: the size
        of the screen projection of its influence sphere (its effective range), clamped to the
        atlas tile sizes. The parameters are those of makeRequest().
    */
    int getLightTileSize (const LightNode& light, float distanceToCamera, float yFov, int viewportHeight, float orthographicHalfHeight = 0.0f) const noexcept;

    //==============================================================================
    /** The tile size for a light whose influence sphere projects to @p projectedRadiusPx
        pixels: the next power of two of twice that, clamped to [minTile, maxTile].
        A radius that is not positive gives minTile.
    */
    static int getDesiredTileSize (float projectedRadiusPx, int minTile, int maxTile) noexcept;

    /** Projected radius in pixels of a sphere for a perspective camera, or a huge value when
        the camera is inside it. A yFov or viewportHeight that is not positive gives 0.
    */
    static float getProjectedRadius (float sphereRadius, float distanceToCamera, float yFov, int viewportHeight) noexcept;

    /** Projected radius in pixels of a sphere for an orthographic camera whose view is
        2 * @p halfHeight world units high: sphereRadius / halfHeight * viewportHeight / 2, whatever
        its distance. A halfHeight or viewportHeight that is not positive gives 0.
    */
    static float getOrthographicProjectedRadius (float sphereRadius, float halfHeight, int viewportHeight) noexcept;

    /** The angle in radians added to a face's field of view so that a 4-texel filter kernel at
        the edge of a tile of @p tileSize texels stays inside it: 2 * atan (1 + 4 / tileSize) - pi / 2.
    */
    static float getGuardAngle (int tileSize) noexcept;

private:
    //==============================================================================
    int getLevel (int size) const noexcept;
    int getCapacity (int size) const noexcept;
    Tile takeNode (int level);

    int atlasSize = 0;
    int minTileSize = 0;
    int maxTileSize = 0;

    std::vector<std::vector<Tile>> freeNodes; // per level, level 0 = the whole atlas; the back is taken first
    std::vector<Allocation> allocations;
    std::vector<Tile> tiles;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShadowAtlas)
};

} // namespace yup
