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

/** All mutable state produced by parsing a single SVG document. */
struct SVGData
{
    Rectangle<float> viewBox;
    Size<float> size;
    Rectangle<float> bounds;
    AffineTransform transform;

    std::vector<SVGElement::Ptr> elements;
    HashMap<String, SVGElement::Ptr> elementsById;

    std::vector<SVGGradient::Ptr> gradients;
    HashMap<String, SVGGradient::Ptr> gradientsById;

    std::vector<SVGFilter::Ptr> filters;
    HashMap<String, SVGFilter::Ptr> filtersById;

    std::vector<SVGClipPath::Ptr> clipPaths;
    HashMap<String, SVGClipPath::Ptr> clipPathsById;

    std::vector<SVGCssRule> cssRules;

    bool rootHasFill = true;
    bool rootHasStroke = false;
    std::optional<Color> rootFillColor;
    std::optional<Color> rootStrokeColor;
};

//==============================================================================
/** Reference-counted container for a fully-parsed SVG document.

    Parsing is performed by SVGParser; consumers access parsed state via visit().
*/
class YUP_API SVGDocument : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<SVGDocument>;

    //==============================================================================
    /** Options controlling how the SVG is parsed. */
    struct ParseOptions
    {
        /** Base directory used to resolve local image hrefs. */
        File baseDirectory;

        /** Allows image data embedded as data: URIs. */
        bool allowDataImages = true;

        /** Allows local file image hrefs relative to baseDirectory. */
        bool allowLocalImages = true;

        /** Optional custom image resolver. Return std::nullopt to use the default. */
        std::function<std::optional<Image> (StringRef href, const File& baseDirectory)> imageResolver;

        /** Optional custom font resolver. Return std::nullopt to use the default.
            @param family  CSS font-family name (may be empty).
            @param size    Font size in pixels.
            @param weight  CSS font-weight value (400 = normal, 700 = bold).
            @param italic  True when font-style is italic or oblique.
        */
        std::function<std::optional<Font> (StringRef family, float size, int weight, bool italic)> fontResolver;
    };

    //==============================================================================
    /** Clears all parsed state and resets to the default empty state. */
    void clear();

    /** Returns the bounding rectangle of the SVG content. */
    Rectangle<float> getBounds() const;

    /** Provides read-only access to the parsed SVG data. */
    void visit (std::function<void (const SVGData&)> visitor) const;

    /** Provides mutable access to the parsed SVG data (used by SVGParser). */
    void visit (std::function<void (SVGData&)> visitor);

    /** Returns the parse options used when building this document. */
    const ParseOptions& getParseOptions() const;

private:
    Rectangle<float> calculateBounds() const;

    SVGData data;
    ParseOptions parseOptions;

    friend class SVGParser;
};

} // namespace yup
