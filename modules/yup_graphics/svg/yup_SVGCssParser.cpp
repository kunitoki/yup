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

SVGCssParser::SVGCssParser (SVGData& dataRef)
    : data (dataRef)
{
}

//==============================================================================

void SVGCssParser::parseCSSStyle (const String& styleString, SVGElement& e)
{
    YUP_DRAWABLE_LOG ("parseCSSStyle - tag: " << e.tagName
                                              << " id: " << e.id.value_or (String ("none"))
                                              << " style: " << styleString);

    auto declarations = StringArray::fromTokens (styleString, ";", "");

    for (const auto& declaration : declarations)
    {
        auto colonPos = declaration.indexOf (":");
        if (colonPos > 0)
        {
            String property = declaration.substring (0, colonPos).trim();
            String value = declaration.substring (colonPos + 1).trim();

            YUP_DRAWABLE_LOG ("Applying inline CSS declaration - property: " << property << " value: " << value);
            applyStyleProperty (property, value, e);
        }
    }
}

//==============================================================================

void SVGCssParser::applyStyleProperty (StringRef propertyRef, StringRef valueRef, SVGElement& e)
{
    String property (propertyRef.text);
    String value (valueRef.text);

    property = property.trim().toLowerCase();
    value = value.trim();

    YUP_DRAWABLE_LOG ("applyStyleProperty - tag: " << e.tagName
                                                   << " id: " << e.id.value_or (String ("none"))
                                                   << " property: " << property
                                                   << " value: " << value);

    if (property == "fill")
    {
        e.fillCurrentColor = false;
        e.fillUrl.reset();
        e.fillColor.reset();
        e.noFill = false;

        if (value == "none")
            e.noFill = true;
        else if (value == "currentColor")
            e.fillCurrentColor = true;
        else if (auto url = SVGParser::extractUrlId (value); url.isNotEmpty())
            e.fillUrl = url;
        else if (value.isNotEmpty())
            e.fillColor = Color::fromString (value);
    }
    else if (property == "stroke")
    {
        e.strokeCurrentColor = false;
        e.strokeUrl.reset();
        e.strokeColor.reset();
        e.noStroke = false;

        if (value == "none")
            e.noStroke = true;
        else if (value == "currentColor")
            e.strokeCurrentColor = true;
        else if (auto url = SVGParser::extractUrlId (value); url.isNotEmpty())
            e.strokeUrl = url;
        else if (value.isNotEmpty())
            e.strokeColor = Color::fromString (value);
    }
    else if (property == "color")
    {
        if (value != "currentColor" && value != "inherit")
            e.color = Color::fromString (value);
    }
    else if (property == "stroke-width")
    {
        float strokeWidth = SVGParser::parseUnit (value, e.strokeWidth.value_or (1.0f), e.fontSize.value_or (12.0f));
        if (strokeWidth >= 0.0f)
            e.strokeWidth = strokeWidth;
    }
    else if (property == "stroke-linejoin")
    {
        if (value == "round")
            e.strokeJoin = StrokeJoin::Round;
        else if (value == "miter")
            e.strokeJoin = StrokeJoin::Miter;
        else if (value == "bevel")
            e.strokeJoin = StrokeJoin::Bevel;
    }
    else if (property == "stroke-linecap")
    {
        if (value == "round")
            e.strokeCap = StrokeCap::Round;
        else if (value == "square")
            e.strokeCap = StrokeCap::Square;
        else if (value == "butt")
            e.strokeCap = StrokeCap::Butt;
    }
    else if (property == "opacity")
    {
        float opacity = value.getFloatValue();
        if (opacity >= 0.0f && opacity <= 1.0f)
            e.opacity = opacity;
    }
    else if (property == "display")
    {
        if (value == "none")
            e.hidden = true;
    }
    else if (property == "visibility")
    {
        e.hidden = value == "hidden" || value == "collapse";
    }
    else if (property == "font-family")
    {
        e.fontFamily = value.unquoted();
    }
    else if (property == "font-size")
    {
        float fontSize = SVGParser::parseUnit (value, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
        if (fontSize > 0.0f)
            e.fontSize = fontSize;
    }
    else if (property == "text-anchor")
    {
        e.textAnchor = value;
    }
    else if (property == "letter-spacing")
    {
        if (value != "normal")
            e.letterSpacing = SVGParser::parseUnit (value, 0.0f, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
    }
    else if (property == "word-spacing")
    {
        if (value != "normal")
            e.wordSpacing = SVGParser::parseUnit (value, 0.0f, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
    }
    else if (property == "font-weight")
    {
        if (value == "bold" || value == "bolder")
            e.fontWeight = 700;
        else if (value == "normal" || value == "lighter")
            e.fontWeight = 400;
        else
        {
            const int numericWeight = value.getIntValue();
            if (numericWeight >= 100 && numericWeight <= 900)
                e.fontWeight = numericWeight;
        }
    }
    else if (property == "font-style")
    {
        e.fontItalic = (value == "italic" || value == "oblique");
    }
    else if (property == "font-variant")
    {
        YUP_DRAWABLE_LOG ("CSS font-variant currently not applied - value: " << value);
    }
    else if (property == "font-stretch")
    {
        YUP_DRAWABLE_LOG ("CSS font-stretch currently not applied - value: " << value);
    }
    else if (property == "font")
    {
        YUP_DRAWABLE_LOG ("CSS font shorthand currently not parsed - value: " << value);
    }
    else if (property == "dominant-baseline")
    {
        YUP_DRAWABLE_LOG ("CSS dominant-baseline currently not applied - value: " << value);
    }
    else if (property == "alignment-baseline")
    {
        YUP_DRAWABLE_LOG ("CSS alignment-baseline currently not applied - value: " << value);
    }
    else if (property == "baseline-shift")
    {
        YUP_DRAWABLE_LOG ("CSS baseline-shift currently not applied - value: " << value);
    }
    else if (property == "clip-path")
    {
        String clipPathUrl = SVGParser::extractUrlId (value);
        if (clipPathUrl.isNotEmpty())
            e.clipPathUrl = clipPathUrl;
    }
    else if (property == "filter")
    {
        if (value == "none")
            e.filterUrl.reset();
        else if (auto filterUrl = SVGParser::extractUrlId (value); filterUrl.isNotEmpty())
            e.filterUrl = filterUrl;
        else
            YUP_DRAWABLE_LOG ("CSS filter currently only supports url(...) - value: " << value);
    }
    else if (property == "stroke-dasharray")
    {
        if (value == "none")
            e.strokeDashArray.reset();
        else
        {
            auto dashes = SVGParser::parseLengthList (value, e.fontSize.value_or (12.0f), 100.0f);
            if (! dashes.isEmpty())
                e.strokeDashArray = dashes;
        }
    }
    else if (property == "stroke-dashoffset")
    {
        e.strokeDashOffset = SVGParser::parseUnit (value);
    }
    else if (property == "fill-opacity")
    {
        float opacity = value.getFloatValue();
        if (opacity >= 0.0f && opacity <= 1.0f)
            e.fillOpacity = opacity;
    }
    else if (property == "stroke-opacity")
    {
        float opacity = value.getFloatValue();
        if (opacity >= 0.0f && opacity <= 1.0f)
            e.strokeOpacity = opacity;
    }
    else if (property == "fill-rule")
    {
        if (value == "evenodd" || value == "nonzero")
            e.fillRule = value;
    }
    else if (property == "clip-rule")
    {
        if (value == "evenodd" || value == "nonzero")
            e.clipRule = value;
    }
    else
    {
        YUP_DRAWABLE_LOG ("Unsupported CSS property ignored - property: " << property << " value: " << value);
    }
}

//==============================================================================

void SVGCssParser::applyStylesheetRules (const XmlElement& xmlElement, SVGElement& e)
{
    std::vector<const SVGCssRule*> matchedRules;

    for (const auto& rule : data.cssRules)
    {
        if (matchesCssSelector (xmlElement, rule))
            matchedRules.push_back (std::addressof (rule));
    }

    std::stable_sort (matchedRules.begin(), matchedRules.end(), [] (const SVGCssRule* a, const SVGCssRule* b)
    {
        if (a->specificity != b->specificity)
            return a->specificity < b->specificity;

        return a->order < b->order;
    });

    if (! matchedRules.empty())
    {
        YUP_DRAWABLE_LOG ("applyStylesheetRules - tag: " << e.tagName
                                                         << " id: " << e.id.value_or (String ("none"))
                                                         << " matchedRules: " << matchedRules.size());
    }

    for (const auto* rule : matchedRules)
    {
        YUP_DRAWABLE_LOG ("Applying stylesheet rule - selector: " << rule->selector
                                                                  << " specificity: " << rule->specificity
                                                                  << " order: " << rule->order
                                                                  << " declarationCount: " << rule->declarations.size());

        for (const auto& declaration : rule->declarations)
        {
            auto colonPos = declaration.indexOf (":");
            if (colonPos > 0)
                applyStyleProperty (declaration.substring (0, colonPos).trim(), declaration.substring (colonPos + 1).trim(), e);
        }
    }
}

//==============================================================================

void SVGCssParser::parseStyleElement (const XmlElement& element)
{
    auto css = element.getAllSubText();
    int ruleOrder = static_cast<int> (data.cssRules.size());

    YUP_DRAWABLE_LOG ("parseStyleElement - cssLength: " << css.length() << " existingRules: " << data.cssRules.size());

    while (css.isNotEmpty())
    {
        auto openBrace = css.indexOf ("{");
        auto closeBrace = css.indexOf ("}");
        if (openBrace <= 0 || closeBrace <= openBrace)
            break;

        auto selectorText = css.substring (0, openBrace).trim();
        auto declarationText = css.substring (openBrace + 1, closeBrace).trim();

        css = css.substring (closeBrace + 1);

        auto selectors = StringArray::fromTokens (selectorText, ",", "");
        auto declarations = StringArray::fromTokens (declarationText, ";", "");

        for (auto selector : selectors)
        {
            selector = selector.trim();
            if (selector.isEmpty())
                continue;

            SVGCssRule rule;
            rule.selector = selector;
            rule.declarations = declarations;
            rule.order = ruleOrder++;

            if (selector.startsWithChar ('#'))
                rule.specificity = 100;
            else if (selector.startsWithChar ('.'))
                rule.specificity = 10;
            else if (selector.containsChar ('#'))
                rule.specificity = 101;
            else if (selector.containsChar ('.'))
                rule.specificity = 11;
            else
                rule.specificity = 1;

            YUP_DRAWABLE_LOG ("Parsed CSS rule - selector: " << rule.selector
                                                             << " specificity: " << rule.specificity
                                                             << " declarationCount: " << rule.declarations.size()
                                                             << " order: " << rule.order);
            data.cssRules.push_back (std::move (rule));
        }
    }

    YUP_DRAWABLE_LOG ("parseStyleElement result - totalRules: " << data.cssRules.size());
}

//==============================================================================

bool SVGCssParser::matchesCssSelector (const XmlElement& xmlElement, const SVGCssRule& rule) const
{
    auto selector = rule.selector.trim();
    if (selector.isEmpty() || selector.containsChar (' ') || selector.containsChar ('>') || selector.containsChar ('+'))
        return false;

    String tagName;
    String id;
    String className;

    auto hashIndex = selector.indexOf ("#");
    auto dotIndex = selector.indexOf (".");
    auto splitIndex = -1;

    if (hashIndex >= 0 && dotIndex >= 0)
        splitIndex = jmin (hashIndex, dotIndex);
    else
        splitIndex = jmax (hashIndex, dotIndex);

    if (splitIndex > 0)
        tagName = selector.substring (0, splitIndex);

    if (hashIndex == 0)
        id = selector.substring (1);
    else if (hashIndex > 0)
        id = selector.substring (hashIndex + 1, dotIndex > hashIndex ? dotIndex : selector.length());

    if (dotIndex == 0)
        className = selector.substring (1);
    else if (dotIndex > 0)
        className = selector.substring (dotIndex + 1);

    if (splitIndex < 0 && ! selector.startsWithChar ('#') && ! selector.startsWithChar ('.'))
        tagName = selector;

    if (tagName.isNotEmpty() && tagName != xmlElement.getTagNameWithoutNamespace())
        return false;

    if (id.isNotEmpty() && id != xmlElement.getStringAttribute ("id"))
        return false;

    if (className.isNotEmpty())
    {
        auto classes = StringArray::fromTokens (xmlElement.getStringAttribute ("class"), " \t\r\n", "");
        if (! classes.contains (className))
            return false;
    }

    return tagName.isNotEmpty() || id.isNotEmpty() || className.isNotEmpty();
}

} // namespace yup
