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
namespace
{

struct SceneLightComponentIds
{
    static inline const Identifier type { "type" };
    static inline const Identifier color { "color" };
    static inline const Identifier intensity { "intensity" };
    static inline const Identifier range { "range" };
    static inline const Identifier innerCone { "innerCone" };
    static inline const Identifier outerCone { "outerCone" };
    static inline const Identifier radius { "radius" };
    static inline const Identifier castsShadows { "castsShadows" };
};

} // namespace

//==============================================================================

SceneLightComponent::SceneLightComponent (StringRef componentID)
    : SceneNodeComponent (componentID)
{
}

//==============================================================================

template <class Value>
void SceneLightComponent::setValue (Value& value, const Value& newValue, const Identifier& property)
{
    if (value == newValue)
        return;

    value = newValue;

    updateLight();
    notifyPropertyChanged (property);
}

void SceneLightComponent::setType (LightNode::Type newType)
{
    setValue (settings.type, newType, SceneLightComponentIds::type);
}

void SceneLightComponent::setLightColor (Color newColor)
{
    // The light shades with linear values, the colors of the interface are sRGB encoded
    settings.color = { lightingSrgbToLinear (newColor.getRedFloat()),
                       lightingSrgbToLinear (newColor.getGreenFloat()),
                       lightingSrgbToLinear (newColor.getBlueFloat()) };

    setValue (color, newColor, SceneLightComponentIds::color);
}

void SceneLightComponent::setIntensity (float newIntensity)
{
    setValue (settings.intensity, newIntensity, SceneLightComponentIds::intensity);
}

void SceneLightComponent::setRange (float newRange)
{
    setValue (settings.range, newRange, SceneLightComponentIds::range);
}

void SceneLightComponent::setInnerConeAngle (float newAngle)
{
    setValue (settings.innerConeAngle, newAngle, SceneLightComponentIds::innerCone);
}

void SceneLightComponent::setOuterConeAngle (float newAngle)
{
    setValue (settings.outerConeAngle, newAngle, SceneLightComponentIds::outerCone);
}

void SceneLightComponent::setRadius (float newRadius)
{
    setValue (settings.radius, newRadius, SceneLightComponentIds::radius);
}

void SceneLightComponent::setCastingShadows (bool shouldCastShadows)
{
    setValue (settings.castsShadows, shouldCastShadows, SceneLightComponentIds::castsShadows);
}

//==============================================================================

void SceneLightComponent::updateLight()
{
    auto entity = getEntity();

    if (! entity.isValid())
        return;

    auto& light = entity.getOrAttach<LightNode>();

    light.type = settings.type;
    light.color = settings.color;
    light.intensity = settings.intensity;
    light.range = settings.range;
    light.innerConeAngle = settings.innerConeAngle;
    light.outerConeAngle = settings.outerConeAngle;
    light.radius = settings.radius;
    light.castsShadows = settings.castsShadows;

    repaintScene();
}

void SceneLightComponent::entityChanged()
{
    updateLight();
}

//==============================================================================

const ComponentTypeInfo& SceneLightComponent::getClassTypeInfo()
{
    static const auto info = []
    {
        const LightNode defaults {};

        return ComponentTypeBuilder<SceneLightComponent> (Identifier ("Light3D"), SceneNodeComponent::getClassTypeInfo())
            .enumeration<LightNode::Type> (
                SceneLightComponentIds::type,
                { { "directional", LightNode::Type::directional },
                  { "point", LightNode::Type::point },
                  { "spot", LightNode::Type::spot } },
                &SceneLightComponent::getType,
                &SceneLightComponent::setType,
                "The kind of light: parallel rays down the -z axis of the node (directional), rays from its position in every direction (point), or within a cone around its -z axis (spot).")
            .styleableProperty (SceneLightComponentIds::color, &SceneLightComponent::getLightColor, &SceneLightComponent::setLightColor, Colors::white, "The color of the light.")
            .styleableProperty (SceneLightComponentIds::intensity, &SceneLightComponent::getIntensity, &SceneLightComponent::setIntensity, defaults.intensity, "The brightness of the light: in lux for a directional light, in candela for a point or a spot light.")
            .styleableProperty (SceneLightComponentIds::range, &SceneLightComponent::getRange, &SceneLightComponent::setRange, defaults.range, "The distance where a point or a spot light reaches zero, 0 for it to fade with the distance alone.")
            .styleableProperty (SceneLightComponentIds::innerCone, &SceneLightComponent::getInnerConeAngle, &SceneLightComponent::setInnerConeAngle, defaults.innerConeAngle, "The angle from its axis where a spot light starts to fade, in radians.")
            .styleableProperty (SceneLightComponentIds::outerCone, &SceneLightComponent::getOuterConeAngle, &SceneLightComponent::setOuterConeAngle, defaults.outerConeAngle, "The angle from its axis where a spot light ends, in radians.")
            .styleableProperty (SceneLightComponentIds::radius, &SceneLightComponent::getRadius, &SceneLightComponent::setRadius, defaults.radius, "The size of the emitter, which softens its highlights and its shadows: the radius of a point or a spot light in world units, the angular radius of a directional light in radians. 0 for a light without a size.")
            .styleableProperty (SceneLightComponentIds::castsShadows, &SceneLightComponent::isCastingShadows, &SceneLightComponent::setCastingShadows, defaults.castsShadows, "Whether the light casts the shadows of the opaque objects.")
            .build();
    }();

    return info;
}

} // namespace yup
