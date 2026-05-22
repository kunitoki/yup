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

SVGParser::SVGParser (SVGDocument& doc)
    : document (doc)
    , data (doc.data)
    , cssParser (data)
{
}

//==============================================================================

SVGDocument::Ptr SVGParser::parse (const File& svgFile, const SVGDocument::ParseOptions& options)
{
    YUP_DRAWABLE_LOG ("SVGParser::parse(file) - file: " << svgFile.getFullPathName());

    SVGDocument::ParseOptions effectiveOptions = options;
    if (effectiveOptions.baseDirectory.getFullPathName().isEmpty())
        effectiveOptions.baseDirectory = svgFile.getParentDirectory();

    SVGDocument::Ptr doc = new SVGDocument;
    doc->parseOptions = effectiveOptions;

    SVGParser parser (*doc);
    XmlDocument xmlDoc (svgFile);
    auto svgRoot = std::unique_ptr<XmlElement> (xmlDoc.getDocumentElement());

    if (! parser.parseDocument (std::move (svgRoot)))
        return nullptr;

    return doc;
}

SVGDocument::Ptr SVGParser::parse (StringRef svgText, const SVGDocument::ParseOptions& options)
{
    YUP_DRAWABLE_LOG ("SVGParser::parse(text) - length: " << String (svgText.text).length());

    SVGDocument::Ptr doc = new SVGDocument;
    doc->parseOptions = options;

    SVGParser parser (*doc);
    XmlDocument xmlDoc (String (svgText.text));
    auto svgRoot = std::unique_ptr<XmlElement> (xmlDoc.getDocumentElement());

    if (! parser.parseDocument (std::move (svgRoot)))
        return nullptr;

    return doc;
}

//==============================================================================

bool SVGParser::parseDocument (std::unique_ptr<XmlElement> svgRoot)
{
    if (svgRoot == nullptr || ! svgRoot->hasTagName ("svg"))
    {
        YUP_DRAWABLE_LOG ("parseDocument failed - root is " << (svgRoot == nullptr ? "null" : svgRoot->getTagName()));
        return false;
    }

    YUP_DRAWABLE_LOG ("parseDocument - root attributes width: " << svgRoot->getStringAttribute ("width")
                                                                << " height: " << svgRoot->getStringAttribute ("height")
                                                                << " viewBox: " << svgRoot->getStringAttribute ("viewBox"));

    if (auto view = svgRoot->getStringAttribute ("viewBox"); view.isNotEmpty())
    {
        auto coords = StringArray::fromTokens (view, " ,", "");
        if (coords.size() == 4)
        {
            data.viewBox.setX (coords.getReference (0).getFloatValue());
            data.viewBox.setY (coords.getReference (1).getFloatValue());
            data.viewBox.setWidth (coords.getReference (2).getFloatValue());
            data.viewBox.setHeight (coords.getReference (3).getFloatValue());
        }
        else
        {
            YUP_DRAWABLE_LOG ("Invalid root viewBox - expected 4 coordinates, got: " << coords.size() << " value: " << view);
        }
    }

    auto width = svgRoot->getFloatAttribute ("width");
    data.size.setWidth (width == 0.0f ? data.viewBox.getWidth() : width);

    auto height = svgRoot->getFloatAttribute ("height");
    data.size.setHeight (height == 0.0f ? data.viewBox.getHeight() : height);

    YUP_DRAWABLE_LOG ("Parse complete - viewBox: " << data.viewBox.toString() << " size: " << data.size.getWidth() << "x" << data.size.getHeight());

    std::function<void (const XmlElement&)> collectStyleElements = [&] (const XmlElement& xml)
    {
        if (xml.hasTagName ("style"))
            cssParser.parseStyleElement (xml);

        for (auto* child = xml.getFirstChildElement(); child != nullptr; child = child->getNextElement())
            collectStyleElements (*child);
    };

    collectStyleElements (*svgRoot);

    auto result = parseElement (*svgRoot, true, {});

    resolvePatternHrefs();

    if (result)
    {
        data.bounds = document.calculateBounds();
        YUP_DRAWABLE_LOG ("parseDocument result - success: true"
                          << " topLevelElements: " << data.elements.size()
                          << " ids: " << data.elementsById.size()
                          << " gradients: " << data.gradients.size()
                          << " filters: " << data.filters.size()
                          << " clipPaths: " << data.clipPaths.size()
                          << " cssRules: " << data.cssRules.size()
                          << " bounds: " << data.bounds.toString()
                          << " rootHasFill: " << (data.rootHasFill ? "true" : "false")
                          << " rootHasStroke: " << (data.rootHasStroke ? "true" : "false")
                          << " rootFillColor: " << (data.rootFillColor ? data.rootFillColor->toString() : "none")
                          << " rootStrokeColor: " << (data.rootStrokeColor ? data.rootStrokeColor->toString() : "none"));
    }
    else
    {
        YUP_DRAWABLE_LOG ("parseDocument result - success: false");
    }

    return result;
}

//==============================================================================

bool SVGParser::parseElement (const XmlElement& element, bool parentIsRoot, AffineTransform currentTransform, SVGElement* parent)
{
    SVGElement::Ptr e = new SVGElement;
    bool isDocumentRoot = parent == nullptr && element.hasTagName ("svg");
    e->tagName = element.getTagNameWithoutNamespace();

    YUP_DRAWABLE_LOG ("parseElement - tag: " << e->tagName
                                             << " id: " << element.getStringAttribute ("id", "none")
                                             << " parent: " << (parent != nullptr ? parent->tagName : "none")
                                             << " parentIsRoot: " << (parentIsRoot ? "true" : "false")
                                             << " currentTransform: " << currentTransform.toString());

    if (auto classes = element.getStringAttribute ("class"); classes.isNotEmpty())
        e->classNames = StringArray::fromTokens (classes, " \t\r\n", "");

    if (auto id = element.getStringAttribute ("id"); id.isNotEmpty())
    {
        e->id = id;
        data.elementsById.set (id, e);
    }

    const float inheritedFontSize = parent != nullptr && parent->fontSize ? *parent->fontSize : 12.0f;
    const float viewportWidth = data.viewBox.getWidth() > 0.0f ? data.viewBox.getWidth() : (data.size.getWidth() > 0.0f ? data.size.getWidth() : 100.0f);
    const float viewportHeight = data.viewBox.getHeight() > 0.0f ? data.viewBox.getHeight() : (data.size.getHeight() > 0.0f ? data.size.getHeight() : 100.0f);
    const float viewportDiagonal = std::sqrt ((viewportWidth * viewportWidth + viewportHeight * viewportHeight) * 0.5f);

    if (element.hasTagName ("path"))
    {
        auto path = Path();

        String pathData = element.getStringAttribute ("d");
        auto trimmedPathData = pathData.trimStart();
        if (trimmedPathData.isNotEmpty() && ! String ("MmZzLlHhVvCcSsQqTtAa").containsChar (trimmedPathData[0]))
        {
            YUP_DRAWABLE_LOG ("parseElement failed - invalid path command start: " << trimmedPathData[0]
                                                                                   << " id: " << element.getStringAttribute ("id", "none"));
            return false;
        }

        if (! pathData.isEmpty() && ! path.fromString (pathData))
        {
            YUP_DRAWABLE_LOG ("parseElement failed - path.fromString failed"
                              << " id: " << element.getStringAttribute ("id", "none")
                              << " pathLength: " << pathData.length());
            return false;
        }

        e->path = std::move (path);
        YUP_DRAWABLE_LOG ("Parsed path - id: " << element.getStringAttribute ("id", "none")
                                               << " pathLength: " << pathData.length()
                                               << " bounds: " << e->path->getBounds().toString()
                                               << " empty: " << (e->path->isEmpty() ? "true" : "false"));

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);

        if (e->fillRule && *e->fillRule == "evenodd")
            e->path->setUsingNonZeroWinding (false);
    }
    else if (element.hasTagName ("g"))
    {
        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("use"))
    {
        String href = element.getStringAttribute ("href");
        if (href.isEmpty())
            href = element.getStringAttribute ("xlink:href");

        if (href.isNotEmpty() && href.startsWith ("#"))
            e->reference = href.substring (1);

        YUP_DRAWABLE_LOG ("Parsed use attributes - id: " << element.getStringAttribute ("id", "none")
                                                         << " href: " << href
                                                         << " reference: " << (e->reference ? *e->reference : "none"));

        auto x = parseLengthAttribute (element, "x", 0.0f, inheritedFontSize, viewportWidth);
        auto y = parseLengthAttribute (element, "y", 0.0f, inheritedFontSize, viewportHeight);
        auto width = parseLengthAttribute (element, "width", 0.0f, inheritedFontSize, viewportWidth);
        auto height = parseLengthAttribute (element, "height", 0.0f, inheritedFontSize, viewportHeight);
        if (width > 0.0f && height > 0.0f)
            e->viewportSize = Size<float> (width, height);

        YUP_DRAWABLE_LOG ("Parsed use geometry - reference: " << (e->reference ? *e->reference : "none")
                                                              << " x: " << x
                                                              << " y: " << y
                                                              << " width: " << width
                                                              << " height: " << height
                                                              << " hasViewportSize: " << (e->viewportSize ? "true" : "false"));

        AffineTransform useTransform;
        if (x != 0.0f || y != 0.0f)
            useTransform = AffineTransform::translation (x, y);

        currentTransform = parseTransform (element, currentTransform, *e);

        if (! useTransform.isIdentity())
        {
            if (e->transform.has_value())
                e->transform = useTransform.followedBy (*e->transform);
            else
                e->transform = useTransform;
        }

        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("ellipse"))
    {
        auto cx = parseLengthAttribute (element, "cx", 0.0f, inheritedFontSize, viewportWidth);
        auto cy = parseLengthAttribute (element, "cy", 0.0f, inheritedFontSize, viewportHeight);
        auto rx = parseLengthAttribute (element, "rx", 0.0f, inheritedFontSize, viewportDiagonal);
        auto ry = parseLengthAttribute (element, "ry", 0.0f, inheritedFontSize, viewportDiagonal);

        auto path = Path();
        path.addCenteredEllipse (cx, cy, rx, ry);
        e->path = std::move (path);

        YUP_DRAWABLE_LOG ("Parsed ellipse - id: " << element.getStringAttribute ("id", "none")
                                                  << " cx: " << cx << " cy: " << cy
                                                  << " rx: " << rx << " ry: " << ry
                                                  << " bounds: " << e->path->getBounds().toString());

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("circle"))
    {
        auto cx = parseLengthAttribute (element, "cx", 0.0f, inheritedFontSize, viewportWidth);
        auto cy = parseLengthAttribute (element, "cy", 0.0f, inheritedFontSize, viewportHeight);
        auto r = parseLengthAttribute (element, "r", 0.0f, inheritedFontSize, viewportDiagonal);

        auto path = Path();
        path.addCenteredEllipse (cx, cy, r, r);
        e->path = std::move (path);

        YUP_DRAWABLE_LOG ("Parsed circle - id: " << element.getStringAttribute ("id", "none")
                                                 << " cx: " << cx << " cy: " << cy
                                                 << " r: " << r
                                                 << " bounds: " << e->path->getBounds().toString());

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("rect"))
    {
        auto x = parseLengthAttribute (element, "x", 0.0f, inheritedFontSize, viewportWidth);
        auto y = parseLengthAttribute (element, "y", 0.0f, inheritedFontSize, viewportHeight);
        auto width = parseLengthAttribute (element, "width", 0.0f, inheritedFontSize, viewportWidth);
        auto height = parseLengthAttribute (element, "height", 0.0f, inheritedFontSize, viewportHeight);
        auto rx = parseLengthAttribute (element, "rx", 0.0f, inheritedFontSize, viewportWidth);
        auto ry = parseLengthAttribute (element, "ry", 0.0f, inheritedFontSize, viewportHeight);

        auto path = Path();
        if (rx > 0.0f || ry > 0.0f)
        {
            if (rx == 0.0f)
                rx = ry;
            if (ry == 0.0f)
                ry = rx;
            path.addRoundedRectangle (x, y, width, height, rx, ry, rx, ry);
        }
        else
        {
            path.addRectangle (x, y, width, height);
        }

        e->path = std::move (path);

        YUP_DRAWABLE_LOG ("Parsed rect - id: " << element.getStringAttribute ("id", "none")
                                               << " x: " << x << " y: " << y
                                               << " width: " << width << " height: " << height
                                               << " rx: " << rx << " ry: " << ry
                                               << " bounds: " << e->path->getBounds().toString());

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("line"))
    {
        auto x1 = parseLengthAttribute (element, "x1", 0.0f, inheritedFontSize, viewportWidth);
        auto y1 = parseLengthAttribute (element, "y1", 0.0f, inheritedFontSize, viewportHeight);
        auto x2 = parseLengthAttribute (element, "x2", 0.0f, inheritedFontSize, viewportWidth);
        auto y2 = parseLengthAttribute (element, "y2", 0.0f, inheritedFontSize, viewportHeight);

        auto path = Path();
        path.startNewSubPath (x1, y1);
        path.lineTo (x2, y2);
        e->path = std::move (path);

        YUP_DRAWABLE_LOG ("Parsed line - id: " << element.getStringAttribute ("id", "none")
                                               << " x1: " << x1 << " y1: " << y1
                                               << " x2: " << x2 << " y2: " << y2
                                               << " bounds: " << e->path->getBounds().toString());

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("polygon"))
    {
        String points = element.getStringAttribute ("points");
        if (points.isNotEmpty())
        {
            auto path = Path();
            auto coords = StringArray::fromTokens (points, " ,", "");

            if (coords.size() >= 4 && coords.size() % 2 == 0)
            {
                path.startNewSubPath (coords[0].getFloatValue(), coords[1].getFloatValue());

                for (int i = 2; i < coords.size(); i += 2)
                    path.lineTo (coords[i].getFloatValue(), coords[i + 1].getFloatValue());

                path.closeSubPath();
            }

            e->path = std::move (path);
            YUP_DRAWABLE_LOG ("Parsed polygon - id: " << element.getStringAttribute ("id", "none")
                                                      << " coordinateCount: " << coords.size()
                                                      << " bounds: " << e->path->getBounds().toString()
                                                      << " empty: " << (e->path->isEmpty() ? "true" : "false"));
        }
        else
        {
            YUP_DRAWABLE_LOG ("Parsed polygon with empty points - id: " << element.getStringAttribute ("id", "none"));
        }

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("polyline"))
    {
        String points = element.getStringAttribute ("points");
        if (points.isNotEmpty())
        {
            auto path = Path();
            auto coords = StringArray::fromTokens (points, " ,", "");

            if (coords.size() >= 4 && coords.size() % 2 == 0)
            {
                path.startNewSubPath (coords[0].getFloatValue(), coords[1].getFloatValue());

                for (int i = 2; i < coords.size(); i += 2)
                    path.lineTo (coords[i].getFloatValue(), coords[i + 1].getFloatValue());
            }

            e->path = std::move (path);
            YUP_DRAWABLE_LOG ("Parsed polyline - id: " << element.getStringAttribute ("id", "none")
                                                       << " coordinateCount: " << coords.size()
                                                       << " bounds: " << e->path->getBounds().toString()
                                                       << " empty: " << (e->path->isEmpty() ? "true" : "false"));
        }
        else
        {
            YUP_DRAWABLE_LOG ("Parsed polyline with empty points - id: " << element.getStringAttribute ("id", "none"));
        }

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("text") || element.hasTagName ("tspan"))
    {
        const auto defaultTextPosition = parent != nullptr && parent->textPosition ? *parent->textPosition : Point<float> (0.0f, 0.0f);

        float x = parseLengthAttribute (element, "x", defaultTextPosition.getX(), inheritedFontSize, viewportWidth);
        float y = parseLengthAttribute (element, "y", defaultTextPosition.getY(), inheritedFontSize, viewportHeight);
        e->textPosition = Point<float> (x, y);

        String directText;
        for (auto* child : element.getChildIterator())
        {
            if (child->isTextElement())
                directText += child->getText();
        }

        auto text = directText.isNotEmpty() ? directText : (element.hasTagName ("tspan") ? element.getAllSubText() : String());
        auto normalizedText = text.replaceCharacters ("\t\r\n", "   ").trim();
        while (normalizedText.contains ("  "))
            normalizedText = normalizedText.replace ("  ", " ");

        e->text = normalizedText;

        YUP_DRAWABLE_LOG ("Parsed text - tag: " << e->tagName
                                                << " id: " << element.getStringAttribute ("id", "none")
                                                << " textLength: " << (e->text ? e->text->length() : 0)
                                                << " position: " << e->textPosition->toString());

        if (auto xList = element.getStringAttribute ("x"); xList.isNotEmpty())
            e->textX = parseLengthList (xList, inheritedFontSize, viewportWidth);

        if (auto yList = element.getStringAttribute ("y"); yList.isNotEmpty())
            e->textY = parseLengthList (yList, inheritedFontSize, viewportHeight);

        if (auto dxList = element.getStringAttribute ("dx"); dxList.isNotEmpty())
            e->textDx = parseLengthList (dxList, inheritedFontSize, viewportWidth);

        if (auto dyList = element.getStringAttribute ("dy"); dyList.isNotEmpty())
            e->textDy = parseLengthList (dyList, inheritedFontSize, viewportHeight);

        if (auto fontFamily = element.getStringAttribute ("font-family"); fontFamily.isNotEmpty())
            e->fontFamily = fontFamily;

        float fontSize = parseLengthAttribute (element, "font-size", 0.0f, inheritedFontSize, inheritedFontSize);
        if (fontSize > 0.0f)
            e->fontSize = fontSize;

        if (auto textAnchor = element.getStringAttribute ("text-anchor"); textAnchor.isNotEmpty())
            e->textAnchor = textAnchor;

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("image"))
    {
        auto x = parseLengthAttribute (element, "x", 0.0f, inheritedFontSize, viewportWidth);
        auto y = parseLengthAttribute (element, "y", 0.0f, inheritedFontSize, viewportHeight);
        auto width = parseLengthAttribute (element, "width", 0.0f, inheritedFontSize, viewportWidth);
        auto height = parseLengthAttribute (element, "height", 0.0f, inheritedFontSize, viewportHeight);

        e->imageBounds = Rectangle<float> (x, y, width, height);

        if (auto preserveAspectRatio = element.getStringAttribute ("preserveAspectRatio"); preserveAspectRatio.isNotEmpty())
        {
            e->preserveAspectRatioFitting = parsePreserveAspectRatio (preserveAspectRatio);
            e->preserveAspectRatioJustification = parseAspectRatioAlignment (preserveAspectRatio);
        }

        String href = element.getStringAttribute ("href");
        if (href.isEmpty())
            href = element.getStringAttribute ("xlink:href");

        if (href.isNotEmpty())
        {
            e->imageHref = href;
            e->image = loadImageFromHref (document.parseOptions, href);
        }

        YUP_DRAWABLE_LOG ("Parsed image - id: " << element.getStringAttribute ("id", "none")
                                                << " href: " << href
                                                << " bounds: " << e->imageBounds->toString()
                                                << " loaded: " << (e->image ? "true" : "false"));

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("svg") || element.hasTagName ("symbol"))
    {
        e->isSymbol = element.hasTagName ("symbol");
        if (e->isSymbol)
            e->hidden = true;

        if (auto view = element.getStringAttribute ("viewBox"); view.isNotEmpty())
        {
            auto coords = StringArray::fromTokens (view, " ,", "");
            if (coords.size() == 4)
                e->viewBox = Rectangle<float> (coords[0].getFloatValue(), coords[1].getFloatValue(), coords[2].getFloatValue(), coords[3].getFloatValue());
            else
                YUP_DRAWABLE_LOG ("Invalid nested viewBox - tag: " << e->tagName << " value: " << view << " coordinateCount: " << coords.size());
        }

        auto width = parseLengthAttribute (element, "width", e->viewBox ? e->viewBox->getWidth() : viewportWidth, inheritedFontSize, viewportWidth);
        auto height = parseLengthAttribute (element, "height", e->viewBox ? e->viewBox->getHeight() : viewportHeight, inheritedFontSize, viewportHeight);
        if (width > 0.0f && height > 0.0f)
            e->viewportSize = Size<float> (width, height);

        if (auto preserveAspectRatio = element.getStringAttribute ("preserveAspectRatio"); preserveAspectRatio.isNotEmpty())
        {
            e->preserveAspectRatioFitting = parsePreserveAspectRatio (preserveAspectRatio);
            e->preserveAspectRatioJustification = parseAspectRatioAlignment (preserveAspectRatio);
        }

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);

        YUP_DRAWABLE_LOG ("Parsed container - tag: " << e->tagName
                                                     << " id: " << element.getStringAttribute ("id", "none")
                                                     << " viewBox: " << (e->viewBox ? e->viewBox->toString() : "none")
                                                     << " hidden: " << (e->hidden ? "true" : "false"));
    }
    else if (element.hasTagName ("defs"))
    {
        e->hidden = true;

        YUP_DRAWABLE_LOG ("Parsing defs - id: " << element.getStringAttribute ("id", "none"));
    }
    else if (element.hasTagName ("style"))
    {
        YUP_DRAWABLE_LOG ("Parsing style element");
        return true;
    }
    else
    {
        YUP_DRAWABLE_LOG ("Unsupported SVG element parsed as container only - tag: " << e->tagName
                                                                                     << " id: " << element.getStringAttribute ("id", "none"));
    }

    if (parent != nullptr)
    {
        if (! e->fontFamily && parent->fontFamily)
            e->fontFamily = parent->fontFamily;
        if (! e->fontSize && parent->fontSize)
            e->fontSize = parent->fontSize;
        if (! e->fontWeight && parent->fontWeight)
            e->fontWeight = parent->fontWeight;
        if (! e->fontItalic && parent->fontItalic)
            e->fontItalic = parent->fontItalic;
        if (! e->textAnchor && parent->textAnchor)
            e->textAnchor = parent->textAnchor;
        if (! e->letterSpacing && parent->letterSpacing)
            e->letterSpacing = parent->letterSpacing;
        if (! e->wordSpacing && parent->wordSpacing)
            e->wordSpacing = parent->wordSpacing;
        if (! e->color && parent->color)
            e->color = parent->color;
    }

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->isTextElement())
            continue;

        if (child->hasTagName ("linearGradient") || child->hasTagName ("radialGradient"))
            parseGradient (*child);
        else if (child->hasTagName ("filter"))
            parseFilter (*child);
        else if (child->hasTagName ("clipPath"))
            parseClipPath (*child);
        else if (child->hasTagName ("mask"))
            parseMask (*child);
        else if (child->hasTagName ("marker"))
            parseMarker (*child);
        else if (child->hasTagName ("pattern"))
            parsePattern (*child);
        else if (child->hasTagName ("style"))
            continue;
        else
            parseElement (*child, isDocumentRoot, currentTransform, e.get());
    }

    if (e->tagName == "text" || e->tagName == "tspan")
    {
        auto cursor = e->textPosition.value_or (Point<float> (0.0f, 0.0f));

        for (auto& childElement : e->children)
        {
            if (childElement->tagName != "tspan")
                continue;

            auto position = cursor;

            if (childElement->textX && ! childElement->textX->isEmpty())
                position.setX (childElement->textX->getFirst());

            if (childElement->textY && ! childElement->textY->isEmpty())
                position.setY (childElement->textY->getFirst());

            if (childElement->textDx && ! childElement->textDx->isEmpty())
                position.setX (position.getX() + childElement->textDx->getFirst());

            if (childElement->textDy && ! childElement->textDy->isEmpty())
                position.setY (position.getY() + childElement->textDy->getFirst());

            childElement->textPosition = position;
            childElement->textX.reset();
            childElement->textY.reset();
            childElement->textDx.reset();
            childElement->textDy.reset();

            cursor = position;
        }
    }

    if (isDocumentRoot)
    {
        if (e->fillColor)
        {
            data.rootFillColor = e->fillColor;
            data.rootHasFill = true;
        }
        else if (e->noFill)
        {
            data.rootHasFill = false;
        }

        if (e->strokeColor)
        {
            data.rootStrokeColor = e->strokeColor;
            data.rootHasStroke = true;
        }
        else if (e->noStroke)
        {
            data.rootHasStroke = false;
        }

        YUP_DRAWABLE_LOG ("Root element parsed - rootHasFill: " << (data.rootHasFill ? "true" : "false")
                                                                << " rootHasStroke: " << (data.rootHasStroke ? "true" : "false"));
        return true;
    }

    if (parent != nullptr && ! parentIsRoot)
        parent->children.push_back (std::move (e));
    else
        data.elements.push_back (std::move (e));

    return true;
}

//==============================================================================

void SVGParser::parseStyle (const XmlElement& element, const AffineTransform& currentTransform, SVGElement& e)
{
    String styleAttr = element.getStringAttribute ("style");

    YUP_DRAWABLE_LOG ("parseStyle - tag: " << e.tagName
                                           << " id: " << e.id.value_or (String ("none"))
                                           << " fillAttr: " << element.getStringAttribute ("fill")
                                           << " strokeAttr: " << element.getStringAttribute ("stroke")
                                           << " style: " << styleAttr
                                           << " currentTransform: " << currentTransform.toString());

    String fill = element.getStringAttribute ("fill");
    if (fill.isNotEmpty())
    {
        if (fill != "none")
        {
            String gradientUrl = extractGradientUrl (fill);
            if (gradientUrl.isNotEmpty())
                e.fillUrl = gradientUrl;
            else if (fill == "currentColor")
                e.fillCurrentColor = true;
            else
                e.fillColor = Color::fromString (fill);
        }
        else
        {
            e.noFill = true;
        }
    }

    String stroke = element.getStringAttribute ("stroke");
    if (stroke.isNotEmpty())
    {
        if (stroke != "none")
        {
            String gradientUrl = extractGradientUrl (stroke);
            if (gradientUrl.isNotEmpty())
                e.strokeUrl = gradientUrl;
            else if (stroke == "currentColor")
                e.strokeCurrentColor = true;
            else
                e.strokeColor = Color::fromString (stroke);
        }
        else
        {
            e.noStroke = true;
        }
    }

    String strokeJoin = element.getStringAttribute ("stroke-linejoin");
    if (strokeJoin == "round")
        e.strokeJoin = StrokeJoin::Round;
    else if (strokeJoin == "miter")
        e.strokeJoin = StrokeJoin::Miter;
    else if (strokeJoin == "bevel")
        e.strokeJoin = StrokeJoin::Bevel;

    String strokeCap = element.getStringAttribute ("stroke-linecap");
    if (strokeCap == "round")
        e.strokeCap = StrokeCap::Round;
    else if (strokeCap == "square")
        e.strokeCap = StrokeCap::Square;
    else if (strokeCap == "butt")
        e.strokeCap = StrokeCap::Butt;

    float strokeWidth = element.getFloatAttribute ("stroke-width", -1.0f);
    if (strokeWidth > 0.0f)
        e.strokeWidth = strokeWidth;

    float opacity = element.getFloatAttribute ("opacity", -1.0f);
    if (opacity >= 0.0f && opacity <= 1.0f)
        e.opacity = opacity;

    String clipPath = element.getStringAttribute ("clip-path");
    if (clipPath.isNotEmpty())
    {
        String clipPathUrl = extractGradientUrl (clipPath);
        if (clipPathUrl.isNotEmpty())
            e.clipPathUrl = clipPathUrl;
    }

    String filter = element.getStringAttribute ("filter");
    if (filter.isNotEmpty())
    {
        if (filter == "none")
            e.filterUrl.reset();
        else if (auto filterUrl = extractUrlId (filter); filterUrl.isNotEmpty())
            e.filterUrl = filterUrl;
    }

    String mask = element.getStringAttribute ("mask");
    if (mask.isNotEmpty())
    {
        if (auto maskUrl = extractUrlId (mask); maskUrl.isNotEmpty())
            e.maskUrl = maskUrl;
    }

    String markerStart = element.getStringAttribute ("marker-start");
    if (markerStart.isNotEmpty())
    {
        if (auto url = extractUrlId (markerStart); url.isNotEmpty())
            e.markerStart = url;
    }

    String markerMid = element.getStringAttribute ("marker-mid");
    if (markerMid.isNotEmpty())
    {
        if (auto url = extractUrlId (markerMid); url.isNotEmpty())
            e.markerMid = url;
    }

    String markerEnd = element.getStringAttribute ("marker-end");
    if (markerEnd.isNotEmpty())
    {
        if (auto url = extractUrlId (markerEnd); url.isNotEmpty())
            e.markerEnd = url;
    }

    String markerShorthand = element.getStringAttribute ("marker");
    if (markerShorthand.isNotEmpty())
    {
        if (auto url = extractUrlId (markerShorthand); url.isNotEmpty())
        {
            e.markerStart = url;
            e.markerMid = url;
            e.markerEnd = url;
        }
    }

    float strokeMiterLimit = element.getFloatAttribute ("stroke-miterlimit", -1.0f);
    if (strokeMiterLimit >= 0.0f)
        e.strokeMiterLimit = std::max (1.0f, strokeMiterLimit);

    String dashArray = element.getStringAttribute ("stroke-dasharray");
    if (dashArray.isNotEmpty() && dashArray != "none")
    {
        auto dashValues = StringArray::fromTokens (dashArray, " ,", "");
        if (! dashValues.isEmpty())
        {
            Array<float> dashes;
            for (const auto& dash : dashValues)
            {
                float value = parseUnit (dash);
                if (value >= 0.0f)
                    dashes.add (value);
            }

            if (! dashes.isEmpty())
                e.strokeDashArray = dashes;
        }
    }

    String dashOffset = element.getStringAttribute ("stroke-dashoffset");
    if (dashOffset.isNotEmpty())
        e.strokeDashOffset = parseUnit (dashOffset);

    float fillOpacity = element.getFloatAttribute ("fill-opacity", -1.0f);
    if (fillOpacity >= 0.0f && fillOpacity <= 1.0f)
        e.fillOpacity = fillOpacity;

    float strokeOpacity = element.getFloatAttribute ("stroke-opacity", -1.0f);
    if (strokeOpacity >= 0.0f && strokeOpacity <= 1.0f)
        e.strokeOpacity = strokeOpacity;

    String fillRule = element.getStringAttribute ("fill-rule");
    if (fillRule == "evenodd" || fillRule == "nonzero")
        e.fillRule = fillRule;

    String clipRule = element.getStringAttribute ("clip-rule");
    if (clipRule == "evenodd" || clipRule == "nonzero")
        e.clipRule = clipRule;

    String color = element.getStringAttribute ("color");
    if (color.isNotEmpty() && color != "currentColor")
        e.color = Color::fromString (color);

    String display = element.getStringAttribute ("display");
    String visibility = element.getStringAttribute ("visibility");
    if (display == "none" || visibility == "hidden" || visibility == "collapse")
        e.hidden = true;

    String mixBlendMode = element.getStringAttribute ("mix-blend-mode");
    if (mixBlendMode.isNotEmpty())
    {
        if (mixBlendMode == "multiply")
            e.blendMode = BlendMode::Multiply;
        else if (mixBlendMode == "screen")
            e.blendMode = BlendMode::Screen;
        else if (mixBlendMode == "overlay")
            e.blendMode = BlendMode::Overlay;
        else if (mixBlendMode == "darken")
            e.blendMode = BlendMode::Darken;
        else if (mixBlendMode == "lighten")
            e.blendMode = BlendMode::Lighten;
        else if (mixBlendMode == "color-dodge")
            e.blendMode = BlendMode::ColorDodge;
        else if (mixBlendMode == "color-burn")
            e.blendMode = BlendMode::ColorBurn;
        else if (mixBlendMode == "hard-light")
            e.blendMode = BlendMode::HardLight;
        else if (mixBlendMode == "soft-light")
            e.blendMode = BlendMode::SoftLight;
        else if (mixBlendMode == "difference")
            e.blendMode = BlendMode::Difference;
        else if (mixBlendMode == "exclusion")
            e.blendMode = BlendMode::Exclusion;
        else if (mixBlendMode == "hue")
            e.blendMode = BlendMode::Hue;
        else if (mixBlendMode == "saturation")
            e.blendMode = BlendMode::Saturation;
        else if (mixBlendMode == "color")
            e.blendMode = BlendMode::Color;
        else if (mixBlendMode == "luminosity")
            e.blendMode = BlendMode::Luminosity;
    }

    String fontFamily = element.getStringAttribute ("font-family");
    if (fontFamily.isNotEmpty())
        e.fontFamily = fontFamily;

    String fontSize = element.getStringAttribute ("font-size");
    if (fontSize.isNotEmpty())
        e.fontSize = parseUnit (fontSize, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));

    String fontWeight = element.getStringAttribute ("font-weight");
    if (fontWeight.isNotEmpty())
    {
        if (fontWeight == "bold" || fontWeight == "bolder")
            e.fontWeight = 700;
        else if (fontWeight == "normal" || fontWeight == "lighter")
            e.fontWeight = 400;
        else
        {
            const int numericWeight = fontWeight.getIntValue();
            if (numericWeight >= 100 && numericWeight <= 900)
                e.fontWeight = numericWeight;
        }
    }

    String fontStyle = element.getStringAttribute ("font-style");
    if (fontStyle.isNotEmpty())
        e.fontItalic = (fontStyle == "italic" || fontStyle == "oblique");

    String letterSpacing = element.getStringAttribute ("letter-spacing");
    if (letterSpacing.isNotEmpty() && letterSpacing != "normal")
        e.letterSpacing = parseUnit (letterSpacing, 0.0f, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));

    String wordSpacing = element.getStringAttribute ("word-spacing");
    if (wordSpacing.isNotEmpty() && wordSpacing != "normal")
        e.wordSpacing = parseUnit (wordSpacing, 0.0f, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));

    cssParser.applyStylesheetRules (element, e);

    if (styleAttr.isNotEmpty())
        cssParser.parseCSSStyle (styleAttr, e);
}

//==============================================================================

AffineTransform SVGParser::parseTransform (const XmlElement& element, const AffineTransform& currentTransform, SVGElement& e)
{
    AffineTransform result;

    if (auto transformString = element.getStringAttribute ("transform"); transformString.isNotEmpty())
    {
        YUP_DRAWABLE_LOG ("parseTransform(element) - tag: " << e.tagName
                                                            << " id: " << e.id.value_or (String ("none"))
                                                            << " raw: " << transformString
                                                            << " incoming: " << currentTransform.toString());

        result = parseTransform (transformString);

        if (auto transformOrigin = element.getStringAttribute ("transform-origin"); transformOrigin.isNotEmpty())
        {
            auto origin = parseLengthList (transformOrigin, 12.0f, 100.0f);
            if (origin.size() >= 2)
            {
                result = AffineTransform::translation (-origin[0], -origin[1])
                             .followedBy (result)
                             .followedBy (AffineTransform::translation (origin[0], origin[1]));
            }
        }

        e.transform = result;
        e.localTransform = result;

        YUP_DRAWABLE_LOG ("Parsed element transform: " << result.toString());
    }
    else
    {
        YUP_DRAWABLE_LOG ("parseTransform(element) - no transform tag: " << e.tagName
                                                                         << " id: " << e.id.value_or (String ("none")));
    }

    return currentTransform.followedBy (result);
}

//==============================================================================

AffineTransform SVGParser::parseTransform (const String& transformString)
{
    if (transformString.isEmpty())
        return AffineTransform::identity();

    YUP_DRAWABLE_LOG ("parseTransform(string) - raw: " << transformString);

    AffineTransform result;
    auto data = transformString.getCharPointer();

    while (! data.isEmpty())
    {
        while (data.isWhitespace())
            ++data;

        if (data.isEmpty())
            break;

        String type;
        while (! data.isEmpty() && CharacterFunctions::isLetter (*data))
        {
            type += *data;
            ++data;
        }

        while (data.isWhitespace() || *data == '(')
            ++data;

        Array<float> params;
        while (! data.isEmpty() && *data != ')')
        {
            if (*data == ',' || data.isWhitespace())
            {
                ++data;
                continue;
            }

            String number;
            while (! data.isEmpty() && (*data == '-' || *data == '+' || *data == '.' || *data == 'e' || *data == 'E' || (*data >= '0' && *data <= '9')))
            {
                number += *data;
                ++data;
            }

            if (! number.isEmpty())
                params.add (number.getFloatValue());

            while (data.isWhitespace() || *data == ',')
                ++data;
        }

        if (*data == ')')
            ++data;

        if (type == "translate" && (params.size() == 1 || params.size() == 2))
        {
            const auto tx = params[0];
            const auto ty = (params.size() == 2) ? params[1] : 0.0f;
            result = result.prependedBy (AffineTransform::translation (tx, ty));
        }
        else if (type == "scale" && (params.size() == 1 || params.size() == 2))
        {
            const auto sx = params[0];
            const auto sy = (params.size() == 2) ? params[1] : params[0];
            result = result.prependedBy (AffineTransform::scaling (sx, sy));
        }
        else if (type == "rotate" && (params.size() == 1 || params.size() == 3))
        {
            if (params.size() == 1)
                result = result.prependedBy (AffineTransform::rotation (degreesToRadians (params[0])));
            else
                result = result.prependedBy (AffineTransform::rotation (degreesToRadians (params[0]), params[1], params[2]));
        }
        else if (type == "skewX" && params.size() == 1)
        {
            result = result.prependedBy (AffineTransform::shearing (tanf (degreesToRadians (params[0])), 0.0f));
        }
        else if (type == "skewY" && params.size() == 1)
        {
            result = result.prependedBy (AffineTransform::shearing (0.0f, tanf (degreesToRadians (params[0]))));
        }
        else if (type == "matrix" && params.size() == 6)
        {
            result = result.prependedBy (AffineTransform (
                params[0], params[2], params[4], params[1], params[3], params[5]));
        }
        else
        {
            YUP_DRAWABLE_LOG ("Ignored transform operation - type: " << type << " paramCount: " << params.size());
        }
    }

    YUP_DRAWABLE_LOG ("parseTransform(string) result: " << result.toString());
    return result;
}

//==============================================================================

void SVGParser::parseGradient (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
        return;

    YUP_DRAWABLE_LOG ("Parsing gradient with ID: " << id);

    SVGGradient::Ptr gradient = new SVGGradient;
    gradient->id = id;
    gradient->start = { 0.0f, 0.0f };
    gradient->end = { 1.0f, 0.0f };
    gradient->center = { 0.5f, 0.5f };
    gradient->radius = 0.5f;
    gradient->focal = gradient->center;

    auto parseCoordinate = [&element] (const String& name, float defaultValue, bool& hasValue) -> float
    {
        const auto value = element.getStringAttribute (name);
        if (value.isEmpty())
            return defaultValue;

        hasValue = true;

        if (value.containsChar ('%'))
            return value.upToFirstOccurrenceOf ("%", false, false).getFloatValue() / 100.0f;

        return value.getFloatValue();
    };

    String href = element.getStringAttribute ("href");
    if (href.isEmpty())
        href = element.getStringAttribute ("xlink:href");

    if (href.isNotEmpty() && href.startsWith ("#"))
    {
        gradient->href = href.substring (1);
        YUP_DRAWABLE_LOG ("Gradient references: " << gradient->href);
    }

    if (element.hasTagName ("linearGradient"))
    {
        gradient->type = SVGGradient::Linear;
        bool hasX1 = false, hasY1 = false, hasX2 = false, hasY2 = false;
        gradient->start = { parseCoordinate ("x1", gradient->start.getX(), hasX1), parseCoordinate ("y1", gradient->start.getY(), hasY1) };
        gradient->end = { parseCoordinate ("x2", gradient->end.getX(), hasX2), parseCoordinate ("y2", gradient->end.getY(), hasY2) };
        gradient->hasStart = hasX1 || hasY1;
        gradient->hasEnd = hasX2 || hasY2;
    }
    else if (element.hasTagName ("radialGradient"))
    {
        gradient->type = SVGGradient::Radial;
        bool hasCx = false, hasCy = false, hasR = false;
        bool hasFx = false, hasFy = false;
        gradient->center = { parseCoordinate ("cx", gradient->center.getX(), hasCx), parseCoordinate ("cy", gradient->center.getY(), hasCy) };
        gradient->radius = parseCoordinate ("r", gradient->radius, hasR);

        auto fx = parseCoordinate ("fx", gradient->center.getX(), hasFx);
        auto fy = parseCoordinate ("fy", gradient->center.getY(), hasFy);
        gradient->focal = { fx, fy };
        gradient->hasCenter = hasCx || hasCy;
        gradient->hasRadius = hasR;
        gradient->hasFocal = hasFx || hasFy;
    }

    String gradientUnits = element.getStringAttribute ("gradientUnits");
    if (gradientUnits.isNotEmpty())
    {
        gradient->hasUnits = true;
        gradient->units = (gradientUnits == "userSpaceOnUse") ? SVGGradient::UserSpaceOnUse : SVGGradient::ObjectBoundingBox;
    }
    else
    {
        gradient->units = SVGGradient::ObjectBoundingBox;
    }

    String gradientTransform = element.getStringAttribute ("gradientTransform");
    if (gradientTransform.isNotEmpty())
        gradient->transform = parseTransform (gradientTransform);

    String spreadMethod = element.getStringAttribute ("spreadMethod");
    if (spreadMethod == "pad" || spreadMethod == "reflect" || spreadMethod == "repeat")
    {
        gradient->spreadMethod = spreadMethod;
        gradient->hasSpreadMethod = true;
    }

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (! child->hasTagName ("stop"))
            continue;

        SVGGradientStop stop;
        const auto offsetString = child->getStringAttribute ("offset");
        if (offsetString.containsChar ('%'))
            stop.offset = offsetString.upToFirstOccurrenceOf ("%", false, false).getFloatValue() * 0.01f;
        else
            stop.offset = child->getFloatAttribute ("offset");

        stop.offset = jlimit (0.0f, 1.0f, stop.offset);

        String stopColor = child->getStringAttribute ("stop-color");
        float stopOpacity = child->getFloatAttribute ("stop-opacity", 1.0f);

        if (stopColor.isEmpty())
        {
            String styleAttr = child->getStringAttribute ("style");
            if (styleAttr.isNotEmpty())
            {
                auto declarations = StringArray::fromTokens (styleAttr, ";", "");
                for (const auto& declaration : declarations)
                {
                    auto colonPos = declaration.indexOf (":");
                    if (colonPos > 0)
                    {
                        String property = declaration.substring (0, colonPos).trim();
                        String value = declaration.substring (colonPos + 1).trim();

                        if (property == "stop-color")
                            stopColor = value;
                        else if (property == "stop-opacity")
                            stopOpacity = value.getFloatValue();
                    }
                }
            }
        }

        if (stopColor.isNotEmpty())
            stop.color = Color::fromString (stopColor);

        stop.opacity = stopOpacity;
        gradient->stops.push_back (stop);
    }

    if (! gradient->stops.empty())
    {
        std::sort (gradient->stops.begin(), gradient->stops.end(), [] (const SVGGradientStop& a, const SVGGradientStop& b)
        {
            return a.offset < b.offset;
        });

        if (gradient->stops.front().offset > 0.0f)
        {
            auto first = gradient->stops.front();
            first.offset = 0.0f;
            gradient->stops.insert (gradient->stops.begin(), first);
        }

        if (gradient->stops.back().offset < 1.0f)
        {
            auto last = gradient->stops.back();
            last.offset = 1.0f;
            gradient->stops.push_back (last);
        }
    }

    YUP_DRAWABLE_LOG ("Gradient parsed with " << gradient->stops.size() << " stops");

    data.gradients.push_back (gradient);
    data.gradientsById.set (id, gradient);
}

//==============================================================================

SVGGradient::Ptr SVGParser::getGradientById (const String& id) const
{
    return data.gradientsById[id];
}

//==============================================================================

SVGGradient::Ptr SVGParser::resolveGradient (SVGGradient::Ptr gradient) const
{
    if (gradient == nullptr || gradient->href.isEmpty())
        return gradient;

    auto referencedGradient = getGradientById (gradient->href);
    if (referencedGradient == nullptr)
    {
        YUP_DRAWABLE_LOG ("Referenced gradient not found: " << gradient->href);
        return gradient;
    }

    referencedGradient = resolveGradient (referencedGradient);

    SVGGradient::Ptr resolvedGradient = new SVGGradient;
    resolvedGradient->type = gradient->type;
    resolvedGradient->id = gradient->id;
    resolvedGradient->units = referencedGradient->units;
    resolvedGradient->spreadMethod = referencedGradient->spreadMethod;
    resolvedGradient->start = referencedGradient->start;
    resolvedGradient->end = referencedGradient->end;
    resolvedGradient->center = referencedGradient->center;
    resolvedGradient->radius = referencedGradient->radius;
    resolvedGradient->focal = referencedGradient->focal;
    resolvedGradient->transform = referencedGradient->transform;
    resolvedGradient->stops = referencedGradient->stops;
    resolvedGradient->hasUnits = referencedGradient->hasUnits;
    resolvedGradient->hasSpreadMethod = referencedGradient->hasSpreadMethod;

    if (gradient->hasStart)
        resolvedGradient->start = gradient->start;
    if (gradient->hasEnd)
        resolvedGradient->end = gradient->end;
    if (gradient->hasCenter)
        resolvedGradient->center = gradient->center;
    if (gradient->hasRadius)
        resolvedGradient->radius = gradient->radius;
    if (gradient->hasFocal)
        resolvedGradient->focal = gradient->focal;

    if (! gradient->transform.isIdentity())
        resolvedGradient->transform = gradient->transform;
    if (gradient->hasUnits)
    {
        resolvedGradient->units = gradient->units;
        resolvedGradient->hasUnits = true;
    }
    if (gradient->hasSpreadMethod)
    {
        resolvedGradient->spreadMethod = gradient->spreadMethod;
        resolvedGradient->hasSpreadMethod = true;
    }
    if (! gradient->stops.empty())
        resolvedGradient->stops = gradient->stops;

    YUP_DRAWABLE_LOG ("Resolved gradient " << gradient->id << " from reference " << gradient->href);
    return resolvedGradient;
}

//==============================================================================

void SVGParser::parseFilter (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
    {
        YUP_DRAWABLE_LOG ("parseFilter skipped - missing id");
        return;
    }

    SVGFilter::Ptr filter = new SVGFilter;
    filter->id = id;

    auto href = element.getStringAttribute ("href");
    if (href.isEmpty())
        href = element.getStringAttribute ("xlink:href");

    if (href.isNotEmpty() && href.startsWith ("#"))
        filter->href = href.substring (1);

    YUP_DRAWABLE_LOG ("parseFilter - id: " << id);

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (! child->hasTagName ("feGaussianBlur"))
        {
            YUP_DRAWABLE_LOG ("Unsupported filter primitive ignored - filter: " << id << " tag: " << child->getTagNameWithoutNamespace());
            continue;
        }

        const auto stdDeviationString = child->getStringAttribute ("stdDeviation");
        if (stdDeviationString.isEmpty())
        {
            YUP_DRAWABLE_LOG ("GaussianBlur primitive missing stdDeviation - filter: " << id);
            continue;
        }

        const auto values = StringArray::fromTokens (stdDeviationString, " ,", "");
        float stdDeviation = 0.0f;

        if (values.size() >= 2)
        {
            const auto stdDeviationX = parseUnit (values[0]);
            const auto stdDeviationY = parseUnit (values[1]);
            stdDeviation = jmax (stdDeviationX, stdDeviationY);
        }
        else if (values.size() == 1)
        {
            stdDeviation = parseUnit (values[0]);
        }

        if (stdDeviation > 0.0f)
            filter->gaussianBlurStdDeviation = jmax (filter->gaussianBlurStdDeviation.value_or (0.0f), stdDeviation);
    }

    data.filters.push_back (filter);
    data.filtersById.set (id, filter);
}

//==============================================================================

SVGFilter::Ptr SVGParser::getFilterById (const String& id) const
{
    return data.filtersById[id];
}

//==============================================================================

SVGFilter::Ptr SVGParser::resolveFilter (SVGFilter::Ptr filter) const
{
    if (filter == nullptr || filter->href.isEmpty())
        return filter;

    auto referencedFilter = resolveFilter (getFilterById (filter->href));
    if (referencedFilter == nullptr)
    {
        YUP_DRAWABLE_LOG ("Referenced filter not found: " << filter->href);
        return filter;
    }

    SVGFilter::Ptr resolvedFilter = new SVGFilter;
    resolvedFilter->id = filter->id;
    resolvedFilter->href = filter->href;
    resolvedFilter->gaussianBlurStdDeviation = filter->gaussianBlurStdDeviation
                                                 ? filter->gaussianBlurStdDeviation
                                                 : referencedFilter->gaussianBlurStdDeviation;

    YUP_DRAWABLE_LOG ("Resolved filter " << filter->id << " from reference " << filter->href);
    return resolvedFilter;
}

//==============================================================================

void SVGParser::parseClipPath (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
    {
        YUP_DRAWABLE_LOG ("parseClipPath skipped - missing id");
        return;
    }

    SVGClipPath::Ptr clipPath = new SVGClipPath;
    clipPath->id = id;

    if (element.getStringAttribute ("clipPathUnits") == "objectBoundingBox")
        clipPath->units = SVGClipPath::ObjectBoundingBox;

    YUP_DRAWABLE_LOG ("parseClipPath - id: " << id
                                             << " units: " << (clipPath->units == SVGClipPath::ObjectBoundingBox ? "objectBoundingBox" : "userSpaceOnUse"));

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        SVGElement::Ptr clipElement = new SVGElement;
        clipElement->tagName = child->getTagNameWithoutNamespace();

        if (auto childId = child->getStringAttribute ("id"); childId.isNotEmpty())
            clipElement->id = childId;

        if (child->hasTagName ("path"))
        {
            auto path = Path();
            String pathData = child->getStringAttribute ("d");
            if (pathData.isNotEmpty() && path.fromString (pathData))
                clipElement->path = std::move (path);
        }
        else if (child->hasTagName ("rect"))
        {
            auto x = child->getFloatAttribute ("x");
            auto y = child->getFloatAttribute ("y");
            auto width = child->getFloatAttribute ("width");
            auto height = child->getFloatAttribute ("height");
            auto rx = child->getFloatAttribute ("rx");
            auto ry = child->getFloatAttribute ("ry");

            auto path = Path();
            if (rx > 0.0f || ry > 0.0f)
            {
                if (rx == 0.0f)
                    rx = ry;
                if (ry == 0.0f)
                    ry = rx;
                path.addRoundedRectangle (x, y, width, height, rx, ry, rx, ry);
            }
            else
            {
                path.addRectangle (x, y, width, height);
            }

            clipElement->path = std::move (path);
        }
        else if (child->hasTagName ("circle"))
        {
            auto cx = child->getFloatAttribute ("cx");
            auto cy = child->getFloatAttribute ("cy");
            auto r = child->getFloatAttribute ("r");

            auto path = Path();
            path.addCenteredEllipse (cx, cy, r, r);
            clipElement->path = std::move (path);
        }
        else if (child->hasTagName ("ellipse"))
        {
            auto cx = child->getFloatAttribute ("cx");
            auto cy = child->getFloatAttribute ("cy");
            auto rx = child->getFloatAttribute ("rx");
            auto ry = child->getFloatAttribute ("ry");

            auto path = Path();
            path.addCenteredEllipse (cx, cy, rx, ry);
            clipElement->path = std::move (path);
        }
        else if (child->hasTagName ("polygon") || child->hasTagName ("polyline"))
        {
            String points = child->getStringAttribute ("points");
            auto coords = StringArray::fromTokens (points, " ,", "");
            auto path = Path();

            if (coords.size() >= 4 && coords.size() % 2 == 0)
            {
                path.startNewSubPath (coords[0].getFloatValue(), coords[1].getFloatValue());

                for (int i = 2; i < coords.size(); i += 2)
                    path.lineTo (coords[i].getFloatValue(), coords[i + 1].getFloatValue());

                if (child->hasTagName ("polygon"))
                    path.closeSubPath();
            }

            clipElement->path = std::move (path);
        }
        else
        {
            YUP_DRAWABLE_LOG ("Unsupported clip path child - id: " << id << " tag: " << child->getTagNameWithoutNamespace());
        }

        if (clipElement->path)
        {
            auto clipChildTransform = parseTransform (*child, AffineTransform::identity(), *clipElement);
            parseStyle (*child, clipChildTransform, *clipElement);

            if (clipElement->clipRule && *clipElement->clipRule == "evenodd")
                clipElement->path->setUsingNonZeroWinding (false);

            clipPath->elements.push_back (clipElement);
        }
    }

    data.clipPaths.push_back (clipPath);
    data.clipPathsById.set (id, clipPath);
    YUP_DRAWABLE_LOG ("parseClipPath result - id: " << id << " elementCount: " << clipPath->elements.size());
}

//==============================================================================

SVGClipPath::Ptr SVGParser::getClipPathById (const String& id) const
{
    return data.clipPathsById[id];
}

//==============================================================================

void SVGParser::parseMask (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
    {
        YUP_DRAWABLE_LOG ("parseMask skipped - missing id");
        return;
    }

    SVGMask::Ptr mask = new SVGMask;
    mask->id = id;

    auto maskUnitsStr = element.getStringAttribute ("maskUnits", "objectBoundingBox");
    mask->maskUnits = (maskUnitsStr == "userSpaceOnUse")
                        ? SVGMask::UserSpaceOnUse
                        : SVGMask::ObjectBoundingBox;

    auto maskContentUnitsStr = element.getStringAttribute ("maskContentUnits", "userSpaceOnUse");
    mask->maskContentUnits = (maskContentUnitsStr == "objectBoundingBox")
                               ? SVGMask::ObjectBoundingBox
                               : SVGMask::UserSpaceOnUse;

    if (element.hasAttribute ("x"))
        mask->x = parseLengthAttribute (element, "x", mask->x, 12.0f, 100.0f);
    if (element.hasAttribute ("y"))
        mask->y = parseLengthAttribute (element, "y", mask->y, 12.0f, 100.0f);
    if (element.hasAttribute ("width"))
        mask->width = parseLengthAttribute (element, "width", mask->width, 12.0f, 100.0f);
    if (element.hasAttribute ("height"))
        mask->height = parseLengthAttribute (element, "height", mask->height, 12.0f, 100.0f);

    SVGElement::Ptr containerElement = new SVGElement;
    containerElement->tagName = "mask";

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->isTextElement())
            continue;

        parseElement (*child, false, AffineTransform::identity(), containerElement.get());
    }

    mask->elements = std::move (containerElement->children);

    data.masks.push_back (mask);
    data.masksById.set (id, mask);

    YUP_DRAWABLE_LOG ("parseMask result - id: " << id << " elementCount: " << mask->elements.size());
}

//==============================================================================

void SVGParser::parseMarker (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
    {
        YUP_DRAWABLE_LOG ("parseMarker skipped - missing id");
        return;
    }

    SVGMarker::Ptr marker = new SVGMarker;
    marker->id = id;

    auto markerUnitsStr = element.getStringAttribute ("markerUnits", "strokeWidth");
    marker->markerUnits = (markerUnitsStr == "userSpaceOnUse")
                            ? SVGMarker::UserSpaceOnUse
                            : SVGMarker::StrokeWidth;

    marker->refX = element.getStringAttribute ("refX", "0").getFloatValue();
    marker->refY = element.getStringAttribute ("refY", "0").getFloatValue();
    marker->markerWidth = element.getStringAttribute ("markerWidth", "3").getFloatValue();
    marker->markerHeight = element.getStringAttribute ("markerHeight", "3").getFloatValue();

    if (element.hasAttribute ("orient"))
    {
        auto orientStr = element.getStringAttribute ("orient");
        if (orientStr == "auto-start-reverse")
            marker->orientAutoStartReverse = true;
        else if (orientStr != "auto")
            marker->orient = orientStr.getFloatValue();
    }

    if (auto viewBoxStr = element.getStringAttribute ("viewBox"); viewBoxStr.isNotEmpty())
    {
        auto coords = StringArray::fromTokens (viewBoxStr, " ,", "");
        if (coords.size() == 4)
            marker->viewBox = Rectangle<float> (coords[0].getFloatValue(), coords[1].getFloatValue(), coords[2].getFloatValue(), coords[3].getFloatValue());
    }

    SVGElement::Ptr containerElement = new SVGElement;
    containerElement->tagName = "marker";

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->isTextElement())
            continue;

        parseElement (*child, false, AffineTransform::identity(), containerElement.get());
    }

    marker->elements = std::move (containerElement->children);

    data.markers.push_back (marker);
    data.markersById.set (id, marker);

    YUP_DRAWABLE_LOG ("parseMarker result - id: " << id << " elementCount: " << marker->elements.size());
}

//==============================================================================

void SVGParser::parsePattern (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
    {
        YUP_DRAWABLE_LOG ("parsePattern skipped - missing id");
        return;
    }

    SVGPattern::Ptr pattern = new SVGPattern;
    pattern->id = id;

    auto patternUnitsStr = element.getStringAttribute ("patternUnits", "objectBoundingBox");
    pattern->patternUnits = (patternUnitsStr == "userSpaceOnUse")
                              ? SVGPattern::UserSpaceOnUse
                              : SVGPattern::ObjectBoundingBox;

    auto patternContentUnitsStr = element.getStringAttribute ("patternContentUnits", "userSpaceOnUse");
    pattern->patternContentUnits = (patternContentUnitsStr == "objectBoundingBox")
                                     ? SVGPattern::ObjectBoundingBox
                                     : SVGPattern::UserSpaceOnUse;

    pattern->x = element.getStringAttribute ("x", "0").getFloatValue();
    pattern->y = element.getStringAttribute ("y", "0").getFloatValue();
    pattern->width = element.getStringAttribute ("width", "0").getFloatValue();
    pattern->height = element.getStringAttribute ("height", "0").getFloatValue();

    if (auto patternTransformStr = element.getStringAttribute ("patternTransform"); patternTransformStr.isNotEmpty())
        pattern->patternTransform = parseTransform (patternTransformStr);

    if (auto viewBoxStr = element.getStringAttribute ("viewBox"); viewBoxStr.isNotEmpty())
    {
        auto coords = StringArray::fromTokens (viewBoxStr, " ,", "");
        if (coords.size() == 4)
            pattern->viewBox = Rectangle<float> (coords[0].getFloatValue(), coords[1].getFloatValue(), coords[2].getFloatValue(), coords[3].getFloatValue());
    }

    String href = element.getStringAttribute ("href");
    if (href.isEmpty())
        href = element.getStringAttribute ("xlink:href");
    if (href.startsWith ("#"))
        pattern->href = href.substring (1);

    SVGElement::Ptr containerElement = new SVGElement;
    containerElement->tagName = "pattern";

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->isTextElement())
            continue;

        parseElement (*child, false, AffineTransform::identity(), containerElement.get());
    }

    pattern->elements = std::move (containerElement->children);

    data.patterns.push_back (pattern);
    data.patternsById.set (id, pattern);

    YUP_DRAWABLE_LOG ("parsePattern result - id: " << id << " elementCount: " << pattern->elements.size());
}

//==============================================================================

void SVGParser::resolvePatternHrefs()
{
    for (auto& pattern : data.patterns)
    {
        if (! pattern->href)
            continue;

        auto basePattern = data.patternsById[*pattern->href];
        if (basePattern == nullptr)
        {
            YUP_DRAWABLE_LOG ("resolvePatternHrefs - referenced pattern not found: " << *pattern->href);
            continue;
        }

        if (pattern->width == 0.0f && basePattern->width != 0.0f)
            pattern->width = basePattern->width;

        if (pattern->height == 0.0f && basePattern->height != 0.0f)
            pattern->height = basePattern->height;

        if (pattern->x == 0.0f && basePattern->x != 0.0f)
            pattern->x = basePattern->x;

        if (pattern->y == 0.0f && basePattern->y != 0.0f)
            pattern->y = basePattern->y;

        if (! pattern->viewBox && basePattern->viewBox)
            pattern->viewBox = basePattern->viewBox;

        if (pattern->patternTransform.isIdentity() && ! basePattern->patternTransform.isIdentity())
            pattern->patternTransform = basePattern->patternTransform;

        if (pattern->elements.empty() && ! basePattern->elements.empty())
            pattern->elements = basePattern->elements;
    }
}

//==============================================================================

float SVGParser::parseLengthAttribute (const XmlElement& element, StringRef attributeName, float defaultValue, float fontSize, float viewportSize) const
{
    auto value = element.getStringAttribute (attributeName);
    if (value.isEmpty())
        return defaultValue;

    return parseUnit (value, defaultValue, fontSize, viewportSize);
}

//==============================================================================

Fitting SVGParser::parsePreserveAspectRatio (const String& preserveAspectRatio) const
{
    if (preserveAspectRatio.isEmpty() || preserveAspectRatio == "xMidYMid meet")
        return Fitting::scaleToFit;

    if (preserveAspectRatio.contains ("none"))
        return Fitting::fill;

    if (preserveAspectRatio.contains ("slice"))
        return Fitting::scaleToFill;

    return Fitting::scaleToFit;
}

//==============================================================================

Justification SVGParser::parseAspectRatioAlignment (const String& preserveAspectRatio) const
{
    if (preserveAspectRatio.isEmpty())
        return Justification::center;

    Justification result = Justification::left;

    if (preserveAspectRatio.contains ("xMin"))
        result = result | Justification::left;
    else if (preserveAspectRatio.contains ("xMax"))
        result = result | Justification::right;
    else
        result = result | Justification::horizontalCenter;

    if (preserveAspectRatio.contains ("YMin"))
        result = result | Justification::top;
    else if (preserveAspectRatio.contains ("YMax"))
        result = result | Justification::bottom;
    else
        result = result | Justification::verticalCenter;

    return result;
}

//==============================================================================

float SVGParser::parseUnit (const String& value, float defaultValue, float fontSize, float viewportSize)
{
    if (value.isEmpty())
        return defaultValue;

    String trimmed = value.trim();
    if (trimmed.isEmpty())
        return defaultValue;

    const char* begin = trimmed.toRawUTF8();
    char* end = nullptr;
    const double numericValue = std::strtod (begin, &end);

    if (end == begin)
        return defaultValue;

    String unit = String (CharPointer_UTF8 (end)).trim().toLowerCase();

    if (unit.isEmpty() || unit == "px")
        return static_cast<float> (numericValue);
    if (unit == "pt")
        return static_cast<float> (numericValue * 1.333333);
    if (unit == "pc")
        return static_cast<float> (numericValue * 16.0);
    if (unit == "mm")
        return static_cast<float> (numericValue * 3.779528);
    if (unit == "cm")
        return static_cast<float> (numericValue * 37.79528);
    if (unit == "in")
        return static_cast<float> (numericValue * 96.0);
    if (unit == "em")
        return static_cast<float> (numericValue * fontSize);
    if (unit == "ex")
        return static_cast<float> (numericValue * fontSize * 0.5);
    if (unit == "%")
        return static_cast<float> (numericValue * viewportSize * 0.01);

    return static_cast<float> (numericValue);
}

//==============================================================================

Array<float> SVGParser::parseLengthList (const String& value, float fontSize, float viewportSize)
{
    Array<float> result;
    auto tokens = StringArray::fromTokens (value, " ,\t\r\n", "");

    for (const auto& token : tokens)
    {
        if (token.isNotEmpty())
            result.add (parseUnit (token, 0.0f, fontSize, viewportSize));
    }

    return result;
}

//==============================================================================

String SVGParser::extractGradientUrl (const String& value)
{
    return extractUrlId (value);
}

String SVGParser::extractUrlId (const String& value)
{
    int urlStart = value.indexOf ("url(");
    if (urlStart == -1)
        return String();

    int urlEnd = value.indexOf (urlStart, ")");
    if (urlEnd == -1)
        return String();

    String url = value.substring (urlStart + 4, urlEnd).trim().unquoted();
    if (! url.startsWithChar ('#'))
        return String();

    url = url.substring (1);
    YUP_DRAWABLE_LOG ("Extracted gradient URL: '" << url << "' from: '" << value << "'");
    return url;
}

//==============================================================================

std::optional<Image> SVGParser::loadImageFromHref (const SVGDocument::ParseOptions& options, const String& href)
{
    YUP_DRAWABLE_LOG ("loadImageFromHref - href: " << href
                                                   << " baseDirectory: " << options.baseDirectory.getFullPathName()
                                                   << " allowDataImages: " << (options.allowDataImages ? "true" : "false")
                                                   << " allowLocalImages: " << (options.allowLocalImages ? "true" : "false")
                                                   << " hasResolver: " << (options.imageResolver ? "true" : "false"));

    if (options.imageResolver)
    {
        if (auto resolved = options.imageResolver (href, options.baseDirectory))
        {
            YUP_DRAWABLE_LOG ("loadImageFromHref - resolved by custom resolver");
            return resolved;
        }

        YUP_DRAWABLE_LOG ("loadImageFromHref - custom resolver returned no image");
    }

    if (href.startsWithIgnoreCase ("http:") || href.startsWithIgnoreCase ("https:"))
    {
        YUP_DRAWABLE_LOG ("loadImageFromHref skipped - network URLs are disabled");
        return std::nullopt;
    }

    MemoryBlock imageData;

    if (href.startsWithIgnoreCase ("data:"))
    {
        if (! options.allowDataImages)
        {
            YUP_DRAWABLE_LOG ("loadImageFromHref skipped - data images disabled");
            return std::nullopt;
        }

        auto comma = href.indexOfChar (',');
        if (comma < 0)
        {
            YUP_DRAWABLE_LOG ("loadImageFromHref failed - malformed data URI");
            return std::nullopt;
        }

        auto metadata = href.substring (0, comma).toLowerCase();
        auto payload = href.substring (comma + 1);

        if (metadata.contains (";base64"))
        {
            MemoryOutputStream decoded;
            if (! Base64::convertFromBase64 (decoded, payload))
            {
                YUP_DRAWABLE_LOG ("loadImageFromHref failed - base64 decode failed");
                return std::nullopt;
            }

            imageData = decoded.getMemoryBlock();
        }
        else
        {
            imageData = MemoryBlock (payload.toRawUTF8(), static_cast<size_t> (payload.getNumBytesAsUTF8()));
        }
    }
    else
    {
        if (! options.allowLocalImages || options.baseDirectory.getFullPathName().isEmpty())
        {
            YUP_DRAWABLE_LOG ("loadImageFromHref skipped - local images disabled or base directory empty");
            return std::nullopt;
        }

        auto imageFile = options.baseDirectory.getChildFile (href);
        if (! imageFile.existsAsFile() || ! imageFile.loadFileAsData (imageData))
        {
            YUP_DRAWABLE_LOG ("loadImageFromHref failed - local file missing or unreadable: " << imageFile.getFullPathName());
            return std::nullopt;
        }
    }

    if (imageData.isEmpty())
    {
        YUP_DRAWABLE_LOG ("loadImageFromHref failed - image data empty");
        return std::nullopt;
    }

    auto result = Image::loadFromData (imageData.asBytes());
    if (result.failed())
    {
        YUP_DRAWABLE_LOG ("loadImageFromHref failed - Image::loadFromData failed: " << result.getErrorMessage());
        return std::nullopt;
    }

    YUP_DRAWABLE_LOG ("loadImageFromHref result - image decoded");
    return result.getValue();
}

} // namespace yup
