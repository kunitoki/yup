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

namespace
{

struct PitchWheelComponentIds
{
    static inline const Identifier value { "value" };
    static inline const Identifier defaultValue { "defaultValue" };
    static inline const Identifier midiChannel { "midiChannel" };
    static inline const Identifier mouseDragSensitivity { "mouseDragSensitivity" };
    static inline const Identifier resetOnRelease { "resetOnRelease" };
};

} // namespace

//==============================================================================
const Identifier PitchWheelComponent::Style::bodyTopColorId { "PitchWheel_bodyTopColorId" };
const Identifier PitchWheelComponent::Style::bodyBottomColorId { "PitchWheel_bodyBottomColorId" };
const Identifier PitchWheelComponent::Style::outlineColorId { "PitchWheel_outlineColorId" };
const Identifier PitchWheelComponent::Style::gripColorId { "PitchWheel_gripColorId" };
const Identifier PitchWheelComponent::Style::gripOverColorId { "PitchWheel_gripOverColorId" };
const Identifier PitchWheelComponent::Style::gripDownColorId { "PitchWheel_gripDownColorId" };

//==============================================================================
const ComponentTypeInfo& PitchWheelComponent::getClassTypeInfo()
{
    using W = PitchWheelComponent;

    static const auto info = ComponentTypeBuilder<W> (Identifier ("PitchWheel"))
                                 .color ("bodyTopColor", Style::bodyTopColorId)
                                 .color ("bodyBottomColor", Style::bodyBottomColorId)
                                 .color ("outlineColor", Style::outlineColorId)
                                 .color ("gripColor", Style::gripColorId)
                                 .color ("gripOverColor", Style::gripOverColorId)
                                 .color ("gripDownColor", Style::gripDownColorId)
                                 .property (PitchWheelComponentIds::defaultValue, &W::getDefaultValue, &W::setDefaultValue, 0.0, "The position the wheel returns to on a double click, and on release when it resets.")
                                 .property (PitchWheelComponentIds::value, &W::getValue, [] (W& w, double v) { w.setValue (v); }, 0.0, "The position of the wheel, from -1 to 1.")
                                 .property (PitchWheelComponentIds::midiChannel, &W::getMidiChannel, [] (W& w, int v) { w.setMidiChannel (jlimit (1, 16, v)); }, 1, "The midi channel whose pitch-wheel position the wheel follows, from 1 to 16.")
                                 .property (PitchWheelComponentIds::mouseDragSensitivity, &W::getMouseDragSensitivity, &W::setMouseDragSensitivity, 1.0, "How far a drag moves the wheel: 1 takes the height of the wheel across its whole range.")
                                 .property (PitchWheelComponentIds::resetOnRelease, &W::getResetOnRelease, &W::setResetOnRelease, true, "Whether the wheel springs back to its default position when released.")
                                 .signal ("onValueChanged", &W::onValueChanged, "Emitted with the new position of the wheel, except when it follows its state.", { { Identifier ("value"), ComponentProperty::Type::number } })
                                 .build();

    return info;
}

//==============================================================================
PitchWheelComponent::PitchWheelComponent (StringRef componentID)
    : MidiWheelComponent (componentID, Source::pitchWheel, true)
{
}

PitchWheelComponent::PitchWheelComponent (MidiKeyboardState& stateToUse, StringRef componentID)
    : PitchWheelComponent (componentID)
{
    setState (&stateToUse);
}

//==============================================================================
void PitchWheelComponent::paint (Graphics& g)
{
    // Stays in the class: a subclass is painted with the style of the wheel type named here
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

} // namespace yup
