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
    String value (valueRef.text);
    value = value.trim();

    String property (propertyRef.text);
    property = property.trim().toLowerCase();

    if (property.isEmpty())
        return;

    YUP_DRAWABLE_LOG ("applyStyleProperty - tag: " << e.tagName
                                                   << " id: " << e.id.value_or (String ("none"))
                                                   << " property: " << property
                                                   << " value: " << value);

    // Dispatch table mapping CSS property names to handlers, grouped by semantic category
    using Handler = void (*) (const String&, SVGElement&);

    struct PropertyHandlers
    {
        HashMap<StringRef, Handler> map;

        PropertyHandlers()
        {
            map.set ("fill", [] (const String& v, SVGElement& el)
            {
                el.fillCurrentColor = false;
                el.fillUrl.reset();
                el.fillColor.reset();
                el.noFill = false;
                if (v == "none")
                {
                    el.noFill = true;
                }
                else if (v == "currentColor")
                {
                    el.fillCurrentColor = true;
                }
                else if (auto url = SVGParser::extractUrlId (v); url.isNotEmpty())
                {
                    el.fillUrl = url;
                }
                else if (v.isNotEmpty())
                {
                    el.fillColor = Color::fromString (v);
                }
            });
            map.set ("stroke", [] (const String& v, SVGElement& el)
            {
                el.strokeCurrentColor = false;
                el.strokeUrl.reset();
                el.strokeColor.reset();
                el.noStroke = false;
                if (v == "none")
                {
                    el.noStroke = true;
                }
                else if (v == "currentColor")
                {
                    el.strokeCurrentColor = true;
                }
                else if (auto url = SVGParser::extractUrlId (v); url.isNotEmpty())
                {
                    el.strokeUrl = url;
                }
                else if (v.isNotEmpty())
                {
                    el.strokeColor = Color::fromString (v);
                }
            });
            map.set ("color", [] (const String& v, SVGElement& el)
            {
                if (v != "currentColor" && v != "inherit")
                    el.color = Color::fromString (v);
            });

            // Stroke properties
            map.set ("stroke-width", [] (const String& v, SVGElement& el)
            {
                float sw = SVGParser::parseUnit (v, el.strokeWidth.value_or (1.0f), el.fontSize.value_or (12.0f));
                if (sw >= 0.0f)
                    el.strokeWidth = sw;
            });
            map.set ("stroke-linejoin", [] (const String& v, SVGElement& el)
            {
                if (v == "round")
                    el.strokeJoin = StrokeJoin::Round;
                else if (v == "miter")
                    el.strokeJoin = StrokeJoin::Miter;
                else if (v == "bevel")
                    el.strokeJoin = StrokeJoin::Bevel;
            });
            map.set ("stroke-linecap", [] (const String& v, SVGElement& el)
            {
                if (v == "round")
                    el.strokeCap = StrokeCap::Round;
                else if (v == "square")
                    el.strokeCap = StrokeCap::Square;
                else if (v == "butt")
                    el.strokeCap = StrokeCap::Butt;
            });
            map.set ("stroke-miterlimit", [] (const String& v, SVGElement& el)
            {
                float val = v.getFloatValue();
                el.strokeMiterLimit = std::max (1.0f, val);
            });
            map.set ("stroke-dasharray", [] (const String& v, SVGElement& el)
            {
                if (v == "none")
                {
                    el.strokeDashArray.reset();
                    el.strokeDashArrayNone = true;
                }
                else
                {
                    Array<float> dashes;
                    for (const auto dash : SVGParser::parseLengthList (v, el.fontSize.value_or (12.0f), 100.0f))
                    {
                        if (dash >= 0.0f)
                            dashes.add (dash);
                    }
                    if (! dashes.isEmpty())
                    {
                        el.strokeDashArray = dashes;
                        el.strokeDashArrayNone = false;
                    }
                }
            });
            map.set ("stroke-dashoffset", [] (const String& v, SVGElement& el)
            {
                el.strokeDashOffset = SVGParser::parseUnit (v);
            });

            // Opacity
            map.set ("opacity", [] (const String& v, SVGElement& el)
            {
                float op = v.getFloatValue();
                if (op >= 0.0f && op <= 1.0f)
                    el.opacity = op;
            });
            map.set ("fill-opacity", [] (const String& v, SVGElement& el)
            {
                float op = v.getFloatValue();
                if (op >= 0.0f && op <= 1.0f)
                    el.fillOpacity = op;
            });
            map.set ("stroke-opacity", [] (const String& v, SVGElement& el)
            {
                float op = v.getFloatValue();
                if (op >= 0.0f && op <= 1.0f)
                    el.strokeOpacity = op;
            });

            // Visibility
            map.set ("display", [] (const String& v, SVGElement& el)
            {
                if (v == "none")
                    el.hidden = true;
            });
            map.set ("visibility", [] (const String& v, SVGElement& el)
            {
                el.hidden = (v == "hidden" || v == "collapse");
            });

            // Font
            map.set ("font-family", [] (const String& v, SVGElement& el)
            {
                el.fontFamily = v.unquoted();
            });
            map.set ("font-size", [] (const String& v, SVGElement& el)
            {
                float fs = SVGParser::parseUnit (v, el.fontSize.value_or (12.0f), el.fontSize.value_or (12.0f), el.fontSize.value_or (12.0f));
                if (fs > 0.0f)
                    el.fontSize = fs;
            });
            map.set ("font-weight", [] (const String& v, SVGElement& el)
            {
                if (v == "bold" || v == "bolder")
                    el.fontWeight = 700;
                else if (v == "normal" || v == "lighter")
                    el.fontWeight = 400;
                else
                {
                    int nw = v.getIntValue();
                    if (nw >= 100 && nw <= 900)
                        el.fontWeight = nw;
                }
            });
            map.set ("font-style", [] (const String& v, SVGElement& el)
            {
                el.fontItalic = (v == "italic" || v == "oblique");
            });

            // Text
            map.set ("text-anchor", [] (const String& v, SVGElement& el)
            {
                el.textAnchor = v;
            });
            map.set ("letter-spacing", [] (const String& v, SVGElement& el)
            {
                if (v != "normal")
                    el.letterSpacing = SVGParser::parseUnit (v, 0.0f, el.fontSize.value_or (12.0f), el.fontSize.value_or (12.0f));
            });
            map.set ("word-spacing", [] (const String& v, SVGElement& el)
            {
                if (v != "normal")
                    el.wordSpacing = SVGParser::parseUnit (v, 0.0f, el.fontSize.value_or (12.0f), el.fontSize.value_or (12.0f));
            });

            // Clip/Mask/Filter
            map.set ("clip-path", [] (const String& v, SVGElement& el)
            {
                String url = SVGParser::extractUrlId (v);
                if (url.isNotEmpty())
                    el.clipPathUrl = url;
            });
            map.set ("mask", [] (const String& v, SVGElement& el)
            {
                if (auto url = SVGParser::extractUrlId (v); url.isNotEmpty())
                    el.maskUrl = url;
            });
            map.set ("filter", [] (const String& v, SVGElement& el)
            {
                if (v == "none")
                    el.filterUrl.reset();
                else if (auto url = SVGParser::extractUrlId (v); url.isNotEmpty())
                    el.filterUrl = url;
                else
                    YUP_DRAWABLE_LOG ("CSS filter currently only supports url(...) - value: " << v);
            });

            // Markers
            map.set ("marker-start", [] (const String& v, SVGElement& el)
            {
                if (auto url = SVGParser::extractUrlId (v); url.isNotEmpty())
                    el.markerStart = url;
            });
            map.set ("marker-mid", [] (const String& v, SVGElement& el)
            {
                if (auto url = SVGParser::extractUrlId (v); url.isNotEmpty())
                    el.markerMid = url;
            });
            map.set ("marker-end", [] (const String& v, SVGElement& el)
            {
                if (auto url = SVGParser::extractUrlId (v); url.isNotEmpty())
                    el.markerEnd = url;
            });
            map.set ("marker", [] (const String& v, SVGElement& el)
            {
                if (auto url = SVGParser::extractUrlId (v); url.isNotEmpty())
                {
                    el.markerStart = url;
                    el.markerMid = url;
                    el.markerEnd = url;
                }
            });

            // Rules
            map.set ("fill-rule", [] (const String& v, SVGElement& el)
            {
                if (v == "evenodd" || v == "nonzero")
                    el.fillRule = v;
            });
            map.set ("clip-rule", [] (const String& v, SVGElement& el)
            {
                if (v == "evenodd" || v == "nonzero")
                    el.clipRule = v;
            });

            // Blend mode
            map.set ("mix-blend-mode", [] (const String& v, SVGElement& el)
            {
                el.blendMode = SVGParser::parseBlendMode (v).value_or (BlendMode::SrcOver);
            });
        }
    };

    static const PropertyHandlers handlers;

    if (auto* handler = handlers.map.getPointer (property))
        (*handler) (value, e);
    else
    {
        // Log-only properties (not yet handled)
        YUP_DRAWABLE_LOG ("Unsupported CSS property ignored - property: " << property << " value: " << String (valueRef.text));
    }
}

//==============================================================================

void SVGCssParser::applyStylesheetRules (const XmlElement& xmlElement, SVGElement& e)
{
    if (data.cssRules.empty())
        return;

    // Collect candidate rule indices from each index bucket
    std::vector<int> candidateIndices;

    const auto tagName = xmlElement.getTagNameWithoutNamespace();
    if (auto* tagRules = ruleIndex.byTag.getPointer (tagName))
        candidateIndices.insert (candidateIndices.end(), tagRules->begin(), tagRules->end());

    const auto id = xmlElement.getStringAttribute ("id");
    if (id.isNotEmpty())
    {
        if (auto* idRules = ruleIndex.byId.getPointer (id))
            candidateIndices.insert (candidateIndices.end(), idRules->begin(), idRules->end());
    }

    const auto classAttr = xmlElement.getStringAttribute ("class");
    if (classAttr.isNotEmpty())
    {
        const auto classNames = StringArray::fromTokens (classAttr, " \t\r\n", "");
        for (const auto& className : classNames)
        {
            if (auto* classRules = ruleIndex.byClass.getPointer (className))
                candidateIndices.insert (candidateIndices.end(), classRules->begin(), classRules->end());
        }
    }

    if (candidateIndices.empty())
        return;

    // Deduplicate
    std::sort (candidateIndices.begin(), candidateIndices.end());
    candidateIndices.erase (std::unique (candidateIndices.begin(), candidateIndices.end()), candidateIndices.end());

    // Re-verify each candidate: the index returns candidates by single dimension
    // (tag, id, or class), but multi-part selectors like "circle.highlight" need
    // all parts to match. Filter out candidates where the full selector doesn't match.
    const auto elementId = xmlElement.getStringAttribute ("id");
    const auto elementClasses = classAttr.isNotEmpty()
                                  ? StringArray::fromTokens (classAttr, " \t\r\n", "")
                                  : StringArray();

    const auto selectorMatches = [&] (const SVGCssRule& rule) -> bool
    {
        const auto& sel = rule.selector;
        if (sel.isEmpty() || sel.containsChar (' ') || sel.containsChar ('>') || sel.containsChar ('+'))
            return false;

        const auto hashIdx = sel.indexOf ("#");
        const auto dotIdx = sel.indexOf (".");

        if (hashIdx < 0 && dotIdx < 0)
            return true; // simple tag selector, already matched by byTag[index]

        if (hashIdx >= 0)
        {
            const auto idEnd = (dotIdx > hashIdx) ? dotIdx : sel.length();
            if (sel.substring (hashIdx + 1, idEnd) != elementId)
                return false;
        }

        if (dotIdx >= 0)
        {
            const auto className = sel.substring (dotIdx + 1);
            if (! elementClasses.contains (className))
                return false;
        }

        return true;
    };

    candidateIndices.erase (std::remove_if (candidateIndices.begin(), candidateIndices.end(), [&] (int i)
    {
        return ! selectorMatches (data.cssRules[static_cast<size_t> (i)]);
    }),
                            candidateIndices.end());

    if (candidateIndices.empty())
        return;

    std::vector<const SVGCssRule*> matchedRules;
    matchedRules.reserve (candidateIndices.size());

    for (const auto index : candidateIndices)
        matchedRules.push_back (std::addressof (data.cssRules[static_cast<size_t> (index)]));

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
        auto declarationTokens = StringArray::fromTokens (declarationText, ";", "");
        StringArray declarations;

        for (const auto& declaration : declarationTokens)
        {
            auto trimmedDeclaration = declaration.trim();
            if (trimmedDeclaration.isNotEmpty())
                declarations.add (trimmedDeclaration);
        }

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

void SVGCssParser::buildCssRuleIndex()
{
    ruleIndex = {};
    ruleIndex.buildFrom (data.cssRules);

    YUP_DRAWABLE_LOG ("buildCssRuleIndex - totalRules: " << data.cssRules.size()
                                                         << " byTag: " << ruleIndex.byTag.size()
                                                         << " byId: " << ruleIndex.byId.size()
                                                         << " byClass: " << ruleIndex.byClass.size());
}

} // namespace yup
