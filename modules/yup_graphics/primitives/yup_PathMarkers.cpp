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
PathMarkers::PathMarkers (const Path& path)
{
    Point<float> firstPoint, previousPoint, subpathStart;
    bool hasFirstPoint = false;
    float previousTangent = 0.0f;
    bool firstSegmentAfterMove = false;

    for (const auto& segment : path)
    {
        if (segment.verb == Path::Verb::MoveTo)
        {
            firstPoint = segment.point;
            previousPoint = segment.point;
            subpathStart = segment.point;
            hasFirstPoint = true;
            firstSegmentAfterMove = true;
            previousTangent = 0.0f;
            ends.clear();
            continue;
        }

        if (! hasFirstPoint)
            continue;

        if (segment.verb == Path::Verb::Close)
        {
            if (! firstSegmentAfterMove)
                mids.push_back ({ previousPoint, previousTangent });

            previousPoint = subpathStart;
            firstSegmentAfterMove = true;
            ends.clear();
            continue;
        }

        Point<float> startDirectionPoint = segment.point;
        Point<float> endDirectionPoint = previousPoint;

        if (segment.verb == Path::Verb::QuadTo)
        {
            startDirectionPoint = segment.controlPoint1;
            endDirectionPoint = segment.controlPoint1;
        }
        else if (segment.verb == Path::Verb::CubicTo)
        {
            startDirectionPoint = segment.controlPoint1;
            endDirectionPoint = segment.controlPoint2;
        }

        const auto startAngle = std::atan2 (startDirectionPoint.getY() - previousPoint.getY(),
                                            startDirectionPoint.getX() - previousPoint.getX());
        const auto endAngle = std::atan2 (segment.point.getY() - endDirectionPoint.getY(),
                                          segment.point.getX() - endDirectionPoint.getX());

        if (firstSegmentAfterMove)
        {
            starts.push_back ({ firstPoint, startAngle });
            firstSegmentAfterMove = false;
        }
        else
        {
            mids.push_back ({ previousPoint, previousTangent });
        }

        previousPoint = segment.point;
        previousTangent = endAngle;
        ends = { { segment.point, endAngle } };
    }
}

//==============================================================================
AffineTransform PathMarkers::getTransform (const Placement& placement, const Shape& shape, float strokeWidth) noexcept
{
    float scale = shape.scalesWithStrokeWidth ? strokeWidth : 1.0f;

    if (shape.viewBox && shape.viewBox->getWidth() > 0.0f && shape.viewBox->getHeight() > 0.0f)
        scale *= jmin (shape.size.getWidth() / shape.viewBox->getWidth(), shape.size.getHeight() / shape.viewBox->getHeight());

    return AffineTransform::translation (-shape.reference.getX(), -shape.reference.getY())
        .followedBy (AffineTransform::scaling (scale))
        .followedBy (AffineTransform::rotation (shape.angle.value_or (placement.angle)))
        .followedBy (AffineTransform::translation (placement.position.getX(), placement.position.getY()));
}

} // namespace yup
