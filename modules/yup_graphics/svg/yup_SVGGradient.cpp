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

AffineTransform getGradientSpaceTransform (const SVGGradient& gradient, const Rectangle<float>* objectBounds)
{
    const bool hasBounds = objectBounds != nullptr && objectBounds->getWidth() > 0.0f && objectBounds->getHeight() > 0.0f;
    AffineTransform unitsTransform = AffineTransform::identity();

    if (gradient.units == SVGGradient::ObjectBoundingBox && hasBounds)
    {
        unitsTransform = AffineTransform::scaling (objectBounds->getWidth(), objectBounds->getHeight())
                             .translated (objectBounds->getX(), objectBounds->getY());
    }

    return unitsTransform.followedBy (gradient.transform);
}

SVGGradient makeLocalSpaceGradient (const SVGGradient& gradient)
{
    SVGGradient localGradient;

    localGradient.type = gradient.type;
    localGradient.id = gradient.id;
    localGradient.units = SVGGradient::UserSpaceOnUse;
    localGradient.spreadMethod = gradient.spreadMethod;
    localGradient.start = gradient.start;
    localGradient.end = gradient.end;
    localGradient.center = gradient.center;
    localGradient.radius = gradient.radius;
    localGradient.focal = gradient.focal;
    localGradient.stops = gradient.stops;
    localGradient.hasStart = gradient.hasStart;
    localGradient.hasEnd = gradient.hasEnd;
    localGradient.hasCenter = gradient.hasCenter;
    localGradient.hasRadius = gradient.hasRadius;
    localGradient.hasFocal = gradient.hasFocal;
    localGradient.hasUnits = true;
    localGradient.hasSpreadMethod = gradient.hasSpreadMethod;

    return localGradient;
}

ColorGradient::Spread toGradientSpread (const String& spreadMethod)
{
    if (spreadMethod == "reflect")
        return ColorGradient::Spread::Reflect;

    if (spreadMethod == "repeat")
        return ColorGradient::Spread::Repeat;

    return ColorGradient::Spread::Pad;
}

} // namespace

//==============================================================================
ColorGradient SVGGradient::toColorGradient (const Rectangle<float>* objectBounds) const
{
    YUP_DRAWABLE_LOG ("Creating ColorGradient from SVG gradient ID: " << id
                                                                      << " type: " << (type == Linear ? "Linear" : "Radial")
                                                                      << " units: " << (units == UserSpaceOnUse ? "userSpaceOnUse" : "objectBoundingBox"));

    if (stops.empty())
    {
        YUP_DRAWABLE_LOG ("No stops in gradient, returning empty");
        return ColorGradient();
    }

    const AffineTransform gradientSpaceTransform = getGradientSpaceTransform (*this, objectBounds);

    // Non uniform radial gradients are built in their own space and drawn through the gradient transform.
    if (type == Radial
        && ! transform.isIdentity()
        && objectBounds != nullptr
        && std::abs (gradientSpaceTransform.getDeterminant()) > 1.0e-6f)
    {
        return makeLocalSpaceGradient (*this).toColorGradient().withTransform (gradientSpaceTransform);
    }

    const auto toDrawingSpace = [&gradientSpaceTransform] (Point<float> p)
    {
        return p.transformed (gradientSpaceTransform);
    };

    const auto computeRadiusFrom = [&] (Point<float> origin) -> float
    {
        if (radius <= 0.0f)
            return 0.0f;

        const Point<float> edgePoints[] = {
            toDrawingSpace (Point<float> (center.getX() + radius, center.getY())),
            toDrawingSpace (Point<float> (center.getX() - radius, center.getY())),
            toDrawingSpace (Point<float> (center.getX(), center.getY() + radius)),
            toDrawingSpace (Point<float> (center.getX(), center.getY() - radius))
        };

        float maxRadius = 0.0f;
        for (const auto& edgePoint : edgePoints)
            maxRadius = jmax (maxRadius, Line<float> (origin, edgePoint).length());

        return maxRadius;
    };

    std::vector<ColorGradient::ColorStop> colorStops;
    colorStops.reserve (stops.size());

    if (type == Radial)
    {
        const auto radialCenter = toDrawingSpace (hasFocal ? focal : center);
        const auto radialRadius = computeRadiusFrom (radialCenter);
        const auto effectiveRadius = (radialRadius > 0.0f) ? radialRadius : 1.0f;

        for (const auto& stop : stops)
        {
            const auto offset = jlimit (0.0f, 1.0f, stop.offset);
            const auto radialPoint = Point<float> (radialCenter.getX() + effectiveRadius * offset, radialCenter.getY());
            colorStops.emplace_back (stop.color.withMultipliedAlpha (stop.opacity), radialPoint, offset);
        }
    }
    else
    {
        const auto startPoint = toDrawingSpace (start);
        const auto endPoint = toDrawingSpace (end);

        for (const auto& stop : stops)
        {
            const auto interpolated = Point<float> (startPoint.getX() + stop.offset * (endPoint.getX() - startPoint.getX()),
                                                    startPoint.getY() + stop.offset * (endPoint.getY() - startPoint.getY()));
            colorStops.emplace_back (stop.color.withMultipliedAlpha (stop.opacity), interpolated, stop.offset);
        }
    }

    return ColorGradient (type == Linear ? ColorGradient::Linear : ColorGradient::Radial, std::move (colorStops))
        .withSpread (toGradientSpread (spreadMethod));
}

} // namespace yup
