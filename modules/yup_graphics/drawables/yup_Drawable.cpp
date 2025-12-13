/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
#ifndef YUP_DRAWABLE_LOGGING
#define YUP_DRAWABLE_LOGGING 0
#endif

#if YUP_DRAWABLE_LOGGING
#define YUP_DRAWABLE_LOG(textToWrite) YUP_DBG (textToWrite)
#else
#define YUP_DRAWABLE_LOG(textToWrite) \
    {                                 \
    }
#endif

//==============================================================================

Drawable::Drawable()
{
}

//==============================================================================

bool Drawable::parseSVG (const File& svgFile)
{
    clear();

    XmlDocument svgDoc (svgFile);
    std::unique_ptr<XmlElement> svgRoot (svgDoc.getDocumentElement());

    if (svgRoot == nullptr || ! svgRoot->hasTagName ("svg"))
        return false;

    if (auto view = svgRoot->getStringAttribute ("viewBox"); view.isNotEmpty())
    {
        auto coords = StringArray::fromTokens (view, " ,", "");
        if (coords.size() == 4)
        {
            viewBox.setX (coords.getReference (0).getFloatValue());
            viewBox.setY (coords.getReference (1).getFloatValue());
            viewBox.setWidth (coords.getReference (2).getFloatValue());
            viewBox.setHeight (coords.getReference (3).getFloatValue());
        }
    }

    auto width = svgRoot->getFloatAttribute ("width");
    size.setWidth (width == 0.0f ? viewBox.getWidth() : width);

    auto height = svgRoot->getFloatAttribute ("height");
    size.setHeight (height == 0.0f ? viewBox.getHeight() : height);

    // ViewBox transform is now calculated at render-time based on actual target area
    YUP_DRAWABLE_LOG ("Parse complete - viewBox: " << viewBox.toString() << " size: " << size.getWidth() << "x" << size.getHeight());

    auto result = parseElement (*svgRoot, true, {});

    if (result)
        bounds = calculateBounds();

    return result;
}

//==============================================================================

void Drawable::clear()
{
    viewBox = { 0.0f, 0.0f, 0.0f, 0.0f };
    size = { 0.0f, 0.0f };
    bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    transform = AffineTransform::identity();

    elements.clear();
    elementsById.clear();
    gradients.clear();
    gradientsById.clear();
    clipPaths.clear();
    clipPathsById.clear();
}

//==============================================================================

Rectangle<float> Drawable::getBounds() const
{
    return bounds;
}

//==============================================================================

void Drawable::paint (Graphics& g)
{
    const auto savedState = g.saveState();

    g.setStrokeWidth (1.0f);
    g.setFillColor (Colors::black);

    if (! transform.isIdentity())
        g.addTransform (transform);

    for (const auto& element : elements)
        paintElement (g, *element, true, false);
}

void Drawable::paint (Graphics& g, const Rectangle<float>& targetArea, Fitting fitting, Justification justification)
{
    YUP_DRAWABLE_LOG ("Fitted paint called - bounds: " << bounds.toString() << " targetArea: " << targetArea.toString());

    if (bounds.isEmpty())
        return;

    const auto savedState = g.saveState();

    auto finalBounds = viewBox.isEmpty() ? bounds : viewBox;
    auto finalTransform = calculateTransformForTarget (finalBounds, targetArea, fitting, justification);
    if (! finalTransform.isIdentity())
        g.addTransform (finalTransform);

    g.setStrokeWidth (1.0f);
    g.setFillColor (Colors::black);

    for (const auto& element : elements)
        paintElement (g, *element, true, false);
}

//==============================================================================

void Drawable::paintElement (Graphics& g, const Element& element, bool hasParentFillEnabled, bool hasParentStrokeEnabled)
{
    const auto savedState = g.saveState();

    bool isFillDefined = hasParentFillEnabled;
    bool isStrokeDefined = hasParentStrokeEnabled;

    YUP_DRAWABLE_LOG ("paintElement called - hasPath: " << (element.path ? "true" : "false") << " hasTransform: " << (element.transform ? "true" : "false"));

    // Apply element transform if present - use proper composition for coordinate systems
    if (element.transform)
    {
        YUP_DRAWABLE_LOG ("Applying element transform - before: " << g.getTransform().toString() << " adding: " << element.transform->toString());
        // For proper coordinate system handling, we need to apply element transform
        // in the element's local space, then transform to viewport space
        g.setTransform (element.transform->followedBy (g.getTransform()));
        YUP_DRAWABLE_LOG ("After transform: " << g.getTransform().toString());
    }

    if (element.opacity)
        g.setOpacity (g.getOpacity() * (*element.opacity));

    // Apply clipping path if specified
    bool hasClipping = false;
    if (element.clipPathUrl)
    {
        if (auto clipPath = getClipPathById (*element.clipPathUrl))
        {
            // Create a combined path from all clip path elements
            Path combinedClipPath;
            for (const auto& clipElement : clipPath->elements)
            {
                if (clipElement->path)
                    combinedClipPath.appendPath (*clipElement->path);
            }

            if (! combinedClipPath.isEmpty())
            {
                g.setClipPath (combinedClipPath);
                hasClipping = true;
            }
        }
    }

    // Setup fill
    if (element.fillColor)
    {
        Color fillColor = *element.fillColor;
        if (element.fillOpacity)
            fillColor = fillColor.withMultipliedAlpha (*element.fillOpacity);
        g.setFillColor (fillColor);
        isFillDefined = true;
    }
    else if (element.fillUrl)
    {
        YUP_DRAWABLE_LOG ("Looking for gradient with ID: " << *element.fillUrl);
        if (auto gradient = getGradientById (*element.fillUrl))
        {
            YUP_DRAWABLE_LOG ("Found gradient, resolving references...");
            auto resolvedGradient = resolveGradient (gradient);
            std::optional<Rectangle<float>> gradientBounds;

            if (element.path)
                gradientBounds = element.path->getBounds();
            else if (element.reference)
            {
                if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
                    gradientBounds = refElement->path->getBounds();
            }

            ColorGradient colorGradient = createColorGradientFromSVG (*resolvedGradient,
                                                                      gradientBounds ? std::addressof (*gradientBounds) : nullptr);
            g.setFillColorGradient (colorGradient);
            isFillDefined = true;
            YUP_DRAWABLE_LOG ("Applied gradient to fill");
        }
        else
        {
            YUP_DRAWABLE_LOG ("Gradient not found for ID: " << *element.fillUrl);
        }
    }
    else if (hasParentFillEnabled)
    {
        // Inherit parent fill - don't change graphics state, just mark as defined
        isFillDefined = true;
    }

    if (isFillDefined && ! element.noFill)
    {
        if (element.path)
        {
            if (element.path->isClosed())
            {
                // TODO: Apply fill-rule when Graphics class supports it
                // if (element.fillRule)
                //     g.setFillRule (*element.fillRule == "evenodd" ? FillRule::EvenOdd : FillRule::NonZero);
                g.fillPath (*element.path);
            }
        }
        else if (element.reference)
        {
            if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
            {
                YUP_DRAWABLE_LOG ("Rendering use element - reference: " << *element.reference);
                YUP_DRAWABLE_LOG ("Use element transform: " << (element.transform ? element.transform->toString() : "none"));
                YUP_DRAWABLE_LOG ("Referenced element local transform: " << (refElement->localTransform ? refElement->localTransform->toString() : "none"));
                YUP_DRAWABLE_LOG ("Graphics transform during use fill: " << g.getTransform().toString());

                // For <use> elements, apply only the referenced element's own transform (not inherited parents)
                const auto savedTransform = g.getTransform();
                if (refElement->localTransform)
                    g.setTransform (refElement->localTransform->followedBy (savedTransform));

                if (refElement->path->isClosed())
                {
                    // TODO: Apply fill-rule when Graphics class supports it
                    // if (element.fillRule)
                    //     g.setFillRule (*element.fillRule == "evenodd" ? FillRule::EvenOdd : FillRule::NonZero);
                    g.fillPath (*refElement->path);
                }

                if (refElement->localTransform)
                    g.setTransform (savedTransform);
            }
        }
        else if (element.text && element.textPosition)
        {
            /*
            // Create StyledText for text rendering
            StyledText styledText;
            styledText.setText (*element.text);

            // Set font properties
            if (element.fontSize)
                styledText.setFontSize (*element.fontSize);
            else
                styledText.setFontSize (12.0f);

            if (element.fontFamily)
                styledText.setFontFamily (*element.fontFamily);

            // Set text alignment based on text-anchor
            if (element.textAnchor)
            {
                if (*element.textAnchor == "middle")
                    styledText.setHorizontalAlignment (StyledText::HorizontalAlignment::Center);
                else if (*element.textAnchor == "end")
                    styledText.setHorizontalAlignment (StyledText::HorizontalAlignment::Right);
                else
                    styledText.setHorizontalAlignment (StyledText::HorizontalAlignment::Left);
            }

            // Render text at specified position
            auto textBounds = styledText.getBounds();
            g.drawStyledText (styledText, element.textPosition->getX(), element.textPosition->getY() - textBounds.getHeight());
            */
        }
        else if (element.imageHref && element.imageBounds)
        {
            // TODO: Load and render image
            // For now, draw a placeholder rectangle
            g.setFillColor (Colors::lightgray);
            g.fillRect (*element.imageBounds);
            g.setStrokeColor (Colors::darkgray);
            g.setStrokeWidth (1.0f);
            g.strokeRect (*element.imageBounds);
        }
    }

    // Setup stroke
    if (element.strokeColor)
    {
        Color strokeColor = *element.strokeColor;
        if (element.strokeOpacity)
            strokeColor = strokeColor.withMultipliedAlpha (*element.strokeOpacity);
        g.setStrokeColor (strokeColor);
        isStrokeDefined = true;
    }
    else if (element.strokeUrl)
    {
        if (auto gradient = getGradientById (*element.strokeUrl))
        {
            auto resolvedGradient = resolveGradient (gradient);
            std::optional<Rectangle<float>> gradientBounds;

            if (element.path)
                gradientBounds = element.path->getBounds();
            else if (element.reference)
            {
                if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
                    gradientBounds = refElement->path->getBounds();
            }

            ColorGradient colorGradient = createColorGradientFromSVG (*resolvedGradient,
                                                                      gradientBounds ? std::addressof (*gradientBounds) : nullptr);
            g.setStrokeColorGradient (colorGradient);
            isStrokeDefined = true;
        }
    }
    else if (hasParentStrokeEnabled)
    {
        // Inherit parent stroke - don't change graphics state, just mark as defined
        isStrokeDefined = true;
    }

    if (element.strokeJoin)
        g.setStrokeJoin (*element.strokeJoin);

    if (element.strokeCap)
        g.setStrokeCap (*element.strokeCap);

    if (element.strokeWidth)
        g.setStrokeWidth (*element.strokeWidth);

    // Apply stroke dash patterns
    if (element.strokeDashArray)
    {
        // Convert Array<float> to what Graphics expects
        const auto& dashArray = *element.strokeDashArray;
        if (! dashArray.isEmpty())
        {
            // TODO: Graphics class needs stroke dash pattern support
            // For now, this is prepared for when Graphics supports it
            // g.setStrokeDashPattern (dashArray.getRawDataPointer(), dashArray.size());
            // if (element.strokeDashOffset)
            //     g.setStrokeDashOffset (*element.strokeDashOffset);
        }
    }

    if (isStrokeDefined && ! element.noStroke)
    {
        if (element.path)
        {
            g.strokePath (*element.path);
        }
        else if (element.reference)
        {
            if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
            {
                YUP_DRAWABLE_LOG ("Stroking use element - reference: " << *element.reference);
                YUP_DRAWABLE_LOG ("Graphics transform during stroke: " << g.getTransform().toString());

                // For <use> elements, apply only the referenced element's own transform (not inherited parents)
                const auto savedTransform = g.getTransform();
                if (refElement->localTransform)
                    g.setTransform (refElement->localTransform->followedBy (savedTransform));

                g.strokePath (*refElement->path);

                if (refElement->localTransform)
                    g.setTransform (savedTransform);
            }
        }
    }

    for (const auto& childElement : element.children)
    {
        YUP_DRAWABLE_LOG ("Rendering child element - current graphics transform: " << g.getTransform().toString());
        paintElement (g, *childElement, isFillDefined, isStrokeDefined);
    }

    // paintDebugElement (g, element);
}

//==============================================================================

bool Drawable::parseElement (const XmlElement& element, bool parentIsRoot, AffineTransform currentTransform, Element* parent)
{
    Element::Ptr e = new Element;
    bool isRootElement = element.hasTagName ("svg");

    if (auto id = element.getStringAttribute ("id"); id.isNotEmpty())
    {
        e->id = id;
        elementsById.set (id, e);
    }

    if (element.hasTagName ("path"))
    {
        auto path = Path();

        String pathData = element.getStringAttribute ("d");
        if (pathData.isEmpty() || ! path.fromString (pathData))
            return false;

        e->path = std::move (path);

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("g"))
    {
        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("use"))
    {
        String href = element.getStringAttribute ("href");
        if (href.isNotEmpty() && href.startsWith ("#"))
            e->reference = href.substring (1);

        // Handle x,y positioning for use elements (SVG spec requirement)
        auto x = element.getFloatAttribute ("x");
        auto y = element.getFloatAttribute ("y");
        AffineTransform useTransform;
        if (x != 0.0f || y != 0.0f)
            useTransform = AffineTransform::translation (x, y);

        currentTransform = parseTransform (element, currentTransform, *e);

        // Combine use element positioning with any explicit transform
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
        auto cx = element.getFloatAttribute ("cx");
        auto cy = element.getFloatAttribute ("cy");
        auto rx = element.getFloatAttribute ("rx");
        auto ry = element.getFloatAttribute ("ry");

        auto path = Path();
        path.addCenteredEllipse (cx, cy, rx, ry);
        e->path = std::move (path);

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("circle"))
    {
        auto cx = element.getFloatAttribute ("cx");
        auto cy = element.getFloatAttribute ("cy");
        auto r = element.getFloatAttribute ("r");

        auto path = Path();
        path.addCenteredEllipse (cx, cy, r, r);
        e->path = std::move (path);

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("rect"))
    {
        auto x = element.getFloatAttribute ("x");
        auto y = element.getFloatAttribute ("y");
        auto width = element.getFloatAttribute ("width");
        auto height = element.getFloatAttribute ("height");
        auto rx = element.getFloatAttribute ("rx");
        auto ry = element.getFloatAttribute ("ry");

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

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("line"))
    {
        auto x1 = element.getFloatAttribute ("x1");
        auto y1 = element.getFloatAttribute ("y1");
        auto x2 = element.getFloatAttribute ("x2");
        auto y2 = element.getFloatAttribute ("y2");

        auto path = Path();
        path.startNewSubPath (x1, y1);
        path.lineTo (x2, y2);
        e->path = std::move (path);

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
        }

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("text"))
    {
        float x = element.getFloatAttribute ("x");
        float y = element.getFloatAttribute ("y");
        e->textPosition = Point<float> (x, y);

        e->text = element.getAllSubText();

        String fontFamily = element.getStringAttribute ("font-family");
        if (fontFamily.isNotEmpty())
            e->fontFamily = fontFamily;

        float fontSize = element.getFloatAttribute ("font-size");
        if (fontSize > 0.0f)
            e->fontSize = fontSize;

        String textAnchor = element.getStringAttribute ("text-anchor");
        if (textAnchor.isNotEmpty())
            e->textAnchor = textAnchor;

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("image"))
    {
        auto x = element.getFloatAttribute ("x");
        auto y = element.getFloatAttribute ("y");
        auto width = element.getFloatAttribute ("width");
        auto height = element.getFloatAttribute ("height");

        e->imageBounds = Rectangle<float> (x, y, width, height);

        String href = element.getStringAttribute ("href");
        if (href.isEmpty())
            href = element.getStringAttribute ("xlink:href");

        if (href.isNotEmpty())
            e->imageHref = href;

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("defs"))
    {
        // Parse definitions like gradients and clip paths
        for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
        {
            if (child->hasTagName ("linearGradient") || child->hasTagName ("radialGradient"))
                parseGradient (*child);
            else if (child->hasTagName ("clipPath"))
                parseClipPath (*child);
        }
    }

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        // Parse gradients and clip paths regardless of whether they're in <defs> or not
        if (child->hasTagName ("linearGradient") || child->hasTagName ("radialGradient"))
            parseGradient (*child);
        else if (child->hasTagName ("clipPath"))
            parseClipPath (*child);
        else
            parseElement (*child, isRootElement, currentTransform, e.get());
    }

    if (isRootElement)
        return true;

    if (parent != nullptr && ! parentIsRoot)
        parent->children.push_back (std::move (e));
    else
        elements.push_back (std::move (e));

    return true;
}

//==============================================================================

void Drawable::parseStyle (const XmlElement& element, const AffineTransform& currentTransform, Element& e)
{
    // Parse CSS style attribute first
    String styleAttr = element.getStringAttribute ("style");
    if (styleAttr.isNotEmpty())
        parseCSSStyle (styleAttr, e);

    // Parse individual attributes (these override style attribute values)
    String fill = element.getStringAttribute ("fill");
    if (fill.isNotEmpty())
    {
        if (fill != "none")
        {
            String gradientUrl = extractGradientUrl (fill);
            if (gradientUrl.isNotEmpty())
                e.fillUrl = gradientUrl;
            else
            {
                e.fillColor = Color::fromString (fill);
                YUP_DRAWABLE_LOG ("Parsed fill color: " << fill << " -> " << e.fillColor->toString());
            }
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

    // Parse stroke-dasharray
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

    // Parse stroke-dashoffset
    String dashOffset = element.getStringAttribute ("stroke-dashoffset");
    if (dashOffset.isNotEmpty())
        e.strokeDashOffset = parseUnit (dashOffset);

    // Parse fill-opacity
    float fillOpacity = element.getFloatAttribute ("fill-opacity", -1.0f);
    if (fillOpacity >= 0.0f && fillOpacity <= 1.0f)
        e.fillOpacity = fillOpacity;

    // Parse stroke-opacity
    float strokeOpacity = element.getFloatAttribute ("stroke-opacity", -1.0f);
    if (strokeOpacity >= 0.0f && strokeOpacity <= 1.0f)
        e.strokeOpacity = strokeOpacity;

    // Parse fill-rule
    String fillRule = element.getStringAttribute ("fill-rule");
    if (fillRule == "evenodd" || fillRule == "nonzero")
        e.fillRule = fillRule;
}

//==============================================================================

AffineTransform Drawable::parseTransform (const XmlElement& element, const AffineTransform& currentTransform, Element& e)
{
    AffineTransform result;

    if (auto transformString = element.getStringAttribute ("transform"); transformString.isNotEmpty())
    {
        result = parseTransform (transformString);

        e.transform = result;
        e.localTransform = result; // Store the local transform separately for use by <use> elements

        YUP_DRAWABLE_LOG ("Parsed element transform: " << result.toString());
    }

    return currentTransform.followedBy (result);
}

//==============================================================================

AffineTransform Drawable::parseTransform (const String& transformString)
{
    if (transformString.isEmpty())
        return AffineTransform::identity();

    AffineTransform result;
    auto data = transformString.getCharPointer();

    while (! data.isEmpty())
    {
        // Skip whitespace
        while (data.isWhitespace())
            ++data;

        if (data.isEmpty())
            break;

        // Parse transform type
        String type;
        while (! data.isEmpty() && CharacterFunctions::isLetter (*data))
        {
            type += *data;
            ++data;
        }

        // Skip whitespace and the opening parenthesis
        while (data.isWhitespace() || *data == '(')
            ++data;

        // Parse parameters
        Array<float> params;
        while (! data.isEmpty() && *data != ')')
        {
            if (*data == ',' || *data == ' ')
            {
                ++data;
                continue;
            }

            String number;
            while (! data.isEmpty() && (*data == '-' || *data == '.' || *data == 'e' || (*data >= '0' && *data <= '9')))
            {
                number += *data;
                ++data;
            }

            if (! number.isEmpty())
                params.add (number.getFloatValue());

            // Skip whitespace or commas
            while (data.isWhitespace() || *data == ',')
                ++data;
        }

        // Skip the closing parenthesis
        if (*data == ')')
            ++data;

        // Apply the parsed transform
        if (type == "translate" && (params.size() == 1 || params.size() == 2))
        {
            const auto tx = params[0];
            const auto ty = (params.size() == 2) ? params[1] : 0.0f;
            result = result.translated (tx, ty);
        }
        else if (type == "scale" && (params.size() == 1 || params.size() == 2))
        {
            const auto sx = params[0];
            const auto sy = (params.size() == 2) ? params[1] : params[0];
            result = result.scaled (sx, sy);
        }
        else if (type == "rotate" && (params.size() == 1 || params.size() == 3))
        {
            if (params.size() == 1)
                result = result.rotated (degreesToRadians (params[0]));
            else
                result = result.rotated (degreesToRadians (params[0]), params[1], params[2]);
        }
        else if (type == "skewX" && params.size() == 1)
        {
            result = result.sheared (tanf (degreesToRadians (params[0])), 0.0f);
        }
        else if (type == "skewY" && params.size() == 1)
        {
            result = result.sheared (0.0f, tanf (degreesToRadians (params[0])));
        }
        else if (type == "matrix" && params.size() == 6)
        {
            result = result.followedBy (AffineTransform (
                params[0], params[2], params[4], params[1], params[3], params[5]));
        }
    }

    return result;
}

//==============================================================================

void Drawable::paintDebugElement (Graphics& g, const Element& element)
{
    if (! element.path)
        return;

    for (const auto& segment : *element.path)
    {
        auto color = Color::opaqueRandom();

        g.setFillColor (color);
        g.fillRect (segment.point.getX() - 4, segment.point.getY() - 4, 8, 8);

        g.setStrokeColor (Colors::white);
        g.setStrokeWidth (2.0f);
        g.strokeRect (segment.point.getX() - 4, segment.point.getY() - 4, 8, 8);

        if (segment.verb == Path::Verb::CubicTo)
        {
            g.setFillColor (color.brighter (0.05f));
            g.fillRect (segment.controlPoint1.getX() - 4, segment.controlPoint1.getY() - 4, 8, 8);

            g.setFillColor (color.brighter (0.1f));
            g.fillRect (segment.controlPoint2.getX() - 4, segment.controlPoint2.getY() - 4, 8, 8);
        }
    }
}

//==============================================================================

void Drawable::parseGradient (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
        return;

    YUP_DRAWABLE_LOG ("Parsing gradient with ID: " << id);

    Gradient::Ptr gradient = new Gradient;
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

    // Parse xlink:href reference
    String href = element.getStringAttribute ("xlink:href");
    if (href.isNotEmpty() && href.startsWith ("#"))
    {
        gradient->href = href.substring (1); // Remove the # prefix
        YUP_DRAWABLE_LOG ("Gradient references: " << gradient->href);
    }

    if (element.hasTagName ("linearGradient"))
    {
        gradient->type = Gradient::Linear;
        bool hasX1 = false, hasY1 = false, hasX2 = false, hasY2 = false;
        gradient->start = { parseCoordinate ("x1", gradient->start.getX(), hasX1), parseCoordinate ("y1", gradient->start.getY(), hasY1) };
        gradient->end = { parseCoordinate ("x2", gradient->end.getX(), hasX2), parseCoordinate ("y2", gradient->end.getY(), hasY2) };
        gradient->hasStart = hasX1 || hasY1;
        gradient->hasEnd = hasX2 || hasY2;

        YUP_DRAWABLE_LOG ("Linear gradient - start: (" << gradient->start.getX() << ", " << gradient->start.getY() << ") end: (" << gradient->end.getX() << ", " << gradient->end.getY() << ")");
    }
    else if (element.hasTagName ("radialGradient"))
    {
        gradient->type = Gradient::Radial;
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

        YUP_DRAWABLE_LOG ("Radial gradient - center: (" << gradient->center.getX() << ", " << gradient->center.getY() << ") radius: " << gradient->radius);
    }

    // Parse gradientUnits attribute
    String gradientUnits = element.getStringAttribute ("gradientUnits");
    if (gradientUnits == "userSpaceOnUse")
    {
        gradient->units = Gradient::UserSpaceOnUse;
        YUP_DRAWABLE_LOG ("Gradient units: userSpaceOnUse");
    }
    else
    {
        gradient->units = Gradient::ObjectBoundingBox;
        YUP_DRAWABLE_LOG ("Gradient units: objectBoundingBox (default)");
    }

    // Parse gradientTransform attribute
    String gradientTransform = element.getStringAttribute ("gradientTransform");
    if (gradientTransform.isNotEmpty())
    {
        YUP_DRAWABLE_LOG ("Parsing gradientTransform: " << gradientTransform);
        gradient->transform = parseTransform (gradientTransform);
        YUP_DRAWABLE_LOG ("Gradient transform: " << gradient->transform.toString());
    }

    // Parse gradient stops
    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->hasTagName ("stop"))
        {
            GradientStop stop;
            const auto offsetString = child->getStringAttribute ("offset");
            if (offsetString.containsChar ('%'))
                stop.offset = offsetString.upToFirstOccurrenceOf ("%", false, false).getFloatValue() * 0.01f;
            else
                stop.offset = child->getFloatAttribute ("offset");

            stop.offset = jlimit (0.0f, 1.0f, stop.offset);

            // First try to get stop-color from attributes
            String stopColor = child->getStringAttribute ("stop-color");
            float stopOpacity = child->getFloatAttribute ("stop-opacity", 1.0f);

            // If not found in attributes, parse from CSS style
            if (stopColor.isEmpty())
            {
                String styleAttr = child->getStringAttribute ("style");
                if (styleAttr.isNotEmpty())
                {
                    YUP_DRAWABLE_LOG ("Parsing CSS style for gradient stop: " << styleAttr);

                    // Parse CSS-style stop-color
                    auto declarations = StringArray::fromTokens (styleAttr, ";", "");
                    for (const auto& declaration : declarations)
                    {
                        auto colonPos = declaration.indexOf (":");
                        if (colonPos > 0)
                        {
                            String property = declaration.substring (0, colonPos).trim();
                            String value = declaration.substring (colonPos + 1).trim();

                            if (property == "stop-color")
                            {
                                stopColor = value;
                                YUP_DRAWABLE_LOG ("Found stop-color in CSS: " << stopColor);
                            }
                            else if (property == "stop-opacity")
                            {
                                stopOpacity = value.getFloatValue();
                                YUP_DRAWABLE_LOG ("Found stop-opacity in CSS: " << stopOpacity);
                            }
                        }
                    }
                }
            }

            if (stopColor.isNotEmpty())
            {
                YUP_DRAWABLE_LOG ("Parsing color string: '" << stopColor << "' (length: " << stopColor.length() << ")");
                stop.color = Color::fromString (stopColor);
                YUP_DRAWABLE_LOG ("Gradient stop - offset: " << stop.offset << " color: " << stopColor << " parsed: " << stop.color.toString());
            }

            stop.opacity = stopOpacity;

            gradient->stops.push_back (stop);
        }
    }

    if (! gradient->stops.empty())
    {
        std::sort (gradient->stops.begin(), gradient->stops.end(), [] (const GradientStop& a, const GradientStop& b)
        {
            return a.offset < b.offset;
        });

        // Ensure implicit first/last stops per SVG spec
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

    gradients.push_back (gradient);
    gradientsById.set (id, gradient);
}

//==============================================================================

Drawable::Gradient::Ptr Drawable::getGradientById (const String& id)
{
    return gradientsById[id];
}

//==============================================================================

Drawable::Gradient::Ptr Drawable::resolveGradient (Gradient::Ptr gradient)
{
    if (gradient == nullptr || gradient->href.isEmpty())
        return gradient;

    auto referencedGradient = getGradientById (gradient->href);
    if (referencedGradient == nullptr)
    {
        YUP_DRAWABLE_LOG ("Referenced gradient not found: " << gradient->href);
        return gradient;
    }

    // Recursively resolve the referenced gradient first
    referencedGradient = resolveGradient (referencedGradient);

    // Create a new gradient that inherits from the referenced gradient
    Gradient::Ptr resolvedGradient = new Gradient;

    // Copy properties from referenced gradient
    resolvedGradient->type = referencedGradient->type;
    resolvedGradient->id = gradient->id; // Keep the original ID
    resolvedGradient->units = referencedGradient->units;
    resolvedGradient->start = referencedGradient->start;
    resolvedGradient->end = referencedGradient->end;
    resolvedGradient->center = referencedGradient->center;
    resolvedGradient->radius = referencedGradient->radius;
    resolvedGradient->focal = referencedGradient->focal;
    resolvedGradient->transform = referencedGradient->transform;
    resolvedGradient->stops = referencedGradient->stops;

    // Override with properties from the current gradient (if specified)
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
    if (gradient->units != Gradient::ObjectBoundingBox) // Only override if explicitly set
        resolvedGradient->units = gradient->units;
    if (! gradient->stops.empty()) // Use local stops if defined
        resolvedGradient->stops = gradient->stops;

    YUP_DRAWABLE_LOG ("Resolved gradient " << gradient->id << " from reference " << gradient->href);
    return resolvedGradient;
}

//==============================================================================

ColorGradient Drawable::createColorGradientFromSVG (const Gradient& gradient, const Rectangle<float>* objectBounds)
{
    YUP_DRAWABLE_LOG ("Creating ColorGradient from SVG gradient ID: " << gradient.id << " type: " << (gradient.type == Gradient::Linear ? "Linear" : "Radial") << " units: " << (gradient.units == Gradient::UserSpaceOnUse ? "userSpaceOnUse" : "objectBoundingBox"));

    if (gradient.stops.empty())
    {
        YUP_DRAWABLE_LOG ("No stops in gradient, returning empty");
        return ColorGradient();
    }

    // Ensure we always have a valid bounds transform for objectBoundingBox gradients
    const bool hasBounds = objectBounds != nullptr && objectBounds->getWidth() > 0.0f && objectBounds->getHeight() > 0.0f;
    AffineTransform unitsTransform = AffineTransform::identity();

    if (gradient.units == Gradient::ObjectBoundingBox && hasBounds)
    {
        // Normalize gradient space to the element bounds (0..1 -> bounds)
        unitsTransform = AffineTransform::translation (objectBounds->getX(), objectBounds->getY())
                             .scaled (objectBounds->getWidth(), objectBounds->getHeight());
    }

    // Per SVG spec, gradientUnits are applied first, then gradientTransform
    const AffineTransform gradientSpaceTransform = unitsTransform.followedBy (gradient.transform);

    // Helper to apply transforms to a point
    auto transformPoint = [&gradientSpaceTransform] (Point<float> p)
    {
        float x = p.getX();
        float y = p.getY();
        if (! gradientSpaceTransform.isIdentity())
            gradientSpaceTransform.transformPoint (x, y);
        return Point<float> (x, y);
    };

    // Prepare start/end/center after transforms (in local space; Graphics will apply viewport transforms)
    const Point<float> start = transformPoint (gradient.start);
    const Point<float> end = transformPoint (gradient.end);
    const Point<float> center = transformPoint (gradient.center);

    // Compute radial radius in transformed space
    auto computeRadius = [&]() -> float
    {
        if (gradient.radius <= 0.0f)
            return 0.0f;

        const auto edgePoint = transformPoint (Point<float> (gradient.center.getX() + gradient.radius, gradient.center.getY()));
        return Line<float> (center, edgePoint).length();
    };

    const auto radius = gradient.type == Gradient::Radial ? computeRadius() : 0.0f;

    // Build color stops in gradient space
    std::vector<ColorGradient::ColorStop> colorStops;
    colorStops.reserve (gradient.stops.size());

    for (const auto& stop : gradient.stops)
    {
        Color color = stop.color.withMultipliedAlpha (stop.opacity);

        if (gradient.type == Gradient::Linear)
        {
            const auto interpolated = Point<float> (start.getX() + stop.offset * (end.getX() - start.getX()),
                                                    start.getY() + stop.offset * (end.getY() - start.getY()));

            colorStops.emplace_back (color, interpolated, stop.offset);
        }
        else
        {
            // Radial gradient: position lies along the radius vector to preserve radius computation
            const auto radialPoint = Point<float> (center.getX() + radius * stop.offset, center.getY());
            colorStops.emplace_back (color, radialPoint, stop.offset);
        }
    }

    ColorGradient::Type type = (gradient.type == Gradient::Linear) ? ColorGradient::Linear : ColorGradient::Radial;
    ColorGradient result (type, colorStops);

    YUP_DRAWABLE_LOG ("Created ColorGradient with " << colorStops.size() << " stops");
    return result;
}

//==============================================================================

void Drawable::parseClipPath (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
        return;

    ClipPath::Ptr clipPath = new ClipPath;
    clipPath->id = id;

    // Parse child elements that make up the clipping path
    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        Element::Ptr clipElement = new Element;

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

            auto path = Path();
            path.addRectangle (x, y, width, height);
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

        if (clipElement->path)
            clipPath->elements.push_back (clipElement);
    }

    clipPaths.push_back (clipPath);
    clipPathsById.set (id, clipPath);
}

//==============================================================================

Drawable::ClipPath::Ptr Drawable::getClipPathById (const String& id)
{
    return clipPathsById[id];
}

//==============================================================================

void Drawable::parseCSSStyle (const String& styleString, Element& e)
{
    // Parse CSS style declarations separated by semicolons
    auto declarations = StringArray::fromTokens (styleString, ";", "");

    for (const auto& declaration : declarations)
    {
        auto colonPos = declaration.indexOf (":");
        if (colonPos > 0)
        {
            String property = declaration.substring (0, colonPos).trim();
            String value = declaration.substring (colonPos + 1).trim();

            if (property == "fill")
            {
                if (value != "none")
                {
                    String gradientUrl = extractGradientUrl (value);
                    if (gradientUrl.isNotEmpty())
                        e.fillUrl = gradientUrl;
                    else
                        e.fillColor = Color::fromString (value);
                }
                else
                {
                    e.noFill = true;
                }
            }
            else if (property == "stroke")
            {
                if (value != "none")
                {
                    String gradientUrl = extractGradientUrl (value);
                    if (gradientUrl.isNotEmpty())
                        e.strokeUrl = gradientUrl;
                    else
                        e.strokeColor = Color::fromString (value);
                }
                else
                {
                    e.noStroke = true;
                }
            }
            else if (property == "stroke-width")
            {
                float strokeWidth = value.getFloatValue();
                if (strokeWidth > 0.0f)
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
            else if (property == "font-family")
            {
                e.fontFamily = value;
            }
            else if (property == "font-size")
            {
                float fontSize = value.getFloatValue();
                if (fontSize > 0.0f)
                    e.fontSize = fontSize;
            }
            else if (property == "text-anchor")
            {
                e.textAnchor = value;
            }
            else if (property == "clip-path")
            {
                String clipPathUrl = extractGradientUrl (value);
                if (clipPathUrl.isNotEmpty())
                    e.clipPathUrl = clipPathUrl;
            }
            else if (property == "stroke-dasharray")
            {
                if (value != "none")
                {
                    auto dashValues = StringArray::fromTokens (value, " ,", "");
                    if (! dashValues.isEmpty())
                    {
                        Array<float> dashes;
                        for (const auto& dash : dashValues)
                        {
                            float dashValue = parseUnit (dash);
                            if (dashValue >= 0.0f)
                                dashes.add (dashValue);
                        }

                        if (! dashes.isEmpty())
                            e.strokeDashArray = dashes;
                    }
                }
            }
            else if (property == "stroke-dashoffset")
            {
                e.strokeDashOffset = parseUnit (value);
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
        }
    }
}

//==============================================================================

float Drawable::parseUnit (const String& value, float defaultValue, float fontSize, float viewportSize)
{
    if (value.isEmpty())
        return defaultValue;

    String trimmed = value.trim();
    if (trimmed.isEmpty())
        return defaultValue;

    // Extract numeric part and unit
    int unitStart = 0;
    while (unitStart < trimmed.length() && (CharacterFunctions::isDigit (trimmed[unitStart]) || trimmed[unitStart] == '.' || trimmed[unitStart] == '-' || trimmed[unitStart] == '+'))
    {
        unitStart++;
    }

    float numericValue = trimmed.substring (0, unitStart).getFloatValue();
    String unit = trimmed.substring (unitStart).trim().toLowerCase();

    // Handle different SVG units
    if (unit.isEmpty() || unit == "px")
        return numericValue; // Default user units or pixels

    else if (unit == "pt")
        return numericValue * 1.333333f; // 1pt = 1.333px

    else if (unit == "pc")
        return numericValue * 16.0f; // 1pc = 16px

    else if (unit == "mm")
        return numericValue * 3.779528f; // 1mm = 3.779528px (96 DPI)

    else if (unit == "cm")
        return numericValue * 37.79528f; // 1cm = 37.79528px (96 DPI)

    else if (unit == "in")
        return numericValue * 96.0f; // 1in = 96px (96 DPI)

    else if (unit == "em")
        return numericValue * fontSize; // Relative to font size

    else if (unit == "ex")
        return numericValue * fontSize * 0.5f; // Approximately 0.5em

    else if (unit == "%")
        return numericValue * viewportSize * 0.01f; // Percentage of viewport

    else
        return numericValue; // Unknown unit, treat as user units
}

//==============================================================================

Rectangle<float> Drawable::calculateBounds() const
{
    // Use viewBox if available, otherwise use size
    if (! viewBox.isEmpty())
        return viewBox;

    if (size.getWidth() > 0 && size.getHeight() > 0)
        return Rectangle<float> (0, 0, size.getWidth(), size.getHeight());

    // Fallback: calculate bounds from all elements with their transforms applied
    // This gives us the actual visual bounds of the rendered content
    Rectangle<float> bounds;
    bool hasValidBounds = false;

    for (const auto& element : elements)
    {
        if (element->path)
        {
            auto pathBounds = element->path->getBounds();
            if (element->transform)
                pathBounds = element->path->getBoundsTransformed (*element->transform);

            if (hasValidBounds)
                bounds = bounds.unionWith (pathBounds);
            else
            {
                bounds = pathBounds;
                hasValidBounds = true;
            }
        }
    }

    return hasValidBounds ? bounds : Rectangle<float> (0, 0, 100, 100);
}

//==============================================================================

AffineTransform Drawable::calculateTransformForTarget (const Rectangle<float>& sourceBounds, const Rectangle<float>& targetArea, Fitting fitting, Justification justification) const
{
    if (sourceBounds.isEmpty() || targetArea.isEmpty())
        return AffineTransform::identity();

    float scaleX = targetArea.getWidth() / sourceBounds.getWidth();
    float scaleY = targetArea.getHeight() / sourceBounds.getHeight();

    // Apply scaling based on fitting mode
    switch (fitting)
    {
        case Fitting::none:
            scaleX = scaleY = 1.0f;
            break;

        case Fitting::scaleToFit:
            scaleX = scaleY = jmin (scaleX, scaleY); // Scale to fit both dimensions
            break;

        case Fitting::fitWidth:
            scaleY = scaleX; // Scale to fit width, preserve aspect ratio
            break;

        case Fitting::fitHeight:
            scaleX = scaleY; // Scale to fit height, preserve aspect ratio
            break;

        case Fitting::scaleToFill:
        case Fitting::centerCrop:
            scaleX = scaleY = jmax (scaleX, scaleY); // Scale to fill, may crop
            break;

        case Fitting::fill:
            // Use calculated scales as-is (non-uniform scaling)
            break;

        case Fitting::centerInside:
            // Like scaleToFit but don't upscale beyond original size
            scaleX = scaleY = jmin (1.0f, jmin (scaleX, scaleY));
            break;

        case Fitting::stretchWidth:
            scaleY = 1.0f; // Stretch horizontally only
            break;

        case Fitting::stretchHeight:
            scaleX = 1.0f; // Stretch vertically only
            break;

        case Fitting::tile:
            // For tile mode, use no scaling (tiling would be handled elsewhere)
            scaleX = scaleY = 1.0f;
            break;
    }

    // Calculate scaled size
    float scaledWidth = sourceBounds.getWidth() * scaleX;
    float scaledHeight = sourceBounds.getHeight() * scaleY;

    // Calculate offset based on justification
    float offsetX = targetArea.getX();
    float offsetY = targetArea.getY();

    // Horizontal justification
    if (justification.testFlags (Justification::horizontalCenter))
        offsetX += (targetArea.getWidth() - scaledWidth) * 0.5f;
    else if (justification.testFlags (Justification::right))
        offsetX += targetArea.getWidth() - scaledWidth;

    // Vertical justification
    if (justification.testFlags (Justification::verticalCenter))
        offsetY += (targetArea.getHeight() - scaledHeight) * 0.5f;
    else if (justification.testFlags (Justification::bottom))
        offsetY += targetArea.getHeight() - scaledHeight;

    // Create transform: translate to origin, scale, then translate to target position
    return AffineTransform::translation (-sourceBounds.getX(), -sourceBounds.getY())
        .scaled (scaleX, scaleY)
        .translated (offsetX, offsetY);
}

//==============================================================================

Fitting Drawable::parsePreserveAspectRatio (const String& preserveAspectRatio)
{
    if (preserveAspectRatio.isEmpty() || preserveAspectRatio == "xMidYMid meet")
        return Fitting::scaleToFit; // Default SVG behavior

    if (preserveAspectRatio.contains ("none"))
        return Fitting::fill; // Non-uniform scaling allowed

    if (preserveAspectRatio.contains ("slice"))
        return Fitting::scaleToFill; // Scale to fill, may crop

    // Default to uniform scaling (meet)
    return Fitting::scaleToFit;
}

Justification Drawable::parseAspectRatioAlignment (const String& preserveAspectRatio)
{
    if (preserveAspectRatio.isEmpty())
        return Justification::center; // Default SVG alignment

    Justification result = Justification::left;

    // Parse horizontal alignment
    if (preserveAspectRatio.contains ("xMin"))
        result = result | Justification::left;
    else if (preserveAspectRatio.contains ("xMax"))
        result = result | Justification::right;
    else // xMid (default)
        result = result | Justification::horizontalCenter;

    // Parse vertical alignment
    if (preserveAspectRatio.contains ("YMin"))
        result = result | Justification::top;
    else if (preserveAspectRatio.contains ("YMax"))
        result = result | Justification::bottom;
    else // YMid (default)
        result = result | Justification::verticalCenter;

    return result;
}

//==============================================================================

String Drawable::extractGradientUrl (const String& value)
{
    if (! value.contains ("url(#"))
        return String();

    // Find the start of the URL
    int urlStart = value.indexOf ("url(#");
    if (urlStart == -1)
        return String();

    // Find the end of the URL (first closing parenthesis after the URL start)
    int urlEnd = value.indexOf (urlStart, ")");
    if (urlEnd == -1)
        return String();

    // Extract the ID part (between "url(#" and ")")
    String url = value.substring (urlStart + 5, urlEnd); // +5 to skip "url(#"
    YUP_DRAWABLE_LOG ("Extracted gradient URL: '" << url << "' from: '" << value << "'");
    return url;
}

} // namespace yup
