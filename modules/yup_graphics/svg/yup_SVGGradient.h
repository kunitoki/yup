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

/** A single color stop in an SVG gradient. */
struct SVGGradientStop
{
    float offset = 0.0f;
    Color color;
    float opacity = 1.0f;
};

/** A parsed SVG gradient (linear or radial). */
struct YUP_API SVGGradient : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<SVGGradient>;

    enum Type
    {
        Linear,
        Radial
    };

    enum Units
    {
        UserSpaceOnUse,
        ObjectBoundingBox
    };

    Type type = Linear;
    String id;
    Units units = ObjectBoundingBox;
    String href;
    String spreadMethod = "pad";

    Point<float> start;
    Point<float> end;
    Point<float> center;
    float radius = 0.0f;
    Point<float> focal;

    std::vector<SVGGradientStop> stops;
    AffineTransform transform;

    bool hasStart = false;
    bool hasEnd = false;
    bool hasCenter = false;
    bool hasRadius = false;
    bool hasFocal = false;
    bool hasUnits = false;
    bool hasSpreadMethod = false;

    /** Converts the gradient to the ColorGradient it is drawn with.

        The stops, the spread and the gradient transform are kept. A focal point is drawn as the center
        of the gradient, and radial gradients scaled unevenly by their units only are drawn as circles.

        @param objectBounds  The bounding box of the shape being painted, which gradients using
                             ObjectBoundingBox units are placed in. Pass nullptr when there is none.

        @return The gradient to draw, or an empty gradient when there are no stops.
    */
    ColorGradient toColorGradient (const Rectangle<float>* objectBounds = nullptr) const;
};

} // namespace yup
