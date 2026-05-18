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
    slider.setColor (yup::Slider::Style::trackColorId, accent.withAlpha (0.65f));
    slider.setColor (yup::Slider::Style::thumbColorId, accent);
    slider.setColor (yup::Slider::Style::thumbOverColorId, accent.brighter (0.15f));
    slider.setColor (yup::Slider::Style::thumbDownColorId, accent.darker (0.15f));
}

inline yup::Rectangle<float> getInlineSliderBounds (const yup::Component& component, int preferredWidth)
{
    const auto bounds = component.getLocalBounds();
    const auto scale = bounds.getWidth() / static_cast<float> (preferredWidth);
    return { 62.0f * scale,
             49.0f * scale,
             yup::jmax (42.0f * scale, bounds.getWidth() - (150.0f * scale)),
             20.0f * scale };
}

} // namespace NodeViewHelpers
