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

    path->cubic (rive::Vec2D::lerp (last, rive::Vec2D (x1, y1), 2 / 3.f),
                 rive::Vec2D::lerp (rive::Vec2D (x, y), rive::Vec2D (x1, y1), 2 / 3.f),
                 rive::Vec2D (x, y));

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
    reserveSpace (size() + 10);

    radiusTopLeft = jmin (radiusTopLeft, jmin (width / 2.0f, height / 2.0f));
    radiusTopRight = jmin (radiusTopRight, jmin (width / 2.0f, height / 2.0f));
    radiusBottomLeft = jmin (radiusBottomLeft, jmin (width / 2.0f, height / 2.0f));
    radiusBottomRight = jmin (radiusBottomRight, jmin (width / 2.0f, height / 2.0f));

    moveTo (x + radiusTopLeft, y);
    lineTo (x + width - radiusTopRight, y);
    cubicTo (x + width - radiusTopRight * 0.55f, y, x + width, y + radiusTopRight * 0.45f, x + width, y + radiusTopRight);
    lineTo (x + width, y + height - radiusBottomRight);
    cubicTo (x + width, y + height - radiusBottomRight * 0.55f, x + width - radiusBottomRight * 0.55f, y + height, x + width - radiusBottomRight, y + height);
    lineTo (x + radiusBottomLeft, y + height);
    cubicTo (x + radiusBottomLeft * 0.55f, y + height, x, y + height - radiusBottomLeft * 0.55f, x, y + height - radiusBottomLeft);
    lineTo (x, y + radiusTopLeft);
    cubicTo (x, y + radiusTopLeft * 0.55f, x + radiusTopLeft * 0.55f, y, x + radiusTopLeft, y);
    lineTo (x + radiusTopLeft, y);

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

    const float rx = width * 0.5f;
    const float ry = height * 0.5f;
    const float cx = x + rx;
    const float cy = y + ry;
    const float dx = rx * 0.5522847498;
    const float dy = ry * 0.5522847498;

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

    const float rx = radiusX;
    const float ry = radiusY;
    const float cx = centerX;
    const float cy = centerY;
    const float dx = rx * 0.5522847498;
    const float dy = ry * 0.5522847498;

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
    const int segments = jlimit (2, 54, static_cast<int> ((toRadians - fromRadians) / 0.1f));

    const float delta = (toRadians - fromRadians) / segments;
    const float cosTheta = std::cos (rotationOfEllipse);
    const float sinTheta = std::sin (rotationOfEllipse);

    // Initialize variables for the loop
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
Rectangle<float> Path::getBoundingBox() const
{
    const auto& aabb = path->getBounds();
    return { aabb.left(), aabb.top(), aabb.width(), aabb.height() };
}

rive::RiveRenderPath* Path::getRenderPath() const
{
    return path.get();
}

//==============================================================================
namespace
{
bool isControlMarker (String::CharPointerType data)
{
    return ! data.isEmpty() && String ("MmLlHhVvQqCcSsZz").containsChar (*data);
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

bool parseFlag (String::CharPointerType& data, int& flag)
{
    skipWhitespace (data);

    String number;

    while (! data.isEmpty())
    {
        if (data.isWhitespace() || *data == '.' || *data == ',' || *data == '-' || isControlMarker (data))
            break;

        if (! (*data >= '0' && *data <= '9'))
            break;

        number += *data;
        ++data;
    }

    if (number.isNotEmpty())
    {
        flag = number.getIntValue();

        skipWhitespaceOrComma (data);
        return true;
    }

    return false;
}

bool parseCoordinate (String::CharPointerType& data, float& coord)
{
    skipWhitespace (data);

    String number;
    bool isNegative = false;
    bool pointFound = false;

    if (*data == '-')
    {
        isNegative = true;
        ++data;
    }

    while (! data.isEmpty())
    {
        if (data.isWhitespace() || *data == ',' || *data == '-' || isControlMarker (data))
            break;

        if (*data == '.')
        {
            if (pointFound)
                break;
            pointFound = true;
        }
        else if (! (*data >= '0' && *data <= '9'))
        {
            break;
        }

        number += *data;
        ++data;
    }

    if (number.isNotEmpty())
    {
        coord = number.getFloatValue();
        if (isNegative)
            coord = -coord;

        skipWhitespaceOrComma (data);
        return true;
    }

    return false;
}

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

void handleMoveTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, float& startX, float& startY, bool relative)
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

        path.moveTo (x, y);

        currentX = startX = x;
        currentY = startY = y;

        skipWhitespace (data);
    }
}

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

void handleQuadTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, bool relative)
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

        path.quadTo (x1, y1, x, y);

        currentX = x;
        currentY = y;

        skipWhitespace (data);
    }
}

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

        path.quadTo (cx, cy, x, y);

        // Update the current position
        currentX = x;
        currentY = y;

        // Update the last control point for reflection
        lastQuadX = cx;
        lastQuadY = cy;

        skipWhitespace (data);
    }
}

void handleCubicTo (String::CharPointerType& data, Path& path, float& currentX, float& currentY, bool relative)
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

        skipWhitespace (data);
    }
}

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
            float sqrtFactor = std::sqrt ((rxSq * rySq - rxSq * y1PrimeSq - rySq * x1PrimeSq) / (rxSq * y1PrimeSq + rySq * x1PrimeSq));
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

            // Add the arc to the path
            path.addCenteredArc (centreX, centreY, rx, ry, xAxisRotation, startAngle, startAngle + deltaAngle, true);

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

} // namespace

bool Path::parsePathData (const String& pathData)
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
                break;

            case 'L': // Line to absolute
            case 'l': // Line to relative
                handleLineTo (data, *this, currentX, currentY, command == 'l');
                break;

            case 'H': // Horizontal line to absolute
            case 'h': // Horizontal line to relative
                handleHorizontalLineTo (data, *this, currentX, currentY, command == 'h');
                break;

            case 'V': // Vertical line to absolute
            case 'v': // Vertical line to relative
                handleVerticalLineTo (data, *this, currentX, currentY, command == 'v');
                break;

            case 'Q': // Quadratic Bezier curve to absolute
            case 'q': // Quadratic Bezier curve to relative
                handleQuadTo (data, *this, currentX, currentY, command == 'q');
                lastQuadX = currentX;
                lastQuadY = currentY;
                break;

            case 'T': // Quadratic Smooth Bezier curve to absolute
            case 't': // Quadratic Smooth Bezier curve to relative
                handleSmoothQuadTo (data, *this, currentX, currentY, lastQuadX, lastQuadY, command == 'q');
                break;

            case 'C': // Cubic Bezier curve to absolute
            case 'c': // Cubic Bezier curve to relative
                handleCubicTo (data, *this, currentX, currentY, command == 'c');
                lastControlX = currentX;
                lastControlY = currentY;
                break;

            case 'S': // Cubic Smooth Bezier curve to absolute
            case 's': // Cubic Smooth Bezier curve to relative
                handleSmoothCubicTo (data, *this, currentX, currentY, lastControlX, lastControlY, command == 's');
                break;

            case 'A': // Elliptical Arc to absolute
            case 'a': // Elliptical Arc to relative
                handleEllipticalArc (data, *this, currentX, currentY, command == 'a');
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

} // namespace yup
