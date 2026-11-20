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

constexpr const char* backgroundImage = "BackgroundImage";

// The blend a filter ends with when it puts what it made over what is behind the element, or nullptr
const SVGFEBlend* findBackdropBlend (const SVGFilter& filter)
{
    if (filter.primitives.empty())
        return nullptr;

    const auto* blend = dynamic_cast<const SVGFEBlend*> (filter.primitives.back().get());

    return blend != nullptr && (blend->in == backgroundImage || blend->in2 == backgroundImage) ? blend : nullptr;
}

} // namespace

//==============================================================================

Rectangle<float> SVGFilter::getRegion (const Rectangle<float>& objectBounds) const
{
    auto area = region;

    if (units == ObjectBoundingBox)
    {
        area = { objectBounds.getX() + region.getX() * objectBounds.getWidth(),
                 objectBounds.getY() + region.getY() * objectBounds.getHeight(),
                 region.getWidth() * objectBounds.getWidth(),
                 region.getHeight() * objectBounds.getHeight() };
    }

    return area.isEmpty() ? Rectangle<float>() : area.enlarged (margin);
}

//==============================================================================

FilterGraph SVGFilter::toFilterGraph (const Rectangle<float>& objectBounds) const
{
    const auto origin = getRegion (objectBounds).getTopLeft();

    // Lengths along no axis in particular are parts of the diagonal of the bounding box
    const bool usesBounds = primitiveUnits == ObjectBoundingBox;
    const Point<float> unit (usesBounds ? objectBounds.getWidth() : 1.0f, usesBounds ? objectBounds.getHeight() : 1.0f);
    const float diagonalUnit = usesBounds ? std::hypot (unit.getX(), unit.getY()) / MathConstants<float>::sqrt2 : 1.0f;

    FilterGraph filter;
    filter.linearColors = linearColors;

    const auto* backdropBlend = findBackdropBlend (*this);

    for (const auto& item : primitives)
    {
        FilterGraph::Primitive primitive;
        auto in = item->in;

        if (item.get() == backdropBlend)
        {
            // The result is what the blend puts over what is behind the element, which is blended when painted
            in = backdropBlend->in == backgroundImage ? backdropBlend->in2 : backdropBlend->in;

            if (in.isEmpty() || (in == FilterGraph::sourceGraphic && filter.primitives.empty()))
                break;

            primitive.type = FilterGraph::Type::offset;
        }
        else if (const auto* blur = dynamic_cast<const SVGFEGaussianBlur*> (item.get()))
        {
            primitive.type = FilterGraph::Type::blur;
            primitive.deviation = { blur->stdDeviation, blur->stdDeviation };
        }
        else if (const auto* blend = dynamic_cast<const SVGFEBlend*> (item.get()))
        {
            primitive.type = FilterGraph::Type::blend;
            primitive.in2 = blend->in2;
            primitive.blendMode = blend->mode;
        }
        else if (const auto* other = dynamic_cast<const SVGFEPrimitive*> (item.get()))
        {
            primitive = other->primitive;
        }
        else
        {
            continue;
        }

        primitive.in = in;
        primitive.result = item->result;
        primitive.deviation = primitive.deviation * unit;
        primitive.offset = primitive.offset * unit;
        primitive.radius = primitive.radius * unit;

        if (primitive.type == FilterGraph::Type::displacement)
            primitive.scale *= diagonalUnit;

        if (primitive.type == FilterGraph::Type::lighting && primitive.light == FilterGraph::Light::point)
        {
            auto& position = primitive.lightPosition;

            position[0] = (usesBounds ? objectBounds.getX() + position[0] * unit.getX() : position[0]) - origin.getX();
            position[1] = (usesBounds ? objectBounds.getY() + position[1] * unit.getY() : position[1]) - origin.getY();
            position[2] *= diagonalUnit;
        }

        filter.primitives.push_back (std::move (primitive));
    }

    return filter;
}

//==============================================================================

std::optional<BlendMode> SVGFilter::getBackdropBlend() const
{
    if (const auto* blend = findBackdropBlend (*this))
        return blend->mode;

    return std::nullopt;
}

} // namespace yup
