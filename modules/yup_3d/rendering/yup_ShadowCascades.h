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
/** Cascaded shadow maps for one directional light: splits, stable fitting and texel snapping.

    Follows Engel, "Cascaded Shadow Maps" (ShaderX5, 2006): the view depth range is cut with the
    practical split scheme of Zhang et al. ("Parallel-Split Shadow Maps for Large-scale Virtual
    Environments", VRCIA 2006), every slice of the camera frustum is wrapped in the smallest sphere
    centered on the view axis (Valient, "Stable Rendering of Cascaded Shadow Maps", ShaderX6, 2008),
    whose radius is quantized so it does not change with the camera orientation, and the light
    projection is snapped to whole texels so shadow edges do not shimmer while the camera moves.

    The depth range comes from the receivers (everything a shadow can land on) clamped to the
    camera planes, so no cascade is spent on empty space; the casters only extend the light-space
    depth range towards the light, so casters outside a slice still land in its map.

    A compact scene can instead be covered by one map fitted to the whole scene (fitScene()), which
    does not depend on the camera: an orbit, pan or zoom then leaves the map as it is. It replaces
    the cascades only while its texels are no larger than those of the first cascade
    (prefersSceneFit()), so no shadow gets blurrier.

    @code
    ShadowCascades::Inputs inputs;
    inputs.setView (viewData);
    inputs.lightDirection = sunDirection;
    inputs.receiverBounds = visibleBounds;
    inputs.casterBounds = opaqueBounds;

    std::array<ShadowCascades::Cascade, ShadowCascades::maxCascades> cascades;
    const auto numCascades = ShadowCascades::build (inputs, cascades);
    @endcode

    @internal
*/
class YUP_API ShadowCascades
{
public:
    //==============================================================================
    /** The largest number of cascades of one light. */
    static constexpr int maxCascades = 4;

    /** The view depth shadows reach with an infinite far plane, the cap LightClusters uses too. */
    static constexpr float maxShadowDistance = 1000.0f;

    //==============================================================================
    /** One cascade: the light projection of one slice of the view frustum. */
    struct Cascade
    {
        Matrix4 viewProjection;       ///< World to shadow clip space, depth in [0, 1].
        float splitFar = 0.0f;        ///< The view depth where the cascade ends.
        float texelWorldSize = 0.0f;  ///< The world size of one shadow map texel.
        Vector3<float> center;        ///< The center of the bounding sphere of the slice, on the view axis, in world space.
        float radius = 0.0f;          ///< The padded, quantized radius: the half extent of the projection before snapping.
        float depthRange = 0.0f;      ///< The light-space distance mapped to depth 1; depth 0 is the near plane of the light view.
    };

    /** What the cascades are fitted to. */
    struct Inputs
    {
        Matrix4 cameraWorld;           ///< Camera to world.
        Matrix4 projection;            ///< The unjittered camera projection, perspective or orthographic.
        float nearPlane = 0.1f;        ///< The camera near plane, clipPlanes[0] of the ViewData block.
        float farPlane = 1000.0f;      ///< The camera far plane, clipPlanes[1] of the ViewData block: 0 for infinite.
        Vector3<float> lightDirection; ///< The unit direction the light shines towards.
        BoundingBox receiverBounds;    ///< World bounds of what shadows land on (opaque, masked and blended items).
        BoundingBox casterBounds;      ///< World bounds of the shadow casters (opaque and masked items).
        int count = maxCascades;       ///< The number of cascades, in [1, maxCascades].
        int resolution = 2048;         ///< The width and height of a cascade map in texels.
        float lambda = 0.75f;          ///< 0 for uniform splits, 1 for logarithmic splits.

        /** Takes the camera fields from a view: its world matrix, unjittered projection and clip planes. */
        void setView (const ViewData& view) noexcept;
    };

    //==============================================================================
    /** Practical split scheme: a blend of logarithmic and uniform splits (Zhang et al. 2006).

        @param nearPlane  The view depth of the first split, must be positive.
        @param farPlane   The view depth of the last split, larger than nearPlane.
        @param count      The number of cascades, clamped to [1, maxCascades].
        @param lambda     The weight of the logarithmic splits against the uniform ones.

        @return count + 1 view depths from nearPlane to farPlane; the entries after them repeat farPlane.
    */
    static std::array<float, maxCascades + 1> computeSplits (float nearPlane, float farPlane, int count, float lambda) noexcept;

    /** The view depth range shadows are fitted to: the receiver bounds clamped to the camera planes.

        The range is empty when no receiver lies between the camera planes. Otherwise it starts at the
        camera near plane or the nearest receiver, whichever is farther, and ends at the camera far plane
        (maxShadowDistance when it is infinite) or the deepest receiver, whichever is nearer; receivers
        parallel to the image plane get a small depth, so a non-empty range always has some thickness.

        @param inputs  The camera, its clip planes and the receiver bounds.

        @return The view depth range, or an empty range when no receiver lies between the camera planes.
    */
    static Range<float> computeShadowRange (const Inputs& inputs) noexcept;

    /** True when receivers lie beyond the end of computeShadowRange(): the camera far plane, or
        maxShadowDistance for an infinite one, cut the range short. The last cascade then fades out
        towards its far split, instead of covering every receiver up to it.

        Decided against the limit itself, never against the split, which equals the deepest
        receiver whenever nothing cuts the range.

        @param inputs  The camera, its clip planes and the receiver bounds.
    */
    static bool cutsOffReceivers (const Inputs& inputs) noexcept;

    /** Translates a light view-projection so the world origin lands on a whole texel (Engel 2006).

        @param lightViewProjection  An orthographic light view-projection.
        @param resolution           The width and height of the shadow map in texels.

        @return The same projection followed by a clip-space translation of less than half a texel.
    */
    static Matrix4 snapToTexels (const Matrix4& lightViewProjection, int resolution) noexcept;

    /** True when a caster can draw into a cascade: its world bounds overlap the cascade's light
        projection sideways, and do not lie wholly beyond its far depth. The near side is left open:
        nothing toward the light is ever culled.

        Only valid for the cascades of build(), whose projections are orthographic (the test ignores
        w; a perspective projection culls wrongly), and for casters inside the Inputs::casterBounds
        they were built from: build() starts the light view at the nearest of them, so none of
        them is clipped by the near plane.

        @param cascade        A cascade of build().
        @param casterBounds   The world bounds of the caster, inside the Inputs::casterBounds of the cascade.

        @return False when nothing of the caster can land in the cascade's map, true otherwise.
    */
    static bool castsInto (const Cascade& cascade, const BoundingBox& casterBounds) noexcept;

    /** Fits one map to the whole scene, whatever the camera: the receiver bounds seen from the light
        in a basis that only depends on the light direction, padded by four texels (so every lookup
        of a receiver, moved by its normal offset and widened by the filter, stays inside), its
        radius rounded up to a 128th of the next power of two (so a caster moving at the edge of the
        scene seldom changes the texel size) and snapped to whole texels as build() does, its depth
        range from the nearest caster or receiver towards the light to the deepest receiver. Every
        receiver lies inside it, so it never fades out.

        Its splitFar is maxShadowDistance, and castsInto() culls against it as against a cascade.

        @param inputs   The light, the receiver and caster bounds and the resolution; the camera is ignored.
        @param cascade  Receives the map when the function returns true.

        @return False when there is nothing to shadow or nothing casting.
    */
    static bool fitScene (const Inputs& inputs, Cascade& cascade) noexcept;

    /** True when a scene fit (fitScene()) should replace the cascades of build().

        It takes over once its texels are no larger than those the first cascade would have one
        quantization step of its radius smaller, and holds while they are no larger than those of the
        first cascade. So it is never blurrier than the first cascade, and a camera moving the first
        cascade back and forth across one step of its radius does not switch between the two.

        @param sceneFit       The map of fitScene().
        @param firstCascade   The first cascade of build().
        @param usesSceneFit   True when the previous frame used the scene fit.
    */
    static bool prefersSceneFit (const Cascade& sceneFit, const Cascade& firstCascade, bool usesSceneFit) noexcept;

    /** Fits every cascade.

        @param inputs     The camera, the light, the receiver and caster bounds, and the cascade settings.
        @param cascades   Receives the cascades; the entries past the returned count are left untouched.

        @return The number of cascades written: 0 when there is nothing to shadow or nothing casting.
    */
    static int build (const Inputs& inputs, std::array<Cascade, maxCascades>& cascades) noexcept;
};

} // namespace yup
