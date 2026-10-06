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

inline yup::String toSvgMatrix (const yup::AffineTransform& t)
{
    return "matrix(" + toSvgNumber (t.getScaleX()) + " " + toSvgNumber (t.getShearY()) + " "
         + toSvgNumber (t.getShearX()) + " " + toSvgNumber (t.getScaleY()) + " "
         + toSvgNumber (t.getTranslateX()) + " " + toSvgNumber (t.getTranslateY()) + ")";
}

/** Guesses the mime type of a base64 encoded image from the start of its file signature. */
inline yup::String getImageMimeType (const yup::String& base64Data)
{
    if (base64Data.startsWith ("/9j/"))
        return "image/jpeg";

    if (base64Data.startsWith ("R0lG"))
        return "image/gif";

    if (base64Data.startsWith ("UklGR"))
        return "image/webp";

    return "image/png";
}

/** Writes the document as an SVG string.

    Some features have no SVG equivalent and are approximated:
    - feather is a gaussian blur filter, and tint a color matrix in the same filter;
    - image paints are patterns, which always tile, so clamped and mirrored images repeat;
    - inside strokes are drawn twice as wide and clipped to the path, outside strokes are centered;
    - the additive amount is not exported, plus-lighter is always fully additive;
    - stroke clips become masks with a centered white stroke, and inverted masks are luminance
      masks over white; stacked clips and masks become nested groups.
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

        if (kind == "image")
        {
            const auto& image = getPaintImage (paint);
            const auto imageData = getPaintImageData (paint);
            const auto id = "image" + yup::String (++numDefinitions);
            const auto imageWidth = toSvgNumber (static_cast<float> (image.getWidth()));
            const auto imageHeight = toSvgNumber (static_cast<float> (image.getHeight()));

            defs << "    <pattern id=\"" << id << "\" patternUnits=\"userSpaceOnUse\""
                 << " width=\"" << imageWidth << "\" height=\"" << imageHeight << "\""
                 << " patternTransform=\"" << toSvgMatrix (getPaintImageTransform (paint)) << "\">"
                 << "<image href=\"data:" << getImageMimeType (imageData) << ";base64," << imageData << "\""
                 << " width=\"" << imageWidth << "\" height=\"" << imageHeight << "\"";

            if (getString (paint, Ids::filter) == "pixelated")
                defs << " style=\"image-rendering:pixelated\"";

            defs << "/></pattern>\n";

            return yup::String (" ") + attribute + "=\"url(#" + id + ")\"";
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

    // The <path> of a layer: its geometry, paints, opacity and filter, without any group around it
    auto writeLayerElement = [&] (const yup::DataTree& layer, const yup::String& pathData) -> yup::String
    {
        const auto stroke = getStroke (layer);
        const float strokeWidth = getFloat (stroke, Ids::width);

        yup::String element;
        element << "<path d=\"" << pathData << "\"" << writePaint (getFill (layer), "fill")
                << " fill-rule=\"" << getString (layer, Ids::fillRule) << "\"";

        if (isPaintVisible (stroke) && strokeWidth > 0.0f)
        {
            // Feathered strokes are drawn centered on the canvas
            const bool insideStroke = getString (stroke, Ids::position) == "inside" && getFloat (layer, Ids::feather) <= 0.0f;

            element << writePaint (stroke, "stroke")
                    << " stroke-width=\"" << toSvgNumber (insideStroke ? strokeWidth * 2.0f : strokeWidth) << "\""
                    << " stroke-linejoin=\"" << getString (stroke, Ids::join) << "\""
                    << " stroke-linecap=\"" << getString (stroke, Ids::cap) << "\"";

            if (insideStroke)
            {
                const auto id = "clip" + yup::String (++numDefinitions);
                defs << "    <clipPath id=\"" << id << "\"><path d=\"" << pathData << "\""
                     << " clip-rule=\"" << getString (layer, Ids::fillRule) << "\"/></clipPath>\n";

                element << " clip-path=\"url(#" << id << ")\"";
            }
        }

        if (const float opacity = getFloat (layer, Ids::opacity); opacity < 1.0f)
            element << " opacity=\"" << toSvgNumber (opacity) << "\"";

        const float feather = getFloat (layer, Ids::feather);
        const auto tint = getColor (layer, Ids::tint);
        const bool tinted = tint != yup::Color (0xffffffff);

        if (feather > 0.0f || tinted)
        {
            const auto id = "filter" + yup::String (++numDefinitions);
            defs << "    <filter id=\"" << id << "\" x=\"-50%\" y=\"-50%\" width=\"200%\" height=\"200%\" color-interpolation-filters=\"sRGB\">";

            if (feather > 0.0f)
                defs << "<feGaussianBlur stdDeviation=\"" << toSvgNumber (feather * 0.5f) << "\"/>";

            if (tinted)
            {
                defs << "<feColorMatrix type=\"matrix\" values=\""
                     << toSvgNumber (tint.getRedFloat()) << " 0 0 0 0 "
                     << "0 " << toSvgNumber (tint.getGreenFloat()) << " 0 0 0 "
                     << "0 0 " << toSvgNumber (tint.getBlueFloat()) << " 0 0 "
                     << "0 0 0 " << toSvgNumber (tint.getAlphaFloat()) << " 0\"/>";
            }

            defs << "</filter>\n";

            element << " filter=\"url(#" << id << ")\"";
        }

        return element + "/>";
    };

    // The clip-path or mask attribute a clip or mask layer puts on the layer below it
    auto writeModifier = [&] (const yup::DataTree& modifier) -> yup::String
    {
        const auto useAs = getString (modifier, Ids::useAs);
        const auto pathData = buildPath (modifier).toString();

        if (useAs == "clip-fill")
        {
            const auto id = "clip" + yup::String (++numDefinitions);
            defs << "    <clipPath id=\"" << id << "\"><path d=\"" << pathData << "\""
                 << " clip-rule=\"" << getString (modifier, Ids::fillRule) << "\"/></clipPath>\n";

            return " clip-path=\"url(#" + id + ")\"";
        }

        const auto id = "mask" + yup::String (++numDefinitions);
        defs << "    <mask id=\"" << id << "\" maskUnits=\"userSpaceOnUse\" x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height << "\"";

        if (useAs == "mask-alpha")
            defs << " style=\"mask-type:alpha\"";

        defs << ">";

        if (useAs == "clip-stroke")
        {
            // A centered white stroke: SVG cannot clip to a stroke, but it can mask with one
            const auto stroke = getStroke (modifier);
            defs << "<path d=\"" << pathData << "\" fill=\"none\" stroke=\"white\""
                 << " stroke-width=\"" << toSvgNumber (getFloat (stroke, Ids::width)) << "\""
                 << " stroke-linejoin=\"" << getString (stroke, Ids::join) << "\""
                 << " stroke-linecap=\"" << getString (stroke, Ids::cap) << "\"/>";
        }
        else if (useAs == "mask-alpha" || useAs == "mask-luminance")
        {
            defs << writeLayerElement (modifier, pathData);
        }
        else
        {
            // Inverted masks are luminance masks over white: an inverted copy (luminance) or a
            // black copy (alpha) of the mask layer hides what it covers
            const auto filterId = "filter" + yup::String (++numDefinitions);
            const auto values = useAs == "mask-inverted-luminance" ? "-1 0 0 0 1 0 -1 0 0 1 0 0 -1 0 1 0 0 0 1 0"
                                                                   : "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0";

            defs << "<filter id=\"" << filterId << "\" color-interpolation-filters=\"sRGB\"><feColorMatrix type=\"matrix\" values=\"" << values << "\"/></filter>"
                 << "<rect width=\"" << width << "\" height=\"" << height << "\" fill=\"white\"/>"
                 << "<g filter=\"url(#" << filterId << ")\">" << writeLayerElement (modifier, pathData) << "</g>";
        }

        defs << "</mask>\n";
        return " mask=\"url(#" + id + ")\"";
    };

    for (int i = 0; i < document.getNumChildren(); ++i)
    {
        const auto layer = document.getChild (i);

        const auto pathData = buildPath (layer).toString();
        if (isModifierLayer (layer) || ! getBool (layer, Ids::visible) || pathData.isEmpty())
            continue;

        const auto blendMode = getString (layer, Ids::blendMode);

        // Blend the finished (blurred, clipped or masked) layer as a group: mix-blend-mode on the filtered
        // element itself blends the low alpha fringe of the blur pixel by pixel and shows speckles in browsers.
        // Inside it, each clip or mask layer stacked above wraps the layer in a group of its own.
        yup::StringArray groups;

        if (blendMode != "normal")
            groups.add (" style=\"mix-blend-mode:" + blendMode + "\"");

        for (const auto& modifier : getModifierLayers (document, i))
            groups.add (writeModifier (modifier));

        for (int depth = 0; depth < groups.size(); ++depth)
            body << yup::String::repeatedString ("  ", depth + 1) << "<g" << groups[depth] << ">\n";

        body << yup::String::repeatedString ("  ", groups.size() + 1) << writeLayerElement (layer, pathData) << "\n";

        for (int depth = groups.size(); --depth >= 0;)
            body << yup::String::repeatedString ("  ", depth + 1) << "</g>\n";
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
