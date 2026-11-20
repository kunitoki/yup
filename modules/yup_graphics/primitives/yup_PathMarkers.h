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

#pragma once

namespace yup
{

//==============================================================================
/** The vertices of a path where markers are drawn, with the direction of the path at each of them.

    Markers decorate the vertices of a path, like the arrowheads of a line. The first vertex of
    every sub-path is a start, the vertices joining its segments are mids, and the last vertex of
    the path is its end, unless the path ends by closing a sub-path.

    @code
    const PathMarkers markers (path);

    for (const auto& placement : markers.getEnds())
    {
        const auto savedState = g.saveState();
        g.setTransform (PathMarkers::getTransform (placement, arrowShape, g.getStrokeWidth()).followedBy (g.getTransform()));
        g.fillPath (arrow);
    }
    @endcode

    @see Path

    @tags{Graphics}
*/
class YUP_API PathMarkers
{
public:
    //==============================================================================
    /** A vertex of a path and the direction of the path there. */
    struct Placement
    {
        /** The position of the vertex. */
        Point<float> position;

        /** The direction of the path at the vertex, in radians from the positive x axis towards the positive y axis. */
        float angle = 0.0f;
    };

    /** The size and the orientation of the content drawn at a vertex. */
    struct Shape
    {
        /** The point of the content that is placed on the vertex. */
        Point<float> reference;

        /** The size the view box is scaled to. */
        Size<float> size;

        /** The area of the content scaled to the size, keeping its proportions. Without it the content is not scaled. */
        std::optional<Rectangle<float>> viewBox;

        /** The angle of the content in radians, instead of the direction of the path. */
        std::optional<float> angle;

        /** Scales the content by the width of the stroke of the path. */
        bool scalesWithStrokeWidth = true;
    };

    //==============================================================================
    /** Creates an empty set of placements. */
    PathMarkers() = default;

    /** Finds the vertices of a path. */
    explicit PathMarkers (const Path& path);

    //==============================================================================
    /** Returns the first vertex of every sub-path. */
    Span<const Placement> getStarts() const noexcept { return starts; }

    /** Returns the vertices joining the segments of the path. */
    Span<const Placement> getMids() const noexcept { return mids; }

    /** Returns the last vertex of the path, if it doesn't end by closing a sub-path. */
    Span<const Placement> getEnds() const noexcept { return ends; }

    //==============================================================================
    /** Returns the transform that moves the content of a marker to a vertex.

        @param placement    The vertex to draw the content at.
        @param shape        The size and the orientation of the content.
        @param strokeWidth  The width of the stroke of the path.
    */
    static AffineTransform getTransform (const Placement& placement, const Shape& shape, float strokeWidth) noexcept;

private:
    std::vector<Placement> starts;
    std::vector<Placement> mids;
    std::vector<Placement> ends;
};

} // namespace yup
