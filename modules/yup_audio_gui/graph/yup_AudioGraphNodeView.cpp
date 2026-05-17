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
constexpr float baseHeaderHeight = 32.0f;
constexpr float basePortRowHeight = 24.0f;
constexpr float basePortTopPadding = 8.0f;
constexpr float baseParameterRowHeight = 25.0f;
constexpr float basePortRadius = 6.0f;
constexpr float baseContentHeight = 8.0f;

} // namespace

//==============================================================================
const Identifier AudioGraphNodeView::Style::shadowColorId ("audioGraphNodeShadow");
const Identifier AudioGraphNodeView::Style::accentBackgroundColorId ("audioGraphNodeAccentBackground");
const Identifier AudioGraphNodeView::Style::bodyBackgroundColorId ("audioGraphNodeBodyBackground");
const Identifier AudioGraphNodeView::Style::headerBackgroundColorId ("audioGraphNodeHeaderBackground");
const Identifier AudioGraphNodeView::Style::textColorId ("audioGraphNodeText");
const Identifier AudioGraphNodeView::Style::subtitleTextColorId ("audioGraphNodeSubtitleText");
const Identifier AudioGraphNodeView::Style::parameterBackgroundColorId ("audioGraphNodeParameterBackground");
const Identifier AudioGraphNodeView::Style::parameterValueBackgroundColorId ("audioGraphNodeParameterValueBackground");
const Identifier AudioGraphNodeView::Style::portHoleColorId ("audioGraphNodePortHole");

//==============================================================================
AudioGraphNodeView::AudioGraphNodeView (AudioGraphNodeID nodeIDIn)
    : nodeID (nodeIDIn)
{
    enableRenderingUnclipped (true);
    setWantsMouseEvents (true, true);
}

AudioGraphNodeView::~AudioGraphNodeView() = default;

//==============================================================================
Color AudioGraphNodeView::getNodeColor() const
{
    return Colors::dimgray;
}

String AudioGraphNodeView::getNodeSubtitle() const
{
    return {};
}

AudioGraphNodeView::PortInfo AudioGraphNodeView::getInputPortInfo (int busIndex) const
{
    return { String ("in ") + String (busIndex + 1), getPortKindColor (PortKind::audio), PortKind::audio };
}

AudioGraphNodeView::PortInfo AudioGraphNodeView::getOutputPortInfo (int busIndex) const
{
    return { String ("out ") + String (busIndex + 1), getPortKindColor (PortKind::audio), PortKind::audio };
}

int AudioGraphNodeView::getNumParameterRows() const
{
    return 0;
}

AudioGraphNodeView::ParameterInfo AudioGraphNodeView::getParameterInfo (int parameterIndex) const
{
    return { String ("param ") + String (parameterIndex + 1), {}, getPortKindColor (PortKind::parameter), -1.0f, PortKind::parameter };
}

void AudioGraphNodeView::paintNodeContent (Graphics&, Rectangle<float>) const
{
}

int AudioGraphNodeView::getPreferredWidth() const
{
    return 140;
}

int AudioGraphNodeView::getPreferredHeight() const
{
    // TODO - this needs to be moved to the style somehow
    const auto portRows = jmax (1, jmax (getNumInputPorts(), getNumOutputPorts()));
    return roundToInt (baseHeaderHeight
                       + baseContentHeight
                       + (static_cast<float> (getNumParameterRows()) * baseParameterRowHeight)
                       + basePortTopPadding
                       + (static_cast<float> (portRows) * basePortRowHeight)
                       + 10.0f);
}

Point<float> AudioGraphNodeView::getInputPortCenter (int busIndex) const
{
    return getPortCenter (busIndex, true);
}

Point<float> AudioGraphNodeView::getOutputPortCenter (int busIndex) const
{
    return getPortCenter (busIndex, false);
}

float AudioGraphNodeView::getPortRadius() const
{
    return basePortRadius * viewScale;
}

std::optional<AudioGraphNodeView::PortHit> AudioGraphNodeView::hitTestPort (Point<float> localPos) const
{
    const auto radiusSquared = getPortRadius() * getPortRadius();

    for (int i = 0; i < getNumInputPorts(); ++i)
    {
        if (localPos.distanceToSquared (getInputPortCenter (i)) <= radiusSquared)
            return PortHit { i, true };
    }

    for (int i = 0; i < getNumOutputPorts(); ++i)
    {
        if (localPos.distanceToSquared (getOutputPortCenter (i)) <= radiusSquared)
            return PortHit { i, false };
    }

    return {};
}

Color AudioGraphNodeView::getPortKindColor (PortKind kind)
{
    switch (kind)
    {
        case PortKind::audio:
            return Color (0xffffc43b);
        case PortKind::midi:
            return Color (0xffff4d67);
        case PortKind::parameter:
            return Color (0xff2f8cff);
    }

    return Colors::dimgray;
}

void AudioGraphNodeView::setViewScale (float newScale)
{
    viewScale = jlimit (0.1f, 4.0f, newScale);
}

//==============================================================================
void AudioGraphNodeView::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

Point<float> AudioGraphNodeView::getPortCenter (int busIndex, bool isInput) const
{
    const auto portCount = isInput ? getNumInputPorts() : getNumOutputPorts();
    if (busIndex < 0 || busIndex >= portCount)
        return {};

    const auto portArea = getPortArea();
    const auto y = portArea.getY() + basePortTopPadding * viewScale + (static_cast<float> (busIndex) + 0.5f) * basePortRowHeight * viewScale;
    const auto x = isInput ? portArea.getX() : portArea.getRight();
    return { x, y };
}

Rectangle<float> AudioGraphNodeView::getPortArea() const
{
    const auto bodyBounds = getLocalBounds().reduced (getPortRadius() * 0.5f + 2.0f * viewScale, 2.0f * viewScale);
    const auto y = bodyBounds.getY() + (baseHeaderHeight + baseContentHeight + 4.0f + static_cast<float> (getNumParameterRows()) * baseParameterRowHeight) * viewScale;
    return { bodyBounds.getX(), y, bodyBounds.getWidth(), jmax (0.0f, bodyBounds.getBottom() - y - 6.0f * viewScale) };
}

} // namespace yup
