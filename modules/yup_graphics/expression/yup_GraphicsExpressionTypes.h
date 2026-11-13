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
/** Registers the graphics types with an ExpressionContext.

    The following types become available to expressions, all using `float` coordinates:

    - `point` (Point<float>): `point (x, y)`, members `x y length`, swizzles (`p.yx`),
      methods `distanceTo angleTo dot normalized translated scaled lerp transformed onCircle`,
      operators `+ -` with points, `* /` with points or floats, unary `-`, `== !=`.
    - `size` (Size<float>): `size (width, height)`, members `width height`, `* /` with floats, `== !=`.
    - `rect` (Rectangle<float>): `rect (x, y, width, height)`, `rect (point, size)`, members
      `x y width height left top right bottom centerX centerY center position size topLeft
      topRight bottomLeft bottomRight`, methods `withX withY withWidth withHeight withPosition
      withSize withCenter withSizeKeepingCenter translated scaled reduced enlarged contains
      intersects intersection unionWith largestFittingSquare transformed`, `== !=`.
    - `line` (Line<float>): `line (x1, y1, x2, y2)`, `line (start, end)`, members `start end length`,
      methods `pointAt translated transformed reversed`, `== !=`.
    - `color` (Color): `rgb (r, g, b)` and `rgba (r, g, b, a)` with ints in [0, 255] or floats
      in [0, 1] (`rgba (255, 0, 0, 0.5)` takes an int color and a float alpha), `hsl (h, s, l)`,
      `hsla (h, s, l, a)` with floats in [0, 1], `color (0xAARRGGBB)` and the literals `#rgb`,
      `#rgba`, `#rrggbb`, `#rrggbbaa`; members `r g b a hue
      saturation luminance` (floats in [0, 1]); methods `withAlpha withMultipliedAlpha brighter
      darker contrasting inverted mix`, `== !=`.
    - `transform` (AffineTransform): `transform()` (identity), `transform.translation (x, y)`,
      `transform.rotation (radians)`, `transform.rotation (radians, cx, cy)`,
      `transform.scaling (factor)`, `transform.scaling (fx, fy)`, `transform.shearing (fx, fy)`;
      methods `translated rotated scaled followedBy inverted`, `== !=`.
    - `path` (Path): `path()` (empty), `path ("M 0 0 L 10 10 Z")` (SVG path data); members `bounds length
      isEmpty`; methods `pointAt` (normalized 0-1 position) `moveTo lineTo quadTo cubicTo close addRectangle addRoundedRectangle
      addEllipse addLine addArc addTriangle addPolygon addStar appendPath transformed scaledToFit
      withRoundedCorners withNonZeroWinding`. Paths are values: methods return a modified copy, as in
      `path().moveTo (0, 0).lineTo (10, 10).close()`. Programs using paths may allocate when evaluated.
    - `font` (Font): `font()` (empty), usually provided as an input; members `height isEmpty`;
      methods `withHeight withAxisValue (tag, value)`, `== !=`.
    - `gradient` (ColorGradient): `linearGradient (color1, point1, color2, point2)`,
      `radialGradient (centerColor, center, edgeColor, edge)`, `radialGradient (centerColor, center,
      edgeColor, radius)`, `angularGradient (color1, center, color2, direction)`; methods
      `withStop (color, delta)` (adds a stop along the gradient, delta in [0, 1]) `withAlpha
      withMultipliedAlpha withTransform`, `== !=`. Programs using gradients may allocate when evaluated.

    The enumerations `cap` (`butt round square`), `join` (`miter round bevel`), `strokePosition`
    (`inside center outside`), `blend` (`srcOver screen overlay darken lighten colorDodge colorBurn
    hardLight softLight difference exclusion multiply hue saturation color luminosity additive`) and
    `justification` (`left right horizontalCenter top bottom verticalCenter topLeft topRight bottomLeft
    bottomRight centerLeft centerTop center centerRight centerBottom`) map to StrokeCap, StrokeJoin,
    StrokePosition, BlendMode and Justification::Flags.

    The function `polar (center, radius, angle)` returns the point at `radius` from `center`, with the
    angle in radians measured from the positive x axis towards the positive y axis (clockwise on screen).
    `mix (a, b, t)` interpolates two points or two colors.

    Calling it again on the same context is harmless.

    @code
    ExpressionContext context;
    addGraphicsExpressionTypes (context);

    auto program = CompiledExpression::compile ("rect (0, 0, 100, 50).reduced (4).center", context);
    @endcode

    @see ExpressionContext, ExpressionDrawing

    @tags{Graphics}
*/
YUP_API void addGraphicsExpressionTypes (ExpressionContext& context);

} // namespace yup
