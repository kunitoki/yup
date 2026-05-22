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

/** A parsed SVG marker element used for arrowheads and path-end decorators. */
struct SVGMarker : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<SVGMarker>;

    enum Units
    {
        StrokeWidth,
        UserSpaceOnUse
    };

    String id;
    Units markerUnits = StrokeWidth;
    float refX = 0.0f;
    float refY = 0.0f;
    float markerWidth = 3.0f;
    float markerHeight = 3.0f;
    std::optional<float> orient;         // nullopt = "auto"
    bool orientAutoStartReverse = false; // true when orient="auto-start-reverse"
    std::optional<Rectangle<float>> viewBox;
    std::vector<SVGElement::Ptr> elements;
};

} // namespace yup
