/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
    //==============================================================================
    /** Options used when parsing SVG data. */
    struct ParseOptions
    {
        /** Base directory used to resolve local image hrefs. */
        File baseDirectory;

        /** Allows image data embedded as data: URIs. */
        bool allowDataImages = true;

        /** Allows local file image hrefs relative to baseDirectory. Network URLs are never loaded. */
        bool allowLocalImages = true;

        /** Optional custom image resolver. Return std::nullopt to use the default resolver. */
        std::function<std::optional<Image> (StringRef href, const File& baseDirectory)> imageResolver;

        /** Optional custom font resolver. Return std::nullopt to use the default font. */
        std::function<std::optional<Font> (StringRef family, float size)> fontResolver;
    };

    //==============================================================================
    /** Constructor. */
    Drawable();

    //==============================================================================
    /** Parses an SVG file.

        @param svgFile The SVG file to parse.

        @return True if the SVG file was parsed successfully, false otherwise.
    */
    bool parseSVG (const File& svgFile);

    /** Parses SVG text.

        @param svgText The SVG XML text to parse.

        @return True if the SVG text was parsed successfully, false otherwise.
    */
    bool parseSVG (StringRef svgText);

    /** Parses an SVG file with custom parse options. */
    bool parseSVG (const File& svgFile, const ParseOptions& options);

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
    struct Element : public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<Element>;

        String tagName;
        std::optional<String> id;
        StringArray classNames;

        std::optional<AffineTransform> transform;
        std::optional<AffineTransform> localTransform; // Transform from the element itself (not accumulated)
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
        std::optional<String> fillRule; // "evenodd" or "nonzero"
        bool noFill = false;
        bool noStroke = false;

        std::optional<float> opacity;

        // Text properties
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

        // Gradient properties
        std::optional<String> fillUrl;
        std::optional<String> strokeUrl;

        // Image properties
        std::optional<String> imageHref;
        std::optional<Rectangle<float>> imageBounds;
        std::optional<Image> image;

        // Clipping properties
        std::optional<String> clipPathUrl;
        std::optional<Rectangle<float>> viewBox;
        std::optional<Size<float>> viewportSize;
        Fitting preserveAspectRatioFitting = Fitting::scaleToFit;
        Justification preserveAspectRatioJustification = Justification::center;
        bool isSymbol = false;
        bool hidden = false;

        std::vector<Element::Ptr> children;
    };

    struct GradientStop
    {
        float offset;
        Color color;
        float opacity = 1.0f;
    };

    struct Gradient : public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<Gradient>;

        enum Type
        {
            Linear,
            Radial
        };

        enum Units
        {
            UserSpaceOnUse,
            ObjectBoundingBox
        };

        Type type;
        String id;
        Units units = ObjectBoundingBox; // Default per SVG spec
        String href;                     // xlink:href reference to another gradient

        // Linear gradient properties
        Point<float> start;
        Point<float> end;

        // Radial gradient properties
        Point<float> center;
        float radius = 0.0f;
        Point<float> focal;

        std::vector<GradientStop> stops;
        AffineTransform transform;

        bool hasStart = false;
        bool hasEnd = false;
        bool hasCenter = false;
        bool hasRadius = false;
        bool hasFocal = false;
    };

    struct ClipPath : public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<ClipPath>;

        String id;
        std::vector<Element::Ptr> elements;
    };

    struct CssRule
    {
        String selector;
        StringArray declarations;
        int specificity = 0;
        int order = 0;
    };

    void paintElement (Graphics& g, const Element& element, bool hasParentFillEnabled, bool hasParentStrokeEnabled, Color currentColor, int recursionDepth = 0);
    void paintDebugElement (Graphics& g, const Element& element);
    bool parseElement (const XmlElement& element, bool parentIsRoot, AffineTransform currentTransform, Element* parent = nullptr);
    void parseStyle (const XmlElement& element, const AffineTransform& currentTransform, Element& e);
    AffineTransform parseTransform (const XmlElement& element, const AffineTransform& currentTransform, Element& e);
    void parseGradient (const XmlElement& element);
    Gradient::Ptr getGradientById (const String& id);
    Gradient::Ptr resolveGradient (Gradient::Ptr gradient);
    ColorGradient createColorGradientFromSVG (const Gradient& gradient, const Rectangle<float>* objectBounds = nullptr);
    void parseClipPath (const XmlElement& element);
    ClipPath::Ptr getClipPathById (const String& id);
    void parseCSSStyle (const String& styleString, Element& e);
    void applyStyleProperty (StringRef property, StringRef value, Element& e);
    void applyStylesheetRules (const XmlElement& xmlElement, Element& e);
    void parseStyleElement (const XmlElement& element);
    float parseUnit (const String& value, float defaultValue = 0.0f, float fontSize = 12.0f, float viewportSize = 100.0f);
    float parseLengthAttribute (const XmlElement& element, StringRef attributeName, float defaultValue, float fontSize, float viewportSize);
    Array<float> parseLengthList (const String& value, float fontSize, float viewportSize);
    AffineTransform parseTransform (const String& transformString);
    String extractGradientUrl (const String& value);
    String extractUrlId (const String& value);
    bool parseDocument (std::unique_ptr<XmlElement> svgRoot);
    bool matchesCssSelector (const XmlElement& xmlElement, const CssRule& rule) const;
    std::optional<Image> loadImageFromHref (const String& href) const;
    Font resolveFont (const Element& element) const;
    Path createDashedPath (const Path& source, const Array<float>& dashArray, float dashOffset) const;
    void renderTextElement (Graphics& g, const Element& element);
    void renderImageElement (Graphics& g, const Element& element);

    // SVG preserveAspectRatio parsing
    Fitting parsePreserveAspectRatio (const String& preserveAspectRatio);
    Justification parseAspectRatioAlignment (const String& preserveAspectRatio);

    // Helper methods for layout and painting
    Rectangle<float> calculateBounds() const;
    AffineTransform calculateTransformForTarget (const Rectangle<float>& sourceBounds, const Rectangle<float>& targetArea, Fitting fitting, Justification justification) const;

    Rectangle<float> viewBox;
    Size<float> size;
    Rectangle<float> bounds;
    AffineTransform transform;
    std::vector<Element::Ptr> elements;
    HashMap<String, Element::Ptr> elementsById;
    std::vector<Gradient::Ptr> gradients;
    HashMap<String, Gradient::Ptr> gradientsById;
    std::vector<ClipPath::Ptr> clipPaths;
    HashMap<String, ClipPath::Ptr> clipPathsById;
    std::vector<CssRule> cssRules;
    ParseOptions parseOptions;

    // Root SVG element's default presentation attributes
    bool rootHasFill = true;    // SVG default fill is black
    bool rootHasStroke = false; // SVG default stroke is none
    std::optional<Color> rootFillColor;
    std::optional<Color> rootStrokeColor;
};

} // namespace yup
