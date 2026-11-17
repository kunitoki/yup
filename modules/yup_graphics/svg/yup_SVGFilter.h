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

/** Base class for SVG filter primitives.

    Each filter primitive takes inputs, applies an operation, and produces a
    named result that can be referenced by subsequent primitives in the chain.

    @see SVGFilter, SVGFEBlend, SVGFEGaussianBlur, SVGFEPrimitive
*/
struct SVGFilterPrimitive : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<SVGFilterPrimitive>;

    /** The input source for this primitive (e.g. "SourceGraphic", "SourceAlpha",
        or the result of a previous primitive).
    */
    String in;

    /** An optional name for this primitive's output, which can be referenced
        by subsequent primitives via their `in` attribute.
    */
    String result;
};

//==============================================================================
/** An SVG feBlend filter primitive.

    Composites two inputs together using one of the CSS blend modes
    defined in the Compositing and Blending specification.

    @see SVGFilterPrimitive, BlendMode
*/
struct SVGFEBlend : public SVGFilterPrimitive
{
    using Ptr = ReferenceCountedObjectPtr<SVGFEBlend>;

    /** The blend mode to use when compositing the two inputs.
        Valid values correspond to the `mode` attribute of the feBlend element
        (e.g. "normal", "multiply", "screen", "darken", "lighten").
    */
    BlendMode mode = BlendMode::SrcOver;

    /** The second input source to blend with `in`.
        Common values are "SourceGraphic", "SourceAlpha", "BackgroundImage",
        or the result name of a previous filter primitive.
    */
    String in2;
};

//==============================================================================
/** An SVG feGaussianBlur filter primitive.

    Applies a Gaussian blur to the input image.

    @see SVGFilterPrimitive
*/
struct SVGFEGaussianBlur : public SVGFilterPrimitive
{
    using Ptr = ReferenceCountedObjectPtr<SVGFEGaussianBlur>;

    /** The standard deviation of the Gaussian blur.
        Larger values produce a stronger blur effect.
    */
    float stdDeviation = 0.0f;
};

//==============================================================================
/** Any other SVG filter primitive, kept as the FilterGraph primitive it is rendered with.

    This is what feOffset, feFlood, feColorMatrix, feComposite, feMorphology, feMerge,
    feDiffuseLighting, feSpecularLighting, feDisplacementMap and feDropShadow are parsed
    into. Its lengths are in the units of the filter (see SVGFilter::primitiveUnits), and
    the `in` and `result` of the SVGFilterPrimitive are the ones that count.

    @see SVGFilterPrimitive, FilterGraph::Primitive
*/
struct SVGFEPrimitive : public SVGFilterPrimitive
{
    using Ptr = ReferenceCountedObjectPtr<SVGFEPrimitive>;

    /** What the primitive does, see FilterGraph::Primitive. */
    FilterGraph::Primitive primitive;
};

//==============================================================================
/** A parsed SVG filter element.

    Contains an ordered list of filter primitives that are applied
    sequentially to produce the final filtered result.

    @see SVGFilterPrimitive, SVGFEBlend, SVGFEGaussianBlur, SVGFEPrimitive, FilterGraph
*/
struct SVGFilter : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<SVGFilter>;

    /** The coordinate system of the `filterUnits` and `primitiveUnits` attributes. */
    enum Units
    {
        UserSpaceOnUse,   /**< The user space of the filtered element. */
        ObjectBoundingBox /**< Fractions of the bounding box of the filtered element. */
    };

    /** The `id` attribute of the filter element, used for url() references. */
    String id;

    /** The `href` (or `xlink:href`) attribute, for filter inheritance. */
    String href;

    /** The ordered list of filter primitives that make up this filter's effect. */
    std::vector<SVGFilterPrimitive::Ptr> primitives;

    /** The `filterUnits` attribute: the coordinate system of the region. */
    Units units = ObjectBoundingBox;

    /** The `primitiveUnits` attribute: the coordinate system of the lengths of the primitives. */
    Units primitiveUnits = UserSpaceOnUse;

    /** The `x`, `y`, `width` and `height` attributes: the area the filter is rendered in. */
    Rectangle<float> region { -0.1f, -0.1f, 1.2f, 1.2f };

    /** What is added around the region, in user space. The CSS filter functions, which
        have no region, are given the bounding box of the element with the room they need.
    */
    float margin = 0.0f;

    /** The `color-interpolation-filters` property: true to filter linear colors, false sRGB ones. */
    bool linearColors = true;

    /** Returns the area the filter is rendered in for an element, in its user space.

        @param objectBounds The bounding box of the filtered element.
    */
    Rectangle<float> getRegion (const Rectangle<float>& objectBounds) const;

    /** Returns the filter to render an element through, its lengths in user space
        and its positions measured from the top left corner of the region.

        It is empty when the filter only blends the element with what is behind it,
        see getBackdropBlend().

        @param objectBounds The bounding box of the filtered element.
    */
    FilterGraph toFilterGraph (const Rectangle<float>& objectBounds) const;

    /** Returns the blend mode what the filter makes is painted with, when the filter sets one.

        A filter ending with an feBlend reading the `BackgroundImage` blends what it made with
        what is behind the element. A filter can't read that, so the blend is left out of
        toFilterGraph() and its mode is the one to paint the element, or its filtered picture, with.
    */
    std::optional<BlendMode> getBackdropBlend() const;
};

} // namespace yup
