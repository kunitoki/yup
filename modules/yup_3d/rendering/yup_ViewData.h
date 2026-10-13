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
/** The camera of one frame: matrices, the previous frame's matrix and the TAA jitter.

    update() picks the active camera of the scene (or a default perspective camera at (0, 0, 5)
    looking at the origin) and fills the Block, a std140 mirror of the ViewBlock in lib/view.glsl.

    @internal
*/
class YUP_API ViewData
{
public:
    //==============================================================================
    /** The view depth standing for an infinite far plane where a finite one is needed: how far the
        light clusters and the sun shadows reach. */
    static constexpr float infiniteFarDistance = 1000.0f;

    /** The frames of the cycle of the shadow noise: clipPlanes.z holds the frame index modulo this. */
    static constexpr uint64 noiseFrameCycle = 64;

    //==============================================================================
    /** std140 mirror of ViewBlock in lib/view.glsl. */
    struct Block
    {
        float view[16];
        float projection[16];                ///< Jittered when requested.
        float viewProjection[16];            ///< Jittered when requested.
        float inverseViewProjection[16];
        float unjitteredViewProjection[16];
        float previousViewProjection[16];    ///< The unjittered matrix of the previous update().
        float cameraPosition[4];             ///< xyz position, w scene exposure.
        float viewport[4];                   ///< Width, height, 1 / width, 1 / height in pixels.
        float jitter[4];                     ///< xy jitter in NDC, zw previous jitter in NDC.
        float clipPlanes[4];                 ///< zNear, zFar, frame index modulo noiseFrameCycle, tone mapping operator.
    };

    //==============================================================================
    /** Computes this frame's matrices from the scene's active camera (or the default camera).

        @param scene        The scene to take the camera, exposure and tone mapping from.
        @param widthPx      The viewport width in pixels.
        @param heightPx     The viewport height in pixels.
        @param jittered     True to apply the TAA sub-pixel jitter to the projection.
        @param taaSamples   The number of points in the jitter cycle.
        @param frameIndex   The index of the frame, which picks the jitter; clipPlanes.z holds it
                            modulo noiseFrameCycle, exact in a float however long the view runs.
    */
    void update (const Scene& scene, int widthPx, int heightPx, bool jittered, int taaSamples, uint64 frameIndex);

    /** Sets the matrices of a capture, such as a face of a reflection probe cube: exposure 1, no
        jitter, frame index 0, no motion (the previous matrix is the current one), and Reinhard
        (operator 0) for an inline tone mapped output.

        @param viewMatrix        World to view.
        @param projectionMatrix  The perspective projection, depth 0 to 1, whose clip planes the block keeps.
        @param position          The world position of the capture.
        @param size              The width and height of the capture in pixels.
    */
    void setCapture (const Matrix4& viewMatrix, const Matrix4& projectionMatrix, Vector3<float> position, int size);

    /** Forgets the previous frame: the next update() uses its own matrices as the previous ones. */
    void reset() noexcept;

    //==============================================================================
    /** Returns the block to upload. */
    const Block& getBlock() const noexcept { return block; }

    /** Returns the view matrix. */
    const Matrix4& getView() const noexcept { return view; }

    /** Returns the projection matrix, unjittered. */
    const Matrix4& getProjection() const noexcept { return projection; }

    /** Returns the projection matrix as rasterized: jittered when update() was asked to. */
    const Matrix4& getJitteredProjection() const noexcept { return jitteredProjection; }

    /** Returns the view projection matrix, unjittered. */
    const Matrix4& getUnjitteredViewProjection() const noexcept { return unjitteredViewProjection; }

    /** Returns the world position of the camera. */
    Vector3<float> getCameraPosition() const noexcept { return cameraPosition; }

    //==============================================================================
    /** The operator index of yupToneMap in lib/color.glsl: 0 Reinhard, 1 ACES, 2 AgX. */
    static float getToneMappingOperator (Scene::ToneMapping toneMapping) noexcept;

    /** Element @p index (1-based) of the Halton sequence in @p base, in [0, 1). */
    static float halton (int index, int base) noexcept;

    /** Sub-pixel jitter of a TAA frame in pixels, in [-0.5, 0.5), cycling through @p sampleCount Halton (2, 3) points. */
    static Point<float> getJitter (uint64 frameIndex, int sampleCount) noexcept;

private:
    Block block {};
    Matrix4 view;
    Matrix4 projection;
    Matrix4 jitteredProjection;
    Matrix4 unjitteredViewProjection;
    Vector3<float> cameraPosition;
    Point<float> previousJitter;
    bool hasPrevious = false;
};

static_assert (sizeof (ViewData::Block) == 448);

} // namespace yup
