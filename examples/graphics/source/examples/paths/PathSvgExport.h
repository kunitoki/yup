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

namespace PathEditor
{

//==============================================================================

inline yup::String toSvgColor (yup::Color color)
{
    return "#" + yup::String::toHexString (color.getRed()).paddedLeft ('0', 2)
         + yup::String::toHexString (color.getGreen()).paddedLeft ('0', 2)
         + yup::String::toHexString (color.getBlue()).paddedLeft ('0', 2);
}

inline yup::String toSvgNumber (float value)
{
    return yup::String (value, 3).trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
}

/** Writes the document as an SVG string.

    Feather has no SVG equivalent, it is approximated with a gaussian blur filter.
*/
inline yup::String exportSvg (const yup::DataTree& document)
{
    const auto width = toSvgNumber (getFloat (document, Ids::width));
    const auto height = toSvgNumber (getFloat (document, Ids::height));

    yup::String defs;
    yup::String body;
    int numDefinitions = 0;

    auto writePaint = [&] (const yup::DataTree& paint, const char* attribute) -> yup::String
    {
        const auto kind = getString (paint, Ids::kind);

        if (kind == "solid")
        {
            const auto color = getColor (paint, Ids::color);
            yup::String result;
            result << " " << attribute << "=\"" << toSvgColor (color) << "\"";

            if (! color.isOpaque())
                result << " " << attribute << "-opacity=\"" << toSvgNumber (color.getAlphaFloat()) << "\"";

            return result;
        }

        if (! isGradientKind (kind))
            return yup::String (" ") + attribute + "=\"none\"";

        const auto id = "gradient" + yup::String (++numDefinitions);
        const auto start = getPoint (paint, Ids::x1, Ids::y1);
        const auto end = getPoint (paint, Ids::x2, Ids::y2);

        if (kind == "linear")
        {
            defs << "    <linearGradient id=\"" << id << "\" gradientUnits=\"userSpaceOnUse\""
                 << " x1=\"" << toSvgNumber (start.getX()) << "\" y1=\"" << toSvgNumber (start.getY()) << "\""
                 << " x2=\"" << toSvgNumber (end.getX()) << "\" y2=\"" << toSvgNumber (end.getY()) << "\">\n";
        }
        else
        {
            defs << "    <radialGradient id=\"" << id << "\" gradientUnits=\"userSpaceOnUse\""
                 << " cx=\"" << toSvgNumber (start.getX()) << "\" cy=\"" << toSvgNumber (start.getY()) << "\""
                 << " r=\"" << toSvgNumber (start.distanceTo (end)) << "\">\n";
        }

        for (const auto& stop : getPaddedStops (paint))
        {
            defs << "      <stop offset=\"" << toSvgNumber (stop.offset) << "\" stop-color=\"" << toSvgColor (stop.color) << "\""
                 << " stop-opacity=\"" << toSvgNumber (stop.color.getAlphaFloat()) << "\"/>\n";
        }

        defs << (kind == "linear" ? "    </linearGradient>\n" : "    </radialGradient>\n");

        return yup::String (" ") + attribute + "=\"url(#" + id + ")\"";
    };

    for (const auto& layer : document)
    {
        const auto pathData = buildPath (layer).toString();
        if (! getBool (layer, Ids::visible) || pathData.isEmpty())
            continue;

        const auto stroke = getStroke (layer);
        const float strokeWidth = getFloat (stroke, Ids::width);

        body << "  <path d=\"" << pathData << "\"" << writePaint (getFill (layer), "fill")
             << " fill-rule=\"" << getString (layer, Ids::fillRule) << "\"";

        if (isPaintVisible (stroke) && strokeWidth > 0.0f)
        {
            body << writePaint (stroke, "stroke")
                 << " stroke-width=\"" << toSvgNumber (strokeWidth) << "\""
                 << " stroke-linejoin=\"" << getString (stroke, Ids::join) << "\""
                 << " stroke-linecap=\"" << getString (stroke, Ids::cap) << "\"";
        }

        if (const float opacity = getFloat (layer, Ids::opacity); opacity < 1.0f)
            body << " opacity=\"" << toSvgNumber (opacity) << "\"";

        if (const auto blendMode = getString (layer, Ids::blendMode); blendMode != "normal")
            body << " style=\"mix-blend-mode:" << blendMode << "\"";

        if (const float feather = getFloat (layer, Ids::feather); feather > 0.0f)
        {
            const auto id = "feather" + yup::String (++numDefinitions);
            defs << "    <filter id=\"" << id << "\" x=\"-50%\" y=\"-50%\" width=\"200%\" height=\"200%\">"
                 << "<feGaussianBlur stdDeviation=\"" << toSvgNumber (feather * 0.5f) << "\"/></filter>\n";

            body << " filter=\"url(#" << id << ")\"";
        }

        body << "/>\n";
    }

    yup::String svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height << "\""
        << " viewBox=\"0 0 " << width << " " << height << "\">\n";

    if (defs.isNotEmpty())
        svg << "  <defs>\n" << defs << "  </defs>\n";

    if (const auto background = getColor (document, Ids::background); ! background.isTransparent())
    {
        svg << "  <rect width=\"" << width << "\" height=\"" << height << "\" fill=\"" << toSvgColor (background) << "\"";

        if (! background.isOpaque())
            svg << " fill-opacity=\"" << toSvgNumber (background.getAlphaFloat()) << "\"";

        svg << "/>\n";
    }

    svg << body << "</svg>\n";
    return svg;
}

} // namespace PathEditor
