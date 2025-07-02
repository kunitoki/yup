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

class YUP_API Drawable
{
public:
    //==============================================================================
    Drawable();

    //==============================================================================
    bool parseSVG (const File& svgFile);

    //==============================================================================
    void clear();

    //==============================================================================
    /** Gets the bounds of the drawable content.

        @return The bounding rectangle of the drawable's content.
    */
    Rectangle<float> getBounds() const;

    //==============================================================================
    void paint (Graphics& g);

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

        std::optional<String> id;

        std::optional<AffineTransform> transform;
        std::optional<AffineTransform> localTransform; // Transform from the element itself (not accumulated)
        std::optional<Path> path;
        std::optional<String> reference;

        std::optional<Color> fillColor;
        std::optional<Color> strokeColor;
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

        // Gradient properties
        std::optional<String> fillUrl;
        std::optional<String> strokeUrl;

        // Image properties
        std::optional<String> imageHref;
        std::optional<Rectangle<float>> imageBounds;

        // Clipping properties
        std::optional<String> clipPathUrl;

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
    };

    struct ClipPath : public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<ClipPath>;

        String id;
        std::vector<Element::Ptr> elements;
    };

    void paintElement (Graphics& g, const Element& element, bool hasParentFillEnabled, bool hasParentStrokeEnabled);
    void paintDebugElement (Graphics& g, const Element& element);
    bool parseElement (const XmlElement& element, bool parentIsRoot, AffineTransform currentTransform, Element* parent = nullptr);
    void parseStyle (const XmlElement& element, const AffineTransform& currentTransform, Element& e);
    AffineTransform parseTransform (const XmlElement& element, const AffineTransform& currentTransform, Element& e);
    void parseGradient (const XmlElement& element);
    Gradient::Ptr getGradientById (const String& id);
    Gradient::Ptr resolveGradient (Gradient::Ptr gradient);
    ColorGradient createColorGradientFromSVG (const Gradient& gradient, const AffineTransform& currentTransform = AffineTransform::identity());
    void parseClipPath (const XmlElement& element);
    ClipPath::Ptr getClipPathById (const String& id);
    void parseCSSStyle (const String& styleString, Element& e);
    float parseUnit (const String& value, float defaultValue = 0.0f, float fontSize = 12.0f, float viewportSize = 100.0f);
    AffineTransform parseTransform (const String& transformString);
    String extractGradientUrl (const String& value);

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
};

} // namespace yup
