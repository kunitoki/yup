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
    Drawable();

    bool parseSVG (const File& svgFile);

    void clear();

    void paint (Graphics& g);

private:
    struct Element
    {
        std::optional<String> id;

        std::optional<AffineTransform> transform;
        std::optional<Path> path;
        std::optional<String> reference;

        std::optional<Color> fillColor;
        std::optional<Color> strokeColor;
        std::optional<float> strokeWidth;
        std::optional<StrokeJoin> strokeJoin;
        std::optional<StrokeCap> strokeCap;
        bool noFill = false;
        bool noStroke = false;

        std::optional<float> opacity;

        std::vector<std::shared_ptr<Element>> children;
    };

    void paintElement (Graphics& g, const Element& element, bool hasParentFillEnabled, bool hasParentStrokeEnabled);
    void paintDebugElement (Graphics& g, const Element& element);
    bool parseElement (const XmlElement& element, bool parentIsRoot, AffineTransform currentTransform, Element* parent = nullptr);
    void parseStyle (const XmlElement& element, const AffineTransform& currentTransform, Element& e);
    AffineTransform parseTransform (const XmlElement& element, const AffineTransform& currentTransform, Element& e);

    Rectangle<float> viewBox;
    Size<float> size;
    AffineTransform transform;
    std::vector<std::shared_ptr<Element>> elements;
    HashMap<String, std::shared_ptr<Element>> elementsById;
};

} // namespace yup
