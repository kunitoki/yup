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
/** A part that bakes the diffuse light inside a box around its entity on a grid of probes,
    used by the surfaces inside the box instead of the diffuse light of the environment.

    Each probe stores two bands of spherical harmonics and the distances to the surrounding
    geometry, so light does not leak through walls. Baked on request, like ReflectionProbeNode:
    when the part is attached, when requestBake() is called, when SceneRenderer::bakeProbes()
    runs, and automatically when one of its own fields or the world matrix of its entity
    changes. Changes to the rest of the scene never rebake it by themselves.

    The probes sit at the centers of the resolutionX x resolutionY x resolutionZ cells the box is
    divided into. Each one captures the scene once, lit by the environment and the direct lights
    only (one bounce: the volumes and reflection probes do not light their own captures). A frame
    or SceneRenderer::bakeProbes() call captures at most ProbeBaker::maxVolumeProbesPerBake probe
    positions, so a larger volume bakes over several calls, and lights nothing until it is complete.

    Volumes are diffuse only: the reflections still come from the probes and the environment.
    Where volumes overlap, the smaller one is used, and where it fades out over its falloff, or none
    of its probes can see a surface, the environment takes the rest, not an enclosing volume: give a
    volume nested in another a falloff of 0.

    @see ReflectionProbeNode, SceneRenderer, SceneDrawList
*/
class YUP_API IrradianceVolumeNode : public Node
{
public:
    /** Creates a volume with a new bake id. */
    IrradianceVolumeNode() noexcept;

    /** The most probes along each axis: 7, so the largest volume (343 probes) fits the atlases,
        49 x 18 texels of the 1024 wide visibility atlas. Several volumes may still fill them: a volume
        left without room lights nothing, logged once. */
    static constexpr int maxResolution = 7;

    Vector3<float> extents { 5.0f, 2.5f, 5.0f }; ///< Box half extents, in entity space. Any of them 0 or less lights nothing: the volume is left out.
    int resolutionX = 4;                         ///< Probes along X, from 2 to maxResolution.
    int resolutionY = 2;                         ///< Probes along Y, from 2 to maxResolution.
    int resolutionZ = 4;                         ///< Probes along Z, from 2 to maxResolution.
    float falloff = 0.5f;                        ///< Distance inside the border over which the volume fades out, in world units (the probes' falloff is in entity space). Negative counts as 0, a hard edge.
    float intensity = 1.0f;                      ///< Multiplier of the baked light.
    float normalBias = 0.05f;                    ///< Offset along the normal before the visibility test, in world units.

    /** Marks the volume for baking again, by giving it a new bake id. */
    void requestBake() noexcept;

    /** A process-unique id that changes with every requestBake(), never reused, never 0. */
    uint64 getBakeId() const noexcept { return bakeId; }

private:
    uint64 bakeId;
};

} // namespace yup
