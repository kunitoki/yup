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

namespace
{

//==============================================================================

bool isControlMarker (String::CharPointerType data)
{
    return ! data.isEmpty() && String ("MmLlHhVvQqTtCcSsAaZz").containsChar (*data);
}

void skipWhitespace (String::CharPointerType& data)
{
    while (! data.isEmpty() && data.isWhitespace())
        ++data;
}

void skipWhitespaceOrComma (String::CharPointerType& data)
{
    while (! data.isEmpty() && (data.isWhitespace() || *data == ','))
        ++data;
}

//==============================================================================

bool parseFlag (String::CharPointerType& data, int& flag)
{
    skipWhitespaceOrComma (data);

    if (data.isEmpty() || (*data != '0' && *data != '1'))
        return false;

    flag = *data - '0';
    ++data;
    skipWhitespaceOrComma (data);
    return true;
}

//==============================================================================

bool parseCoordinate (String::CharPointerType& data, float& coord)
{
    skipWhitespaceOrComma (data);

    auto start = data;

    if (data.isEmpty() || isControlMarker (data))
        return false;

    String number;
    bool hasDigits = false;

    if (*data == '-' || *data == '+')
    {
        number += *data;
        ++data;
    }

    while (! data.isEmpty() && *data >= '0' && *data <= '9')
    {
        hasDigits = true;
        number += *data;
        ++data;
    }

    if (! data.isEmpty() && *data == '.')
    {
        number += *data;
        ++data;

        while (! data.isEmpty() && *data >= '0' && *data <= '9')
        {
            hasDigits = true;
            number += *data;
            ++data;
        }
    }

    if (! hasDigits)
    {
        data = start;
        return false;
    }

    if (! data.isEmpty() && (*data == 'e' || *data == 'E'))
    {
        auto exponentStart = data;
        String exponent;

        exponent += *data;
        ++data;

        if (! data.isEmpty() && (*data == '-' || *data == '+'))
        {
            exponent += *data;
            ++data;
        }

        bool hasExponentDigits = false;
        while (! data.isEmpty() && *data >= '0' && *data <= '9')
        {
            hasExponentDigits = true;
            exponent += *data;
            ++data;
        }

        if (hasExponentDigits)
        {
            number += exponent;
        }
        else
        {
            data = exponentStart;
        }
    }

    coord = number.getFloatValue();
    skipWhitespaceOrComma (data);
    return true;
}

//==============================================================================

bool parseCoordinates (String::CharPointerType& data, float& x, float& y)
{
    if (parseCoordinate (data, x))
    {
        skipWhitespaceOrComma (data);
        if (parseCoordinate (data, y))
        {
            skipWhitespaceOrComma (data);
            return true;
        }
    }

    return false;
}

//==============================================================================

void handleMoveTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, float& startX, float& startY, bool relative)
{
    float x, y;
    bool isFirstCoordinate = true;

    while (! data.isEmpty()
           && ! isControlMarker (data)
           && parseCoordinates (data, x, y))
    {
        if (relative)
        {
            x += currentX;
            y += currentY;
        }

        if (isFirstCoordinate)
        {
            path.moveTo (x, y);
            currentX = startX = x;
            currentY = startY = y;
            isFirstCoordinate = false;
        }
        else
        {
            // Subsequent coordinates are implicit line commands according to SVG spec
            path.lineTo (x, y);
            currentX = x;
            currentY = y;
        }

        skipWhitespace (data);
    }
}

//==============================================================================

void handleLineTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, bool relative)
{
    float x, y;

    while (! data.isEmpty()
           && ! isControlMarker (data)
           && parseCoordinates (data, x, y))
    {
        if (relative)
        {
            x += currentX;
            y += currentY;
        }

        path.lineTo (x, y);

        currentX = x;
        currentY = y;

        skipWhitespace (data);
    }
}

//==============================================================================

void handleHorizontalLineTo (String::CharPointerType& data, Path& path, float& currentX, float currentY, bool relative)
{
    float x;

    while (! data.isEmpty()
           && ! isControlMarker (data)
           && parseCoordinate (data, x))
    {
        if (relative)
            x += currentX;

        path.lineTo (x, currentY);

        currentX = x;

        skipWhitespace (data);
    }
}

//==============================================================================

void handleVerticalLineTo (String::CharPointerType& data, Path& path, float currentX, float& currentY, bool relative)
{
    float y;

    while (! data.isEmpty()
           && ! isControlMarker (data)
           && parseCoordinate (data, y))
    {
        if (relative)
            y += currentY;

        path.lineTo (currentX, y);

        currentY = y;

        skipWhitespace (data);
    }
}

//==============================================================================

void handleQuadTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, float& lastQuadX, float& lastQuadY, bool relative)
{
    float x1, y1, x, y;

    while (! data.isEmpty()
           && ! isControlMarker (data)
           && parseCoordinates (data, x1, y1)
           && parseCoordinates (data, x, y))
    {
        if (relative)
        {
            x1 += currentX;
            y1 += currentY;
            x += currentX;
            y += currentY;
        }

        path.quadTo (x, y, x1, y1);

        currentX = x;
        currentY = y;

        // Update the last control point for smooth curves
        lastQuadX = x1;
        lastQuadY = y1;

        skipWhitespace (data);
    }
}

//==============================================================================

void handleSmoothQuadTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, float& lastQuadX, float& lastQuadY, bool relative)
{
    float x, y;

    while (! data.isEmpty()
           && ! isControlMarker (data)
           && parseCoordinates (data, x, y))
    {
        float cx, cy;

        // Calculate the control point based on the reflection of the last control point
        if (lastQuadX == currentX && lastQuadY == currentY)
        {
            cx = currentX;
            cy = currentY;
        }
        else
        {
            cx = 2.0f * currentX - lastQuadX;
            cy = 2.0f * currentY - lastQuadY;
        }

        // If the coordinates are relative, adjust them
        if (relative)
        {
            x += currentX;
            y += currentY;
        }

        path.quadTo (x, y, cx, cy);

        // Update the current position
        currentX = x;
        currentY = y;

        // Update the last control point for reflection
        lastQuadX = cx;
        lastQuadY = cy;

        skipWhitespace (data);
    }
}

//==============================================================================

void handleCubicTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, float& lastControlX, float& lastControlY, bool relative)
{
    float x1, y1, x2, y2, x, y;

    while (! data.isEmpty()
           && ! isControlMarker (data)
           && parseCoordinates (data, x1, y1)
           && parseCoordinates (data, x2, y2)
           && parseCoordinates (data, x, y))
    {
        if (relative)
        {
            x1 += currentX;
            y1 += currentY;
            x2 += currentX;
            y2 += currentY;
            x += currentX;
            y += currentY;
        }

        path.cubicTo (x1, y1, x2, y2, x, y);

        currentX = x;
        currentY = y;

        // Update the last control point for smooth curves (second control point)
        lastControlX = x2;
        lastControlY = y2;

        skipWhitespace (data);
    }
}

//==============================================================================

void handleSmoothCubicTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, float& lastControlX, float& lastControlY, bool relative)
{
    float x2, y2, x, y;

    while (! data.isEmpty()
           && ! isControlMarker (data)
           && parseCoordinates (data, x2, y2)
           && parseCoordinates (data, x, y))
    {
        float cx1, cy1;

        // Calculate the first control point based on the reflection of the last control point
        if (lastControlX == currentX && lastControlY == currentY)
        {
            cx1 = currentX;
            cy1 = currentY;
        }
        else
        {
            cx1 = 2.0f * currentX - lastControlX;
            cy1 = 2.0f * currentY - lastControlY;
        }

        // If the coordinates are relative, adjust them
        if (relative)
        {
            x2 += currentX;
            y2 += currentY;
            x += currentX;
            y += currentY;
        }

        path.cubicTo (cx1, cy1, x2, y2, x, y);

        // Update the current position
        currentX = x;
        currentY = y;

        // Update the last control point for reflection
        lastControlX = x2;
        lastControlY = y2;

        skipWhitespace (data);
    }
}

//==============================================================================

void handleEllipticalArc (String::CharPointerType& data, Path& path, float& currentX, float& currentY, bool relative)
{
    float rx, ry, xAxisRotation, x, y;
    int largeArc, sweep;

    while (! data.isEmpty() && ! isControlMarker (data))
    {
        if (parseCoordinates (data, rx, ry)
            && parseCoordinate (data, xAxisRotation)
            && parseFlag (data, largeArc)
            && parseFlag (data, sweep)
            && parseCoordinates (data, x, y))
        {
            rx = std::abs (rx);
            ry = std::abs (ry);

            if (relative)
            {
                x += currentX;
                y += currentY;
            }

            if (rx == 0 || ry == 0)
            {
                path.lineTo (x, y);

                currentX = x;
                currentY = y;

                skipWhitespace (data);
                continue;
            }

            // Convert angle from degrees to radians
            const float angleRad = degreesToRadians (xAxisRotation);

            // Calculate the midpoint between the start and end points
            const float dx = (currentX - x) / 2.0f;
            const float dy = (currentY - y) / 2.0f;

            // Apply the rotation to the midpoint
            float cosAngle = std::cos (angleRad);
            float sinAngle = std::sin (angleRad);
            float x1Prime = cosAngle * dx + sinAngle * dy;
            float y1Prime = -sinAngle * dx + cosAngle * dy;

            // Ensure radii are large enough
            float rxSq = rx * rx;
            float rySq = ry * ry;
            float x1PrimeSq = x1Prime * x1Prime;
            float y1PrimeSq = y1Prime * y1Prime;

            // Correct radii if they are too small
            float radiiScale = x1PrimeSq / rxSq + y1PrimeSq / rySq;
            if (radiiScale > 1)
            {
                float scale = std::sqrt (radiiScale);
                rx *= scale;
                ry *= scale;
                rxSq = rx * rx;
                rySq = ry * ry;
            }

            // Calculate the center point (cx, cy)
            float sign = (largeArc != sweep) ? 1.0f : -1.0f;
            float numerator = rxSq * rySq - rxSq * y1PrimeSq - rySq * x1PrimeSq;
            float denominator = rxSq * y1PrimeSq + rySq * x1PrimeSq;
            float sqrtFactor = 0.0f;
            if (denominator > 0.0f && numerator >= 0.0f)
                sqrtFactor = std::sqrt (numerator / denominator);
            float cxPrime = sign * sqrtFactor * (rx * y1Prime / ry);
            float cyPrime = sign * sqrtFactor * (-ry * x1Prime / rx);

            // Transform the center point back to the original coordinate system
            float centreX = cosAngle * cxPrime - sinAngle * cyPrime + (currentX + x) / 2.0f;
            float centreY = sinAngle * cxPrime + cosAngle * cyPrime + (currentY + y) / 2.0f;

            // Calculate the start angle and delta angle
            float ux = (x1Prime - cxPrime) / rx;
            float uy = (y1Prime - cyPrime) / ry;
            float vx = (-x1Prime - cxPrime) / rx;
            float vy = (-y1Prime - cyPrime) / ry;

            float startAngle = std::atan2 (uy, ux);
            float deltaAngle = std::atan2 (ux * vy - uy * vx, ux * vx + uy * vy);

            if (! sweep && deltaAngle > 0)
            {
                deltaAngle -= MathConstants<float>::twoPi;
            }
            else if (sweep && deltaAngle < 0)
            {
                deltaAngle += MathConstants<float>::twoPi;
            }

            // Ensure the delta angle is within the range [-2π, 2π]
            deltaAngle = std::fmod (deltaAngle, MathConstants<float>::twoPi);

            // Add the arc to the path (use angleRad which is already in radians)
            // Don't start new subpath - SVG arcs continue from current position
            path.addCenteredArc (centreX, centreY, rx, ry, angleRad, startAngle, startAngle + deltaAngle, false);

            // Update the current position
            currentX = x;
            currentY = y;

            skipWhitespace (data);
        }
        else
        {
            break;
        }
    }
}

//==============================================================================

void addRoundedSubpath (Path& targetPath, const std::vector<Point<float>>& points, float cornerRadius, bool closed)
{
    if (points.size() < 3)
        return;

    bool first = true;

    for (size_t i = 0; i < points.size(); ++i)
    {
        size_t prevIndex = (i == 0) ? (closed ? points.size() - 1 : 0) : i - 1;
        size_t nextIndex = (i == points.size() - 1) ? (closed ? 0 : i) : i + 1;

        if (! closed && (i == 0 || i == points.size() - 1))
        {
            // Don't round first/last points in open paths
            if (first)
            {
                targetPath.moveTo (points[i]);
                first = false;
            }
            else
            {
                targetPath.lineTo (points[i]);
            }
            continue;
        }

        Point<float> current = points[i];
        Point<float> prev = points[prevIndex];
        Point<float> next = points[nextIndex];

        // Calculate vectors
        Point<float> toPrev = (prev - current).normalized();
        Point<float> toNext = (next - current).normalized();

        // Calculate the angle between vectors
        float dot = toPrev.dotProduct (toNext);
        dot = jlimit (-1.0f, 1.0f, dot); // Clamp to avoid numerical issues

        if (std::abs (dot + 1.0f) < 0.001f) // Vectors are opposite (180 degrees)
        {
            // Straight line, no rounding needed
            if (first)
            {
                targetPath.moveTo (current);
                first = false;
            }
            else
            {
                targetPath.lineTo (current);
            }
            continue;
        }

        // Calculate distances to round corner
        float prevDist = current.distanceTo (prev);
        float nextDist = current.distanceTo (next);
        float maxRadius = jmin (cornerRadius, prevDist * 0.5f, nextDist * 0.5f);

        if (maxRadius <= 0.0f)
        {
            if (first)
            {
                targetPath.moveTo (current);
                first = false;
            }
            else
            {
                targetPath.lineTo (current);
            }
            continue;
        }

        // Calculate corner points
        Point<float> cornerStart = current + toPrev * maxRadius;
        Point<float> cornerEnd = current + toNext * maxRadius;

        if (first)
        {
            targetPath.moveTo (cornerStart);
            first = false;
        }
        else
        {
            targetPath.lineTo (cornerStart);
        }

        // Add rounded corner using quadratic curve
        targetPath.quadTo (cornerEnd.getX(), cornerEnd.getY(), current.getX(), current.getY());
    }

    if (closed)
    {
        targetPath.close();
    }
}

//==============================================================================

using ClipperPathD = Clipper2Lib::PathD;
using ClipperPathsD = Clipper2Lib::PathsD;

[[nodiscard]] bool isFinitePoint (const Point<float>& point) noexcept
{
    return std::isfinite (point.getX()) && std::isfinite (point.getY());
}

[[nodiscard]] bool pointsNearlyEqual (const Point<float>& lhs, const Point<float>& rhs) noexcept
{
    return lhs.distanceTo (rhs) <= 1.0e-4f;
}

void appendClipperPoint (ClipperPathD& path, const Point<float>& point)
{
    if (! isFinitePoint (point))
        return;

    if (! path.empty()
        && std::abs (path.back().x - static_cast<double> (point.getX())) <= 1.0e-6
        && std::abs (path.back().y - static_cast<double> (point.getY())) <= 1.0e-6)
    {
        return;
    }

    path.emplace_back (static_cast<double> (point.getX()),
                       static_cast<double> (point.getY()));
}

void finishClipperContour (ClipperPathsD& paths, ClipperPathD& contour)
{
    if (contour.size() >= 2
        && std::abs (contour.front().x - contour.back().x) <= 1.0e-6
        && std::abs (contour.front().y - contour.back().y) <= 1.0e-6)
    {
        contour.pop_back();
    }

    if (contour.size() >= 3)
        paths.push_back (std::move (contour));

    contour.clear();
}

[[nodiscard]] Point<float> evaluateClipperQuadratic (const Point<float>& start,
                                                     const Point<float>& control,
                                                     const Point<float>& end,
                                                     float t)
{
    const float u = 1.0f - t;
    const float uu = u * u;
    const float tt = t * t;

    return { uu * start.getX() + 2.0f * u * t * control.getX() + tt * end.getX(),
             uu * start.getY() + 2.0f * u * t * control.getY() + tt * end.getY() };
}

[[nodiscard]] Point<float> evaluateClipperCubic (const Point<float>& start,
                                                 const Point<float>& control1,
                                                 const Point<float>& control2,
                                                 const Point<float>& end,
                                                 float t)
{
    const float u = 1.0f - t;
    const float uu = u * u;
    const float uuu = uu * u;
    const float tt = t * t;
    const float ttt = tt * t;

    return { uuu * start.getX() + 3.0f * uu * t * control1.getX() + 3.0f * u * tt * control2.getX() + ttt * end.getX(),
             uuu * start.getY() + 3.0f * uu * t * control1.getY() + 3.0f * u * tt * control2.getY() + ttt * end.getY() };
}

[[nodiscard]] int estimateCurveSegments (float controlPolygonLength)
{
    return jlimit (4, 96, static_cast<int> (std::ceil (controlPolygonLength / 2.0f)));
}

void appendQuadraticAsLines (ClipperPathD& path,
                             const Point<float>& start,
                             const Point<float>& control,
                             const Point<float>& end)
{
    const float controlLength = start.distanceTo (control) + control.distanceTo (end);
    const int numSegments = estimateCurveSegments (controlLength);

    for (int i = 1; i <= numSegments; ++i)
        appendClipperPoint (path, evaluateClipperQuadratic (start, control, end, static_cast<float> (i) / static_cast<float> (numSegments)));
}

void appendCubicAsLines (ClipperPathD& path,
                         const Point<float>& start,
                         const Point<float>& control1,
                         const Point<float>& control2,
                         const Point<float>& end)
{
    const float controlLength = start.distanceTo (control1) + control1.distanceTo (control2) + control2.distanceTo (end);
    const int numSegments = estimateCurveSegments (controlLength);

    for (int i = 1; i <= numSegments; ++i)
        appendClipperPoint (path, evaluateClipperCubic (start, control1, control2, end, static_cast<float> (i) / static_cast<float> (numSegments)));
}

[[nodiscard]] ClipperPathsD toClipperPaths (const Path& source)
{
    ClipperPathsD paths;
    ClipperPathD contour;
    Point<float> current;
    Point<float> contourStart;
    bool hasCurrent = false;

    for (const auto& segment : source)
    {
        switch (segment.verb)
        {
            case Path::Verb::MoveTo:
                finishClipperContour (paths, contour);
                current = segment.point;
                contourStart = current;
                hasCurrent = true;
                appendClipperPoint (contour, current);
                break;

            case Path::Verb::LineTo:
                if (hasCurrent)
                {
                    appendClipperPoint (contour, segment.point);
                    current = segment.point;
                }
                break;

            case Path::Verb::QuadTo:
                if (hasCurrent)
                {
                    appendQuadraticAsLines (contour, current, segment.controlPoint1, segment.point);
                    current = segment.point;
                }
                break;

            case Path::Verb::CubicTo:
                if (hasCurrent)
                {
                    appendCubicAsLines (contour, current, segment.controlPoint1, segment.controlPoint2, segment.point);
                    current = segment.point;
                }
                break;

            case Path::Verb::Close:
                if (hasCurrent)
                {
                    if (! pointsNearlyEqual (current, contourStart))
                        appendClipperPoint (contour, contourStart);

                    finishClipperContour (paths, contour);
                    hasCurrent = false;
                }
                break;
        }
    }

    finishClipperContour (paths, contour);

    return paths;
}

[[nodiscard]] Clipper2Lib::FillRule toClipperFillRule (const Path& path) noexcept
{
    return path.isUsingNonZeroWinding() ? Clipper2Lib::FillRule::NonZero
                                        : Clipper2Lib::FillRule::EvenOdd;
}

[[nodiscard]] Clipper2Lib::ClipType toClipperClipType (Path::BooleanOperation operation) noexcept
{
    switch (operation)
    {
        case Path::BooleanOperation::Union:
            return Clipper2Lib::ClipType::Union;
        case Path::BooleanOperation::Intersect:
            return Clipper2Lib::ClipType::Intersection;
        case Path::BooleanOperation::Subtract:
            return Clipper2Lib::ClipType::Difference;
        case Path::BooleanOperation::Xor:
            return Clipper2Lib::ClipType::Xor;
    }

    return Clipper2Lib::ClipType::Union;
}

[[nodiscard]] Path fromClipperPaths (const ClipperPathsD& paths)
{
    Path result;

    for (const auto& contour : paths)
    {
        if (contour.size() < 3)
            continue;

        result.moveTo (static_cast<float> (contour.front().x),
                       static_cast<float> (contour.front().y));

        for (size_t i = 1; i < contour.size(); ++i)
            result.lineTo (static_cast<float> (contour[i].x),
                           static_cast<float> (contour[i].y));

        result.closeSubPath();
    }

    result.setUsingNonZeroWinding (true);
    return result;
}

} // namespace

//==============================================================================

Path::Iterator::Iterator (const rive::RawPath& rawPath, bool atEnd)
    : rawPath (std::addressof (rawPath))
    , verbIndex (0)
    , pointIndex (0)
    , isAtEnd (atEnd)
{
    if (! isAtEnd)
        updateToValidPosition();
}

Path::Segment Path::Iterator::operator*() const
{
    jassert (! isAtEnd);
    return createCurrentSegment();
}

Path::Iterator& Path::Iterator::operator++()
{
    if (isAtEnd)
        return *this;

    const auto& verbs = rawPath->verbs();

    if (verbIndex >= verbs.size())
    {
        isAtEnd = true;
        return *this;
    }

    auto verb = verbs[verbIndex];

    // Advance point index based on verb type
    switch (verb)
    {
        case rive::PathVerb::move:
        case rive::PathVerb::line:
            pointIndex += 1;
            break;

        case rive::PathVerb::quad:
            pointIndex += 2;
            break;

        case rive::PathVerb::cubic:
            pointIndex += 3;
            break;

        case rive::PathVerb::close:
            // Close doesn't consume points
            break;
    }

    ++verbIndex;
    updateToValidPosition();

    return *this;
}

Path::Iterator Path::Iterator::operator++ (int)
{
    Path::Iterator temp = *this;
    ++(*this);
    return temp;
}

bool Path::Iterator::operator== (const Path::Iterator& other) const
{
    if (isAtEnd && other.isAtEnd)
        return true;

    return rawPath == other.rawPath
        && verbIndex == other.verbIndex
        && pointIndex == other.pointIndex
        && isAtEnd == other.isAtEnd;
}

bool Path::Iterator::operator!= (const Path::Iterator& other) const
{
    return ! (*this == other);
}

void Path::Iterator::updateToValidPosition()
{
    const auto& verbs = rawPath->verbs();

    if (verbIndex >= verbs.size())
    {
        isAtEnd = true;
        return;
    }

    // Ensure we have enough points for the current verb
    const auto& points = rawPath->points();
    auto verb = verbs[verbIndex];

    size_t requiredPoints = 0;
    switch (verb)
    {
        case rive::PathVerb::move:
        case rive::PathVerb::line:
            requiredPoints = 1;
            break;

        case rive::PathVerb::quad:
            requiredPoints = 2;
            break;

        case rive::PathVerb::cubic:
            requiredPoints = 3;
            break;

        case rive::PathVerb::close:
            requiredPoints = 0;
            break;
    }

    if (pointIndex + requiredPoints > points.size() && requiredPoints > 0)
    {
        isAtEnd = true;
        return;
    }
}

Path::Segment Path::Iterator::createCurrentSegment() const
{
    const auto& verbs = rawPath->verbs();
    const auto& points = rawPath->points();

    if (verbIndex >= verbs.size())
        return Path::Segment::close(); // Should not happen with proper usage

    auto verb = verbs[verbIndex];
    switch (verb)
    {
        case rive::PathVerb::move:
            if (pointIndex < points.size())
            {
                return Path::Segment (
                    Path::Verb::MoveTo,
                    Point<float> (points[pointIndex].x, points[pointIndex].y));
            }
            break;

        case rive::PathVerb::line:
            if (pointIndex < points.size())
            {
                return Path::Segment (
                    Path::Verb::LineTo,
                    Point<float> (points[pointIndex].x, points[pointIndex].y));
            }
            break;

        case rive::PathVerb::quad:
            if (pointIndex + 1 < points.size())
            {
                return Path::Segment (
                    Path::Verb::QuadTo,
                    Point<float> (points[pointIndex + 1].x, points[pointIndex + 1].y), // end point
                    Point<float> (points[pointIndex].x, points[pointIndex].y));        // control point
            }
            break;

        case rive::PathVerb::cubic:
            if (pointIndex + 2 < points.size())
            {
                return Path::Segment (
                    Path::Verb::CubicTo,
                    Point<float> (points[pointIndex + 2].x, points[pointIndex + 2].y),  // end point
                    Point<float> (points[pointIndex].x, points[pointIndex].y),          // control point 1
                    Point<float> (points[pointIndex + 1].x, points[pointIndex + 1].y)); // control point 2
            }
            break;

        case rive::PathVerb::close:
            return Path::Segment::close();
    }

    // Fallback for invalid states
    return Path::Segment::close();
}

//==============================================================================

Path::Path()
    : path (rive::make_rcp<rive::RiveRenderPath>())
{
}

Path::Path (float x, float y)
    : Path()
{
    moveTo (x, y);
}

Path::Path (const Point<float>& p)
    : Path()
{
    moveTo (p);
}

Path::Path (rive::rcp<rive::RiveRenderPath> newPath)
    : path (std::move (newPath))
{
    jassert (path != nullptr);
}

//==============================================================================

void Path::reserveSpace (int numSegments)
{
    auto& rawPath = const_cast<rive::RawPath&> (path->getRawPath());

    rawPath.reserve (
        rawPath.verbs().size() + static_cast<std::size_t> (numSegments),
        rawPath.points().size() + static_cast<std::size_t> (numSegments) + 1u);
}

//==============================================================================

int Path::size() const
{
    return static_cast<int> (path->getRawPath().verbs().size());
}

//==============================================================================

void Path::clear()
{
    path->rewind();
}

//==============================================================================

Path& Path::moveTo (float x, float y)
{
    path->moveTo (x, y);

    return *this;
}

Path& Path::moveTo (const Point<float>& p)
{
    return moveTo (p.getX(), p.getY());
}

//==============================================================================

Path& Path::lineTo (float x, float y)
{
    path->lineTo (x, y);

    return *this;
}

Path& Path::lineTo (const Point<float>& p)
{
    return lineTo (p.getX(), p.getY());
}

//==============================================================================

Path& Path::quadTo (float x, float y, float x1, float y1)
{
    const auto& rawPath = path->getRawPath();
    if (rawPath.points().empty())
        moveTo (x, y);

    const rive::Vec2D& last = rawPath.points().back();
    const rive::Vec2D control (x1, y1);
    const rive::Vec2D end (x, y);

    const rive::Vec2D c1 = rive::Vec2D::lerp (last, control, 2.0f / 3.0f);
    const rive::Vec2D c2 = rive::Vec2D::lerp (end, control, 2.0f / 3.0f);

    path->cubic (c1, c2, end);

    return *this;
}

Path& Path::quadTo (const Point<float>& p, float x1, float y1)
{
    return quadTo (p.getX(), p.getY(), x1, y1);
}

//==============================================================================

Path& Path::cubicTo (float x, float y, float x1, float y1, float x2, float y2)
{
    path->cubicTo (x, y, x1, y1, x2, y2);

    return *this;
}

Path& Path::cubicTo (const Point<float>& p, float x1, float y1, float x2, float y2)
{
    return cubicTo (p.getX(), p.getY(), x1, y1, x2, y2);
}

//==============================================================================

Path& Path::close()
{
    path->close();

    return *this;
}

//==============================================================================

Path& Path::addLine (const Point<float>& p1, const Point<float>& p2)
{
    moveTo (p1);
    lineTo (p2);

    return *this;
}

Path& Path::addLine (const Line<float>& line)
{
    moveTo (line.getStart());
    lineTo (line.getEnd());

    return *this;
}

//==============================================================================

Path& Path::addRectangle (float x, float y, float width, float height)
{
    width = jmax (0.0f, width);
    height = jmax (0.0f, height);

    reserveSpace (size() + 5);

    moveTo (x, y);
    lineTo (x + width, y);
    lineTo (x + width, y + height);
    lineTo (x, y + height);
    lineTo (x, y);

    return *this;
}

Path& Path::addRectangle (const Rectangle<float>& rect)
{
    return addRectangle (rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

//==============================================================================

Path& Path::addRoundedRectangle (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    reserveSpace (size() + 9);

    width = jmax (0.0f, width);
    height = jmax (0.0f, height);

    const float centerWidth = width * 0.5f;
    const float centerHeight = height * 0.5f;
    radiusTopLeft = jmin (radiusTopLeft, centerWidth, centerHeight);
    radiusTopRight = jmin (radiusTopRight, centerWidth, centerHeight);
    radiusBottomLeft = jmin (radiusBottomLeft, centerWidth, centerHeight);
    radiusBottomRight = jmin (radiusBottomRight, centerWidth, centerHeight);

    // Use the mathematically correct constant for circular arc approximation with cubic Bezier curves
    // This is 4/3 * tan(pi/8) ≈ 0.5522847498f
    constexpr float kappa = 0.5522847498f;

    moveTo (x + radiusTopLeft, y);
    lineTo (x + width - radiusTopRight, y);

    // Top-right corner
    cubicTo (x + width - radiusTopRight + radiusTopRight * kappa, y, x + width, y + radiusTopRight - radiusTopRight * kappa, x + width, y + radiusTopRight);

    lineTo (x + width, y + height - radiusBottomRight);

    // Bottom-right corner
    cubicTo (x + width, y + height - radiusBottomRight + radiusBottomRight * kappa, x + width - radiusBottomRight + radiusBottomRight * kappa, y + height, x + width - radiusBottomRight, y + height);

    lineTo (x + radiusBottomLeft, y + height);

    // Bottom-left corner
    cubicTo (x + radiusBottomLeft - radiusBottomLeft * kappa, y + height, x, y + height - radiusBottomLeft + radiusBottomLeft * kappa, x, y + height - radiusBottomLeft);

    lineTo (x, y + radiusTopLeft);

    // Top-left corner
    cubicTo (x, y + radiusTopLeft - radiusTopLeft * kappa, x + radiusTopLeft - radiusTopLeft * kappa, y, x + radiusTopLeft, y);

    return *this;
}

Path& Path::addRoundedRectangle (float x, float y, float width, float height, float radius)
{
    return addRoundedRectangle (x, y, width, height, radius, radius, radius, radius);
}

Path& Path::addRoundedRectangle (const Rectangle<float>& rect, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    return addRoundedRectangle (rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);
}

Path& Path::addRoundedRectangle (const Rectangle<float>& rect, float radius)
{
    return addRoundedRectangle (rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), radius, radius, radius, radius);
}

//==============================================================================

Path& Path::addEllipse (float x, float y, float width, float height)
{
    reserveSpace (size() + 6);

    width = jmax (0.0f, width);
    height = jmax (0.0f, height);

    const float rx = width * 0.5f;
    const float ry = height * 0.5f;
    const float cx = x + rx;
    const float cy = y + ry;
    const float dx = rx * 0.5522847498f;
    const float dy = ry * 0.5522847498f;

    moveTo (cx + rx, cy);
    cubicTo (cx + rx, cy - dy, cx + dx, cy - ry, cx, cy - ry);
    cubicTo (cx - dx, cy - ry, cx - rx, cy - dy, cx - rx, cy);
    cubicTo (cx - rx, cy + dy, cx - dx, cy + ry, cx, cy + ry);
    cubicTo (cx + dx, cy + ry, cx + rx, cy + dy, cx + rx, cy);
    close();

    return *this;
}

Path& Path::addEllipse (const Rectangle<float>& r)
{
    return addEllipse (r.getX(), r.getY(), r.getWidth(), r.getHeight());
}

//==============================================================================

Path& Path::addCenteredEllipse (float centerX, float centerY, float radiusX, float radiusY)
{
    reserveSpace (size() + 6);

    radiusX = jmax (0.0f, radiusX);
    radiusY = jmax (0.0f, radiusY);

    const float rx = radiusX;
    const float ry = radiusY;
    const float cx = centerX;
    const float cy = centerY;
    const float dx = rx * 0.5522847498f;
    const float dy = ry * 0.5522847498f;

    moveTo (cx + rx, cy);
    cubicTo (cx + rx, cy - dy, cx + dx, cy - ry, cx, cy - ry);
    cubicTo (cx - dx, cy - ry, cx - rx, cy - dy, cx - rx, cy);
    cubicTo (cx - rx, cy + dy, cx - dx, cy + ry, cx, cy + ry);
    cubicTo (cx + dx, cy + ry, cx + rx, cy + dy, cx + rx, cy);
    close();

    return *this;
}

Path& Path::addCenteredEllipse (const Point<float>& center, float radiusX, float radiusY)
{
    return addCenteredEllipse (center.getX(), center.getY(), radiusX, radiusY);
}

Path& Path::addCenteredEllipse (const Point<float>& center, const Size<float>& diameter)
{
    return addCenteredEllipse (center.getX(), center.getY(), diameter.getWidth() / 2.0f, diameter.getHeight() / 2.0f);
}

//==============================================================================

Path& Path::addArc (float x, float y, float width, float height, float fromRadians, float toRadians, bool startAsNewSubPath)
{
    width = jmax (0.0f, width);
    height = jmax (0.0f, height);

    const float radiusX = width * 0.5f;
    const float radiusY = height * 0.5f;

    addCenteredArc (x + radiusX, y + radiusY, radiusX, radiusY, 0.0f, fromRadians, toRadians, startAsNewSubPath);

    return *this;
}

Path& Path::addArc (const Rectangle<float>& rect,
                    float fromRadians,
                    float toRadians,
                    bool startAsNewSubPath)
{
    return addArc (rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), fromRadians, toRadians, startAsNewSubPath);
}

Path& Path::addCenteredArc (float centerX, float centerY, float radiusX, float radiusY, float rotationOfEllipse, float fromRadians, float toRadians, bool startAsNewSubPath)
{
    const int segments = jlimit (2, 54, static_cast<int> (std::ceil (std::abs (toRadians - fromRadians) / 0.1f)));

    const float delta = (toRadians - fromRadians) / segments;
    const float cosTheta = std::cos (rotationOfEllipse);
    const float sinTheta = std::sin (rotationOfEllipse);

    // Initialize variables for the loop
    radiusX = jmax (0.0f, radiusX);
    radiusY = jmax (0.0f, radiusY);

    float x = std::cos (fromRadians) * radiusX;
    float y = std::sin (fromRadians) * radiusY;
    float rotatedX = x * cosTheta - y * sinTheta + centerX;
    float rotatedY = x * sinTheta + y * cosTheta + centerY;

    // Move to the first point if starting a new subpath
    if (startAsNewSubPath)
        moveTo (rotatedX, rotatedY);
    else
        lineTo (rotatedX, rotatedY);

    // Draw lines between points on the arc
    for (int i = 1; i <= segments; i++)
    {
        float angle = fromRadians + i * delta;
        x = std::cos (angle) * radiusX;
        y = std::sin (angle) * radiusY;

        // Apply rotation and translation
        rotatedX = x * cosTheta - y * sinTheta + centerX;
        rotatedY = x * sinTheta + y * cosTheta + centerY;

        // Line to the next point on the arc
        lineTo (rotatedX, rotatedY);
    }

    return *this;
}

Path& Path::addCenteredArc (const Point<float>& center, float radiusX, float radiusY, float rotationOfEllipse, float fromRadians, float toRadians, bool startAsNewSubPath)
{
    return addCenteredArc (center.getX(), center.getY(), radiusX, radiusY, rotationOfEllipse, fromRadians, toRadians, startAsNewSubPath);
}

Path& Path::addCenteredArc (const Point<float>& center, const Size<float>& diameter, float rotationOfEllipse, float fromRadians, float toRadians, bool startAsNewSubPath)
{
    return addCenteredArc (center.getX(), center.getY(), diameter.getWidth() / 2.0f, diameter.getHeight() / 2.0f, rotationOfEllipse, fromRadians, toRadians, startAsNewSubPath);
}

//==============================================================================

void Path::addTriangle (float x1, float y1, float x2, float y2, float x3, float y3)
{
    addTriangle ({ x1, y1 }, { x2, y2 }, { x3, y3 });
}

void Path::addTriangle (const Point<float>& p1, const Point<float>& p2, const Point<float>& p3)
{
    reserveSpace (size() + 4);

    moveTo (p1);
    lineTo (p2);
    lineTo (p3);
    close();
}

//==============================================================================

void Path::addQuadrilateral (float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    addQuadrilateral ({ x1, y1 }, { x2, y2 }, { x3, y3 }, { x4, y4 });
}

void Path::addQuadrilateral (const Point<float>& p1, const Point<float>& p2, const Point<float>& p3, const Point<float>& p4)
{
    reserveSpace (size() + 5);

    moveTo (p1);
    lineTo (p2);
    lineTo (p3);
    lineTo (p4);
    close();
}

//==============================================================================

Path& Path::addPolygon (const Point<float>& centre, int numberOfSides, float radius, float startAngle)
{
    if (numberOfSides < 3)
        return *this;

    reserveSpace (size() + numberOfSides + 1);

    const float angleIncrement = MathConstants<float>::twoPi / numberOfSides;
    radius = jmax (0.0f, radius);

    // Start with the first vertex
    float angle = startAngle;
    float x = centre.getX() + radius * std::cos (angle);
    float y = centre.getY() + radius * std::sin (angle);

    moveTo (x, y);

    // Add remaining vertices
    for (int i = 1; i < numberOfSides; ++i)
    {
        angle += angleIncrement;
        x = centre.getX() + radius * std::cos (angle);
        y = centre.getY() + radius * std::sin (angle);
        lineTo (x, y);
    }

    close();

    return *this;
}

//==============================================================================

Path& Path::addStar (const Point<float>& centre, int numberOfPoints, float innerRadius, float outerRadius, float startAngle)
{
    if (numberOfPoints < 3)
        return *this;

    reserveSpace (size() + numberOfPoints * 2 + 1);

    const float angleIncrement = MathConstants<float>::twoPi / (numberOfPoints * 2);
    innerRadius = jmax (0.0f, innerRadius);
    outerRadius = jmax (0.0f, outerRadius);

    // Start with the first outer vertex
    float angle = startAngle;
    float x = centre.getX() + outerRadius * std::cos (angle);
    float y = centre.getY() + outerRadius * std::sin (angle);

    moveTo (x, y);

    // Alternate between inner and outer vertices
    for (int i = 1; i < numberOfPoints * 2; ++i)
    {
        angle += angleIncrement;
        float currentRadius = (i % 2 == 0) ? outerRadius : innerRadius;
        x = centre.getX() + currentRadius * std::cos (angle);
        y = centre.getY() + currentRadius * std::sin (angle);
        lineTo (x, y);
    }

    close();

    return *this;
}

//==============================================================================

Path& Path::addBubble (const Rectangle<float>& bodyArea, const Rectangle<float>& maximumArea, const Point<float>& arrowTipPosition, float cornerSize, float arrowBaseWidth)
{
    if (bodyArea.isEmpty() || maximumArea.isEmpty() || arrowBaseWidth <= 0.0f)
        return *this;

    // Clamp corner size to reasonable bounds
    cornerSize = jmin (cornerSize, bodyArea.getWidth() * 0.5f, bodyArea.getHeight() * 0.5f);

    // Check if arrow tip is inside the body area - if so, draw no arrow
    if (bodyArea.contains (arrowTipPosition))
    {
        // Just draw a rounded rectangle
        addRoundedRectangle (bodyArea, cornerSize);
        return *this;
    }

    // Determine which side the arrow should be on based on tip position relative to rectangle
    enum ArrowSide
    {
        Left,
        Right,
        Top,
        Bottom
    } arrowSide;

    Point<float> arrowBase1, arrowBase2;

    // Get rectangle center for direction calculation
    Point<float> rectCenter = bodyArea.getCenter();

    // Calculate relative position of arrow tip
    float deltaX = arrowTipPosition.getX() - rectCenter.getX();
    float deltaY = arrowTipPosition.getY() - rectCenter.getY();

    // Determine primary direction - use the larger absolute offset
    if (std::abs (deltaX) > std::abs (deltaY))
    {
        // Horizontal direction is dominant
        if (deltaX < 0)
        {
            // Arrow tip is to the left of rectangle center
            arrowSide = Left;
            // Ensure arrow base doesn't overlap with corner radius
            float minY = bodyArea.getY() + cornerSize + arrowBaseWidth * 0.5f;
            float maxY = bodyArea.getBottom() - cornerSize - arrowBaseWidth * 0.5f;
            float arrowY = jlimit (minY, maxY, arrowTipPosition.getY());
            // For left edge (going bottom to top in clockwise direction)
            arrowBase1 = Point<float> (bodyArea.getX(), arrowY + arrowBaseWidth * 0.5f); // bottom base point
            arrowBase2 = Point<float> (bodyArea.getX(), arrowY - arrowBaseWidth * 0.5f); // top base point
        }
        else
        {
            // Arrow tip is to the right of rectangle center
            arrowSide = Right;
            // Ensure arrow base doesn't overlap with corner radius
            float minY = bodyArea.getY() + cornerSize + arrowBaseWidth * 0.5f;
            float maxY = bodyArea.getBottom() - cornerSize - arrowBaseWidth * 0.5f;
            float arrowY = jlimit (minY, maxY, arrowTipPosition.getY());
            // For right edge (going top to bottom in clockwise direction)
            arrowBase1 = Point<float> (bodyArea.getRight(), arrowY - arrowBaseWidth * 0.5f); // top base point
            arrowBase2 = Point<float> (bodyArea.getRight(), arrowY + arrowBaseWidth * 0.5f); // bottom base point
        }
    }
    else
    {
        // Vertical direction is dominant
        if (deltaY < 0)
        {
            // Arrow tip is above rectangle center
            arrowSide = Top;
            // Ensure arrow base doesn't overlap with corner radius
            float minX = bodyArea.getX() + cornerSize + arrowBaseWidth * 0.5f;
            float maxX = bodyArea.getRight() - cornerSize - arrowBaseWidth * 0.5f;
            float arrowX = jlimit (minX, maxX, arrowTipPosition.getX());
            // For top edge (going left to right in clockwise direction)
            arrowBase1 = Point<float> (arrowX - arrowBaseWidth * 0.5f, bodyArea.getY()); // left base point
            arrowBase2 = Point<float> (arrowX + arrowBaseWidth * 0.5f, bodyArea.getY()); // right base point
        }
        else
        {
            // Arrow tip is below rectangle center
            arrowSide = Bottom;
            // Ensure arrow base doesn't overlap with corner radius
            float minX = bodyArea.getX() + cornerSize + arrowBaseWidth * 0.5f;
            float maxX = bodyArea.getRight() - cornerSize - arrowBaseWidth * 0.5f;
            float arrowX = jlimit (minX, maxX, arrowTipPosition.getX());
            // For bottom edge (going right to left in clockwise direction)
            arrowBase1 = Point<float> (arrowX + arrowBaseWidth * 0.5f, bodyArea.getBottom()); // right base point
            arrowBase2 = Point<float> (arrowX - arrowBaseWidth * 0.5f, bodyArea.getBottom()); // left base point
        }
    }

    // Use the mathematically correct constant for circular arc approximation with cubic Bezier curves
    constexpr float kappa = 0.5522847498f;

    float x = bodyArea.getX();
    float y = bodyArea.getY();
    float width = bodyArea.getWidth();
    float height = bodyArea.getHeight();

    // Start drawing the integrated path clockwise from top-left
    moveTo (x + cornerSize, y);

    // Top edge(left to right)
    if (arrowSide == Top)
    {
        lineTo (arrowBase1.getX(), arrowBase1.getY());
        lineTo (arrowTipPosition.getX(), arrowTipPosition.getY());
        lineTo (arrowBase2.getX(), arrowBase2.getY());
        lineTo (x + width - cornerSize, y);
    }
    else
    {
        lineTo (x + width - cornerSize, y);
    }

    // Top-right corner
    if (cornerSize > 0.0f)
    {
        cubicTo (x + width - cornerSize + cornerSize * kappa, y, x + width, y + cornerSize - cornerSize * kappa, x + width, y + cornerSize);
    }

    // Right edge (top to bottom)
    if (arrowSide == Right)
    {
        lineTo (arrowBase1.getX(), arrowBase1.getY());
        lineTo (arrowTipPosition.getX(), arrowTipPosition.getY());
        lineTo (arrowBase2.getX(), arrowBase2.getY());
        lineTo (x + width, y + height - cornerSize);
    }
    else
    {
        lineTo (x + width, y + height - cornerSize);
    }

    // Bottom-right corner
    if (cornerSize > 0.0f)
    {
        cubicTo (x + width, y + height - cornerSize + cornerSize * kappa, x + width - cornerSize + cornerSize * kappa, y + height, x + width - cornerSize, y + height);
    }

    // Bottom edge (right to left)
    if (arrowSide == Bottom)
    {
        lineTo (arrowBase1.getX(), arrowBase1.getY());
        lineTo (arrowTipPosition.getX(), arrowTipPosition.getY());
        lineTo (arrowBase2.getX(), arrowBase2.getY());
        lineTo (x + cornerSize, y + height);
    }
    else
    {
        lineTo (x + cornerSize, y + height);
    }

    // Bottom-left corner
    if (cornerSize > 0.0f)
    {
        cubicTo (x + cornerSize - cornerSize * kappa, y + height, x, y + height - cornerSize + cornerSize * kappa, x, y + height - cornerSize);
    }

    // Left edge (bottom to top)
    if (arrowSide == Left)
    {
        lineTo (arrowBase1.getX(), arrowBase1.getY());
        lineTo (arrowTipPosition.getX(), arrowTipPosition.getY());
        lineTo (arrowBase2.getX(), arrowBase2.getY());
        lineTo (x, y + cornerSize);
    }
    else
    {
        lineTo (x, y + cornerSize);
    }

    // Top-left corner
    if (cornerSize > 0.0f)
    {
        cubicTo (x, y + cornerSize - cornerSize * kappa, x + cornerSize - cornerSize * kappa, y, x + cornerSize, y);
    }

    // Close the path
    close();

    return *this;
}

//==============================================================================

void Path::startNewSubPath (float x, float y)
{
    moveTo (x, y);
}

void Path::startNewSubPath (const Point<float>& p)
{
    moveTo (p.getX(), p.getY());
}

void Path::closeSubPath()
{
    close();
}

bool Path::isClosed (float tolerance) const
{
    auto first = begin();
    auto last = end();
    if (first == last)
        return false;

    Point<float> firstPoint (0.0f, 0.0f);
    Point<float> lastPoint (0.0f, 0.0f);
    bool hasFirstPoint = false;

    for (; first != last; ++first)
    {
        auto segment = *first;
        if (segment.verb == Path::Verb::MoveTo && ! hasFirstPoint)
        {
            firstPoint = segment.point;
            hasFirstPoint = true;
        }

        if (segment.verb != Path::Verb::Close)
            lastPoint = segment.point;
        else
            return true;
    }

    if (! hasFirstPoint)
        return false;

    return firstPoint.distanceTo (lastPoint) <= tolerance;
}

bool Path::isExplicitlyClosed() const
{
    for (const auto& segment : *this)
    {
        if (segment.verb == Path::Verb::Close)
            return true;
    }

    return false;
}

//==============================================================================

void Path::setUsingNonZeroWinding (bool shouldUseNonZeroWinding)
{
    path->fillRule (shouldUseNonZeroWinding ? rive::FillRule::nonZero : rive::FillRule::evenOdd);
}

bool Path::isUsingNonZeroWinding() const
{
    return path->getFillRule() == rive::FillRule::nonZero;
}

//==============================================================================

Path Path::combinedWith (const Path& other, BooleanOperation operation) const
{
    static constexpr int clipperDecimalPrecision = 3;

    const auto subjectPaths = toClipperPaths (*this);
    if (subjectPaths.empty())
        return (operation == BooleanOperation::Union || operation == BooleanOperation::Xor) ? other : Path();

    const auto clipPaths = toClipperPaths (other);
    if (clipPaths.empty())
        return operation == BooleanOperation::Intersect ? Path() : *this;

    ClipperPathsD resultPaths;
    Clipper2Lib::ClipperD clipper (clipperDecimalPrecision);
    clipper.AddSubject (subjectPaths);
    clipper.AddClip (clipPaths);
    clipper.Execute (toClipperClipType (operation), toClipperFillRule (*this), resultPaths);

    return fromClipperPaths (resultPaths);
}

Path& Path::combineWith (const Path& other, BooleanOperation operation)
{
    *this = combinedWith (other, operation);
    return *this;
}

//==============================================================================

Path& Path::appendPath (const Path& other)
{
    path->addRenderPath (other.getRenderPath(), rive::Mat2D());

    return *this;
}

Path& Path::appendPath (const Path& other, const AffineTransform& transform)
{
    path->addRenderPath (other.getRenderPath(), transform.toMat2D());

    return *this;
}

void Path::appendPath (rive::rcp<rive::RiveRenderPath> other)
{
    path->addRenderPath (other.get(), rive::Mat2D());
}

void Path::appendPath (rive::rcp<rive::RiveRenderPath> other, const AffineTransform& transform)
{
    path->addRenderPath (other.get(), transform.toMat2D());
}

//==============================================================================

void Path::swapWithPath (Path& other) noexcept
{
    path.swap (other.path);
}

//==============================================================================

Path Path::createCopy() const
{
    auto newPath = rive::make_rcp<rive::RiveRenderPath>();
    newPath->addRenderPath (path.get(), {});
    return Path { std::move (newPath) };
}

//==============================================================================

Path& Path::transform (const AffineTransform& t)
{
    auto newPath = rive::make_rcp<rive::RiveRenderPath>();
    newPath->addRenderPath (path.get(), t.toMat2D());
    path = std::move (newPath);
    return *this;
}

Path Path::transformed (const AffineTransform& t) const
{
    auto newPath = rive::make_rcp<rive::RiveRenderPath>();
    newPath->addRenderPath (path.get(), t.toMat2D());
    return Path { std::move (newPath) };
}

//==============================================================================

Rectangle<float> Path::getBounds() const
{
    const auto& aabb = path->getBounds();
    return { aabb.left(), aabb.top(), aabb.width(), aabb.height() };
}

Rectangle<float> Path::getBoundsTransformed (const AffineTransform& transform) const
{
    return getBounds().transformed (transform);
}

//==============================================================================

void Path::scaleToFit (float x, float y, float width, float height, bool preserveProportions) noexcept
{
    if (width <= 0.0f || height <= 0.0f)
        return;

    Rectangle<float> currentBounds = getBounds();

    if (currentBounds.isEmpty())
        return;

    float scaleX = width / currentBounds.getWidth();
    float scaleY = height / currentBounds.getHeight();

    if (preserveProportions)
    {
        float scale = jmin (scaleX, scaleY);
        scaleX = scaleY = scale;
    }

    // Calculate translation to move to target position
    float translateX = x - currentBounds.getX() * scaleX;
    float translateY = y - currentBounds.getY() * scaleY;

    // Apply the transformation
    AffineTransform transform = AffineTransform::scaling (scaleX, scaleY).translated (translateX, translateY);

    *this = transformed (transform);
}

//==============================================================================

Path::Iterator Path::begin()
{
    return Path::Iterator (path->getRawPath(), false);
}

Path::Iterator Path::begin() const
{
    return Path::Iterator (path->getRawPath(), false);
}

Path::Iterator Path::end()
{
    return Path::Iterator (path->getRawPath(), true);
}

Path::Iterator Path::end() const
{
    return Path::Iterator (path->getRawPath(), true);
}

//==============================================================================

rive::RiveRenderPath* Path::getRenderPath() const
{
    return path.get();
}

//==============================================================================

String Path::toString() const
{
    const auto& rawPath = path->getRawPath();
    const auto& points = rawPath.points();
    const auto& verbs = rawPath.verbs();

    if (points.empty() || verbs.empty())
        return String();

    String result;
    result.preallocateBytes (points.size() * 20); // Rough estimate for performance

    size_t pointIndex = 0;

    for (size_t i = 0; i < verbs.size(); ++i)
    {
        auto verb = verbs[i];

        switch (verb)
        {
            case rive::PathVerb::move:
                if (pointIndex < points.size())
                {
                    result << "M " << points[pointIndex].x << " " << points[pointIndex].y << " ";
                    pointIndex++;
                }
                break;

            case rive::PathVerb::line:
                if (pointIndex < points.size())
                {
                    result << "L " << points[pointIndex].x << " " << points[pointIndex].y << " ";
                    pointIndex++;
                }
                break;

            case rive::PathVerb::quad:
                // Rive doesn't seem to use quad verbs based on the existing code
                // But if it does, we'll handle it
                if (pointIndex + 1 < points.size())
                {
                    result << "Q "
                           << points[pointIndex].x << " " << points[pointIndex].y << " "
                           << points[pointIndex + 1].x << " " << points[pointIndex + 1].y << " ";
                    pointIndex += 2;
                }
                break;

            case rive::PathVerb::cubic:
                if (pointIndex + 2 < points.size())
                {
                    result << "C "
                           << points[pointIndex].x << " " << points[pointIndex].y << " "
                           << points[pointIndex + 1].x << " " << points[pointIndex + 1].y << " "
                           << points[pointIndex + 2].x << " " << points[pointIndex + 2].y << " ";
                    pointIndex += 3;
                }
                break;

            case rive::PathVerb::close:
                result << "Z ";
                break;
        }
    }

    // Remove trailing space if present
    if (result.endsWithChar (' '))
        result = result.trimEnd();

    return result;
}

bool Path::fromString (const String& pathData)
{
    // https://dev.w3.org/SVG/tools/svgweb/samples/svg-files/

    String::CharPointerType data = pathData.getCharPointer();

    float currentX = 0.0f, currentY = 0.0f;
    float startX = 0.0f, startY = 0.0f;
    float lastControlX = currentX, lastControlY = currentY;
    float lastQuadX = currentX, lastQuadY = currentY;

    while (! data.isEmpty())
    {
        yup_wchar command = *data;

        data++;

        skipWhitespace (data);

        switch (command)
        {
            case 'M': // Move to absolute
            case 'm': // Move to relative
                handleMoveTo (data, *this, currentX, currentY, startX, startY, command == 'm');
                // Reset control points after move
                lastControlX = currentX;
                lastControlY = currentY;
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            case 'L': // Line to absolute
            case 'l': // Line to relative
                handleLineTo (data, *this, currentX, currentY, command == 'l');
                // Reset control points after line (non-curve command)
                lastControlX = currentX;
                lastControlY = currentY;
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            case 'H': // Horizontal line to absolute
            case 'h': // Horizontal line to relative
                handleHorizontalLineTo (data, *this, currentX, currentY, command == 'h');
                // Reset control points after line (non-curve command)
                lastControlX = currentX;
                lastControlY = currentY;
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            case 'V': // Vertical line to absolute
            case 'v': // Vertical line to relative
                handleVerticalLineTo (data, *this, currentX, currentY, command == 'v');
                // Reset control points after line (non-curve command)
                lastControlX = currentX;
                lastControlY = currentY;
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            case 'Q': // Quadratic Bezier curve to absolute
            case 'q': // Quadratic Bezier curve to relative
                handleQuadTo (data, *this, currentX, currentY, lastQuadX, lastQuadY, command == 'q');
                // Reset cubic control points (quadratic doesn't affect cubic control points)
                lastControlX = currentX;
                lastControlY = currentY;
                break;

            case 'T': // Quadratic Smooth Bezier curve to absolute
            case 't': // Quadratic Smooth Bezier curve to relative
                handleSmoothQuadTo (data, *this, currentX, currentY, lastQuadX, lastQuadY, command == 't');
                // Reset cubic control points (quadratic doesn't affect cubic control points)
                lastControlX = currentX;
                lastControlY = currentY;
                break;

            case 'C': // Cubic Bezier curve to absolute
            case 'c': // Cubic Bezier curve to relative
                handleCubicTo (data, *this, currentX, currentY, lastControlX, lastControlY, command == 'c');
                // Reset quadratic control points (cubic doesn't affect quadratic control points)
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            case 'S': // Cubic Smooth Bezier curve to absolute
            case 's': // Cubic Smooth Bezier curve to relative
                handleSmoothCubicTo (data, *this, currentX, currentY, lastControlX, lastControlY, command == 's');
                // Reset quadratic control points (cubic doesn't affect quadratic control points)
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            case 'A': // Elliptical Arc to absolute
            case 'a': // Elliptical Arc to relative
                handleEllipticalArc (data, *this, currentX, currentY, command == 'a');
                // Reset control points after arc (non-curve command)
                lastControlX = currentX;
                lastControlY = currentY;
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            case 'Z': // Close path
            case 'z': // Close path
                close();
                currentX = startX;
                currentY = startY;
                lastControlX = currentX;
                lastControlY = currentY;
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            default:
                break;
        }

        skipWhitespace (data);
    }

    return true;
}

//==============================================================================

namespace
{

struct PathMeasureSegment
{
    Path::Verb verb = Path::Verb::LineTo;
    Point<float> start;
    Point<float> controlPoint1;
    Point<float> controlPoint2;
    Point<float> end;
    float startDistance = 0.0f;
    float endDistance = 0.0f;
    bool startsSubPath = false;
};

[[nodiscard]] float positiveModulo (float value, float modulus) noexcept
{
    const float result = std::fmod (value, modulus);
    return result < 0.0f ? result + modulus : result;
}

[[nodiscard]] Point<float> evaluateQuadratic (Point<float> p0, Point<float> p1, Point<float> p2, float t) noexcept
{
    const float u = 1.0f - t;
    return {
        u * u * p0.getX() + 2.0f * u * t * p1.getX() + t * t * p2.getX(),
        u * u * p0.getY() + 2.0f * u * t * p1.getY() + t * t * p2.getY()
    };
}

[[nodiscard]] Point<float> evaluateCubic (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3, float t) noexcept
{
    const float u = 1.0f - t;
    const float uu = u * u;
    const float tt = t * t;
    return {
        uu * u * p0.getX() + 3.0f * uu * t * p1.getX() + 3.0f * u * tt * p2.getX() + tt * t * p3.getX(),
        uu * u * p0.getY() + 3.0f * uu * t * p1.getY() + 3.0f * u * tt * p2.getY() + tt * t * p3.getY()
    };
}

[[nodiscard]] float magnitude (Point<float> p) noexcept
{
    return std::sqrt (p.getX() * p.getX() + p.getY() * p.getY());
}

[[nodiscard]] float quadraticSpeed (Point<float> p0, Point<float> p1, Point<float> p2, float t) noexcept
{
    const float u = 1.0f - t;
    return magnitude ({ 2.0f * u * (p1.getX() - p0.getX()) + 2.0f * t * (p2.getX() - p1.getX()),
                        2.0f * u * (p1.getY() - p0.getY()) + 2.0f * t * (p2.getY() - p1.getY()) });
}

[[nodiscard]] float cubicSpeed (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3, float t) noexcept
{
    const float u = 1.0f - t;
    return magnitude ({ 3.0f * u * u * (p1.getX() - p0.getX()) + 6.0f * u * t * (p2.getX() - p1.getX()) + 3.0f * t * t * (p3.getX() - p2.getX()),
                        3.0f * u * u * (p1.getY() - p0.getY()) + 6.0f * u * t * (p2.getY() - p1.getY()) + 3.0f * t * t * (p3.getY() - p2.getY()) });
}

template <typename SpeedFunction>
[[nodiscard]] float integrateSpeedRange (float start, float end, const SpeedFunction& speed)
{
    static constexpr float points[] = { -0.9061798459f, -0.5384693101f, 0.0f, 0.5384693101f, 0.9061798459f };
    static constexpr float weights[] = { 0.2369268850f, 0.4786286705f, 0.5688888889f, 0.4786286705f, 0.2369268850f };

    start = jlimit (0.0f, 1.0f, start);
    end = jlimit (0.0f, 1.0f, end);

    if (end <= start)
        return 0.0f;

    const float halfRange = (end - start) * 0.5f;
    const float center = (start + end) * 0.5f;
    float length = 0.0f;

    for (int i = 0; i < 5; ++i)
        length += weights[i] * speed (halfRange * points[i] + center);

    return length * halfRange;
}

template <typename SpeedFunction>
[[nodiscard]] float integrateSpeedAdaptive (float start, float end, float tolerance, int depth, const SpeedFunction& speed)
{
    const float whole = integrateSpeedRange (start, end, speed);
    const float mid = (start + end) * 0.5f;
    const float split = integrateSpeedRange (start, mid, speed) + integrateSpeedRange (mid, end, speed);

    if (depth >= 8 || std::abs (split - whole) <= tolerance)
        return split;

    return integrateSpeedAdaptive (start, mid, tolerance * 0.5f, depth + 1, speed)
         + integrateSpeedAdaptive (mid, end, tolerance * 0.5f, depth + 1, speed);
}

[[nodiscard]] float getSegmentLengthAt (const PathMeasureSegment& segment, float t, float tolerance)
{
    const float effectiveTolerance = jmax (1.0e-5f, tolerance);

    switch (segment.verb)
    {
        case Path::Verb::LineTo:
        case Path::Verb::Close:
            return segment.start.distanceTo (segment.end) * jlimit (0.0f, 1.0f, t);

        case Path::Verb::QuadTo:
        {
            const auto speed = [&] (float u)
            {
                return quadraticSpeed (segment.start, segment.controlPoint1, segment.end, u);
            };

            return integrateSpeedAdaptive (0.0f, t, effectiveTolerance, 0, speed);
        }

        case Path::Verb::CubicTo:
        {
            const auto speed = [&] (float u)
            {
                return cubicSpeed (segment.start, segment.controlPoint1, segment.controlPoint2, segment.end, u);
            };

            return integrateSpeedAdaptive (0.0f, t, effectiveTolerance, 0, speed);
        }

        case Path::Verb::MoveTo:
            break;
    }

    return 0.0f;
}

[[nodiscard]] float getSegmentSpeed (const PathMeasureSegment& segment, float t)
{
    switch (segment.verb)
    {
        case Path::Verb::LineTo:
        case Path::Verb::Close:
            return segment.start.distanceTo (segment.end);

        case Path::Verb::QuadTo:
            return quadraticSpeed (segment.start, segment.controlPoint1, segment.end, t);

        case Path::Verb::CubicTo:
            return cubicSpeed (segment.start, segment.controlPoint1, segment.controlPoint2, segment.end, t);

        case Path::Verb::MoveTo:
            break;
    }

    return 0.0f;
}

[[nodiscard]] float findParameterForDistance (const PathMeasureSegment& segment, float distance, float tolerance)
{
    const float segmentLength = segment.endDistance - segment.startDistance;

    if (distance <= 0.0f)
        return 0.0f;

    if (distance >= segmentLength)
        return 1.0f;

    float lower = 0.0f;
    float upper = 1.0f;
    float t = jlimit (0.0f, 1.0f, distance / segmentLength);
    const float lengthTolerance = jmax (1.0e-5f, tolerance * 0.01f);

    for (int i = 0; i < 20; ++i)
    {
        const float measured = getSegmentLengthAt (segment, t, tolerance);
        const float error = measured - distance;

        if (std::abs (error) <= lengthTolerance)
            break;

        if (error > 0.0f)
            upper = t;
        else
            lower = t;

        const float speed = getSegmentSpeed (segment, t);
        const float candidate = speed > 1.0e-6f ? t - error / speed : (lower + upper) * 0.5f;

        t = (candidate > lower && candidate < upper) ? candidate : (lower + upper) * 0.5f;
    }

    return jlimit (0.0f, 1.0f, t);
}

void appendMeasuredSegment (std::vector<PathMeasureSegment>& segments,
                            Path::Verb verb,
                            Point<float> start,
                            Point<float> controlPoint1,
                            Point<float> controlPoint2,
                            Point<float> end,
                            float tolerance,
                            float& totalLength,
                            bool& startsSubPath)
{
    PathMeasureSegment segment;
    segment.verb = verb;
    segment.start = start;
    segment.controlPoint1 = controlPoint1;
    segment.controlPoint2 = controlPoint2;
    segment.end = end;
    segment.startDistance = totalLength;
    segment.startsSubPath = startsSubPath;

    const float length = getSegmentLengthAt (segment, 1.0f, tolerance);
    if (length <= 1.0e-5f)
        return;

    segment.endDistance = totalLength + length;
    segments.push_back (segment);
    totalLength += length;
    startsSubPath = false;
}

void splitQuadratic (Point<float> p0,
                     Point<float> p1,
                     Point<float> p2,
                     float t,
                     Point<float> left[3],
                     Point<float> right[3]) noexcept
{
    const auto p01 = p0.pointBetween (p1, t);
    const auto p12 = p1.pointBetween (p2, t);
    const auto p012 = p01.pointBetween (p12, t);

    left[0] = p0;
    left[1] = p01;
    left[2] = p012;
    right[0] = p012;
    right[1] = p12;
    right[2] = p2;
}

void splitCubic (Point<float> p0,
                 Point<float> p1,
                 Point<float> p2,
                 Point<float> p3,
                 float t,
                 Point<float> left[4],
                 Point<float> right[4]) noexcept
{
    const auto p01 = p0.pointBetween (p1, t);
    const auto p12 = p1.pointBetween (p2, t);
    const auto p23 = p2.pointBetween (p3, t);
    const auto p012 = p01.pointBetween (p12, t);
    const auto p123 = p12.pointBetween (p23, t);
    const auto p0123 = p012.pointBetween (p123, t);

    left[0] = p0;
    left[1] = p01;
    left[2] = p012;
    left[3] = p0123;
    right[0] = p0123;
    right[1] = p123;
    right[2] = p23;
    right[3] = p3;
}

void appendTrimmedQuadratic (Path& result, const PathMeasureSegment& segment, float t0, float t1)
{
    Point<float> left[3];
    Point<float> right[3];
    splitQuadratic (segment.start, segment.controlPoint1, segment.end, t1, left, right);

    Point<float> piece[3] = { left[0], left[1], left[2] };
    if (t0 > 0.0f)
    {
        const float localT = t1 > 0.0f ? t0 / t1 : 0.0f;
        splitQuadratic (piece[0], piece[1], piece[2], localT, left, right);
        piece[0] = right[0];
        piece[1] = right[1];
        piece[2] = right[2];
    }

    result.quadTo (piece[2].getX(), piece[2].getY(), piece[1].getX(), piece[1].getY());
}

void appendTrimmedCubic (Path& result, const PathMeasureSegment& segment, float t0, float t1)
{
    Point<float> left[4];
    Point<float> right[4];
    splitCubic (segment.start, segment.controlPoint1, segment.controlPoint2, segment.end, t1, left, right);

    Point<float> piece[4] = { left[0], left[1], left[2], left[3] };
    if (t0 > 0.0f)
    {
        const float localT = t1 > 0.0f ? t0 / t1 : 0.0f;
        splitCubic (piece[0], piece[1], piece[2], piece[3], localT, left, right);
        piece[0] = right[0];
        piece[1] = right[1];
        piece[2] = right[2];
        piece[3] = right[3];
    }

    result.cubicTo (piece[1].getX(), piece[1].getY(), piece[2].getX(), piece[2].getY(), piece[3].getX(), piece[3].getY());
}

[[nodiscard]] std::vector<PathMeasureSegment> collectMeasuredSegments (const Path& path, float tolerance, float& totalLength)
{
    std::vector<PathMeasureSegment> segments;
    totalLength = 0.0f;

    Point<float> current;
    Point<float> subPathStart;
    bool hasCurrent = false;
    bool startsSubPath = true;

    for (const auto segment : path)
    {
        switch (segment.verb)
        {
            case Path::Verb::MoveTo:
                current = segment.point;
                subPathStart = current;
                hasCurrent = true;
                startsSubPath = true;
                break;

            case Path::Verb::LineTo:
                if (hasCurrent)
                    appendMeasuredSegment (segments, Path::Verb::LineTo, current, {}, {}, segment.point, tolerance, totalLength, startsSubPath);

                current = segment.point;
                hasCurrent = true;
                break;

            case Path::Verb::QuadTo:
                if (hasCurrent)
                    appendMeasuredSegment (segments, Path::Verb::QuadTo, current, segment.controlPoint1, {}, segment.point, tolerance, totalLength, startsSubPath);

                current = segment.point;
                hasCurrent = true;
                break;

            case Path::Verb::CubicTo:
                if (hasCurrent)
                    appendMeasuredSegment (segments, Path::Verb::CubicTo, current, segment.controlPoint1, segment.controlPoint2, segment.point, tolerance, totalLength, startsSubPath);

                current = segment.point;
                hasCurrent = true;
                break;

            case Path::Verb::Close:
                if (hasCurrent)
                    appendMeasuredSegment (segments, Path::Verb::Close, current, {}, {}, subPathStart, tolerance, totalLength, startsSubPath);

                current = subPathStart;
                hasCurrent = false;
                startsSubPath = true;
                break;
        }
    }

    return segments;
}

void appendMeasuredRange (Path& result,
                          const std::vector<PathMeasureSegment>& segments,
                          float totalLength,
                          float rangeStart,
                          float rangeEnd,
                          float tolerance)
{
    const float startDistance = jlimit (0.0f, totalLength, rangeStart * totalLength);
    const float endDistance = jlimit (0.0f, totalLength, rangeEnd * totalLength);

    if (endDistance <= startDistance)
        return;

    bool hasOpenSubPath = false;
    float previousDistance = -1.0f;

    for (const auto& segment : segments)
    {
        const float overlapStart = jmax (segment.startDistance, startDistance);
        const float overlapEnd = jmin (segment.endDistance, endDistance);

        if (overlapEnd <= overlapStart)
            continue;

        const float t0 = findParameterForDistance (segment, overlapStart - segment.startDistance, tolerance);
        const float t1 = findParameterForDistance (segment, overlapEnd - segment.startDistance, tolerance);

        if (t1 <= t0 + 1.0e-6f)
            continue;

        const auto p0 = [&]
        {
            switch (segment.verb)
            {
                case Path::Verb::QuadTo:
                    return evaluateQuadratic (segment.start, segment.controlPoint1, segment.end, t0);

                case Path::Verb::CubicTo:
                    return evaluateCubic (segment.start, segment.controlPoint1, segment.controlPoint2, segment.end, t0);

                case Path::Verb::LineTo:
                case Path::Verb::Close:
                case Path::Verb::MoveTo:
                    return segment.start.pointBetween (segment.end, t0);
            }

            return segment.start;
        }();

        if (! hasOpenSubPath || segment.startsSubPath || std::abs (overlapStart - previousDistance) > 1.0e-3f)
        {
            result.moveTo (p0);
            hasOpenSubPath = true;
        }

        switch (segment.verb)
        {
            case Path::Verb::QuadTo:
                appendTrimmedQuadratic (result, segment, t0, t1);
                break;

            case Path::Verb::CubicTo:
                appendTrimmedCubic (result, segment, t0, t1);
                break;

            case Path::Verb::LineTo:
            case Path::Verb::Close:
            case Path::Verb::MoveTo:
                result.lineTo (segment.start.pointBetween (segment.end, t1));
                break;
        }

        previousDistance = overlapEnd;
    }
}

} // namespace

float Path::getLength (float tolerance) const
{
    float totalLength = 0.0f;
    (void) collectMeasuredSegments (*this, tolerance, totalLength);
    return totalLength;
}

Point<float> Path::getPointAlongPath (float distance) const
{
    const float normalizedDistance = jlimit (0.0f, 1.0f, distance);

    float totalLength = 0.0f;
    const auto segments = collectMeasuredSegments (*this, 0.5f, totalLength);

    if (segments.empty() || totalLength <= 1.0e-5f)
        return Point<float> (0.0f, 0.0f);

    const float targetDistance = normalizedDistance * totalLength;
    for (const auto& segment : segments)
    {
        if (targetDistance <= segment.endDistance)
        {
            const float t = findParameterForDistance (segment, targetDistance - segment.startDistance, 0.5f);

            switch (segment.verb)
            {
                case Path::Verb::QuadTo:
                    return evaluateQuadratic (segment.start, segment.controlPoint1, segment.end, t);

                case Path::Verb::CubicTo:
                    return evaluateCubic (segment.start, segment.controlPoint1, segment.controlPoint2, segment.end, t);

                case Path::Verb::LineTo:
                case Path::Verb::Close:
                case Path::Verb::MoveTo:
                    return segment.start.pointBetween (segment.end, t);
            }
        }
    }

    return segments.back().end;
}

Path Path::getTrimmedPath (float start, float end, float offset, float tolerance) const
{
    const float span = end - start;
    if (std::abs (span) >= 1.0f)
        return createCopy();

    const float normalizedStart = positiveModulo (start + offset, 1.0f);
    const float normalizedEnd = positiveModulo (end + offset, 1.0f);

    if (std::abs (normalizedStart - normalizedEnd) <= 1.0e-6f)
        return {};

    float totalLength = 0.0f;
    const auto segments = collectMeasuredSegments (*this, tolerance, totalLength);

    if (segments.empty() || totalLength <= 1.0e-5f)
        return {};

    Path result;
    if (normalizedStart < normalizedEnd)
    {
        appendMeasuredRange (result, segments, totalLength, normalizedStart, normalizedEnd, tolerance);
    }
    else
    {
        appendMeasuredRange (result, segments, totalLength, normalizedStart, 1.0f, tolerance);
        appendMeasuredRange (result, segments, totalLength, 0.0f, normalizedEnd, tolerance);
    }

    result.setUsingNonZeroWinding (isUsingNonZeroWinding());
    return result;
}

Path& Path::trim (float start, float end, float offset, float tolerance)
{
    *this = getTrimmedPath (start, end, offset, tolerance);
    return *this;
}

//==============================================================================

Path Path::createStrokePolygon (float strokeWidth) const
{
    // For now, create a simple approximation by offsetting the path
    // This is a basic implementation - a more sophisticated version would
    // properly handle joins, caps, and curves

    const auto& rawPath = path->getRawPath();
    const auto& points = rawPath.points();
    const auto& verbs = rawPath.verbs();

    if (points.empty() || verbs.empty())
        return Path();

    Path strokePath;
    float halfWidth = strokeWidth * 0.5f;

    // Simple approach: for each line segment, create perpendicular offsets
    Point<float> currentPoint (0.0f, 0.0f);
    Point<float> lastMovePoint (0.0f, 0.0f);

    std::vector<Point<float>> leftSide;
    leftSide.reserve (points.size());
    std::vector<Point<float>> rightSide;
    rightSide.reserve (points.size());

    for (size_t i = 0, pointIndex = 0; i < verbs.size(); ++i)
    {
        auto verb = verbs[i];

        switch (verb)
        {
            case rive::PathVerb::move:
                if (pointIndex < points.size())
                {
                    currentPoint = Point<float> (points[pointIndex].x, points[pointIndex].y);
                    lastMovePoint = currentPoint;
                    leftSide.clear();
                    rightSide.clear();
                    pointIndex++;
                }
                break;

            case rive::PathVerb::line:
                if (pointIndex < points.size())
                {
                    Point<float> nextPoint (points[pointIndex].x, points[pointIndex].y);

                    // Calculate perpendicular direction
                    Point<float> direction = nextPoint - currentPoint;
                    float length = direction.magnitude();
                    if (length > 0.0f)
                    {
                        direction.normalize();
                        Point<float> perpendicular (-direction.getY(), direction.getX());

                        Point<float> leftOffset = perpendicular * halfWidth;
                        Point<float> rightOffset = perpendicular * (-halfWidth);

                        if (leftSide.empty())
                        {
                            leftSide.push_back (currentPoint + leftOffset);
                            rightSide.push_back (currentPoint + rightOffset);
                        }

                        leftSide.push_back (nextPoint + leftOffset);
                        rightSide.push_back (nextPoint + rightOffset);
                    }

                    currentPoint = nextPoint;
                    pointIndex++;
                }
                break;

            case rive::PathVerb::quad:
            case rive::PathVerb::cubic:
                // For curves, approximate with line segments
                if (verb == rive::PathVerb::quad && pointIndex + 1 < points.size())
                {
                    Point<float> endPoint (points[pointIndex + 1].x, points[pointIndex + 1].y);

                    Point<float> direction = endPoint - currentPoint;
                    float length = direction.magnitude();
                    if (length > 0.0f)
                    {
                        direction.normalize();
                        Point<float> perpendicular (-direction.getY(), direction.getX());

                        Point<float> leftOffset = perpendicular * halfWidth;
                        Point<float> rightOffset = perpendicular * (-halfWidth);

                        if (leftSide.empty())
                        {
                            leftSide.push_back (currentPoint + leftOffset);
                            rightSide.push_back (currentPoint + rightOffset);
                        }

                        leftSide.push_back (endPoint + leftOffset);
                        rightSide.push_back (endPoint + rightOffset);
                    }

                    currentPoint = endPoint;
                    pointIndex += 2;
                }
                else if (verb == rive::PathVerb::cubic && pointIndex + 2 < points.size())
                {
                    Point<float> endPoint (points[pointIndex + 2].x, points[pointIndex + 2].y);

                    Point<float> direction = endPoint - currentPoint;
                    float length = direction.magnitude();
                    if (length > 0.0f)
                    {
                        direction.normalize();
                        Point<float> perpendicular (-direction.getY(), direction.getX());

                        Point<float> leftOffset = perpendicular * halfWidth;
                        Point<float> rightOffset = perpendicular * (-halfWidth);

                        if (leftSide.empty())
                        {
                            leftSide.push_back (currentPoint + leftOffset);
                            rightSide.push_back (currentPoint + rightOffset);
                        }

                        leftSide.push_back (endPoint + leftOffset);
                        rightSide.push_back (endPoint + rightOffset);
                    }

                    currentPoint = endPoint;
                    pointIndex += 3;
                }
                break;

            case rive::PathVerb::close:
                // Connect back to start and create the stroke polygon
                if (! leftSide.empty() && ! rightSide.empty())
                {
                    // Create the stroke polygon by combining left and right sides
                    strokePath.moveTo (leftSide[0]);

                    // Add all left side points
                    for (size_t j = 1; j < leftSide.size(); ++j)
                        strokePath.lineTo (leftSide[j]);

                    // Add all right side points in reverse order
                    for (int j = static_cast<int> (rightSide.size()) - 1; j >= 0; --j)
                        strokePath.lineTo (rightSide[j]);

                    strokePath.close();
                }

                currentPoint = lastMovePoint;
                leftSide.clear();
                rightSide.clear();
                break;
        }
    }

    // If path wasn't closed, still create stroke polygon
    if (! leftSide.empty() && ! rightSide.empty())
    {
        strokePath.moveTo (leftSide[0]);

        for (size_t j = 1; j < leftSide.size(); ++j)
            strokePath.lineTo (leftSide[j]);

        for (int j = static_cast<int> (rightSide.size()) - 1; j >= 0; --j)
            strokePath.lineTo (rightSide[j]);

        strokePath.close();
    }

    return strokePath;
}

//==============================================================================

Path Path::withRoundedCorners (float cornerRadius) const
{
    if (cornerRadius <= 0.0f || path == nullptr)
        return *this;

    const auto& rawPath = path->getRawPath();
    const auto& points = rawPath.points();
    const auto& verbs = rawPath.verbs();

    if (points.empty() || verbs.empty())
        return Path();

    Path roundedPath;
    Point<float> currentPoint (0.0f, 0.0f);
    Point<float> lastMovePoint (0.0f, 0.0f);
    Point<float> previousPoint (0.0f, 0.0f);
    bool hasPreviousPoint = false;

    std::vector<Point<float>> pathPoints;
    pathPoints.reserve (points.size());

    for (size_t i = 0, pointIndex = 0; i < verbs.size(); ++i)
    {
        auto verb = verbs[i];

        switch (verb)
        {
            case rive::PathVerb::move:
                if (pointIndex < points.size())
                {
                    if (! pathPoints.empty())
                    {
                        // Process previous subpath
                        if (pathPoints.size() >= 3)
                            addRoundedSubpath (roundedPath, pathPoints, cornerRadius, false);

                        pathPoints.clear();
                    }

                    currentPoint = Point<float> (points[pointIndex].x, points[pointIndex].y);
                    lastMovePoint = currentPoint;
                    pathPoints.push_back (currentPoint);
                    pointIndex++;
                }
                break;

            case rive::PathVerb::line:
                if (pointIndex < points.size())
                {
                    currentPoint = Point<float> (points[pointIndex].x, points[pointIndex].y);
                    pathPoints.push_back (currentPoint);
                    pointIndex++;
                }
                break;

            case rive::PathVerb::quad:
                if (pointIndex + 1 < points.size())
                {
                    currentPoint = Point<float> (points[pointIndex + 1].x, points[pointIndex + 1].y);
                    pathPoints.push_back (currentPoint);
                    pointIndex += 2;
                }
                break;

            case rive::PathVerb::cubic:
                if (pointIndex + 2 < points.size())
                {
                    currentPoint = Point<float> (points[pointIndex + 2].x, points[pointIndex + 2].y);
                    pathPoints.push_back (currentPoint);
                    pointIndex += 3;
                }
                break;

            case rive::PathVerb::close:
                if (pathPoints.size() >= 3)
                    addRoundedSubpath (roundedPath, pathPoints, cornerRadius, true);

                pathPoints.clear();
                currentPoint = lastMovePoint;
                break;
        }
    }

    // Handle remaining subpath
    if (pathPoints.size() >= 3)
        addRoundedSubpath (roundedPath, pathPoints, cornerRadius, false);

    return roundedPath;
}

//==============================================================================

bool operator== (const Path& lhs, const Path& rhs) noexcept
{
    return lhs.path == rhs.path;
}

bool operator!= (const Path& lhs, const Path& rhs) noexcept
{
    return lhs.path != rhs.path;
}

} // namespace yup
