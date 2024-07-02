/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

Slider::Slider (StringRef componentID, const Font& font)
    : Component (componentID)
    , font (font)
{
    setValue (0.0f);
}

//==============================================================================

void Slider::setValue (float newValue)
{
    value = jlimit (0.0f, 1.0f, newValue);

    sendValueChanged();

    updateRenderItems (false);
}

float Slider::getValue() const
{
    return value;
}

void Slider::valueChanged() {}

//==============================================================================

void Slider::resized()
{
    updateRenderItems (true);
}

//==============================================================================

void Slider::mouseEnter (const MouseEvent& event)
{
    isInside = true;

    repaint();
}

void Slider::mouseExit (const MouseEvent& event)
{
    isInside = false;

    repaint();
}

void Slider::mouseDown (const MouseEvent& event)
{
    origin = event.getPosition();

    takeFocus();

    repaint();
}

void Slider::mouseUp (const MouseEvent& event)
{
}

void Slider::mouseDrag (const MouseEvent& event)
{
    //auto [x, y] = event.getPosition();

    const float multiplier =
        (event.getModifiers().isShiftDown() || event.isRightButtonDown()) ? 0.0001f : 0.0025f;

    const float distance = -origin.verticalDistanceTo (event.getPosition()) * multiplier;

    origin = event.getPosition();

    setValue (value + distance);

    repaint();
}

void Slider::mouseWheel (const MouseEvent& event, const MouseWheelData& data)
{
    //auto [x, y] = event.getPosition();

    const float multiplier = event.getModifiers().isShiftDown() ? 0.001f : 0.0025f;
    const float distance = (data.getDeltaX() + data.getDeltaY()) * multiplier;

    origin = event.getPosition();

    setValue (value + distance);

    repaint();
}

//==============================================================================

void Slider::paint (Graphics& g)
{
    auto bounds = getLocalBounds().reduced (proportionOfWidth (0.1f));

    g.setFillColor (Color (0xff3d3d3d));
    g.fillPath (backgroundPath);

    g.setStrokeColor (Color (0xff2b2b2b));
    g.setStrokeWidth (proportionOfWidth (0.0175f));
    g.strokePath (backgroundPath);

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (Color (0xff636363));
    g.setStrokeWidth (proportionOfWidth (0.075f));
    g.strokePath (backgroundArc);

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (isInside ? Color (0xff4ebfff).brighter (0.3f) : Color (0xff4ebfff));
    g.setStrokeWidth (proportionOfWidth (0.075f));
    g.strokePath (foregroundArc);

    g.setStrokeCap (StrokeCap::Round);
    g.setStrokeColor (Color (0xffffffff));
    g.setStrokeWidth (proportionOfWidth (0.03f));
    g.strokePath (foregroundLine);

    g.setStrokeColor (Color (0xffffffff));
    g.strokeFittedText (text, getLocalBounds().reduced (5).removeFromBottom (proportionOfWidth (0.1f)));

    //if (hasFocus())
    //{
    //    g.setStrokeColor (Color (0xffff5f2b));
    //    g.strokeRect (getLocalBounds(), proportionOfWidth (0.0175f));
    //}
}

//==============================================================================

void Slider::updateRenderItems (bool forceAll)
{
    auto bounds = getLocalBounds().reduced (proportionOfWidth (0.1f));
    const auto center = bounds.getCenter();

    constexpr auto fromRadians = degreesToRadians (135.0f);
    constexpr auto toRadians = fromRadians + degreesToRadians (270.0f);

    if (forceAll)
    {
        backgroundPath.clear();
        backgroundPath.addEllipse (bounds.reduced (proportionOfWidth (0.105f)));

        backgroundArc.clear();
        backgroundArc.addCenteredArc (center,
                                      bounds.getWidth() / 2.0f,
                                      bounds.getHeight() / 2.0f,
                                      0.0f,
                                      fromRadians,
                                      toRadians,
                                      true);
    }

    const auto toCurrentRadians = fromRadians + degreesToRadians (270.0f) * value;

    foregroundArc.clear();
    foregroundArc.addCenteredArc (center,
                                  bounds.getWidth() / 2.0f,
                                  bounds.getHeight() / 2.0f,
                                  0.0f,
                                  fromRadians,
                                  toCurrentRadians,
                                  true);

    const auto reducedBounds = bounds.reduced (proportionOfWidth (0.175f));
    const auto pos = center.getPointOnCircumference (
        reducedBounds.getWidth() / 2.0f,
        reducedBounds.getHeight() / 2.0f,
        toCurrentRadians);

    foregroundLine.clear();
    foregroundLine.addLine (Line<float> (pos, center).keepOnlyStart (0.25f));

    /*
    if (font.getFont() != nullptr)
    {
        text.clear();
        text.appendText (font, proportionOfHeight(0.1f), proportionOfHeight(0.1f), String (value, 3).toRawUTF8());
        text.layout (getLocalBounds().reduced (5).removeFromBottom (proportionOfWidth (0.1f)), StyledText::center);
    }
    */
}

//==============================================================================

void Slider::sendValueChanged()
{
    valueChanged();

    if (onValueChanged)
        onValueChanged (getValue());
}

} // namespace yup
