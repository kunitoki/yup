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

/** Factory that parses SVG source into a fully-populated SVGDocument.

    Entry points are the two static parse() overloads. All parse* methods are
    private and drive SVGData via the mutable visit() accessor on SVGDocument.
*/
class YUP_API SVGParser
{
public:
    //==============================================================================
    /** Parses an SVG file and returns a reference-counted document.
        Returns nullptr if the root element is not a valid SVG node.
    */
    static SVGDocument::Ptr parse (const File& svgFile,
                                   const SVGDocument::ParseOptions& options = {});

    /** Parses SVG text and returns a reference-counted document.
        Returns nullptr if the root element is not a valid SVG node.
    */
    static SVGDocument::Ptr parse (StringRef svgText,
                                   const SVGDocument::ParseOptions& options = {});

    //==============================================================================
    /** @name Shared utility methods (also used by SVGCssParser). */
    ///@{
    static float parseUnit (const String& value,
                            float defaultValue = 0.0f,
                            float fontSize = 12.0f,
                            float viewportSize = 100.0f);

    static Array<float> parseLengthList (const String& value,
                                         float fontSize,
                                         float viewportSize);

    static String extractGradientUrl (const String& value);
    static String extractUrlId (const String& value);

    static std::optional<Image> loadImageFromHref (const SVGDocument::ParseOptions& options,
                                                   const String& href);
    ///@}

private:
    explicit SVGParser (SVGDocument& document);

    bool parseDocument (std::unique_ptr<XmlElement> svgRoot);
    bool parseElement (const XmlElement& element, bool parentIsRoot, AffineTransform currentTransform, SVGElement* parent = nullptr);
    void parseStyle (const XmlElement& element, const AffineTransform& currentTransform, SVGElement& e);
    AffineTransform parseTransform (const XmlElement& element, const AffineTransform& currentTransform, SVGElement& e);
    AffineTransform parseTransform (const String& transformString);
    void parseGradient (const XmlElement& element);
    SVGGradient::Ptr getGradientById (const String& id) const;
    SVGGradient::Ptr resolveGradient (SVGGradient::Ptr gradient) const;
    void parseFilter (const XmlElement& element);
    SVGFilter::Ptr getFilterById (const String& id) const;
    SVGFilter::Ptr resolveFilter (SVGFilter::Ptr filter) const;
    void parseClipPath (const XmlElement& element);
    SVGClipPath::Ptr getClipPathById (const String& id) const;
    void parseMask (const XmlElement& element);
    void parseMarker (const XmlElement& element);
    void parsePattern (const XmlElement& element);
    void resolvePatternHrefs();
    float parseLengthAttribute (const XmlElement& element, StringRef attributeName, float defaultValue, float fontSize, float viewportSize) const;
    Fitting parsePreserveAspectRatio (const String& preserveAspectRatio) const;
    Justification parseAspectRatioAlignment (const String& preserveAspectRatio) const;

    SVGDocument& document;
    SVGData& data;
    SVGCssParser cssParser;
};

} // namespace yup
