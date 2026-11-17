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
ColorGradient ColorGradient::withSpreadExpandedOver (const Rectangle<float>& area) const
{
    constexpr int maxLinearRepeats = 32;
    constexpr int maxRadialRepeats = 64;

    auto padded = withSpread (Spread::Pad);

    if (spread == Spread::Pad || type == Type::Angular || stops.size() < 2 || area.isEmpty())
        return padded;

    std::array<Point<float>, 4> corners { area.getTopLeft(), area.getTopRight(), area.getBottomLeft(), area.getBottomRight() };

    if (! transform.isIdentity())
    {
        if (std::abs (transform.getDeterminant()) <= 1.0e-6f)
            return padded;

        const auto inverse = transform.inverted();
        for (auto& corner : corners)
            corner = corner.transformed (inverse);
    }

    const Point<float> start (stops.front().x, stops.front().y);
    auto step = Point<float> (stops.back().x, stops.back().y) - start;

    int firstRepeat = 0;
    int lastRepeat = 1;

    if (type == Type::Linear)
    {
        const auto lengthSquared = step.dotProduct (step);
        if (lengthSquared <= 0.0f)
            return padded;

        float minT = 0.0f;
        float maxT = 1.0f;

        for (const auto& corner : corners)
        {
            const auto t = (corner - start).dotProduct (step) / lengthSquared;
            minT = jmin (minT, t);
            maxT = jmax (maxT, t);
        }

        firstRepeat = static_cast<int> (std::floor (minT));
        lastRepeat = static_cast<int> (std::ceil (maxT));

        if (lastRepeat - firstRepeat > maxLinearRepeats)
        {
            firstRepeat = (firstRepeat + lastRepeat) / 2 - maxLinearRepeats / 2;
            lastRepeat = firstRepeat + maxLinearRepeats;
        }
    }
    else
    {
        if (radius <= 0.0f)
            return padded;

        float maxT = 1.0f;
        for (const auto& corner : corners)
            maxT = jmax (maxT, start.distanceTo (corner) / radius);

        lastRepeat = jlimit (1, maxRadialRepeats, static_cast<int> (std::ceil (maxT)));

        // The stops of a radial gradient only need to be a radius apart
        const auto length = step.magnitude();
        step = length > 0.0f ? step * (radius / length) : Point<float> (radius, 0.0f);
    }

    if (firstRepeat == 0 && lastRepeat == 1)
        return padded;

    const auto range = static_cast<float> (lastRepeat - firstRepeat);

    std::vector<ColorStop> expanded;
    expanded.reserve (stops.size() * static_cast<size_t> (lastRepeat - firstRepeat));

    const auto append = [&] (const ColorStop& stop, int repeat, bool isReflected)
    {
        const auto offset = static_cast<float> (repeat) + (isReflected ? 1.0f - stop.delta : stop.delta);
        expanded.emplace_back (stop.color, start + step * offset, jlimit (0.0f, 1.0f, (offset - static_cast<float> (firstRepeat)) / range));
    };

    for (int repeat = firstRepeat; repeat < lastRepeat; ++repeat)
    {
        if (spread == Spread::Reflect && (std::abs (repeat) % 2) == 1)
        {
            for (auto stop = stops.rbegin(); stop != stops.rend(); ++stop)
                append (*stop, repeat, true);
        }
        else
        {
            for (const auto& stop : stops)
                append (stop, repeat, false);
        }
    }

    // The ends of the gradient are where its first and last stops are, whatever their deltas
    const auto first = start + step * static_cast<float> (firstRepeat);
    const auto last = start + step * static_cast<float> (lastRepeat);
    expanded.front().x = first.getX();
    expanded.front().y = first.getY();
    expanded.back().x = last.getX();
    expanded.back().y = last.getY();

    ColorGradient result (type, std::move (expanded));
    result.transform = transform;
    return result;
}

} // namespace yup
