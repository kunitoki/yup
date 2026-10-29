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

#include "SynthTimeGrid.h"
#include "SynthTheme.h"

#include "../dsp/SynthFormat.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace SynthTimeGrid
{
namespace
{
constexpr float minorSpacing = 14.0f;
constexpr float labelSpacing = 56.0f;
constexpr int maxLines = 400;
constexpr double epsilon = 1.0e-6;

bool divides (double step, double length)
{
    const auto count = length / step;
    return std::abs (count - std::round (count)) < epsilon;
}

/** The candidate steps in seconds or quarter note beats, shortest first. Musical steps
    shorter than a bar divide it, so their lines fall on every bar. */
std::vector<double> getSteps (const Timing& timing)
{
    std::vector<double> steps;

    if (! timing.musical)
    {
        for (int exponent = -3; exponent <= 2; ++exponent)
            for (const auto mantissa : { 1.0, 2.0, 5.0 })
                steps.push_back (mantissa * std::pow (10.0, exponent));

        return steps;
    }

    const auto bar = timing.getBarLength();

    for (const auto step : { 0.0625, 0.125, 0.25, 0.5, 1.0, 2.0, timing.getBeatLength() })
        if (step < bar && divides (step, bar))
            steps.push_back (step);

    for (const auto bars : { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0 })
        steps.push_back (bar * bars);

    std::sort (steps.begin(), steps.end());
    steps.erase (std::unique (steps.begin(), steps.end(), [] (double a, double b) { return std::abs (a - b) < epsilon; }), steps.end());
    return steps;
}

double pickStep (const std::vector<double>& steps, double pixelsPerStep, float spacing, double atLeast)
{
    for (const auto step : steps)
        if (step >= atLeast - epsilon && step * pixelsPerStep >= spacing)
            return step;

    return steps.back();
}

struct MusicalPosition
{
    int bar = 0;
    int beat = 0;
    int sixteenth = 0;
    int sixtyFourth = 0;
    bool onBar = false;
};

MusicalPosition toMusical (double beats, const Timing& timing)
{
    const auto bar = timing.getBarLength();
    const auto beat = timing.getBeatLength();

    MusicalPosition position;
    position.bar = static_cast<int> (std::floor (beats / bar + epsilon));
    auto rest = yup::jmax (0.0, beats - position.bar * bar);
    position.onBar = rest < epsilon;
    position.beat = static_cast<int> (std::floor (rest / beat + epsilon));
    rest = yup::jmax (0.0, rest - position.beat * beat);
    position.sixteenth = static_cast<int> (std::floor (rest / 0.25 + epsilon));
    rest = yup::jmax (0.0, rest - position.sixteenth * 0.25);
    position.sixtyFourth = static_cast<int> (std::floor (rest / 0.0625 + epsilon));
    return position;
}

std::array<double, 9> getMusicalSnapSteps (const Timing& timing)
{
    return { 0.0, timing.getBarLength(), 2.0, 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
}

constexpr std::array<double, 6> secondSnapSteps { 0.0, 1.0, 0.5, 0.1, 0.05, 0.01 };

} // namespace

//==============================================================================
std::vector<Line> getLines (const Timing& timing, float length, float width)
{
    std::vector<Line> lines;
    const auto total = static_cast<double> (length) * timing.unitLength;

    if (total <= 0.0 || width <= 0.0f)
        return lines;

    const auto steps = getSteps (timing);
    const auto pixels = static_cast<double> (width) / total;
    const auto minor = pickStep (steps, pixels, minorSpacing, 0.0);
    const auto major = pickStep (steps, pixels, labelSpacing, minor);

    for (int index = 1; index <= maxLines && index * minor <= total + epsilon; ++index)
        if (const auto position = index * minor; ! divides (major, position))
            lines.push_back ({ static_cast<float> (position / timing.unitLength), false, {} });

    for (int index = 0; index <= maxLines && index * major <= total + epsilon; ++index)
    {
        const auto position = index * major;
        yup::String label;

        if (timing.musical)
        {
            const auto musical = toMusical (position, timing);
            label = yup::String (musical.bar + 1);

            if (! musical.onBar)
            {
                label << "." << yup::String (musical.beat + 1);

                if (! divides (timing.getBeatLength(), position - musical.bar * timing.getBarLength()))
                {
                    label << "." << yup::String (musical.sixteenth + 1);

                    if (musical.sixtyFourth > 0)
                        label << "." << yup::String (musical.sixtyFourth + 1);
                }
            }
        }
        else
        {
            label = SynthFormat::milliseconds (position);
        }

        lines.push_back ({ static_cast<float> (position / timing.unitLength), true, label });
    }

    return lines;
}

yup::StringArray getSnapNames (bool musical)
{
    if (musical)
        return { "Off", "1 Bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/8 T", "1/16 T" };

    return { "Off", "1 s", "500 ms", "100 ms", "50 ms", "10 ms" };
}

float snap (float time, const Timing& timing, int snapChoice)
{
    auto step = 0.0;

    if (timing.musical && yup::isPositiveAndBelow (snapChoice, static_cast<int> (getMusicalSnapSteps (timing).size())))
        step = getMusicalSnapSteps (timing)[static_cast<std::size_t> (snapChoice)];
    else if (! timing.musical && yup::isPositiveAndBelow (snapChoice, static_cast<int> (secondSnapSteps.size())))
        step = secondSnapSteps[static_cast<std::size_t> (snapChoice)];

    if (step <= 0.0 || timing.unitLength <= 0.0)
        return time;

    const auto position = static_cast<double> (time) * timing.unitLength;
    return static_cast<float> (std::round (position / step) * step / timing.unitLength);
}

yup::String format (float time, const Timing& timing)
{
    const auto position = static_cast<double> (time) * timing.unitLength;

    if (! timing.musical)
        return SynthFormat::milliseconds (position);

    const auto musical = toMusical (position, timing);
    return yup::String (musical.bar + 1) + "." + yup::String (musical.beat + 1) + "." + yup::String (musical.sixteenth + 1);
}

void paint (yup::Graphics& g,
            const yup::Component& scope,
            const std::vector<Line>& lines,
            yup::Rectangle<float> area,
            const yup::Font& font,
            const std::function<std::optional<float> (float)>& toX)
{
    const auto color = SynthTheme::textPrimary (scope);
    auto labelRight = area.getX() - 4.0f;

    for (const auto& line : lines)
    {
        const auto x = toX (line.time);

        if (! x.has_value() || *x < area.getX() || *x > area.getRight())
            continue;

        g.setFillColor (color.withAlpha (line.major ? 0.12f : 0.05f));
        g.fillRect (*x, area.getY(), 1.0f, area.getHeight());

        if (line.label.isEmpty())
            continue;

        const auto labelWidth = yup::jmin (labelSpacing - 6.0f, area.getWidth());
        const auto labelX = yup::jlimit (area.getX(), area.getRight() - labelWidth, *x + 3.0f);

        if (labelX < labelRight + 4.0f)
            continue;

        labelRight = labelX + labelWidth;
        g.setFillColor (SynthTheme::textSecondary (scope).withAlpha (0.7f));
        g.fillFittedText (line.label, font, yup::Rectangle<float> (labelX, area.getY() + 1.0f, labelWidth, 12.0f), yup::Justification::topLeft);
    }
}

} // namespace SynthTimeGrid
