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

/** A parsed SVG pattern element for tiling fills. */
struct SVGPattern : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<SVGPattern>;

    enum Units
    {
        UserSpaceOnUse,
        ObjectBoundingBox
    };

    String id;
    Units patternUnits = ObjectBoundingBox;
    Units patternContentUnits = UserSpaceOnUse;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    AffineTransform patternTransform;
    std::optional<Rectangle<float>> viewBox;
    std::optional<String> href;
    std::vector<SVGElement::Ptr> elements;
};

} // namespace yup
