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

/** An SVG element node produced by the SVG parser.

    Holds all presentation attributes and structural data for a single element
    in the parsed SVG tree.
*/
struct SVGElement : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<SVGElement>;

    String tagName;
    std::optional<String> id;
    StringArray classNames;

    std::optional<AffineTransform> transform;
    std::optional<AffineTransform> localTransform; // own transform for <use> elements

    std::optional<Path> path;
    std::optional<String> reference;

    std::optional<Color> fillColor;
    std::optional<Color> strokeColor;
    std::optional<Color> color;
    bool fillCurrentColor = false;
    bool strokeCurrentColor = false;
    std::optional<float> fillOpacity;
    std::optional<float> strokeOpacity;
    std::optional<float> strokeWidth;
    std::optional<StrokeJoin> strokeJoin;
    std::optional<StrokeCap> strokeCap;
    std::optional<Array<float>> strokeDashArray;
    std::optional<float> strokeDashOffset;
    std::optional<String> fillRule;
    std::optional<String> clipRule;
    bool noFill = false;
    bool noStroke = false;

    std::optional<float> opacity;

    std::optional<String> text;
    std::optional<Point<float>> textPosition;
    std::optional<String> fontFamily;
    std::optional<float> fontSize;
    std::optional<String> textAnchor;
    std::optional<float> letterSpacing;
    std::optional<float> wordSpacing;
    std::optional<Array<float>> textX;
    std::optional<Array<float>> textY;
    std::optional<Array<float>> textDx;
    std::optional<Array<float>> textDy;

    std::optional<String> fillUrl;
    std::optional<String> strokeUrl;
    std::optional<String> filterUrl;

    std::optional<String> imageHref;
    std::optional<Rectangle<float>> imageBounds;
    std::optional<Image> image;

    std::optional<String> clipPathUrl;
    std::optional<Rectangle<float>> viewBox;
    std::optional<Size<float>> viewportSize;
    Fitting preserveAspectRatioFitting = Fitting::scaleToFit;
    Justification preserveAspectRatioJustification = Justification::center;
    bool isSymbol = false;
    bool hidden = false;

    std::vector<SVGElement::Ptr> children;
};

} // namespace yup
