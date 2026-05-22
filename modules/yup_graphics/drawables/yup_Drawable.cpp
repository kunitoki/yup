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

namespace
{
SVGGradient::Ptr getGradientById (const SVGData& data, const String& id)
{
    return data.gradientsById[id];
}

SVGGradient::Ptr resolveGradient (const SVGData& data, SVGGradient::Ptr gradient)
{
    if (gradient == nullptr || gradient->href.isEmpty())
        return gradient;

    auto referencedGradient = getGradientById (data, gradient->href);
    if (referencedGradient == nullptr)
        return gradient;

    referencedGradient = resolveGradient (data, referencedGradient);

    SVGGradient::Ptr resolved = new SVGGradient;
    resolved->type = gradient->type;
    resolved->id = gradient->id;
    resolved->units = referencedGradient->units;
    resolved->spreadMethod = referencedGradient->spreadMethod;
    resolved->start = referencedGradient->start;
    resolved->end = referencedGradient->end;
    resolved->center = referencedGradient->center;
    resolved->radius = referencedGradient->radius;
    resolved->focal = referencedGradient->focal;
    resolved->transform = referencedGradient->transform;
    resolved->stops = referencedGradient->stops;
    resolved->hasUnits = referencedGradient->hasUnits;
    resolved->hasSpreadMethod = referencedGradient->hasSpreadMethod;

    if (gradient->hasStart)
        resolved->start = gradient->start;
    if (gradient->hasEnd)
        resolved->end = gradient->end;
    if (gradient->hasCenter)
        resolved->center = gradient->center;
    if (gradient->hasRadius)
        resolved->radius = gradient->radius;
    if (gradient->hasFocal)
        resolved->focal = gradient->focal;

    if (! gradient->transform.isIdentity())
        resolved->transform = gradient->transform;
    if (gradient->hasUnits)
    {
        resolved->units = gradient->units;
        resolved->hasUnits = true;
    }
    if (gradient->hasSpreadMethod)
    {
        resolved->spreadMethod = gradient->spreadMethod;
        resolved->hasSpreadMethod = true;
    }
    if (! gradient->stops.empty())
        resolved->stops = gradient->stops;

    return resolved;
}

SVGFilter::Ptr getFilterById (const SVGData& data, const String& id)
{
    return data.filtersById[id];
}

SVGFilter::Ptr resolveFilter (const SVGData& data, SVGFilter::Ptr filter)
{
    if (filter == nullptr || filter->href.isEmpty())
        return filter;

    auto referencedFilter = resolveFilter (data, getFilterById (data, filter->href));
    if (referencedFilter == nullptr)
        return filter;

    SVGFilter::Ptr resolved = new SVGFilter;
    resolved->id = filter->id;
    resolved->href = filter->href;
    resolved->gaussianBlurStdDeviation = filter->gaussianBlurStdDeviation
                                           ? filter->gaussianBlurStdDeviation
                                           : referencedFilter->gaussianBlurStdDeviation;
    return resolved;
}

SVGClipPath::Ptr getClipPathById (const SVGData& data, const String& id)
{
    return data.clipPathsById[id];
}
} // namespace

//==============================================================================

Drawable::Drawable()
{
}

//==============================================================================

bool Drawable::parseSVG (const File& svgFile)
{
    YUP_DRAWABLE_LOG ("parseSVG(file) - file: " << svgFile.getFullPathName());

    ParseOptions options;
    options.baseDirectory = svgFile.getParentDirectory();

    return parseSVG (svgFile, options);
}

bool Drawable::parseSVG (StringRef svgText)
{
    YUP_DRAWABLE_LOG ("parseSVG(text) - length: " << String (svgText.text).length());

    return parseSVG (svgText, ParseOptions());
}

bool Drawable::parseSVG (const File& svgFile, const ParseOptions& options)
{
    YUP_DRAWABLE_LOG ("parseSVG(file, options) - file: " << svgFile.getFullPathName());

    document = SVGParser::parse (svgFile, options);
    return document != nullptr;
}

bool Drawable::parseSVG (StringRef svgText, const ParseOptions& options)
{
    YUP_DRAWABLE_LOG ("parseSVG(text, options) - length: " << String (svgText.text).length());

    document = SVGParser::parse (svgText, options);
    return document != nullptr;
}

//==============================================================================

void Drawable::clear()
{
    document = nullptr;
}

//==============================================================================

Rectangle<float> Drawable::getBounds() const
{
    if (document == nullptr)
        return {};

    return document->getBounds();
}

//==============================================================================

void Drawable::paint (Graphics& g)
{
    if (document == nullptr)
        return;

    document->visit ([&] (const SVGData& data)
    {
        YUP_DRAWABLE_LOG ("paint - bounds: " << data.bounds.toString()
                                             << " topLevelElements: " << data.elements.size()
                                             << " rootHasFill: " << (data.rootHasFill ? "true" : "false")
                                             << " rootHasStroke: " << (data.rootHasStroke ? "true" : "false"));

        const auto savedState = g.saveState();

        g.setStrokeWidth (1.0f);

        if (data.rootFillColor)
            g.setFillColor (*data.rootFillColor);
        else
            g.setFillColor (Colors::black);

        if (data.rootStrokeColor)
            g.setStrokeColor (*data.rootStrokeColor);

        if (! data.transform.isIdentity())
            g.addTransform (data.transform);

        for (const auto& element : data.elements)
            paintElement (g, data, *element, data.rootHasFill, data.rootHasStroke, data.rootFillColor.value_or (Colors::black));
    });
}

void Drawable::paint (Graphics& g, const Rectangle<float>& targetArea, Fitting fitting, Justification justification)
{
    if (document == nullptr)
        return;

    document->visit ([&] (const SVGData& data)
    {
        YUP_DRAWABLE_LOG ("Fitted paint called - bounds: " << data.bounds.toString() << " targetArea: " << targetArea.toString());

        if (data.bounds.isEmpty())
        {
            YUP_DRAWABLE_LOG ("Fitted paint skipped - drawable bounds are empty");
            return;
        }

        const auto savedState = g.saveState();

        auto finalBounds = data.viewBox.isEmpty() ? data.bounds : data.viewBox;
        auto finalTransform = calculateTransformForTarget (finalBounds, targetArea, fitting, justification);

        if (! finalTransform.isIdentity())
            g.addTransform (finalTransform);

        g.setStrokeWidth (1.0f);

        if (data.rootFillColor)
            g.setFillColor (*data.rootFillColor);
        else
            g.setFillColor (Colors::black);

        if (data.rootStrokeColor)
            g.setStrokeColor (*data.rootStrokeColor);

        for (const auto& element : data.elements)
            paintElement (g, data, *element, data.rootHasFill, data.rootHasStroke, data.rootFillColor.value_or (Colors::black));
    });
}

//==============================================================================

void Drawable::paintElement (Graphics& g, const SVGData& data, const SVGElement& element, bool hasParentFillEnabled, bool hasParentStrokeEnabled, Color currentColor, int recursionDepth)
{
    if (element.hidden)
    {
        YUP_DRAWABLE_LOG ("paintElement skipped - hidden tag: " << element.tagName
                                                                << " id: " << (element.id ? *element.id : "none")
                                                                << " depth: " << recursionDepth);
        return;
    }

    if (recursionDepth > 64)
    {
        YUP_DRAWABLE_LOG ("paintElement skipped - recursion limit tag: " << element.tagName
                                                                         << " id: " << (element.id ? *element.id : "none")
                                                                         << " depth: " << recursionDepth);
        return;
    }

    const auto savedState = g.saveState();

    bool isFillDefined = hasParentFillEnabled;
    bool isStrokeDefined = hasParentStrokeEnabled;
    float filterFeather = 0.0f;
    if (element.color)
        currentColor = *element.color;

    if (element.transform)
        g.setTransform (element.transform->followedBy (g.getTransform()));

    if (element.opacity)
        g.setOpacity (g.getOpacity() * (*element.opacity));

    if (element.filterUrl)
    {
        if (auto filter = resolveFilter (data, getFilterById (data, *element.filterUrl)))
        {
            if (filter->gaussianBlurStdDeviation)
            {
                const auto svgStdDeviationToFeather = 2.0f;
                filterFeather = *filter->gaussianBlurStdDeviation * svgStdDeviationToFeather;
                g.setFeather (jmax (g.getFeather(), filterFeather));
            }
        }
    }

    if (element.viewBox && element.viewportSize)
    {
        Rectangle<float> viewport (0.0f, 0.0f, element.viewportSize->getWidth(), element.viewportSize->getHeight());
        auto viewBoxTransform = calculateTransformForTarget (*element.viewBox, viewport, element.preserveAspectRatioFitting, element.preserveAspectRatioJustification);
        if (! viewBoxTransform.isIdentity())
            g.addTransform (viewBoxTransform);
    }

    bool hasClipping = false;
    if (element.clipPathUrl)
    {
        if (auto clipPath = getClipPathById (data, *element.clipPathUrl))
        {
            std::optional<Rectangle<float>> clipObjectBounds;

            if (clipPath->units == SVGClipPath::ObjectBoundingBox)
            {
                if (element.path)
                    clipObjectBounds = element.path->getBounds();
                else if (element.reference)
                {
                    if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
                        clipObjectBounds = refElement->path->getBounds();
                }
                else if (element.imageBounds)
                {
                    clipObjectBounds = *element.imageBounds;
                }
            }

            Path combinedClipPath;
            bool clipUsesNonZeroWinding = true;

            for (const auto& clipElement : clipPath->elements)
            {
                if (clipElement->path)
                {
                    if (! clipElement->path->isUsingNonZeroWinding())
                        clipUsesNonZeroWinding = false;

                    AffineTransform clipTransform = clipElement->transform.value_or (AffineTransform::identity());

                    if (clipPath->units == SVGClipPath::ObjectBoundingBox)
                    {
                        if (! clipObjectBounds || clipObjectBounds->isEmpty())
                            continue;

                        auto unitsTransform = AffineTransform::translation (clipObjectBounds->getX(), clipObjectBounds->getY())
                                                  .scaled (clipObjectBounds->getWidth(), clipObjectBounds->getHeight());
                        clipTransform = clipTransform.followedBy (unitsTransform);
                    }

                    if (! clipTransform.isIdentity())
                        combinedClipPath.appendPath (*clipElement->path, clipTransform);
                    else
                        combinedClipPath.appendPath (*clipElement->path);
                }
            }

            if (! combinedClipPath.isEmpty())
            {
                combinedClipPath.setUsingNonZeroWinding (clipUsesNonZeroWinding);

                auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
                auto transformedClipPath = combinedClipPath.transformed (clipTransform);

                const auto savedClipTransform = g.getTransform();
                g.setTransform (AffineTransform::identity());
                g.setClipPath (transformedClipPath);
                g.setTransform (savedClipTransform);

                hasClipping = true;
            }
        }
    }

    // Fill setup
    if (element.fillColor)
    {
        Color fillColor = *element.fillColor;
        if (element.fillOpacity)
            fillColor = fillColor.withMultipliedAlpha (*element.fillOpacity);
        g.setFillColor (fillColor);
        isFillDefined = true;
    }
    else if (element.fillCurrentColor)
    {
        Color fillColor = currentColor;
        if (element.fillOpacity)
            fillColor = fillColor.withMultipliedAlpha (*element.fillOpacity);
        g.setFillColor (fillColor);
        isFillDefined = true;
    }
    else if (element.fillUrl)
    {
        if (auto gradient = getGradientById (data, *element.fillUrl))
        {
            auto resolvedGradient = resolveGradient (data, gradient);
            std::optional<Rectangle<float>> gradientBounds;

            if (element.path)
                gradientBounds = element.path->getBounds();
            else if (element.reference)
            {
                if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
                    gradientBounds = refElement->path->getBounds();
            }

            ColorGradient colorGradient = createColorGradientFromSVG (*resolvedGradient,
                                                                      gradientBounds ? std::addressof (*gradientBounds) : nullptr);
            g.setFillColorGradient (colorGradient);
            isFillDefined = true;
        }
    }
    else if (hasParentFillEnabled)
    {
        isFillDefined = true;
    }

    if (isFillDefined && ! element.noFill)
    {
        if (element.path)
        {
            YUP_DRAWABLE_LOG ("Filling path - tag: " << element.tagName
                                                     << " id: " << (element.id ? *element.id : "none")
                                                     << " bounds: " << element.path->getBounds().toString()
                                                     << " clip: " << (hasClipping ? "true" : "false"));
            g.fillPath (*element.path);
        }
        else if (element.reference)
        {
            if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
            {
                const bool useDefinesFill = element.fillColor || element.fillCurrentColor || element.fillUrl || element.noFill;
                if (useDefinesFill || ! refElement->noFill)
                {
                    const auto savedReferenceState = g.saveState();

                    if (! useDefinesFill)
                    {
                        if (refElement->fillColor)
                        {
                            Color fillColor = *refElement->fillColor;
                            if (refElement->fillOpacity)
                                fillColor = fillColor.withMultipliedAlpha (*refElement->fillOpacity);
                            g.setFillColor (fillColor);
                        }
                        else if (refElement->fillCurrentColor)
                        {
                            Color fillColor = currentColor;
                            if (refElement->fillOpacity)
                                fillColor = fillColor.withMultipliedAlpha (*refElement->fillOpacity);
                            g.setFillColor (fillColor);
                        }
                    }

                    const auto savedTransform = g.getTransform();
                    if (refElement->localTransform)
                        g.setTransform (refElement->localTransform->followedBy (savedTransform));

                    g.fillPath (*refElement->path);

                    if (refElement->localTransform)
                        g.setTransform (savedTransform);
                }
            }
        }
        else if (element.text && element.textPosition)
        {
            renderTextElement (g, element);
        }
        else if ((element.imageHref || element.image) && element.imageBounds)
        {
            renderImageElement (g, element);
        }
    }

    // Stroke setup
    if (element.strokeColor)
    {
        Color strokeColor = *element.strokeColor;
        if (element.strokeOpacity)
            strokeColor = strokeColor.withMultipliedAlpha (*element.strokeOpacity);
        g.setStrokeColor (strokeColor);
        isStrokeDefined = true;
    }
    else if (element.strokeCurrentColor)
    {
        Color strokeColor = currentColor;
        if (element.strokeOpacity)
            strokeColor = strokeColor.withMultipliedAlpha (*element.strokeOpacity);
        g.setStrokeColor (strokeColor);
        isStrokeDefined = true;
    }
    else if (element.strokeUrl)
    {
        if (auto gradient = getGradientById (data, *element.strokeUrl))
        {
            auto resolvedGradient = resolveGradient (data, gradient);
            std::optional<Rectangle<float>> gradientBounds;

            if (element.path)
                gradientBounds = element.path->getBounds();
            else if (element.reference)
            {
                if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
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
        isStrokeDefined = true;
    }

    if (element.strokeJoin)
        g.setStrokeJoin (*element.strokeJoin);
    if (element.strokeCap)
        g.setStrokeCap (*element.strokeCap);
    if (element.strokeWidth)
        g.setStrokeWidth (*element.strokeWidth);

    bool referenceDefinesStroke = false;
    if (! isStrokeDefined && element.reference)
    {
        if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr)
            referenceDefinesStroke = (refElement->strokeColor || refElement->strokeCurrentColor || refElement->strokeUrl) && ! refElement->noStroke;
    }

    if ((isStrokeDefined || referenceDefinesStroke) && ! element.noStroke)
    {
        const Path* pathToStroke = element.path ? std::addressof (*element.path) : nullptr;
        std::optional<Path> dashedPath;

        if (pathToStroke != nullptr && element.strokeDashArray && ! element.strokeDashArray->isEmpty())
        {
            dashedPath = createDashedPath (*pathToStroke, *element.strokeDashArray, element.strokeDashOffset.value_or (0.0f));
            pathToStroke = std::addressof (*dashedPath);
        }

        if (pathToStroke != nullptr)
        {
            YUP_DRAWABLE_LOG ("Stroking path - tag: " << element.tagName
                                                      << " id: " << (element.id ? *element.id : "none")
                                                      << " bounds: " << pathToStroke->getBounds().toString());

            if (filterFeather > 0.0f && ! (isFillDefined && ! element.noFill))
                renderSoftStrokeElement (g, *pathToStroke, filterFeather);
            else
                g.strokePath (*pathToStroke);
        }
        else if (element.reference)
        {
            if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
            {
                const bool useDefinesStroke = element.strokeColor || element.strokeCurrentColor || element.strokeUrl || element.noStroke;
                const auto savedReferenceState = g.saveState();

                if (! useDefinesStroke)
                {
                    if (refElement->strokeColor)
                    {
                        Color strokeColor = *refElement->strokeColor;
                        if (refElement->strokeOpacity)
                            strokeColor = strokeColor.withMultipliedAlpha (*refElement->strokeOpacity);
                        g.setStrokeColor (strokeColor);
                    }
                    else if (refElement->strokeCurrentColor)
                    {
                        Color strokeColor = currentColor;
                        if (refElement->strokeOpacity)
                            strokeColor = strokeColor.withMultipliedAlpha (*refElement->strokeOpacity);
                        g.setStrokeColor (strokeColor);
                    }

                    if (refElement->strokeWidth)
                        g.setStrokeWidth (*refElement->strokeWidth);
                    if (refElement->strokeJoin)
                        g.setStrokeJoin (*refElement->strokeJoin);
                    if (refElement->strokeCap)
                        g.setStrokeCap (*refElement->strokeCap);
                }

                const auto savedTransform = g.getTransform();
                if (refElement->localTransform)
                    g.setTransform (refElement->localTransform->followedBy (savedTransform));

                g.strokePath (*refElement->path);

                if (refElement->localTransform)
                    g.setTransform (savedTransform);
            }
        }
    }

    if (element.reference)
    {
        if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && ! refElement->children.empty())
        {
            const auto savedTransform = g.getTransform();
            if (refElement->localTransform)
                g.setTransform (refElement->localTransform->followedBy (savedTransform));

            if (refElement->viewBox)
            {
                auto viewportSizeToUse = element.viewportSize.value_or (refElement->viewportSize.value_or (Size<float> (refElement->viewBox->getWidth(), refElement->viewBox->getHeight())));
                Rectangle<float> viewport (0.0f, 0.0f, viewportSizeToUse.getWidth(), viewportSizeToUse.getHeight());
                auto viewBoxTransform = calculateTransformForTarget (*refElement->viewBox, viewport, refElement->preserveAspectRatioFitting, refElement->preserveAspectRatioJustification);
                if (! viewBoxTransform.isIdentity())
                    g.addTransform (viewBoxTransform);
            }

            for (const auto& childElement : refElement->children)
                paintElement (g, data, *childElement, isFillDefined && ! element.noFill, isStrokeDefined && ! element.noStroke, currentColor, recursionDepth + 1);

            g.setTransform (savedTransform);
        }
    }

    for (const auto& childElement : element.children)
        paintElement (g, data, *childElement, isFillDefined && ! element.noFill, isStrokeDefined && ! element.noStroke, currentColor, recursionDepth + 1);

    // paintDebugElement (g, element);
}

//==============================================================================

void Drawable::paintDebugElement (Graphics& g, const SVGElement& element)
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

Font Drawable::resolveFont (const SVGElement& element) const
{
    if (document == nullptr)
        return Font().withHeight (element.fontSize.value_or (12.0f));

    const auto& options = document->getParseOptions();
    const auto fontSize = element.fontSize.value_or (12.0f);
    const auto fontWeight = element.fontWeight.value_or (400);
    const auto fontItalic = element.fontItalic.value_or (false);

    if (options.fontResolver)
    {
        if (auto resolved = options.fontResolver (element.fontFamily.value_or (String()), fontSize, fontWeight, fontItalic))
            return resolved->withHeight (fontSize);
    }

    return Font().withHeight (fontSize);
}

//==============================================================================

void Drawable::renderTextElement (Graphics& g, const SVGElement& element)
{
    if (! element.text || ! element.textPosition || element.text->isEmpty())
        return;

    auto position = *element.textPosition;

    if (element.textX && ! element.textX->isEmpty())
        position.setX (element.textX->getFirst());
    if (element.textY && ! element.textY->isEmpty())
        position.setY (element.textY->getFirst());
    if (element.textDx && ! element.textDx->isEmpty())
        position.setX (position.getX() + element.textDx->getFirst());
    if (element.textDy && ! element.textDy->isEmpty())
        position.setY (position.getY() + element.textDy->getFirst());

    const auto font = resolveFont (element);
    const auto fontSize = element.fontSize.value_or (12.0f);

    StyledText styledText;
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (Size<float> (jmax (fontSize, static_cast<float> (element.text->length()) * fontSize * 2.0f),
                                          fontSize * 4.0f));
        modifier.setWrap (StyledText::noWrap);
        modifier.setHorizontalAlign (StyledText::left);
        modifier.setVerticalAlign (StyledText::top);
        modifier.appendText (*element.text, font, -1.0f, element.letterSpacing.value_or (0.0f));
    }

    const auto computedTextBounds = styledText.getComputedTextBounds();
    const auto fontAscent = font.getAscent();
    const auto fontDescent = font.getDescent();
    const auto hasUsableFontMetrics = fontAscent < 0.0f && fontDescent > fontAscent;
    const auto ascent = hasUsableFontMetrics ? fontAscent : -0.8f;
    const auto descent = hasUsableFontMetrics ? fontDescent : 0.2f;
    const auto metricsHeight = (descent - ascent) * fontSize;
    const auto textWidth = jmax (fontSize, computedTextBounds.getWidth());
    const auto textHeight = jmax (computedTextBounds.getHeight(), metricsHeight);
    const auto bottomPadding = fontSize * 0.25f;

    auto textX = position.getX();
    if (element.textAnchor == "middle")
        textX -= textWidth * 0.5f;
    else if (element.textAnchor == "end")
        textX -= textWidth;

    Rectangle<float> textBounds (textX,
                                 position.getY() + (ascent * fontSize),
                                 textWidth,
                                 textHeight + bottomPadding);

    g.fillFittedText (styledText, textBounds);
}

//==============================================================================

void Drawable::renderImageElement (Graphics& g, const SVGElement& element)
{
    if (! element.imageBounds)
        return;

    if (element.image)
    {
        g.drawImage (*element.image, *element.imageBounds);
        return;
    }

    if (element.imageHref)
    {
        if (document != nullptr)
        {
            if (auto image = SVGParser::loadImageFromHref (document->getParseOptions(), *element.imageHref))
                g.drawImage (*image, *element.imageBounds);
        }
    }
}

//==============================================================================

void Drawable::renderSoftStrokeElement (Graphics& g, const Path& path, float feather) const
{
    const auto baseOpacity = g.getOpacity();
    const auto baseStrokeWidth = g.getStrokeWidth();
    const auto layerCount = jlimit (4, 12, static_cast<int> (std::ceil (feather / 3.0f)));
    const auto maxStrokeOutset = feather;

    for (int i = layerCount; i >= 0; --i)
    {
        const auto position = static_cast<float> (i) / static_cast<float> (layerCount);
        const auto gaussian = std::exp (-0.5f * (position * 3.0f) * (position * 3.0f));
        const auto layerOpacity = (i == 0 ? 0.18f : 0.12f) * gaussian;

        if (layerOpacity <= 0.001f)
            continue;

        const auto savedLayerState = g.saveState();

        g.setFeather (0.0f);
        g.setOpacity (baseOpacity * layerOpacity);
        g.setStrokeWidth (baseStrokeWidth + maxStrokeOutset * 2.0f * position);
        g.strokePath (path);
    }
}

//==============================================================================

Path Drawable::createDashedPath (const Path& source, const Array<float>& dashArray, float dashOffset) const
{
    if (dashArray.isEmpty())
        return source;

    Array<float> positiveDashes;
    for (auto dash : dashArray)
    {
        if (dash > 0.0f)
            positiveDashes.add (dash);
    }

    if (positiveDashes.isEmpty())
        return source;

    Path result;
    float totalPatternLength = 0.0f;
    for (auto dash : positiveDashes)
        totalPatternLength += dash;

    if (totalPatternLength <= 0.0f)
        return source;

    int dashIndex = 0;
    float patternPosition = std::fmod (jmax (0.0f, dashOffset), totalPatternLength);
    while (patternPosition > positiveDashes[dashIndex])
    {
        patternPosition -= positiveDashes[dashIndex];
        dashIndex = (dashIndex + 1) % positiveDashes.size();
    }

    auto drawLineDash = [&] (Point<float> start, Point<float> end)
    {
        const auto length = start.distanceTo (end);
        if (length <= 0.0f)
            return;

        auto direction = (end - start) / length;
        float distance = 0.0f;

        while (distance < length)
        {
            const auto remainingInDash = positiveDashes[dashIndex] - patternPosition;
            const auto step = jmin (remainingInDash, length - distance);

            if ((dashIndex % 2) == 0 && step > 0.0f)
            {
                auto dashStart = start + direction * distance;
                auto dashEnd = start + direction * (distance + step);
                result.startNewSubPath (dashStart);
                result.lineTo (dashEnd);
            }

            distance += step;
            patternPosition = 0.0f;
            dashIndex = (dashIndex + 1) % positiveDashes.size();
        }
    };

    Point<float> current;
    Point<float> subPathStart;
    bool hasCurrent = false;

    for (const auto& segment : source)
    {
        switch (segment.verb)
        {
            case Path::Verb::MoveTo:
                current = segment.point;
                subPathStart = current;
                hasCurrent = true;
                break;

            case Path::Verb::LineTo:
                if (hasCurrent)
                    drawLineDash (current, segment.point);
                current = segment.point;
                break;

            case Path::Verb::Close:
                if (hasCurrent)
                    drawLineDash (current, subPathStart);
                current = subPathStart;
                break;

            case Path::Verb::QuadTo:
            case Path::Verb::CubicTo:
                if (hasCurrent)
                    drawLineDash (current, segment.point);
                current = segment.point;
                break;
        }
    }

    return result;
}

//==============================================================================

AffineTransform Drawable::calculateTransformForTarget (const Rectangle<float>& sourceBounds,
                                                       const Rectangle<float>& targetArea,
                                                       Fitting fitting,
                                                       Justification justification) const
{
    if (sourceBounds.isEmpty() || targetArea.isEmpty())
        return AffineTransform::identity();

    float scaleX = targetArea.getWidth() / sourceBounds.getWidth();
    float scaleY = targetArea.getHeight() / sourceBounds.getHeight();

    switch (fitting)
    {
        case Fitting::none:
            scaleX = scaleY = 1.0f;
            break;

        case Fitting::scaleToFit:
            scaleX = scaleY = jmin (scaleX, scaleY);
            break;

        case Fitting::fitWidth:
            scaleY = scaleX;
            break;

        case Fitting::fitHeight:
            scaleX = scaleY;
            break;

        case Fitting::scaleToFill:
        case Fitting::centerCrop:
            scaleX = scaleY = jmax (scaleX, scaleY);
            break;

        case Fitting::fill:
            break;

        case Fitting::centerInside:
            scaleX = scaleY = jmin (1.0f, jmin (scaleX, scaleY));
            break;

        case Fitting::stretchWidth:
            scaleY = 1.0f;
            break;

        case Fitting::stretchHeight:
            scaleX = 1.0f;
            break;

        case Fitting::tile:
            scaleX = scaleY = 1.0f;
            break;
    }

    float scaledWidth = sourceBounds.getWidth() * scaleX;
    float scaledHeight = sourceBounds.getHeight() * scaleY;

    float offsetX = targetArea.getX();
    float offsetY = targetArea.getY();

    if (justification.testFlags (Justification::horizontalCenter))
        offsetX += (targetArea.getWidth() - scaledWidth) * 0.5f;
    else if (justification.testFlags (Justification::right))
        offsetX += targetArea.getWidth() - scaledWidth;

    if (justification.testFlags (Justification::verticalCenter))
        offsetY += (targetArea.getHeight() - scaledHeight) * 0.5f;
    else if (justification.testFlags (Justification::bottom))
        offsetY += targetArea.getHeight() - scaledHeight;

    return AffineTransform::translation (-sourceBounds.getX(), -sourceBounds.getY())
        .scaled (scaleX, scaleY)
        .translated (offsetX, offsetY);
}

//==============================================================================

ColorGradient Drawable::createColorGradientFromSVG (const SVGGradient& gradient, const Rectangle<float>* objectBounds) const
{
    YUP_DRAWABLE_LOG ("Creating ColorGradient from SVG gradient ID: " << gradient.id
                                                                      << " type: " << (gradient.type == SVGGradient::Linear ? "Linear" : "Radial")
                                                                      << " units: " << (gradient.units == SVGGradient::UserSpaceOnUse ? "userSpaceOnUse" : "objectBoundingBox"));

    if (gradient.stops.empty())
    {
        YUP_DRAWABLE_LOG ("No stops in gradient, returning empty");
        return ColorGradient();
    }

    const bool hasBounds = objectBounds != nullptr && objectBounds->getWidth() > 0.0f && objectBounds->getHeight() > 0.0f;
    AffineTransform unitsTransform = AffineTransform::identity();

    if (gradient.units == SVGGradient::ObjectBoundingBox && hasBounds)
    {
        unitsTransform = AffineTransform::translation (objectBounds->getX(), objectBounds->getY())
                             .scaled (objectBounds->getWidth(), objectBounds->getHeight());
    }

    const AffineTransform gradientSpaceTransform = unitsTransform.followedBy (gradient.transform);

    auto transformPoint = [&gradientSpaceTransform] (Point<float> p)
    {
        float x = p.getX();
        float y = p.getY();
        if (! gradientSpaceTransform.isIdentity())
            gradientSpaceTransform.transformPoint (x, y);
        return Point<float> (x, y);
    };

    const Point<float> start = transformPoint (gradient.start);
    const Point<float> end = transformPoint (gradient.end);
    const Point<float> center = transformPoint (gradient.center);

    auto computeRadius = [&]() -> float
    {
        if (gradient.radius <= 0.0f)
            return 0.0f;

        const Point<float> edgePoints[] = {
            transformPoint (Point<float> (gradient.center.getX() + gradient.radius, gradient.center.getY())),
            transformPoint (Point<float> (gradient.center.getX() - gradient.radius, gradient.center.getY())),
            transformPoint (Point<float> (gradient.center.getX(), gradient.center.getY() + gradient.radius)),
            transformPoint (Point<float> (gradient.center.getX(), gradient.center.getY() - gradient.radius))
        };

        float maxRadius = 0.0f;
        for (const auto& edgePoint : edgePoints)
            maxRadius = jmax (maxRadius, Line<float> (center, edgePoint).length());

        return maxRadius;
    };

    const auto radius = gradient.type == SVGGradient::Radial ? computeRadius() : 0.0f;

    std::vector<ColorGradient::ColorStop> colorStops;
    colorStops.reserve (gradient.stops.size());

    if (gradient.type == SVGGradient::Linear
        && objectBounds != nullptr
        && (gradient.spreadMethod == "reflect" || gradient.spreadMethod == "repeat"))
    {
        const auto dx = end.getX() - start.getX();
        const auto dy = end.getY() - start.getY();
        const auto lengthSquared = dx * dx + dy * dy;

        if (lengthSquared > 0.0f)
        {
            const Point<float> corners[] = {
                objectBounds->getTopLeft(),
                objectBounds->getTopRight(),
                objectBounds->getBottomLeft(),
                objectBounds->getBottomRight()
            };

            float minT = 0.0f;
            float maxT = 1.0f;

            for (const auto& corner : corners)
            {
                const auto t = ((corner.getX() - start.getX()) * dx + (corner.getY() - start.getY()) * dy) / lengthSquared;
                minT = jmin (minT, t);
                maxT = jmax (maxT, t);
            }

            int firstRepeat = static_cast<int> (std::floor (minT));
            int lastRepeat = static_cast<int> (std::ceil (maxT));

            constexpr int maxGradientRepeats = 32;
            if (lastRepeat - firstRepeat > maxGradientRepeats)
            {
                const auto centerRepeat = (firstRepeat + lastRepeat) / 2;
                firstRepeat = centerRepeat - (maxGradientRepeats / 2);
                lastRepeat = firstRepeat + maxGradientRepeats;
            }

            const auto repeatStart = static_cast<float> (firstRepeat);
            const auto repeatEnd = static_cast<float> (jmax (lastRepeat, firstRepeat + 1));
            const auto repeatRange = repeatEnd - repeatStart;
            const auto expandedStart = Point<float> (start.getX() + dx * repeatStart, start.getY() + dy * repeatStart);
            const auto expandedEnd = Point<float> (start.getX() + dx * repeatEnd, start.getY() + dy * repeatEnd);

            colorStops.reserve (gradient.stops.size() * static_cast<size_t> (repeatEnd - repeatStart));

            for (int repeat = firstRepeat; repeat < lastRepeat; ++repeat)
            {
                const bool reflected = gradient.spreadMethod == "reflect" && (std::abs (repeat) % 2) == 1;

                for (const auto& stop : gradient.stops)
                {
                    const auto repeatedOffset = static_cast<float> (repeat) + (reflected ? (1.0f - stop.offset) : stop.offset);
                    const auto normalizedOffset = (repeatedOffset - repeatStart) / repeatRange;
                    const auto position = Point<float> (expandedStart.getX() + (expandedEnd.getX() - expandedStart.getX()) * normalizedOffset,
                                                        expandedStart.getY() + (expandedEnd.getY() - expandedStart.getY()) * normalizedOffset);

                    colorStops.emplace_back (stop.color.withMultipliedAlpha (stop.opacity), position, jlimit (0.0f, 1.0f, normalizedOffset));
                }
            }

            std::sort (colorStops.begin(), colorStops.end(), [] (const auto& a, const auto& b)
            {
                return a.delta < b.delta;
            });
        }
    }

    if (colorStops.empty())
    {
        for (const auto& stop : gradient.stops)
        {
            Color color = stop.color.withMultipliedAlpha (stop.opacity);

            if (gradient.type == SVGGradient::Linear)
            {
                const auto interpolated = Point<float> (start.getX() + stop.offset * (end.getX() - start.getX()),
                                                        start.getY() + stop.offset * (end.getY() - start.getY()));

                colorStops.emplace_back (color, interpolated, stop.offset);
            }
            else
            {
                const auto radialPoint = Point<float> (center.getX() + radius * stop.offset, center.getY());
                colorStops.emplace_back (color, radialPoint, stop.offset);
            }
        }
    }

    ColorGradient::Type type = (gradient.type == SVGGradient::Linear) ? ColorGradient::Linear : ColorGradient::Radial;
    return ColorGradient (type, colorStops);
}

} // namespace yup
