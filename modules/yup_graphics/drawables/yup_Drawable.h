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
/** A class that can parse and paint SVG files.

    The Drawable class is used to parse and paint SVG files.
    It can be used to paint SVG files to a Graphics context.
*/
class YUP_API Drawable
{
public:
    /** Options used when parsing SVG data. Alias for SVGDocument::ParseOptions. */
    using ParseOptions = SVGDocument::ParseOptions;

    //==============================================================================
    /** Constructor. */
    Drawable();

    //==============================================================================
    /** Parses an SVG file.

        @param svgFile The SVG file to parse.

        @return True if the SVG file was parsed successfully, false otherwise.
    */
    bool parseSVG (const File& svgFile);

    /** Parses an SVG file with custom parse options. */
    bool parseSVG (const File& svgFile, const ParseOptions& options);

    //==============================================================================
    /** Parses SVG text.

        @param svgText The SVG XML text to parse.

        @return True if the SVG text was parsed successfully, false otherwise.
    */
    bool parseSVG (StringRef svgText);

    /** Parses SVG text with custom parse options. */
    bool parseSVG (StringRef svgText, const ParseOptions& options);

    //==============================================================================
    /** Clears the drawable. */
    void clear();

    //==============================================================================
    /** Gets the bounds of the drawable content.

        @return The bounding rectangle of the drawable's content.
    */
    Rectangle<float> getBounds() const;

    //==============================================================================
    /** Paints the drawable to a Graphics context.

        @param g The graphics context to paint to.
    */
    void paint (Graphics& g);

    //==============================================================================
    /** Paints the drawable with the specified fitting and justification.

        @param g The graphics context to paint to.
        @param targetArea The rectangle to fit the drawable within.
        @param fitting How to scale and fit the drawable to the target area.
        @param justification How to position the drawable within the target area.
    */
    void paint (Graphics& g,
                const Rectangle<float>& targetArea,
                Fitting fitting = Fitting::scaleToFit,
                Justification justification = Justification::center);

private:
    void paintElement (Graphics& g,
                       const SVGData& data,
                       const SVGElement& element,
                       bool hasParentFillEnabled,
                       bool hasParentStrokeEnabled,
                       Color currentColor,
                       std::unordered_set<const SVGElement*>& visitingElements,
                       std::optional<Array<float>> inheritedStrokeDashArray = std::nullopt,
                       float inheritedStrokeDashOffset = 0.0f,
                       int recursionDepth = 0);
    void paintMarker (Graphics& g, const SVGData& data, const SVGMarker& marker, float strokeWidth, Point<float> position, float tangentAngle, std::unordered_set<const SVGElement*>& visitingElements, int recursionDepth);
    void paintPatternFill (Graphics& g, const SVGData& data, const Path& shape, const SVGElement& element, const SVGPattern& pattern, Color currentColor, std::unordered_set<const SVGElement*>& visitingElements, int recursionDepth);
    void paintDebugElement (Graphics& g, const SVGElement& element);
    void renderTextElement (Graphics& g, const SVGElement& element);
    void renderImageElement (Graphics& g, const SVGElement& element);
    Path createDashedPath (const Path& source, const Array<float>& dashArray, float dashOffset) const;
    AffineTransform calculateTransformForTarget (const Rectangle<float>& sourceBounds,
                                                 const Rectangle<float>& targetArea,
                                                 Fitting fitting,
                                                 Justification justification) const;
    Font resolveFont (const SVGElement& element) const;
    ColorGradient createColorGradientFromSVG (const SVGGradient& gradient,
                                              const Rectangle<float>* objectBounds = nullptr) const;

    SVGDocument::Ptr document;
};

} // namespace yup
