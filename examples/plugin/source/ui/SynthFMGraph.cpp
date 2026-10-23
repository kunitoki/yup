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

#include "SynthFMGraph.h"
#include "SynthTheme.h"

SynthFMGraph::SynthFMGraph()
{
    setOpaque (false);
}

void SynthFMGraph::setState (const State& newState)
{
    if (newState == state)
        return;

    state = newState;
    repaint();
}

void SynthFMGraph::setNodePositions (const Row& positions)
{
    nodeX = positions;
    repaint();
}

void SynthFMGraph::paint (yup::Graphics& g)
{
    const auto bounds = getLocalBounds();
    SynthTheme::paintWell (g, *this, bounds);

    const auto area = bounds.reduced (8.0f);

    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return;

    const auto nodeY = area.getY() + area.getHeight() * 0.48f;
    const auto busY = area.getBottom() - 4.0f;
    const auto above = nodeY - nodeRadius - area.getY() - 2.0f;
    const auto below = busY - nodeY - nodeRadius - 8.0f;
    const auto accent = SynthTheme::accent (*this);
    const auto feedbackColor = SynthTheme::lfoAccent();
    const auto count = SynthExample::oscillatorCount;

    const auto alphaOf = [this] (int op) { return state.enabled[static_cast<std::size_t> (op)] ? 1.0f : 0.35f; };

    g.setFillColor (SynthTheme::textPrimary (*this).withAlpha (0.08f));
    g.fillRect (area.getX(), nodeY, area.getWidth(), 1.0f);

    g.setStrokeCap (yup::StrokeCap::Round);

    // Links, each from its modulator to its carrier, with a head at the carrier
    for (int carrier = 0; carrier < count; ++carrier)
    {
        for (int modulator = 0; modulator < count; ++modulator)
        {
            const auto amount = state.amounts[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)];

            if (modulator == carrier || amount <= 0.0f)
                continue;

            const auto from = nodeX[static_cast<std::size_t> (modulator)];
            const auto to = nodeX[static_cast<std::size_t> (carrier)];
            const auto isAbove = modulator > carrier;
            const auto side = isAbove ? -1.0f : 1.0f;
            const auto lift = yup::jmin (isAbove ? above : below, 8.0f + std::abs (to - from) * 0.3f) / 0.75f;
            const auto startY = nodeY + side * nodeRadius;

            const yup::Point<float> start { from, startY };
            const yup::Point<float> end { to, startY };
            const yup::Point<float> control1 { from, startY + side * lift };
            const yup::Point<float> control2 { to, startY + side * lift };

            yup::Path arc;
            arc.moveTo (start);
            arc.cubicTo (control1.getX(), control1.getY(), control2.getX(), control2.getY(), end.getX(), end.getY());

            const auto color = accent.withAlpha ((0.35f + 0.65f * amount) * yup::jmin (alphaOf (modulator), alphaOf (carrier)));
            g.setStrokeColor (color);
            g.setStrokeWidth (1.0f + 2.5f * amount);
            g.strokePath (arc);

            yup::Path head;
            head.moveTo (end);
            head.lineTo (end.getX() - 3.5f, end.getY() + side * 6.0f);
            head.lineTo (end.getX() + 3.5f, end.getY() + side * 6.0f);
            head.close();
            g.setFillColor (color);
            g.fillPath (head);
        }
    }

    // Feedback loops and the stems of the operators that are heard
    auto busStart = area.getRight();
    auto busEnd = area.getX();

    for (int op = 0; op < count; ++op)
    {
        const auto x = nodeX[static_cast<std::size_t> (op)];
        const auto feedback = state.amounts[static_cast<std::size_t> (op)][static_cast<std::size_t> (op)];

        if (feedback > 0.0f)
        {
            g.setStrokeColor (feedbackColor.withAlpha ((0.35f + 0.65f * feedback) * alphaOf (op)));
            g.setStrokeWidth (1.0f + 2.0f * feedback);
            g.strokeEllipse (x + nodeRadius * 0.4f, nodeY - nodeRadius * 1.9f, nodeRadius * 1.2f, nodeRadius * 1.2f);
        }

        if (state.levels[static_cast<std::size_t> (op)] <= 0.0f)
            continue;

        g.setStrokeColor (accent.withAlpha (alphaOf (op)));
        g.setStrokeWidth (1.0f + 1.5f * state.levels[static_cast<std::size_t> (op)]);
        g.strokeLine (x, nodeY + nodeRadius, x, busY);

        busStart = yup::jmin (busStart, x);
        busEnd = yup::jmax (busEnd, x);
    }

    if (busStart <= busEnd)
    {
        g.setStrokeColor (accent);
        g.setStrokeWidth (2.0f);
        g.strokeLine (busStart, busY, area.getRight() - 26.0f, busY);

        g.setFillColor (SynthTheme::textSecondary (*this));
        g.fillFittedText ("OUT", SynthTheme::captionFont (10.5f), { area.getRight() - 24.0f, busY - 7.0f, 24.0f, 14.0f }, yup::Justification::centerLeft);
    }

    // Nodes last, over the links
    for (int op = 0; op < count; ++op)
    {
        const auto x = nodeX[static_cast<std::size_t> (op)];
        const auto isHeard = state.levels[static_cast<std::size_t> (op)] > 0.0f;
        const yup::Rectangle<float> node { x - nodeRadius, nodeY - nodeRadius, nodeRadius * 2.0f, nodeRadius * 2.0f };
        const auto alpha = alphaOf (op);

        g.setFillColor (SynthTheme::wellColor (*this));
        g.fillEllipse (node);

        g.setStrokeColor ((isHeard ? accent : SynthTheme::textSecondary (*this)).withAlpha (alpha));
        g.setStrokeWidth (1.5f);
        g.strokeEllipse (node);

        g.setFillColor (SynthTheme::textPrimary (*this).withAlpha (alpha));
        g.fillFittedText (yup::String (op + 1), SynthTheme::valueFont (12.0f), node, yup::Justification::center);
    }
}
