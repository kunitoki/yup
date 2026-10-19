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
/** A part that captures the scene around its entity into a reflection, used by the surfaces
    inside its influence instead of the environment.

    The capture is taken from the entity origin. It is baked again when the part is attached,
    when requestBake() is called, when SceneRenderer::bakeProbes() runs, and automatically when
    one of its own fields or the world matrix of its entity changes. Changes to the rest of the
    scene never rebake it by themselves: call requestBake() after editing what surrounds it.

    The influence volume is a sphere or a box in entity space, centered on the entity. A surface
    inside it takes its reflection from the probe, faded out over falloff towards the border.
    Where influences overlap, the smaller one takes priority, so a probe nested inside a larger
    one wins near the surfaces it encloses. On the raster tier each draw blends the two probes
    that take priority over it, see SceneDrawList::selectReflectionProbes().

    @see Entity, SceneRenderer, SceneDrawList
*/
class YUP_API ReflectionProbeNode : public Node
{
public:
    /** The shape of the influence volume. */
    enum class Shape
    {
        sphere, ///< A sphere of influenceRadius.
        box     ///< A box of influenceExtents.
    };

    /** Creates a spherical probe with a new bake id. */
    ReflectionProbeNode() noexcept;

    Shape shape = Shape::sphere;                          ///< The shape of the influence volume.
    float influenceRadius = 5.0f;                         ///< Sphere radius, in entity space. 0 or less influences nothing: the probe is left out.
    Vector3<float> influenceExtents { 5.0f, 5.0f, 5.0f }; ///< Box half extents, in entity space. Any of them 0 or less influences nothing: the probe is left out.
    float falloff = 0.5f;                                 ///< Distance inside the border over which the probe fades out, in entity space. Negative counts as 0, a hard edge.
    bool parallaxCorrection = true;                       ///< Projects reflections onto the influence shape (Lagarde 2012).
    float intensity = 1.0f;                               ///< Multiplier of the captured light.
    float clipNear = 0.05f;                               ///< Near plane of the capture, in world units from the entity origin. Values below 0.001 count as 0.001, and values beyond half the capture's far plane (four times the influence's bounding radius, at least 100) as that.

    /** Marks the probe for baking again, by giving it a new bake id. */
    void requestBake() noexcept;

    /** A process-unique id that changes with every requestBake(), never reused, never 0.

        Two probes never share an id, even when one is created at the address of a deleted one.
    */
    uint64 getBakeId() const noexcept { return bakeId; }

private:
    uint64 bakeId;
};

} // namespace yup
