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
// AnimationGradient

void AnimationGradient::addColorStop (float pos, Color color)
{
    ColorStop stop;
    stop.position.withStaticValue (pos);
    stop.color.withStaticValue (color);
    colorStops.push_back (std::move (stop));
}

std::vector<std::pair<float, Color>> AnimationGradient::parseStopsFromFlatArray (
    const std::vector<float>& flat,
    int colorPoints)
{
    std::vector<std::pair<float, Color>> stops;
    const int totalSize = static_cast<int> (flat.size());
    const int count = jmin (colorPoints, totalSize / 4);
    const int colorDataEnd = count * 4;

    // Collect opacity stops (position, opacity) from the tail of the array.
    std::vector<std::pair<float, float>> opacityStops;
    for (int i = colorDataEnd; i + 1 < totalSize; i += 2)
        opacityStops.push_back ({ flat[static_cast<size_t> (i)], flat[static_cast<size_t> (i + 1)] });

    // Linear interpolation of opacity at a given position.
    auto getOpacityAt = [&] (float pos) -> float
    {
        if (opacityStops.empty())
            return 1.0f;
        if (pos <= opacityStops.front().first)
            return opacityStops.front().second;
        if (pos >= opacityStops.back().first)
            return opacityStops.back().second;
        for (size_t j = 1; j < opacityStops.size(); ++j)
        {
            if (opacityStops[j].first >= pos)
            {
                const float span = opacityStops[j].first - opacityStops[j - 1].first;
                const float tVal = span > 1e-6f ? (pos - opacityStops[j - 1].first) / span : 1.0f;
                return opacityStops[j - 1].second + tVal * (opacityStops[j].second - opacityStops[j - 1].second);
            }
        }
        return 1.0f;
    };

    for (int i = 0; i < count; ++i)
    {
        const float pos = flat[static_cast<size_t> (i * 4)];
        const float r = flat[static_cast<size_t> (i * 4 + 1)];
        const float g = flat[static_cast<size_t> (i * 4 + 2)];
        const float b = flat[static_cast<size_t> (i * 4 + 3)];
        const float alpha = getOpacityAt (pos);
        stops.push_back ({ pos, Color::fromRGBA (static_cast<uint8> (r * 255.0f), static_cast<uint8> (g * 255.0f), static_cast<uint8> (b * 255.0f), static_cast<uint8> (alpha * 255.0f)) });
    }
    return stops;
}

ColorGradient AnimationGradient::toColorGradient (float frameNo) const
{
    const Point<float> start = startPoint.getValueAt (frameNo);
    const Point<float> end = endPoint.getValueAt (frameNo);

    auto resolveStops = [this] (float fn) -> std::vector<std::pair<float, Color>>
    {
        // If animated, interpolate the flat value arrays from keyframes
        if (! animatedStops.empty())
        {
            if (fn <= animatedStops.front().frame)
                return parseStopsFromFlatArray (animatedStops.front().values, numColorPoints);
            if (fn >= animatedStops.back().frame)
                return parseStopsFromFlatArray (animatedStops.back().values, numColorPoints);

            int lo = 0;
            int hi = static_cast<int> (animatedStops.size()) - 2;
            while (lo < hi)
            {
                const int mid = (lo + hi + 1) / 2;
                if (animatedStops[mid].frame <= fn)
                    lo = mid;
                else
                    hi = mid - 1;
            }

            const auto& k0 = animatedStops[lo];
            const auto& k1 = animatedStops[lo + 1];
            const float span = k1.frame - k0.frame;
            if (span < 1e-6f)
                return parseStopsFromFlatArray (k0.values, numColorPoints);

            const float t = (fn - k0.frame) / span;
            std::vector<float> interpolated;
            interpolated.resize (k0.values.size());
            for (size_t i = 0; i < k0.values.size(); ++i)
                interpolated[i] = k0.values[i] + (k1.values[i] - k0.values[i]) * t;

            return parseStopsFromFlatArray (interpolated, numColorPoints);
        }

        // Static stops
        std::vector<std::pair<float, Color>> stops;
        for (const auto& cs : colorStops)
            stops.push_back ({ cs.position.getValueAt (fn), cs.color.getValueAt (fn) });
        return stops;
    };

    const auto stops = resolveStops (frameNo);

    if (stops.empty())
        return ColorGradient (Color(), start, Color(), end, gradientType == GradientType::Radial ? ColorGradient::Type::Radial : ColorGradient::Type::Linear);

    // Radial gradient highlight (focal point) — adjusts the center point along the
    // start→end axis by highlightLength ratio, rotated by highlightAngle.
    Point<float> gradStart = start;
    if (gradientType == GradientType::Radial)
    {
        const float hLen = highlightLen.getValueAt (frameNo);
        const float hAngle = highlightAngle.getValueAt (frameNo);
        if (std::abs (hLen) > 1e-5f || std::abs (hAngle) > 1e-5f)
        {
            float progress = hLen / 100.0f;
            if (std::abs (progress - 1.0f) < 1e-5f)
                progress = 0.99f;
            const float radius = start.distanceTo (end);
            const float startAngle = std::atan2 (end.getY() - start.getY(), end.getX() - start.getX());
            const float angle = startAngle + degreesToRadians (hAngle);
            gradStart.setX (start.getX() + std::cos (angle) * progress * radius);
            gradStart.setY (start.getY() + std::sin (angle) * progress * radius);
        }
    }

    const ColorGradient gradient (
        stops.front().second, gradStart, stops.back().second, end, gradientType == GradientType::Radial ? ColorGradient::Type::Radial : ColorGradient::Type::Linear);

    ColorGradient result (gradient);
    for (size_t i = 1; i + 1 < stops.size(); ++i)
        result.addColorStop (stops[i].second, stops[i].first);

    return result;
}

//==============================================================================
// FillPaint

Color FillPaint::colorAt (float frameNo) const
{
    return color.getValueAt (frameNo);
}

float FillPaint::opacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, opacity.getValueAt (frameNo) / 100.0f);
}

//==============================================================================
// StrokePaint

Color StrokePaint::colorAt (float frameNo) const
{
    return color.getValueAt (frameNo);
}

float StrokePaint::opacityAt (float frameNo) const
{
    return jlimit (0.0f, 1.0f, opacity.getValueAt (frameNo) / 100.0f);
}

float StrokePaint::widthAt (float frameNo) const
{
    return width.getValueAt (frameNo);
}

StrokeType StrokePaint::strokeTypeAt (float frameNo) const
{
    return StrokeType (widthAt (frameNo), join, cap);
}

} // namespace yup
