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

struct ModWheelComponentIds
{
    static inline const Identifier value { "value" };
    static inline const Identifier defaultValue { "defaultValue" };
    static inline const Identifier midiChannel { "midiChannel" };
    static inline const Identifier mouseDragSensitivity { "mouseDragSensitivity" };
};

} // namespace

//==============================================================================
const Identifier ModWheelComponent::Style::bodyTopColorId { "ModWheel_bodyTopColorId" };
const Identifier ModWheelComponent::Style::bodyBottomColorId { "ModWheel_bodyBottomColorId" };
const Identifier ModWheelComponent::Style::outlineColorId { "ModWheel_outlineColorId" };
const Identifier ModWheelComponent::Style::gripColorId { "ModWheel_gripColorId" };
const Identifier ModWheelComponent::Style::gripOverColorId { "ModWheel_gripOverColorId" };
const Identifier ModWheelComponent::Style::gripDownColorId { "ModWheel_gripDownColorId" };

//==============================================================================
const ComponentTypeInfo& ModWheelComponent::getClassTypeInfo()
{
    using W = ModWheelComponent;

    static const auto info = ComponentTypeBuilder<W> (Identifier ("ModWheel"))
                                 .color ("bodyTopColor", Style::bodyTopColorId)
                                 .color ("bodyBottomColor", Style::bodyBottomColorId)
                                 .color ("outlineColor", Style::outlineColorId)
                                 .color ("gripColor", Style::gripColorId)
                                 .color ("gripOverColor", Style::gripOverColorId)
                                 .color ("gripDownColor", Style::gripDownColorId)
                                 .property (ModWheelComponentIds::defaultValue, &W::getDefaultValue, &W::setDefaultValue, 0.0, "The position the wheel returns to on a double click.")
                                 .property (ModWheelComponentIds::value, &W::getValue, [] (W& w, double v) { w.setValue (v); }, 0.0, "The position of the wheel, from 0 to 1.")
                                 .property (ModWheelComponentIds::midiChannel, &W::getMidiChannel, [] (W& w, int v) { w.setMidiChannel (jlimit (1, 16, v)); }, 1, "The midi channel whose modulation wheel value the wheel follows, from 1 to 16.")
                                 .property (ModWheelComponentIds::mouseDragSensitivity, &W::getMouseDragSensitivity, &W::setMouseDragSensitivity, 1.0, "How far a drag moves the wheel: 1 takes the height of the wheel across its whole range.")
                                 .signal ("onValueChanged", &W::onValueChanged, "Emitted with the new position of the wheel, except when it follows its state.", { { Identifier ("value"), ComponentProperty::Type::number } })
                                 .build();

    return info;
}

//==============================================================================
ModWheelComponent::ModWheelComponent (StringRef componentID)
    : MidiWheelComponent (componentID, Source::modulationWheel, false)
{
}

ModWheelComponent::ModWheelComponent (MidiKeyboardState& stateToUse, StringRef componentID)
    : ModWheelComponent (componentID)
{
    setState (&stateToUse);
}

//==============================================================================
void ModWheelComponent::paint (Graphics& g)
{
    // Stays in the class: a subclass is painted with the style of the wheel type named here
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

} // namespace yup
