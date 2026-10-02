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
    every visible entity contributes, up to the limit of the renderer.

    @see SceneRenderer, EntityNode
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
};

} // namespace yup
