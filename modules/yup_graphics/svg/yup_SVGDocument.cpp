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

namespace
{

// The box around a shape under a transform. A turned or sheared shape is measured once transformed, as the
// box of a box would be larger than it has to, or smaller when only two of its corners are transformed
Rectangle<float> getTransformedBounds (const Path& path, const AffineTransform& transform)
{
    if (transform.getShearX() == 0.0f && transform.getShearY() == 0.0f)
        return path.getBounds().transformed (transform);

    return path.transformed (transform).getBounds();
}

// Adds what an element paints to a box, under the transform from its coordinates to the ones of the box
void includeContentBounds (const SVGData& data, const SVGElement& element, const AffineTransform& transform, std::optional<Rectangle<float>>& bounds, int depth)
{
    if (depth > 64)
        return;

    const auto include = [&bounds] (const Rectangle<float>& area)
    {
        bounds = bounds ? bounds->unionWith (area) : area;
    };

    const auto includeChildren = [&] (const SVGElement& parent, const AffineTransform& parentTransform)
    {
        for (const auto& child : parent.children)
            if (child != nullptr && ! child->hidden)
                includeContentBounds (data, *child, child->transform.value_or (AffineTransform()).followedBy (parentTransform), bounds, depth + 1);
    };

    if (element.path)
        include (getTransformedBounds (*element.path, transform));

    if (element.imageBounds)
    {
        Path area;
        area.addRectangle (*element.imageBounds);
        include (getTransformedBounds (area, transform));
    }

    includeChildren (element, transform);

    if (! element.reference)
        return;

    if (const auto refElement = data.elementsById[*element.reference]; refElement != nullptr)
    {
        const auto refTransform = refElement->localTransform.value_or (AffineTransform()).followedBy (transform);

        if (refElement->path)
            include (getTransformedBounds (*refElement->path, refTransform));

        includeChildren (*refElement, refTransform);
    }
}

} // namespace

//==============================================================================

Rectangle<float> SVGData::getContentBounds (const SVGElement& element) const
{
    std::optional<Rectangle<float>> bounds;
    includeContentBounds (*this, element, {}, bounds, 0);

    return bounds.value_or (Rectangle<float>());
}

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

    const auto addBounds = [&] (const Rectangle<float>& elementBounds)
    {
        if (hasValidBounds)
            bounds = bounds.unionWith (elementBounds);
        else
        {
            bounds = elementBounds;
            hasValidBounds = true;
        }
    };

    const auto visitElement = [&] (const auto& self,
                                   const SVGElement& element,
                                   const AffineTransform& parentTransform) -> void
    {
        auto elementTransform = parentTransform;
        if (element.transform)
            elementTransform = element.transform->followedBy (parentTransform);

        if (element.path)
            addBounds (element.path->getBoundsTransformed (elementTransform));

        for (const auto& child : element.children)
            self (self, *child, elementTransform);
    };

    for (const auto& element : data.elements)
        visitElement (visitElement, *element, AffineTransform::identity());

    return hasValidBounds ? bounds : Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f);
}

} // namespace yup
