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
/** User-facing switches for how a Scene is rendered.

    Features that the device or the active tier cannot provide are ignored; ask
    RenderCapabilities what is actually in effect.
*/
struct YUP_API RenderSettings
{
    /** Which rendering path to use. */
    enum class TierPreference
    {
        automatic, ///< Full on capable desktop GPUs, raster elsewhere.
        raster,    ///< Force the raster path.
        full       ///< Request the full path (still raster when the device cannot run it).
    };

    TierPreference tier = TierPreference::automatic; ///< automatic picks full on capable desktop GPUs, raster elsewhere.
    int taaSamples = 16;                             ///< Full tier: jittered frames accumulated before the image is final.

    bool bloom = false;           ///< Glow around highlights. Raster: needs an MSAA rgba16float target.
    float bloomThreshold = 1.0f;  ///< Exposed radiance above which pixels glow.
    float bloomIntensity = 0.05f; ///< How much of the blurred glow is added.
    float bloomRadius = 0.85f;    ///< 0 to 1: how far the glow spreads.

    /** Depth of field: what lies away from CameraNode::focusDistance blurs by the camera's
        CameraNode::fStop, with a bokeh shaped by CameraNode::apertureBlades. The raster tier needs
        multisampled rgba16float targets and the depth and normal prepass for it, see
        RenderCapabilities::Feature::depthOfField, and ignores it without them, which is not an
        error. Off by default, costing nothing then; a camera that cannot blur (orthographic, an
        fStop of 0 or less, or no active camera) costs nothing either, on the raster tier too, so
        crossing an fStop of 0 switches the raster frame's route like this setting does. It
        applies after the temporal accumulation: changing it, or the depth of field fields of the
        camera, neither restarts nor discards it, so repaint after such a change.
    */
    bool depthOfField = false;

    /** The largest blur of the depth of field, as a diameter in pixels of the final image, at most
        DepthOfField::maxBlurSize (64). 0 or less blurs nothing.
    */
    float depthOfFieldMaxSize = 32.0f;

    /** Motion blur: what moves on screen, through the camera or its own transform, smears along
        its motion while the shutter is open. Full tier only, see
        RenderCapabilities::Feature::motionBlur: the raster tier has no motion to read and ignores
        it, which is not an error. Off by default, costing nothing then; frames where nothing moved
        (still and converged frames, and the first frame) cost nothing either, and neither do the
        frames after a resize, a tier or taaSamples change, SceneRenderer::restartAccumulation() or
        a failed frame, which have no previous frame to measure the motion against. It applies
        after the temporal accumulation and the depth of field: changing it, or motionBlurShutter,
        neither restarts nor discards the accumulation, so repaint after such a change. A smeared
        frame is not final (SceneRenderer::isConverged()), so the still frame after it is sharp.
    */
    bool motionBlur = false;

    /** The fraction of the frame time the shutter of motion blur stays open, from 0 to 1: a smear
        spans this share of the motion between two frames, at most 32 pixels of the final image.
        0 or less blurs nothing.
    */
    float motionBlurShutter = 0.5f;

    /** Shadow cascades of the shadow casting directional light, from 1 to 4. More cascades keep
        shadows sharp further away, at the cost of one depth pass each. 0 picks the tier default:
        4 on full, RenderCapabilities::defaultRasterCascades on raster. See RenderCapabilities::getCascadeCount().
    */
    int sunCascades = 0;

    /** Texels per side of each sun shadow cascade. Clamped to [256, 4096] on both tiers, see
        RenderCapabilities::getShadowResolution().

        It also sizes the shadow atlas shared by the point and spot lights: this many texels per side
        on raster, twice as many on full, rounded down to a power of two and at most 4096.
    */
    int shadowResolution = 2048;

    /** Soft shadows whose penumbra widens with the size of the light (LightNode::radius) and the
        distance between the caster and the receiver. Unset picks the tier default: on for full, off
        for raster, where true opts in to the same technique at a lower cost.
    */
    std::optional<bool> softShadows;

    /** Short screen-space shadows at the contact of objects, for the lights that ask for them (see
        LightNode::contactShadows). Full tier only: raster cannot provide them and ignores the setting,
        which is not an error.
    */
    bool contactShadows = true;

    /** Ambient occlusion: darkens the indirect light in creases and contacts. Unset picks the tier
        default: on for full, off for raster, where true opts in when the device can draw the depth
        and normal prepass it needs (see RenderCapabilities::Feature::ambientOcclusion). An
        aoDistance of 0 or less, or one that is not finite, turns it off.
    */
    std::optional<bool> ambientOcclusion;

    /** The world-space distance within which surfaces occlude each other for ambient occlusion. A
        distance of 0 or less, or one that is not finite, turns ambient occlusion off.
    */
    float aoDistance = 0.2f;

    /** Screen-space reflections: smooth surfaces reflect what the camera sees, taken from the
        previous frame, and fall back to the reflection probes and the environment where the
        reflection is not on screen. Full tier only, and only with temporal anti-aliasing (taaSamples
        above 1), whose history they read (see RenderCapabilities::Feature::screenSpaceReflections).
        Unset picks the tier default: on for full. The raster tier ignores it, which is not an error.
    */
    std::optional<bool> screenSpaceReflections;

    /** Screen-space diffuse global illumination: the ambient occlusion's horizon search also
        gathers the light of what occludes each surface, taken from the previous frame, so nearby
        lit surfaces bounce their color onto it. On-screen and within aoDistance only, one bounce
        per frame. Full tier only, and only with ambient occlusion and temporal anti-aliasing
        (taaSamples above 1), whose history it reads (see RenderCapabilities::Feature::screenSpaceGI).
        Unset picks the tier default: off on every tier. The raster tier ignores it, which is not an error.
    */
    std::optional<bool> screenSpaceGI;

    /** An intermediate image of the full tier shown in place of the final one, see debugView. */
    enum class DebugView
    {
        none,             ///< The final image.
        ambientOcclusion, ///< The ambient occlusion visibility, as gray: white is unoccluded.
        reflections,      ///< The screen-space reflections, exposed and weighted by their confidence.
        indirectDiffuse,  ///< The light the screen-space diffuse GI gathers, exposed, before the surface colors.
        circleOfConfusion ///< The signed circle of confusion of the depth of field, as a share of depthOfFieldMaxSize: orange in front of the focus, blue behind it, black in focus.
    };

    /** Shows an intermediate image instead of the final one, for tuning the screen-space effects
        and the depth of field. Full tier only: the raster tier ignores it, which is not an error.
        An image the frame did not produce (its feature off or unavailable, or no history to read
        it from yet) shows black. It is shown at the size of the frame without filtering, tone
        mapping, bloom or background. It neither restarts nor discards the temporal accumulation,
        and none, the default, costs nothing.
    */
    DebugView debugView = DebugView::none;

    bool operator== (const RenderSettings&) const = default;
};

} // namespace yup
