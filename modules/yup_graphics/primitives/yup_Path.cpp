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

Path::Path (float x, float y) noexcept
{
    moveTo (x, y);
}

Path::Path (const Point<float>& p) noexcept
{
    moveTo (p);
}

//==============================================================================

void Path::reserveSpace (int numSegments)
{
    data.reserve (static_cast<std::size_t> (numSegments));
}

//==============================================================================

int Path::size() const
{
    return static_cast<int> (data.size());
}

//==============================================================================

void Path::clear()
{
    data.clear();
    lastSubpathIndex = -1;
    boundingBox = {};
}

//==============================================================================

void Path::moveTo (float x, float y)
{
    if (!data.empty())
    {
        auto& segment = data.back();
        if (segment.type == SegmentType::MoveTo)
        {
            segment.x = x;
            segment.y = y;
            return;
        }
    }

    lastSubpathIndex = static_cast<int> (data.size());
    data.emplace_back (SegmentType::MoveTo, x, y);

    updateBoundingBox (x, y);
}

void Path::moveTo (const Point<float>& p)
{
    moveTo (p.getX(), p.getY());
}

//==============================================================================

void Path::lineTo (float x, float y)
{
    data.emplace_back (SegmentType::LineTo, x, y);

    updateBoundingBox (x, y);
}

void Path::lineTo (const Point<float>& p)
{
    lineTo (p.getX(), p.getY());
}

//==============================================================================

void Path::quadTo (float x, float y, float x1, float y1)
{
    data.emplace_back (SegmentType::QuadTo, x, y, x1, y1);

    updateBoundingBox (x, y);
}

void Path::quadTo (const Point<float>& p, float x1, float y1)
{
    quadTo (p.getX(), p.getY(), x1, y1);
}

//==============================================================================

void Path::cubicTo (float x, float y, float x1, float y1, float x2, float y2)
{
    data.emplace_back (SegmentType::CubicTo, x, y, x1, y1, x2, y2);

    updateBoundingBox (x, y);
}

void Path::cubicTo (const Point<float>& p, float x1, float y1, float x2, float y2)
{
    cubicTo (p.getX(), p.getY(), x1, y1, x2, y2);
}

//==============================================================================

void Path::close()
{
    if (data.empty())
        return;

    if (isPositiveAndBelow(lastSubpathIndex, static_cast<int> (data.size())))
    {
        const auto& segment = data[lastSubpathIndex];
        lineTo (segment.x, segment.y);
    }
}

//==============================================================================

void Path::addLine (const Point<float>& p1, const Point<float>& p2)
{
    moveTo (p1);
    lineTo (p2);
}

void Path::addLine (const Line<float>& line)
{
    moveTo (line.getStart());
    lineTo (line.getEnd());
}

//==============================================================================

void Path::addRectangle (float x, float y, float width, float height)
{
    reserveSpace (size() + 5);

    moveTo (x, y);
    lineTo (x + width, y);
    lineTo (x + width, y + height);
    lineTo (x, y + height);
    lineTo (x, y);
}

void Path::addRectangle (const Rectangle<float>& rect)
{
    addRectangle (rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

//==============================================================================

void Path::addRoundedRectangle (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
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
}

void Path::addRoundedRectangle (const Rectangle<float>& rect, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    addRoundedRectangle (rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);
}

//==============================================================================

void Path::addEllipse (float x, float y, float width, float height)
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
}

void Path::addEllipse (const Rectangle<float>& r)
{
    addEllipse (r.getX(), r.getY(), r.getWidth(), r.getHeight());
}

//==============================================================================

void Path::addArc (float x, float y, float width, float height,
                   float fromRadians, float toRadians,
                   bool startAsNewSubPath)
{
    const float radiusX = width * 0.5f;
    const float radiusY = height * 0.5f;

    addCenteredArc (x + radiusX, y + radiusY, radiusX, radiusY,
                    0.0f, fromRadians, toRadians,
                    startAsNewSubPath);
}

void Path::addArc (const Rectangle<float>& rect,
                   float fromRadians, float toRadians,
                   bool startAsNewSubPath)
{
    addArc (rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), fromRadians, toRadians, startAsNewSubPath);
}

void Path::addCenteredArc (float centerX, float centerY, float radiusX, float radiusY,
                           float rotationOfEllipse, float fromRadians, float toRadians,
                           bool startAsNewSubPath)
{
    const int segments = jlimit (2, 54, static_cast<int> ((toRadians - fromRadians) / 0.1f));

    const float delta = (toRadians - fromRadians) / segments;
    const float cosTheta = std::cos (rotationOfEllipse);
    const float sinTheta = std::sin (rotationOfEllipse);

    // Initialize variables for the loop
    float angle = fromRadians;
    float x = std::cos (angle) * radiusX;
    float y = std::sin (angle) * radiusY;
    float rotatedX = x * cosTheta - y * sinTheta + centerX;
    float rotatedY = x * sinTheta + y * cosTheta + centerY;

    // Move to the first point if starting a new subpath
    if (startAsNewSubPath)
        moveTo (rotatedX, rotatedY);

    // Draw lines between points on the arc
    for (int i = 1; i <= segments; i++)
    {
        angle = fromRadians + i * delta;
        x = std::cos (angle) * radiusX;
        y = std::sin (angle) * radiusY;

        // Apply rotation and translation
        rotatedX = x * cosTheta - y * sinTheta + centerX;
        rotatedY = x * sinTheta + y * cosTheta + centerY;

        // Line to the next point on the arc
        lineTo (rotatedX, rotatedY);
    }
}

void Path::addCenteredArc (const Point<float>& center, float radiusX, float radiusY,
                           float rotationOfEllipse, float fromRadians, float toRadians,
                           bool startAsNewSubPath)
{
    addCenteredArc (center.getX(), center.getY(), radiusX, radiusY,
                    rotationOfEllipse, fromRadians, toRadians,
                    startAsNewSubPath);
}

//==============================================================================
void Path::appendPath (const Path& other)
{
    reserveSpace (size() + other.size());

    for (const auto& segment : other)
        data.push_back (segment);

    boundingBox = boundingBox.smallestContainingRectangle (other.boundingBox);
}

void Path::appendPath (const Path& other, const AffineTransform& transform)
{
    reserveSpace (size() + other.size());

    for (auto segment : other)
    {
        if (segment.type == Path::SegmentType::MoveTo)
        {
            transform.transformPoints (segment.x, segment.y);
            moveTo (segment.x, segment.y);
        }
        else if (segment.type == Path::SegmentType::LineTo)
        {
            transform.transformPoints (segment.x, segment.y);
            lineTo (segment.x, segment.y);
        }
        else if (segment.type == Path::SegmentType::QuadTo)
        {
            transform.transformPoints (segment.x, segment.y, segment.x1, segment.y1);
            quadTo (segment.x, segment.y, segment.x1, segment.y1);
        }
        else if (segment.type == Path::SegmentType::CubicTo)
        {
            transform.transformPoints (segment.x, segment.y, segment.x1, segment.y1, segment.x2, segment.y2);
            cubicTo (segment.x, segment.y, segment.x1, segment.y1, segment.x2, segment.y2);
        }
    }
}

//==============================================================================
Rectangle<float> Path::getBoundingBox() const
{
    return boundingBox;
}

//==============================================================================
void Path::updateBoundingBox (float x, float y)
{
    if (x < boundingBox.getX())
        boundingBox = boundingBox.withX (x);
    else if (auto right = boundingBox.getBottomRight().getY(); x > right)
        boundingBox = boundingBox.enlargedRight (x - right);

    if (y < boundingBox.getY())
        boundingBox = boundingBox.withY (y);
    else if (auto left = boundingBox.getBottomRight().getY(); y > left)
        boundingBox = boundingBox.enlargedBottom (y - left);
}

} // namespace yup
