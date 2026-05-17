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
constexpr float baseCornerRadius = 7.0f;
constexpr float baseContentHeight = 8.0f;

Rectangle<float> ellipseBounds (Point<float> center, float radius)
{
    return { center.getX() - radius, center.getY() - radius, radius * 2.0f, radius * 2.0f };
}

void fillFeatheredRoundedRect (Graphics& g, Rectangle<float> bounds, float corner, Color color, float viewScale)
{
    constexpr int numLayers = 5;

    for (int i = numLayers; i > 0; --i)
    {
        const auto amount = static_cast<float> (i) * 1.4f * viewScale;
        const auto alpha = 0.020f + (static_cast<float> (numLayers - i) * 0.018f);
        g.setFillColor (color.withAlpha (alpha));
        g.fillRoundedRect (bounds.reduced (-amount), corner + amount);
    }
}

void fillFeatheredEllipse (Graphics& g, Point<float> center, float radius, Color color)
{
    constexpr int numLayers = 4;

    for (int i = numLayers; i > 0; --i)
    {
        const auto layerRadius = radius * (1.0f + (static_cast<float> (i) * 0.22f));
        const auto alpha = 0.040f + (static_cast<float> (numLayers - i) * 0.045f);
        g.setFillColor (color.withAlpha (alpha));
        g.fillEllipse (ellipseBounds (center, layerRadius));
    }
}
} // namespace

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

void AudioGraphNodeView::paintNodeContent (Graphics&, Rectangle<float>)
{
}

int AudioGraphNodeView::getPreferredWidth() const
{
    return 140;
}

int AudioGraphNodeView::getPreferredHeight() const
{
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
    const auto bounds = getLocalBounds().reduced (getPortRadius() * 0.5f, 0.0f);
    const auto bodyBounds = bounds.reduced (2.0f * viewScale);
    const auto corner = baseCornerRadius * viewScale;
    const auto accent = getNodeColor();
    const auto canvasBackground = Color (0xff101522);
    const auto headerHeight = baseHeaderHeight * viewScale;

    fillFeatheredRoundedRect (g, bodyBounds.translated (0.0f, 3.0f * viewScale), corner, Colors::black, viewScale);

    g.setFillColor (accent.withAlpha (0.11f));
    g.fillRoundedRect (bodyBounds.reduced (-1.5f * viewScale), corner + 2.0f * viewScale);

    g.setFillColor (Color (0xff1e2535));
    g.fillRoundedRect (bodyBounds, corner);

    auto headerBounds = bodyBounds.withHeight (headerHeight);
    g.setFillColor (Color (0xff141a26));
    g.fillRoundedRect (headerBounds, corner, corner, 0.0f, 0.0f);

    g.setStrokeColor (accent.withAlpha (0.70f));
    g.setStrokeWidth (1.35f * viewScale);
    g.strokeRoundedRect (bodyBounds, corner);

    const auto headerInner = headerBounds.reduced (9.0f * viewScale, 5.0f * viewScale);

    const auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();
    g.setFillColor (accent);
    g.fillFittedText (getNodeTitle(), font.withHeight (12.0f * viewScale), headerInner.withHeight (18.0f * viewScale), Justification::left);

    const auto subtitle = getNodeSubtitle();
    if (subtitle.isNotEmpty())
    {
        g.setFillColor (Color (0xffb8b8b8));
        g.fillFittedText (subtitle, font.withHeight (9.0f * viewScale), headerInner.withHeight (18.0f * viewScale), Justification::right);
    }

    const auto ruleY = bodyBounds.getY() + headerHeight;
    g.setStrokeColor (accent.withAlpha (0.16f));
    g.strokeLine ({ bodyBounds.getX(), ruleY }, { bodyBounds.getRight(), ruleY });

    auto contentBounds = bodyBounds.reduced (10.0f * viewScale, 0.0f);
    contentBounds = contentBounds.withY (ruleY + 4.0f * viewScale).withHeight (baseContentHeight * viewScale);
    paintNodeContent (g, contentBounds);

    auto parameterBounds = bodyBounds.reduced (10.0f * viewScale, 0.0f);
    parameterBounds = parameterBounds.withY (ruleY + (baseContentHeight + 5.0f) * viewScale);

    const auto parameterFont = font.withHeight (9.5f * viewScale);
    for (int i = 0; i < getNumParameterRows(); ++i)
    {
        const auto info = getParameterInfo (i);
        auto row = parameterBounds.removeFromTop (baseParameterRowHeight * viewScale).reduced (0.0f, 2.0f * viewScale);
        auto valueBox = row.removeFromRight (58.0f * viewScale);

        g.setFillColor (Color (0xff263044));
        g.fillRoundedRect (row.reduced (0.0f, 1.0f * viewScale), 3.0f * viewScale);

        if (info.normalizedValue >= 0.0f)
        {
            const auto fillWidth = row.getWidth() * jlimit (0.0f, 1.0f, info.normalizedValue);
            g.setFillColor (info.color.withAlpha (0.20f));
            g.fillRoundedRect (row.withWidth (fillWidth).reduced (0.0f, 1.0f * viewScale), 3.0f * viewScale);
        }

        g.setFillColor (Color (0xffd1d5db));
        g.fillFittedText (info.name, parameterFont, row.reduced (6.0f * viewScale, 0.0f), Justification::left);

        g.setFillColor (Color (0xff1a2130));
        g.fillRoundedRect (valueBox.reduced (0.0f, 1.0f * viewScale), 3.0f * viewScale);
        g.setFillColor (info.color);
        g.fillFittedText (info.value, parameterFont, valueBox.reduced (5.0f * viewScale, 0.0f), Justification::right);
    }

    const auto labelFont = font.withHeight (10.5f * viewScale);
    const auto portRadius = getPortRadius();

    for (int i = 0; i < getNumInputPorts(); ++i)
    {
        const auto info = getInputPortInfo (i);
        const auto center = getInputPortCenter (i);

        fillFeatheredEllipse (g, center, portRadius * 1.25f, info.color);
        g.setFillColor (info.color);
        g.fillEllipse (ellipseBounds (center, portRadius));
        g.setFillColor (canvasBackground);
        g.fillEllipse (ellipseBounds (center, portRadius * 0.45f));
        g.setStrokeColor (info.color.withAlpha (0.72f));
        g.setStrokeWidth (1.2f * viewScale);
        g.strokeEllipse (ellipseBounds (center, portRadius * 1.15f));

        g.setFillColor (Color (0xffd6d6d6));
        g.fillFittedText (info.name, labelFont, { center.getX() + 12.0f * viewScale, center.getY() - 8.0f * viewScale, 72.0f * viewScale, 16.0f * viewScale }, Justification::left);
    }

    for (int i = 0; i < getNumOutputPorts(); ++i)
    {
        const auto info = getOutputPortInfo (i);
        const auto center = getOutputPortCenter (i);

        fillFeatheredEllipse (g, center, portRadius * 1.25f, info.color);
        g.setFillColor (info.color);
        g.fillEllipse (ellipseBounds (center, portRadius));
        g.setFillColor (canvasBackground);
        g.fillEllipse (ellipseBounds (center, portRadius * 0.45f));
        g.setStrokeColor (info.color.withAlpha (0.72f));
        g.setStrokeWidth (1.2f * viewScale);
        g.strokeEllipse (ellipseBounds (center, portRadius * 1.15f));

        g.setFillColor (Color (0xffd6d6d6));
        g.fillFittedText (info.name, labelFont, { center.getX() - 84.0f * viewScale, center.getY() - 8.0f * viewScale, 72.0f * viewScale, 16.0f * viewScale }, Justification::right);
    }
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
