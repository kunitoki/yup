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
/** A node that lights the scene: the `Light3D` of a `.yui` document.

    The light shines down the -z axis of its entity, so a directional or a spot light is aimed
    with the `rotation` or the `lookAt` of its node, and a point or a spot light is placed with
    its `position`:

    @code
    Scene3D {
        Light3D { type: directional; rotation: [-45deg, 30deg, 0]; intensity: 3; castsShadows: true; }
        Light3D { type: spot; position: [0, 2, 0]; lookAt: [0, 0, 0]; color: #ffd9a0; intensity: 8; outerCone: 35deg; }
    }
    @endcode

    The `color` is a color of the interface, sRGB encoded: the light gets it as the linear RGB
    it shades with, and the property reads back as it was set. The shadow bias and the contact
    shadows of LightNode have no property: set them on the LightNode of getEntity().

    @see SceneNodeComponent, LightNode
*/
class YUP_API SceneLightComponent : public SceneNodeComponent
{
public:
    //==============================================================================
    /** Creates a white directional light.

        @param componentID The ID of the component.
    */
    explicit SceneLightComponent (StringRef componentID = {});

    //==============================================================================
    /** Changes the kind of light, the `type` of a `Light3D`. */
    void setType (LightNode::Type newType);

    /** Returns the kind of light. */
    LightNode::Type getType() const noexcept { return settings.type; }

    /** Changes the color of the light, the `color` of a `Light3D`.

        @param newColor The color, sRGB encoded as the colors of the interface are. Its alpha is ignored.
    */
    void setLightColor (Color newColor);

    /** Returns the color of the light, as it was set. */
    Color getLightColor() const noexcept { return color; }

    /** Changes the brightness of the light, the `intensity` of a `Light3D`.

        @param newIntensity The brightness, in lux for a directional light and in candela for a
                            point or a spot light.
    */
    void setIntensity (float newIntensity);

    /** Returns the brightness of the light. */
    float getIntensity() const noexcept { return settings.intensity; }

    /** Changes the distance where a point or a spot light reaches zero, the `range` of a `Light3D`.

        @param newRange The distance, or 0 for the light to fade with the distance alone.
    */
    void setRange (float newRange);

    /** Returns the distance where a point or a spot light reaches zero, 0 for none. */
    float getRange() const noexcept { return settings.range; }

    /** Changes the angle from its axis where a spot light starts to fade, the `innerCone` of a `Light3D`.

        @param newAngle The angle in radians.
    */
    void setInnerConeAngle (float newAngle);

    /** Returns the angle from its axis where a spot light starts to fade, in radians. */
    float getInnerConeAngle() const noexcept { return settings.innerConeAngle; }

    /** Changes the angle from its axis where a spot light ends, the `outerCone` of a `Light3D`.

        @param newAngle The angle in radians.
    */
    void setOuterConeAngle (float newAngle);

    /** Returns the angle from its axis where a spot light ends, in radians. */
    float getOuterConeAngle() const noexcept { return settings.outerConeAngle; }

    /** Changes the size of the emitter, the `radius` of a `Light3D`.

        @param newRadius The radius of the sphere a point or a spot light shines from, in world
                         units, the angular radius in radians of a directional light, or 0 for a
                         light without a size.
    */
    void setRadius (float newRadius);

    /** Returns the size of the emitter, 0 for a light without a size. */
    float getRadius() const noexcept { return settings.radius; }

    /** Chooses whether the light casts shadows, the `castsShadows` of a `Light3D`. */
    void setCastingShadows (bool shouldCastShadows);

    /** Returns true if the light casts shadows. */
    bool isCastingShadows() const noexcept { return settings.castsShadows; }

    //==============================================================================
    /** Returns the static type info describing the Light3D type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

protected:
    //==============================================================================
    /** @internal */
    void entityChanged() override;

private:
    //==============================================================================
    template <class Value>
    void setValue (Value& value, const Value& newValue, const Identifier& property);

    void updateLight();

    LightNode settings; // The values the LightNode of the entity is given
    Color color { 0xffffffff };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneLightComponent)
};

} // namespace yup
