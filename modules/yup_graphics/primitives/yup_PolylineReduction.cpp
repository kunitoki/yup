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

size_t reducePolylineToPixelColumns (Span<Point<float>> points, float pixelsPerUnit) noexcept
{
    constexpr size_t maxPointsPerColumn = 4;

    const auto numPoints = points.size();

    if (numPoints <= maxPointsPerColumn || ! (pixelsPerUnit > 0.0f))
        return numPoints;

    // Columns are compared as floats: a position too large or not finite for an integer is still told from the others
    const auto columnOf = [pixelsPerUnit] (const Point<float>& point)
    {
        return std::floor (point.getX() * pixelsPerUnit);
    };

    // Both ends tell how many columns there are: too few points for them to have any to lose are not read at all
    const auto numColumns = columnOf (points.back()) - columnOf (points.front()) + 1.0f;

    if (! (static_cast<float> (numPoints) > numColumns * static_cast<float> (maxPointsPerColumn)))
        return numPoints;

    for (size_t i = 1; i < numPoints; ++i)
    {
        if (points[i].getX() < points[i - 1].getX())
            return numPoints;
    }

    // A column never keeps more points than it has, so a point is always written at or before where it was read
    size_t numKept = 0;
    size_t first = 0;
    auto column = columnOf (points.front());

    while (first < numPoints)
    {
        auto lowest = first;
        auto highest = first;
        auto next = first + 1;
        auto nextColumn = column;

        for (; next < numPoints; ++next)
        {
            nextColumn = columnOf (points[next]);

            if (nextColumn != column)
                break;

            if (points[next].getY() < points[lowest].getY())
                lowest = next;
            else if (points[next].getY() > points[highest].getY())
                highest = next;
        }

        const auto last = next - 1;
        const auto [earlier, later] = std::minmax (lowest, highest);

        points[numKept++] = points[first];

        if (earlier > first && earlier < last)
            points[numKept++] = points[earlier];

        if (later > earlier && later < last)
            points[numKept++] = points[later];

        if (last > first)
            points[numKept++] = points[last];

        first = next;
        column = nextColumn;
    }

    return numKept;
}

} // namespace yup
