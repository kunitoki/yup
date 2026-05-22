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

void SVGDocument::clear()
{
    data = SVGData {};
}

//==============================================================================

Rectangle<float> SVGDocument::getBounds() const
{
    return data.bounds;
}

//==============================================================================

void SVGDocument::visit (std::function<void (const SVGData&)> visitor) const
{
    visitor (data);
}

void SVGDocument::visit (std::function<void (SVGData&)> visitor)
{
    visitor (data);
}

//==============================================================================

const SVGDocument::ParseOptions& SVGDocument::getParseOptions() const
{
    return parseOptions;
}

//==============================================================================

Rectangle<float> SVGDocument::calculateBounds() const
{
    if (! data.viewBox.isEmpty())
        return data.viewBox;

    if (data.size.getWidth() > 0 && data.size.getHeight() > 0)
        return Rectangle<float> (0.0f, 0.0f, data.size.getWidth(), data.size.getHeight());

    Rectangle<float> bounds;
    bool hasValidBounds = false;

    for (const auto& element : data.elements)
    {
        if (element->path)
        {
            auto pathBounds = element->path->getBounds();
            if (element->transform)
                pathBounds = element->path->getBoundsTransformed (*element->transform);

            if (hasValidBounds)
                bounds = bounds.unionWith (pathBounds);
            else
            {
                bounds = pathBounds;
                hasValidBounds = true;
            }
        }
    }

    return hasValidBounds ? bounds : Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f);
}

} // namespace yup
