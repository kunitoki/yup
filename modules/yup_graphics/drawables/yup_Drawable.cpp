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
    resolved->hasStart = referencedGradient->hasStart;
    resolved->hasEnd = referencedGradient->hasEnd;
    resolved->hasCenter = referencedGradient->hasCenter;
    resolved->hasRadius = referencedGradient->hasRadius;
    resolved->hasFocal = referencedGradient->hasFocal;
    resolved->hasUnits = referencedGradient->hasUnits;
    resolved->hasSpreadMethod = referencedGradient->hasSpreadMethod;

    if (gradient->hasStart)
    {
        resolved->start = gradient->start;
        resolved->hasStart = true;
    }
    if (gradient->hasEnd)
    {
        resolved->end = gradient->end;
        resolved->hasEnd = true;
    }
    if (gradient->hasCenter)
    {
        resolved->center = gradient->center;
        resolved->hasCenter = true;
    }
    if (gradient->hasRadius)
    {
        resolved->radius = gradient->radius;
        resolved->hasRadius = true;
    }
    if (gradient->hasFocal)
    {
        resolved->focal = gradient->focal;
        resolved->hasFocal = true;
    }

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

    if (! filter->primitives.empty())
        resolved->primitives = filter->primitives;
    else
        resolved->primitives = referencedFilter->primitives;

    return resolved;
}

AffineTransform createGradientSpaceTransform (const SVGGradient& gradient, const Rectangle<float>* objectBounds)
{
    const bool hasBounds = objectBounds != nullptr && objectBounds->getWidth() > 0.0f && objectBounds->getHeight() > 0.0f;
    AffineTransform unitsTransform = AffineTransform::identity();

    if (gradient.units == SVGGradient::ObjectBoundingBox && hasBounds)
    {
        unitsTransform = AffineTransform::scaling (objectBounds->getWidth(), objectBounds->getHeight())
                             .translated (objectBounds->getX(), objectBounds->getY());
    }

    return unitsTransform.followedBy (gradient.transform);
}

SVGGradient createLocalGradientSpaceGradient (const SVGGradient& gradient)
{
    SVGGradient localGradient;

    localGradient.type = gradient.type;
    localGradient.id = gradient.id;
    localGradient.units = SVGGradient::UserSpaceOnUse;
    localGradient.spreadMethod = gradient.spreadMethod;
    localGradient.start = gradient.start;
    localGradient.end = gradient.end;
    localGradient.center = gradient.center;
    localGradient.radius = gradient.radius;
    localGradient.focal = gradient.focal;
    localGradient.stops = gradient.stops;
    localGradient.hasStart = gradient.hasStart;
    localGradient.hasEnd = gradient.hasEnd;
    localGradient.hasCenter = gradient.hasCenter;
    localGradient.hasRadius = gradient.hasRadius;
    localGradient.hasFocal = gradient.hasFocal;
    localGradient.hasUnits = true;
    localGradient.hasSpreadMethod = gradient.hasSpreadMethod;

    return localGradient;
}

SVGClipPath::Ptr getClipPathById (const SVGData& data, const String& id)
{
    return data.clipPathsById[id];
}

SVGMask::Ptr getMaskById (const SVGData& data, const String& id)
{
    return data.masksById[id];
}

SVGMarker::Ptr getMarkerById (const SVGData& data, const String& id)
{
    return data.markersById[id];
}

SVGPattern::Ptr getPatternById (const SVGData& data, const String& id)
{
    return data.patternsById[id];
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

bool Drawable::parseSVG (const File& svgFile, const ParseOptions& options)
{
    YUP_DRAWABLE_LOG ("parseSVG(file, options) - file: " << svgFile.getFullPathName());

    document = SVGParser::parse (svgFile, options);
    return document != nullptr;
}

//==============================================================================

bool Drawable::parseSVG (StringRef svgText)
{
    YUP_DRAWABLE_LOG ("parseSVG(text) - length: " << String (svgText.text).length());

    return parseSVG (svgText, ParseOptions());
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

        std::unordered_set<const SVGElement*> visiting;
        for (const auto& element : data.elements)
            paintElement (g, data, *element, data.rootHasFill, data.rootHasStroke, data.rootFillColor.value_or (Colors::black), visiting);
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
        auto finalTransform = calculateTransformForTarget (finalBounds,
                                                           targetArea,
                                                           data.rootHasPreserveAspectRatio ? data.rootPreserveAspectRatioFitting : fitting,
                                                           data.rootHasPreserveAspectRatio ? data.rootPreserveAspectRatioJustification : justification);

        if (! finalTransform.isIdentity())
            g.addTransform (finalTransform);

        g.setStrokeWidth (1.0f);

        if (data.rootFillColor)
            g.setFillColor (*data.rootFillColor);
        else
            g.setFillColor (Colors::black);

        if (data.rootStrokeColor)
            g.setStrokeColor (*data.rootStrokeColor);

        std::unordered_set<const SVGElement*> visiting;
        for (const auto& element : data.elements)
            paintElement (g, data, *element, data.rootHasFill, data.rootHasStroke, data.rootFillColor.value_or (Colors::black), visiting);
    });
}

//==============================================================================

void Drawable::paintElement (Graphics& g,
                             const SVGData& data,
                             const SVGElement& element,
                             bool hasParentFillEnabled,
                             bool hasParentStrokeEnabled,
                             Color currentColor,
                             std::unordered_set<const SVGElement*>& visitingElements,
                             std::optional<Array<float>> inheritedStrokeDashArray,
                             float inheritedStrokeDashOffset,
                             int recursionDepth)
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
    std::optional<Path> localGradientFillPath;
    std::optional<AffineTransform> localGradientFillTransform;

    if (element.color)
        currentColor = *element.color;

    if (element.transform)
        g.setTransform (element.transform->followedBy (g.getTransform()));

    if (element.opacity)
        g.setOpacity (g.getOpacity() * (*element.opacity));

    if (element.blendMode)
        g.setBlendMode (*element.blendMode);

    auto currentStrokeDashArray = inheritedStrokeDashArray;
    if (element.strokeDashArrayNone)
        currentStrokeDashArray.reset();
    else if (element.strokeDashArray)
        currentStrokeDashArray = element.strokeDashArray;

    const auto currentStrokeDashOffset = element.strokeDashOffset.value_or (inheritedStrokeDashOffset);

    if (element.filterUrl)
    {
        if (auto filter = resolveFilter (data, getFilterById (data, *element.filterUrl)))
        {
            for (const auto& primitive : filter->primitives)
            {
                if (auto blur = dynamic_cast<const SVGFEGaussianBlur*> (primitive.get()))
                {
                    const auto svgStdDeviationToFeather = 2.0f;
                    const auto feather = jmax (g.getFeather(), blur->stdDeviation * svgStdDeviationToFeather);
                    g.setFeather (feather);
                }
                else if (auto blend = dynamic_cast<const SVGFEBlend*> (primitive.get()))
                {
                    g.setBlendMode (blend->mode);
                }
            }
        }
    }

    const auto setViewportClip = [&g] (const Rectangle<float>& viewportBounds)
    {
        Path viewportClip;
        viewportClip.addRectangle (viewportBounds);
        auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
        auto transformedViewportClip = viewportClip.transformed (clipTransform);

        const auto savedClipTransform = g.getTransform();
        g.setTransform (AffineTransform::identity());
        g.setClipPath (transformedViewportClip);
        g.setTransform (savedClipTransform);
    };

    if (element.viewBox && (element.viewportBounds || element.viewportSize))
    {
        auto viewport = element.viewportBounds != std::nullopt
                          ? Rectangle<float> (0.0f, 0.0f, element.viewportBounds->getWidth(), element.viewportBounds->getHeight())
                          : Rectangle<float> (0.0f, 0.0f, element.viewportSize->getWidth(), element.viewportSize->getHeight());

        auto viewportTransform = calculateTransformForTarget (*element.viewBox, viewport, element.preserveAspectRatioFitting, element.preserveAspectRatioJustification);
        if (element.tagName == "svg" && element.viewportBounds)
        {
            setViewportClip (*element.viewportBounds);
            viewportTransform = viewportTransform.followedBy (AffineTransform::translation (element.viewportBounds->getX(), element.viewportBounds->getY()));
        }

        if (! viewportTransform.isIdentity())
            g.setTransform (viewportTransform.followedBy (g.getTransform()));
    }
    else if (element.tagName == "svg" && element.viewportBounds)
    {
        setViewportClip (*element.viewportBounds);
        auto viewportTransform = AffineTransform::translation (element.viewportBounds->getX(), element.viewportBounds->getY());
        g.setTransform (viewportTransform.followedBy (g.getTransform()));
    }

    bool hasClipping = false;
    if (element.clipPathUrl)
    {
        if (auto clipPath = getClipPathById (data, *element.clipPathUrl))
        {
            const auto buildClipShape = [&] (const SVGClipPath& clip, const Rectangle<float>& objectBounds) -> Path
            {
                std::optional<Rectangle<float>> clipObjectBounds;

                if (clip.units == SVGClipPath::ObjectBoundingBox)
                {
                    if (! objectBounds.isEmpty())
                        clipObjectBounds = objectBounds;
                }

                Path result;

                for (const auto& clipElement : clip.elements)
                {
                    const auto* elementPath = clipElement->path ? std::addressof (*clipElement->path) : nullptr;

                    if (elementPath == nullptr && clipElement->reference)
                    {
                        if (auto refElement = data.elementsById[*clipElement->reference]; refElement != nullptr && refElement->path)
                            elementPath = std::addressof (*refElement->path);
                    }

                    if (elementPath == nullptr)
                        continue;

                    AffineTransform clipTransform = clipElement->transform.value_or (AffineTransform::identity());

                    if (clip.units == SVGClipPath::ObjectBoundingBox)
                    {
                        if (! clipObjectBounds || clipObjectBounds->isEmpty())
                            continue;

                        auto unitsTransform = AffineTransform::translation (clipObjectBounds->getX(), clipObjectBounds->getY())
                                                  .scaled (clipObjectBounds->getWidth(), clipObjectBounds->getHeight());
                        clipTransform = clipTransform.followedBy (unitsTransform);
                    }

                    if (! clipTransform.isIdentity())
                        result.appendPath (*elementPath, clipTransform);
                    else
                        result.appendPath (*elementPath);
                }

                return result;
            };

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

            const auto clipBounds = clipObjectBounds.value_or (Rectangle<float>());

            const auto setClipPath = [&] (const Path& shape)
            {
                auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
                auto transformedClipPath = shape.transformed (clipTransform);

                const auto savedClipTransform = g.getTransform();
                g.setTransform (AffineTransform::identity());
                g.setClipPath (transformedClipPath);
                g.setTransform (savedClipTransform);
            };

            // If the clipPath itself has a nested clip-path, apply it first (intersection)
            if (clipPath->clipPathUrl)
            {
                if (auto nestedClipPath = getClipPathById (data, *clipPath->clipPathUrl))
                {
                    auto nestedClipShape = buildClipShape (*nestedClipPath, clipBounds);
                    if (! nestedClipShape.isEmpty())
                    {
                        setClipPath (nestedClipShape);
                        hasClipping = true;
                    }
                }
            }

            // Apply the clipPath's own elements (intersects with nested clip if present)
            auto clipShape = buildClipShape (*clipPath, clipBounds);
            if (! clipShape.isEmpty())
            {
                setClipPath (clipShape);
                hasClipping = true;
            }
        }
    }

    if (element.maskUrl)
    {
        if (auto mask = getMaskById (data, *element.maskUrl))
        {
            std::optional<Rectangle<float>> maskObjectBounds;

            if (mask->maskUnits == SVGMask::ObjectBoundingBox)
            {
                if (element.path)
                    maskObjectBounds = element.path->getBounds();
                else if (element.reference)
                {
                    if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
                        maskObjectBounds = refElement->path->getBounds();
                }
                else if (element.imageBounds)
                {
                    maskObjectBounds = *element.imageBounds;
                }
            }

            Path combinedMaskPath;

            for (const auto& maskElement : mask->elements)
            {
                if (maskElement->path)
                {
                    AffineTransform maskTransform = maskElement->transform.value_or (AffineTransform::identity());

                    if (mask->maskUnits == SVGMask::ObjectBoundingBox)
                    {
                        if (! maskObjectBounds || maskObjectBounds->isEmpty())
                            continue;

                        auto unitsTransform = AffineTransform::translation (maskObjectBounds->getX(), maskObjectBounds->getY())
                                                  .scaled (maskObjectBounds->getWidth(), maskObjectBounds->getHeight());
                        maskTransform = maskTransform.followedBy (unitsTransform);
                    }

                    if (! maskTransform.isIdentity())
                        combinedMaskPath.appendPath (*maskElement->path, maskTransform);
                    else
                        combinedMaskPath.appendPath (*maskElement->path);
                }
            }

            if (! combinedMaskPath.isEmpty())
            {
                auto maskClipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
                auto transformedMaskPath = combinedMaskPath.transformed (maskClipTransform);

                const auto savedMaskTransform = g.getTransform();
                g.setTransform (AffineTransform::identity());
                g.setClipPath (transformedMaskPath);
                g.setTransform (savedMaskTransform);
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

            if (resolvedGradient->type == SVGGradient::Radial && element.path && gradientBounds && ! resolvedGradient->transform.isIdentity())
            {
                const auto gradientSpaceTransform = createGradientSpaceTransform (*resolvedGradient, std::addressof (*gradientBounds));

                if (! gradientSpaceTransform.isIdentity() && std::abs (gradientSpaceTransform.getDeterminant()) > 1.0e-6f)
                {
                    localGradientFillPath = element.path->transformed (gradientSpaceTransform.inverted());
                    localGradientFillTransform = gradientSpaceTransform;

                    const auto localGradient = createLocalGradientSpaceGradient (*resolvedGradient);
                    const auto localGradientBounds = localGradientFillPath->getBounds();
                    colorGradient = createColorGradientFromSVG (localGradient, std::addressof (localGradientBounds));
                }
            }

            g.setFillColorGradient (colorGradient);
            isFillDefined = true;
        }
        else if (getPatternById (data, *element.fillUrl))
        {
            isFillDefined = true;
        }
    }
    else if (hasParentFillEnabled)
    {
        isFillDefined = true;
    }

    if (isFillDefined && ! element.noFill)
    {
        const auto fillElementPath = [&]
        {
            if (localGradientFillPath && localGradientFillTransform)
            {
                const auto savedFillTransform = g.getTransform();
                g.setTransform (localGradientFillTransform->followedBy (savedFillTransform));
                g.fillPath (*localGradientFillPath);
                g.setTransform (savedFillTransform);
            }
            else
            {
                g.fillPath (*element.path);
            }
        };

        if (element.path)
        {
            if (element.fillUrl)
            {
                if (auto pattern = getPatternById (data, *element.fillUrl))
                {
                    paintPatternFill (g, data, *element.path, element, *pattern, currentColor, visitingElements, recursionDepth);
                }
                else
                {
                    YUP_DRAWABLE_LOG ("Filling path - tag: " << element.tagName
                                                             << " id: " << (element.id ? *element.id : "none")
                                                             << " bounds: " << element.path->getBounds().toString()
                                                             << " clip: " << (hasClipping ? "true" : "false"));
                    fillElementPath();
                }
            }
            else
            {
                YUP_DRAWABLE_LOG ("Filling path - tag: " << element.tagName
                                                         << " id: " << (element.id ? *element.id : "none")
                                                         << " bounds: " << element.path->getBounds().toString()
                                                         << " clip: " << (hasClipping ? "true" : "false"));
                fillElementPath();
            }
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
    g.setStrokeMiterLimit (element.strokeMiterLimit);

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

        if (pathToStroke != nullptr && currentStrokeDashArray && ! currentStrokeDashArray->isEmpty())
        {
            dashedPath = createDashedPath (*pathToStroke, *currentStrokeDashArray, currentStrokeDashOffset);
            pathToStroke = std::addressof (*dashedPath);
        }

        if (pathToStroke != nullptr)
        {
            YUP_DRAWABLE_LOG ("Stroking path - tag: " << element.tagName
                                                      << " id: " << (element.id ? *element.id : "none")
                                                      << " bounds: " << pathToStroke->getBounds().toString());
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

                const Path* referencePathToStroke = std::addressof (*refElement->path);
                std::optional<Path> dashedReferencePath;
                auto referenceStrokeDashArray = currentStrokeDashArray;
                if (refElement->strokeDashArrayNone)
                    referenceStrokeDashArray.reset();
                else if (refElement->strokeDashArray)
                    referenceStrokeDashArray = refElement->strokeDashArray;

                const auto referenceStrokeDashOffset = refElement->strokeDashOffset.value_or (currentStrokeDashOffset);

                if (referenceStrokeDashArray && ! referenceStrokeDashArray->isEmpty())
                {
                    dashedReferencePath = createDashedPath (*referencePathToStroke, *referenceStrokeDashArray, referenceStrokeDashOffset);
                    referencePathToStroke = std::addressof (*dashedReferencePath);
                }

                g.strokePath (*referencePathToStroke);

                if (refElement->localTransform)
                    g.setTransform (savedTransform);
            }
        }
    }

    if (element.path && (element.markerStart || element.markerMid || element.markerEnd))
    {
        struct MarkerPlacement
        {
            Point<float> position;
            float tangentAngle = 0.0f;
        };

        std::vector<MarkerPlacement> startPlacements, midPlacements, endPlacements;
        Point<float> firstPoint, prevPoint, subpathStart;
        bool hasFirstPoint = false;
        float firstTangent = 0.0f, prevTangent = 0.0f;
        bool firstSegmentAfterMove = false;

        for (const auto& seg : *element.path)
        {
            switch (seg.verb)
            {
                case Path::Verb::MoveTo:
                {
                    if (hasFirstPoint && ! endPlacements.empty())
                    {
                        // Flush pending end for the previous sub-path
                    }

                    firstPoint = seg.point;
                    prevPoint = seg.point;
                    subpathStart = seg.point;
                    hasFirstPoint = true;
                    firstSegmentAfterMove = true;
                    firstTangent = 0.0f;
                    prevTangent = 0.0f;
                    endPlacements.clear();
                    break;
                }

                case Path::Verb::LineTo:
                {
                    if (! hasFirstPoint)
                        break;

                    const float dx = seg.point.getX() - prevPoint.getX();
                    const float dy = seg.point.getY() - prevPoint.getY();
                    const float angle = std::atan2 (dy, dx);

                    if (firstSegmentAfterMove)
                    {
                        startPlacements.push_back ({ firstPoint, angle });
                        firstTangent = angle;
                        firstSegmentAfterMove = false;
                    }
                    else
                    {
                        midPlacements.push_back ({ prevPoint, prevTangent });
                    }

                    prevPoint = seg.point;
                    prevTangent = angle;
                    endPlacements = { { seg.point, angle } };
                    break;
                }

                case Path::Verb::QuadTo:
                {
                    if (! hasFirstPoint)
                        break;

                    const float dx = seg.point.getX() - seg.controlPoint1.getX();
                    const float dy = seg.point.getY() - seg.controlPoint1.getY();
                    const float angle = std::atan2 (dy, dx);
                    const float startDx = seg.controlPoint1.getX() - prevPoint.getX();
                    const float startDy = seg.controlPoint1.getY() - prevPoint.getY();
                    const float startAngle = std::atan2 (startDy, startDx);

                    if (firstSegmentAfterMove)
                    {
                        startPlacements.push_back ({ firstPoint, startAngle });
                        firstTangent = startAngle;
                        firstSegmentAfterMove = false;
                    }
                    else
                    {
                        midPlacements.push_back ({ prevPoint, prevTangent });
                    }

                    prevPoint = seg.point;
                    prevTangent = angle;
                    endPlacements = { { seg.point, angle } };
                    break;
                }

                case Path::Verb::CubicTo:
                {
                    if (! hasFirstPoint)
                        break;

                    const float dx = seg.point.getX() - seg.controlPoint2.getX();
                    const float dy = seg.point.getY() - seg.controlPoint2.getY();
                    const float angle = std::atan2 (dy, dx);
                    const float startDx = seg.controlPoint1.getX() - prevPoint.getX();
                    const float startDy = seg.controlPoint1.getY() - prevPoint.getY();
                    const float startAngle = std::atan2 (startDy, startDx);

                    if (firstSegmentAfterMove)
                    {
                        startPlacements.push_back ({ firstPoint, startAngle });
                        firstTangent = startAngle;
                        firstSegmentAfterMove = false;
                    }
                    else
                    {
                        midPlacements.push_back ({ prevPoint, prevTangent });
                    }

                    prevPoint = seg.point;
                    prevTangent = angle;
                    endPlacements = { { seg.point, angle } };
                    break;
                }

                case Path::Verb::Close:
                {
                    if (! hasFirstPoint)
                        break;

                    if (! firstSegmentAfterMove)
                        midPlacements.push_back ({ prevPoint, prevTangent });

                    prevPoint = subpathStart;
                    firstSegmentAfterMove = true;
                    endPlacements.clear();
                    break;
                }
            }
        }

        const float sw = element.strokeWidth.value_or (1.0f);

        if (element.markerStart)
        {
            if (auto marker = getMarkerById (data, *element.markerStart))
            {
                for (const auto& p : startPlacements)
                {
                    float angle = p.tangentAngle;
                    if (marker->orientAutoStartReverse)
                        angle += MathConstants<float>::pi;

                    paintMarker (g, data, *marker, sw, p.position, angle, visitingElements, recursionDepth);
                }
            }
        }

        if (element.markerMid)
        {
            if (auto marker = getMarkerById (data, *element.markerMid))
            {
                for (const auto& p : midPlacements)
                    paintMarker (g, data, *marker, sw, p.position, p.tangentAngle, visitingElements, recursionDepth);
            }
        }

        if (element.markerEnd)
        {
            if (auto marker = getMarkerById (data, *element.markerEnd))
            {
                for (const auto& p : endPlacements)
                    paintMarker (g, data, *marker, sw, p.position, p.tangentAngle, visitingElements, recursionDepth);
            }
        }
    }

    if (element.reference)
    {
        if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && ! refElement->children.empty())
        {
            if (visitingElements.count (refElement.get()) != 0)
            {
                YUP_DRAWABLE_LOG ("paintElement skipped - cycle detected in use reference id: " << *element.reference);
            }
            else
            {
                struct ScopeErase
                {
                    std::unordered_set<const SVGElement*>& set;
                    const SVGElement* elem;

                    ~ScopeErase() { set.erase (elem); }
                } eraseGuard { visitingElements, refElement.get() };

                visitingElements.insert (refElement.get());

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
                    paintElement (g, data, *childElement, isFillDefined && ! element.noFill, isStrokeDefined && ! element.noStroke, currentColor, visitingElements, currentStrokeDashArray, currentStrokeDashOffset, recursionDepth + 1);

                g.setTransform (savedTransform);
            }
        }
    }

    for (const auto& childElement : element.children)
        paintElement (g, data, *childElement, isFillDefined && ! element.noFill, isStrokeDefined && ! element.noStroke, currentColor, visitingElements, currentStrokeDashArray, currentStrokeDashOffset, recursionDepth + 1);

    // paintDebugElement (g, element);
}

//==============================================================================

void Drawable::paintMarker (Graphics& g,
                            const SVGData& data,
                            const SVGMarker& marker,
                            float strokeWidth,
                            Point<float> position,
                            float tangentAngle,
                            std::unordered_set<const SVGElement*>& visitingElements,
                            int recursionDepth)
{
    const auto savedState = g.saveState();

    float scale = (marker.markerUnits == SVGMarker::StrokeWidth) ? strokeWidth : 1.0f;

    if (marker.viewBox
        && marker.viewBox->getWidth() > 0.0f
        && marker.viewBox->getHeight() > 0.0f)
    {
        const float scaleX = marker.markerWidth / marker.viewBox->getWidth();
        const float scaleY = marker.markerHeight / marker.viewBox->getHeight();
        scale *= std::min (scaleX, scaleY);
    }

    // Build the marker transform: T(position) * R(angle) * S(scale) * T(-refX, -refY)
    // Each followedBy call appends a transform that is applied AFTER the current one.
    const float angle = marker.orient ? degreesToRadians (*marker.orient) : tangentAngle;

    const AffineTransform markerTransform = AffineTransform::translation (-marker.refX, -marker.refY)
                                                .followedBy (AffineTransform::scaling (scale))
                                                .followedBy (AffineTransform::rotation (angle))
                                                .followedBy (AffineTransform::translation (position.getX(), position.getY()));

    g.addTransform (markerTransform);

    for (const auto& element : marker.elements)
        paintElement (g, data, *element, true, false, Colors::black, visitingElements, std::nullopt, 0.0f, recursionDepth + 1);
}

//==============================================================================

void Drawable::paintPatternFill (Graphics& g,
                                 const SVGData& data,
                                 const Path& shape,
                                 const SVGElement& element,
                                 const SVGPattern& pattern,
                                 Color currentColor,
                                 std::unordered_set<const SVGElement*>& visitingElements,
                                 int recursionDepth)
{
    float tileW = pattern.width;
    float tileH = pattern.height;
    float originX = pattern.x;
    float originY = pattern.y;

    if (pattern.patternUnits == SVGPattern::ObjectBoundingBox)
    {
        const auto bounds = shape.getBounds();
        tileW *= bounds.getWidth();
        tileH *= bounds.getHeight();
        originX = bounds.getX() + pattern.x * bounds.getWidth();
        originY = bounds.getY() + pattern.y * bounds.getHeight();
    }

    if (tileW <= 0.0f || tileH <= 0.0f)
        return;

    const auto savedState = g.saveState();

    // Clip rendering to the filled shape
    {
        auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
        auto transformedShape = shape.transformed (clipTransform);
        const auto savedClipTransform = g.getTransform();
        g.setTransform (AffineTransform::identity());
        g.setClipPath (transformedShape);
        g.setTransform (savedClipTransform);
    }

    if (! pattern.patternTransform.isIdentity())
        g.addTransform (pattern.patternTransform);

    const auto shapeBounds = shape.getBounds();
    const float startX = std::floor ((shapeBounds.getX() - originX) / tileW) * tileW + originX;
    const float startY = std::floor ((shapeBounds.getY() - originY) / tileH) * tileH + originY;

    for (float tileY = startY; tileY < shapeBounds.getBottom(); tileY += tileH)
    {
        for (float tileX = startX; tileX < shapeBounds.getRight(); tileX += tileW)
        {
            const auto savedTileState = g.saveState();

            g.addTransform (AffineTransform::translation (tileX, tileY));

            if (pattern.viewBox
                && pattern.viewBox->getWidth() > 0.0f
                && pattern.viewBox->getHeight() > 0.0f)
            {
                const float scaleX = tileW / pattern.viewBox->getWidth();
                const float scaleY = tileH / pattern.viewBox->getHeight();
                g.addTransform (AffineTransform::scaling (scaleX, scaleY)
                                    .translated (-pattern.viewBox->getX(), -pattern.viewBox->getY()));
            }

            for (const auto& patternElement : pattern.elements)
                paintElement (g, data, *patternElement, true, false, currentColor, visitingElements, std::nullopt, 0.0f, recursionDepth + 1);
        }
    }
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
        return Font().withHeight (element.fontSize.value_or (16.0f));

    const auto& options = document->getParseOptions();
    const auto fontSize = element.fontSize.value_or (16.0f);
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
    const auto fontSize = element.fontSize.value_or (16.0f);

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

    auto drawImage = [this, &g, &element] (const Image& image)
    {
        if (! image.isValid())
            return;

        const Rectangle<float> imageSourceBounds (0.0f, 0.0f, static_cast<float> (image.getWidth()), static_cast<float> (image.getHeight()));
        const auto imageTransform = calculateTransformForTarget (imageSourceBounds,
                                                                 *element.imageBounds,
                                                                 element.preserveAspectRatioFitting,
                                                                 element.preserveAspectRatioJustification);
        const auto fittedImageBounds = imageSourceBounds.transformed (imageTransform);

        const auto savedState = g.saveState();
        g.setClipPath (*element.imageBounds);
        g.drawImage (image, fittedImageBounds);
    };

    if (element.image)
    {
        drawImage (*element.image);
        return;
    }

    if (element.imageHref)
    {
        if (document != nullptr)
        {
            if (auto image = SVGParser::loadImageFromHref (document->getParseOptions(), *element.imageHref))
                drawImage (*image);
        }
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

    if ((positiveDashes.size() % 2) != 0)
    {
        const auto originalSize = positiveDashes.size();
        for (int i = 0; i < originalSize; ++i)
            positiveDashes.add (positiveDashes[i]);
    }

    Path result;
    float totalPatternLength = 0.0f;
    for (auto dash : positiveDashes)
        totalPatternLength += dash;

    if (totalPatternLength <= 0.0f)
        return source;

    auto dashIndex = 0;
    auto patternPosition = 0.0f;

    const auto resetDashPosition = [&]
    {
        dashIndex = 0;
        patternPosition = std::fmod (dashOffset, totalPatternLength);
        if (patternPosition < 0.0f)
            patternPosition += totalPatternLength;

        while (patternPosition > positiveDashes[dashIndex])
        {
            patternPosition -= positiveDashes[dashIndex];
            dashIndex = (dashIndex + 1) % positiveDashes.size();
        }
    };

    resetDashPosition();

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
                resetDashPosition();
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
        .followedBy (AffineTransform::scaling (scaleX, scaleY))
        .followedBy (AffineTransform::translation (offsetX, offsetY));
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

    const AffineTransform gradientSpaceTransform = createGradientSpaceTransform (gradient, objectBounds);

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
    const Point<float> focal = gradient.hasFocal ? transformPoint (gradient.focal) : center;

    auto computeRadiusFrom = [&] (Point<float> origin) -> float
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
            maxRadius = jmax (maxRadius, Line<float> (origin, edgePoint).length());

        return maxRadius;
    };

    const auto radius = gradient.type == SVGGradient::Radial ? computeRadiusFrom (center) : 0.0f;

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

    if (gradient.type == SVGGradient::Radial
        && objectBounds != nullptr
        && (gradient.spreadMethod == "reflect" || gradient.spreadMethod == "repeat"))
    {
        const auto radialCenter = gradient.hasFocal ? focal : center;
        const auto radialRadius = gradient.hasFocal ? computeRadiusFrom (focal) : radius;

        if (radialRadius > 0.0f)
        {
            const Point<float> corners[] = {
                objectBounds->getTopLeft(),
                objectBounds->getTopRight(),
                objectBounds->getBottomLeft(),
                objectBounds->getBottomRight()
            };

            float maxT = 1.0f;

            for (const auto& corner : corners)
                maxT = jmax (maxT, Line<float> (radialCenter, corner).length() / radialRadius);

            int repeatEnd = static_cast<int> (std::ceil (maxT));

            constexpr int maxGradientRepeats = 64;
            repeatEnd = jlimit (1, maxGradientRepeats, repeatEnd);

            const auto repeatRange = static_cast<float> (repeatEnd);
            const auto expandedRadius = radialRadius * repeatRange;

            colorStops.reserve (gradient.stops.size() * static_cast<size_t> (repeatEnd));

            const auto appendStop = [&] (const SVGGradientStop& stop, float repeatedOffset)
            {
                const auto normalizedOffset = repeatedOffset / repeatRange;
                const auto radialPoint = Point<float> (radialCenter.getX() + expandedRadius * normalizedOffset,
                                                       radialCenter.getY());

                colorStops.emplace_back (stop.color.withMultipliedAlpha (stop.opacity), radialPoint, jlimit (0.0f, 1.0f, normalizedOffset));
            };

            for (int repeat = 0; repeat < repeatEnd; ++repeat)
            {
                const bool reflected = gradient.spreadMethod == "reflect" && (repeat % 2) == 1;

                if (reflected)
                {
                    for (auto stop = gradient.stops.rbegin(); stop != gradient.stops.rend(); ++stop)
                        appendStop (*stop, static_cast<float> (repeat) + (1.0f - stop->offset));
                }
                else
                {
                    for (const auto& stop : gradient.stops)
                        appendStop (stop, static_cast<float> (repeat) + stop.offset);
                }
            }
        }
    }

    if (colorStops.empty())
    {
        if (gradient.type == SVGGradient::Radial)
        {
            const auto radialCenter = gradient.hasFocal ? focal : center;
            const auto radialRadius = gradient.hasFocal ? computeRadiusFrom (focal) : radius;
            const auto effectiveRadius = (radialRadius > 0.0f) ? radialRadius : 1.0f;

            for (const auto& stop : gradient.stops)
            {
                const auto offset = jlimit (0.0f, 1.0f, stop.offset);
                const auto radialPoint = Point<float> (radialCenter.getX() + effectiveRadius * offset,
                                                       radialCenter.getY());
                colorStops.emplace_back (stop.color.withMultipliedAlpha (stop.opacity), radialPoint, offset);
            }
        }
        else
        {
            for (const auto& stop : gradient.stops)
            {
                Color color = stop.color.withMultipliedAlpha (stop.opacity);
                const auto interpolated = Point<float> (start.getX() + stop.offset * (end.getX() - start.getX()),
                                                        start.getY() + stop.offset * (end.getY() - start.getY()));
                colorStops.emplace_back (color, interpolated, stop.offset);
            }
        }
    }

    ColorGradient::Type type = (gradient.type == SVGGradient::Linear) ? ColorGradient::Linear : ColorGradient::Radial;
    return ColorGradient (type, colorStops);
}

} // namespace yup
