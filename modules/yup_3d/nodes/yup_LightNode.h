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

//==============================================================================
/** A part that lights the scene from the position and orientation of its entity.

    Lights follow KHR_lights_punctual: they shine down the -Z axis of their entity, and point
    and spot lights fall off with the inverse square of the distance. Every LightNode slot of
    every visible entity contributes. On the raster tier each draw is shaded by the
    SceneDrawList::maxLightsPerDraw lights that influence it most, see SceneDrawList::selectLights().
    On the full tier every light reaching a surface shades it, up to 63 point and spot lights per
    screen-space cluster.

    @see Entity, and SceneRenderer of the yup_renderer module
*/
class YUP_API LightNode : public Node
{
public:
    /** The kind of light. */
    enum class Type
    {
        directional, ///< Parallel rays shining down -Z, intensity in lux.
        point,       ///< Rays from the entity position in every direction, intensity in candela.
        spot         ///< A point light restricted to a cone around -Z.
    };

    /** Creates a white directional light. */
    LightNode() = default;

    /** Creates a light of a type. */
    explicit LightNode (Type lightType)
        : type (lightType)
    {
    }

    Type type = Type::directional;                                  ///< The kind of light.
    std::array<float, 3> color { 1.0f, 1.0f, 1.0f };                ///< Linear RGB color.
    float intensity = 1.0f;                                         ///< Brightness, see Type for the unit.
    float range = 0.0f;                                             ///< Distance where point and spot lights reach zero, 0 for infinite.
    float innerConeAngle = 0.0f;                                    ///< Spot angle from the axis where the falloff starts.
    float outerConeAngle = MathConstants<float>::pi * 0.25f;        ///< Spot angle from the axis where the light ends.
    float radius = 0.0f;                                            ///< Size of the emitter, 0 for a punctual light: the sphere radius in world units for point and spot lights (spot lights keep their cone), the angular radius in radians for a directional light. With RenderSettings::softShadows it also widens the penumbrae of the light's shadows.
    bool castsShadows = false;                                      ///< True for the light to cast shadows, on both tiers: cascaded shadow maps for the first shadowed directional light, views in a shadow atlas for point and spot lights (the most important ones within the shadow budget of the tier), see SceneRenderer.

    /** A multiplier on the automatic depth bias of the shadow maps, which follows their texel size.
        Raise it when lit surfaces show shadow acne, lower it when shadows detach from their casters.
        0 removes the receiver depth offset; the normal offset and the caster slope bias remain. Both tiers.
    */
    float shadowBias = 1.0f;

    /** True to add short screen-space contact shadows where objects meet, finer than the shadow maps
        resolve. Independent of castsShadows. Full tier only, with RenderSettings::contactShadows on:
        raster ignores it, which is not an error.
    */
    bool contactShadows = false;

    float contactShadowDistance = 0.2f;   ///< How far a contact shadow ray marches towards the light, in world units, negative counts as 0. Full tier only, see contactShadows.
    float contactShadowThickness = 0.05f; ///< How deep a surface hit by a contact shadow ray is assumed to be, in world units, negative counts as 0. Full tier only, see contactShadows.
};

} // namespace yup
