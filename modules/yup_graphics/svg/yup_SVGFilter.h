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

    @see SVGFilter, SVGFEBlend, SVGFEGaussianBlur
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
/** A parsed SVG filter element.

    Contains an ordered list of filter primitives that are applied
    sequentially to produce the final filtered result.

    @see SVGFilterPrimitive, SVGFEBlend, SVGFEGaussianBlur
*/
struct SVGFilter : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<SVGFilter>;

    /** The `id` attribute of the filter element, used for url() references. */
    String id;

    /** The `href` (or `xlink:href`) attribute, for filter inheritance. */
    String href;

    /** The ordered list of filter primitives that make up this filter's effect. */
    std::vector<SVGFilterPrimitive::Ptr> primitives;
};

} // namespace yup
