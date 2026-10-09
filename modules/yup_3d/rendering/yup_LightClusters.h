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
/** Clustered light culling (Olsson et al., "Clustered Deferred and Forward Shading", HPG 2012).

    The view frustum is split into screen tiles times exponential depth slices. For every cluster
    a compute pass lists the packed point and spot lights reaching it, so a full-tier shading pass
    loops over the few lights of its fragment's cluster instead of every light. Directional lights
    are packed first and are not clustered.

    Conventions: tile y = 0 is the top row of pixels (NDC y = +1), as gl_FragCoord is top-left on
    every full-tier backend, and the view depth of a point is -(view * point).z.

    The clusterLights buffer holds a slot of 64 uints per cluster at index c * 64: the count, then
    up to maxLightsPerCluster light indices in ascending order.

    @code
    LightClusters clusters;
    LightClusters::Grid grid;
    const auto clusterLights = frameResources.acquireStorageBuffer (FrameResources::StorageSlot::clusterLights,
                                                                    LightClusters::getClusterBufferSize (grid));
    clusters.dispatch (frame, device, grid, lights, numLights, firstLocalLight,
                       viewData.getView(), viewData.getJitteredProjection().inverted(), clusterLights);
    @endcode

    @internal
*/
class YUP_API LightClusters
{
public:
    //==============================================================================
    /** The froxel grid: screen tiles times exponential depth slices (Olsson et al. 2012). */
    struct Grid
    {
        int tilesX = 16;
        int tilesY = 9;
        int slices = 24;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f; ///< CameraNode::zFar, or 1000 when the camera far plane is infinite.
        bool infiniteFar = false; ///< True when the camera far plane is infinite: the last slice then reaches past farPlane to infinity.

        /** Returns the number of clusters. */
        int getNumClusters() const noexcept { return tilesX * tilesY * slices; }
    };

    /** Slot of 64 uints per cluster: the count, then the indices. */
    static constexpr int maxLightsPerCluster = 63;

    //==============================================================================
    /** Creates the culling pass. The pipeline is compiled by the first dispatch(). */
    LightClusters();

    /** Destructor. */
    ~LightClusters();

    //==============================================================================
    /** The grid of a view: its camera's near plane, and its far plane, or 1000 with
        Grid::infiniteFar when the far plane is infinite (CameraNode::zFar of 0). A near plane of 0,
        which orthographic cameras allow, is moved to 1e-4 so the slices stay finite.
    */
    static Grid getGrid (const ViewData& view) noexcept;

    /** The slice holding a positive view-space depth, clamped to [0, slices - 1]. */
    static int getSliceForDepth (const Grid& grid, float viewDepth) noexcept;

    /** The near view-space depth of a slice; slice == slices gives the far plane. */
    static float getDepthForSlice (const Grid& grid, int slice) noexcept;

    /** The linear cluster index x + tilesX * (y + tilesY * slice) of a pixel (top-left origin) and view depth. */
    static int getClusterIndex (const Grid& grid, Point<float> pixel, Point<float> viewportSize, float viewDepth) noexcept;

    /** The view-space bounds of a cluster, built with the (jittered) inverse projection.

        The four tile corners are unprojected at two depths, and each corner ray is cut by the
        planes of the slice: this holds for perspective cameras, infinite far planes included,
        and orthographic ones. With Grid::infiniteFar the last slice is open: its bounds extend
        to the largest float along every axis its corner rays move on.
    */
    static BoundingBox getClusterBounds (const Grid& grid, int x, int y, int slice, const Matrix4& inverseProjection) noexcept;

    /** True when a packed light can reach view-space bounds.

        Point and spot lights reach a sphere of abs (directionRange.w) around them; a spot light must
        also reach the sphere bounding its cone, as in SceneDrawList::selectLights(): both use
        GpuSceneData::lightReachesBox(). Directional lights and dark lights (a range of 0) reach nothing.
    */
    static bool intersects (const GpuLight& light, const Matrix4& view, const BoundingBox& viewBounds) noexcept;

    /** The reference assignment the GPU must reproduce: for each cluster, the indices of the packed
        non-directional lights reaching it, ascending, at most maxLightsPerCluster.

        @param grid               The cluster grid.
        @param lights             The packed lights, directional ones first (GpuSceneData::packLights()).
        @param firstLocalLight    The index of the first non-directional light.
        @param view               The world to view matrix.
        @param inverseProjection  The inverse of the (jittered) projection.
    */
    static std::vector<std::vector<uint32>> assign (const Grid& grid, Span<const GpuLight> lights, int firstLocalLight, const Matrix4& view, const Matrix4& inverseProjection);

    //==============================================================================
    /** Encodes the culling compute pass into a frame: reads the lights buffer, writes clusterLights.

        Every cluster slot is written, so clusterLights may hold anything beforehand.

        @param frame              The frame to record into.
        @param device             The device of the frame.
        @param grid               The cluster grid.
        @param lights             A storage buffer of numLights packed GpuLight.
        @param numLights          The number of packed lights.
        @param firstLocalLight    The index of the first non-directional light.
        @param view               The world to view matrix.
        @param inverseProjection  The inverse of the (jittered) projection, as rasterized.
        @param clusterLights      A storage buffer of at least getClusterBufferSize (grid) bytes.

        @returns False when the pipeline cannot be compiled, a buffer is too small for the grid or
                 for numLights, or the pass cannot be recorded.
    */
    bool dispatch (GpuFrame& frame, const GpuDevice::Ptr& device, const Grid& grid, GpuBuffer::Ptr lights, int numLights, int firstLocalLight, const Matrix4& view, const Matrix4& inverseProjection, GpuBuffer::Ptr clusterLights);

    /** Size in bytes of the clusterLights buffer for a grid. */
    static size_t getClusterBufferSize (const Grid& grid) noexcept { return static_cast<size_t> (grid.getNumClusters()) * 64u * sizeof (uint32); }

    /** Packs the grid as the shaders read it, in the culling pass and in the LightingBlock of the
        shading passes: @p gridOut gets tilesX, tilesY, slices and @p numLights; @p depthOut gets
        the near and far planes, @p firstLocalLight and 1 when Grid::infiniteFar is set.
    */
    static void packGrid (const Grid& grid, int numLights, int firstLocalLight, float (&gridOut)[4], float (&depthOut)[4]) noexcept;

private:
    Result prepare (const GpuDevice::Ptr& device);

    GpuComputePipeline::Ptr pipeline;
    std::optional<Result> preparation;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LightClusters)
};

} // namespace yup
