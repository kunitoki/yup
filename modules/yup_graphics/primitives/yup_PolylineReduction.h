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
/** Reduces a polyline with more points than the pixels it is drawn across to what each column of pixels shows.

    A signal, a spectrum or a curve often has many points for each pixel of its width: the points sharing a
    column of pixels draw over each other, and only the range they cover shows. For each column this keeps its
    first point, the ones with the lowest and the highest y, and its last, in the order they come and each of
    them once, so the line through the points that are left covers the same pixels as the line through all of
    them, to within a pixel, with at most four points for each column. Only how the line goes between the lowest
    and the highest point of a column is lost: an area filled under a line going up and down many times inside
    a column can be shaded another way between the two.

    The points are reduced in place, the way std::unique works: the ones that are kept are moved to the start
    of the span, in order, and their number is returned. What follows them is unspecified.

    @code
    std::vector<Point<float>> points = ...;  // In the coordinates they are drawn in

    points.resize (reducePolylineToPixelColumns (points, g.getContextScale()));

    path.moveTo (points.front());
    for (size_t i = 1; i < points.size(); ++i)
        path.lineTo (points[i]);
    @endcode

    Nothing is removed, and the points are left as they are, when:
    - the x of a point is lower than the x of the one before it: only a line going one way across its columns
      is reduced, not a closed shape or a curve coming back;
    - there are no more than four points a column on average, counting the columns from the first point to the
      last: this is told from these two points alone, so that such a line costs no pass over its points;
    - `pixelsPerUnit` is not above zero.

    The result depends on the scale: points reduced for one scale lack what a larger one shows, so reduce them
    again from all the points to draw them larger. Drawn smaller, they only have more points than needed.

    Only reduce a line that is drawn straight from point to point: a curve fitted through the points follows
    the ones around each of them, and takes another shape through the points that are left.

    @param points         The points of the polyline, in the order they are joined.
    @param pixelsPerUnit  The pixels a unit of the x of the points takes where they are drawn. Columns are one
                          pixel wide, and start where `x * pixelsPerUnit` is a whole number.

    @returns The number of points kept at the start of `points`: all of them when nothing was removed.

    @tags{Graphics}
*/
YUP_API size_t reducePolylineToPixelColumns (Span<Point<float>> points, float pixelsPerUnit) noexcept;

} // namespace yup
