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

/** Handles all CSS / style-related parsing for an SVG document.

    Constructed with a reference to the mutable SVGData being built.
    The SVGParser drives this class during a parse pass.
*/
class SVGCssParser
{
public:
    explicit SVGCssParser (SVGData& data);

    void parseCSSStyle (const String& styleString, SVGElement& e);
    void applyStyleProperty (StringRef property, StringRef value, SVGElement& e);
    void applyStylesheetRules (const XmlElement& xmlElement, SVGElement& e);
    void parseStyleElement (const XmlElement& element);
    bool matchesCssSelector (const XmlElement& xmlElement, const SVGCssRule& rule) const;

private:
    SVGData& data;
};

} // namespace yup
