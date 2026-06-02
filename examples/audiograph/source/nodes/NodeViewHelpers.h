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

namespace NodeViewHelpers
{

inline void configureParameterSlider (yup::Slider& slider, yup::Color accent)
{
    slider.setSliderType (yup::Slider::LinearBarHorizontal);
    slider.setTextBoxStyle (yup::Slider::NoTextBox);
    slider.setColor (yup::Slider::Style::backgroundColorId, yup::Color (0xff26282c));
    slider.setColor (yup::Slider::Style::trackColorId, yup::Color (0xff26282c).withAlpha (0.65f));
    slider.setColor (yup::Slider::Style::thumbColorId, accent);
    slider.setColor (yup::Slider::Style::thumbOverColorId, accent.brighter (0.15f));
    slider.setColor (yup::Slider::Style::thumbDownColorId, accent.darker (0.15f));
}

inline yup::Rectangle<float> getInlineSliderBounds (const yup::Component& component, int preferredWidth, int rowIndex)
{
    const auto bounds = component.getLocalBounds();
    const auto scale = bounds.getWidth() / static_cast<float> (preferredWidth);
    return { 62.0f * scale,
             (49.0f + 25.0f * static_cast<float> (rowIndex)) * scale,
             yup::jmax (42.0f * scale, bounds.getWidth() - (150.0f * scale)),
             20.0f * scale };
}

inline yup::AudioParameter::Ptr createParameter (yup::StringRef id,
                                                 yup::StringRef name,
                                                 float minValue,
                                                 float maxValue,
                                                 float defaultValue,
                                                 float interval = 0.0f)
{
    auto builder = yup::AudioParameterBuilder()
                       .withID (id)
                       .withName (name)
                       .withRange (yup::NormalisableRange<float> (minValue, maxValue, interval))
                       .withDefault (defaultValue);

    if (interval > 0.0f)
        builder.withStepped (true);

    return builder.build();
}

inline yup::DataTree createParameterState (const yup::Identifier& type, yup::Span<const yup::AudioParameter::Ptr> parameters)
{
    yup::DataTree state (type);
    auto transaction = state.beginTransaction();
    transaction.setProperty ("version", 1);

    for (const auto& parameter : parameters)
        if (parameter != nullptr)
            transaction.setProperty (parameter->getID(), parameter->getValue());

    return state;
}

inline yup::Result loadParameterState (const yup::DataTree& state,
                                       const yup::Identifier& expectedType,
                                       yup::Span<const yup::AudioParameter::Ptr> parameters)
{
    if (! state.isValid() || state.getType() != expectedType)
        return yup::Result::fail ("Invalid node state");

    if (static_cast<int> (state.getProperty ("version", 0)) != 1)
        return yup::Result::fail ("Unsupported node state version");

    for (const auto& parameter : parameters)
    {
        if (parameter == nullptr)
            continue;

        const yup::Identifier propertyName (parameter->getID());
        if (state.hasProperty (propertyName))
            parameter->setValue (static_cast<float> (static_cast<double> (state.getProperty (propertyName, parameter->getValue()))));
    }

    return yup::Result::ok();
}

} // namespace NodeViewHelpers
