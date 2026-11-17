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

SVGFilter::Ptr getFilterById (const SVGData& data, const String& id)
{
    return data.filtersById[id];
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

// A <use> without its own paint instantiates the referenced shape with the shape's own styling.
const SVGElement* getInstancedShape (const SVGData& data, const SVGElement& element)
{
    if (! element.reference || element.path)
        return nullptr;

    const bool useDefinesFill = element.fillColor || element.fillCurrentColor || element.fillUrl || element.noFill;
    const bool useDefinesStroke = element.strokeColor || element.strokeCurrentColor || element.strokeUrl || element.noStroke;
    if (useDefinesFill || useDefinesStroke)
        return nullptr;

    const auto refElement = data.elementsById[*element.reference];
    if (refElement == nullptr || ! refElement->path || ! refElement->children.empty())
        return nullptr;

    return refElement.get();
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
    prepareDocument();
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
    prepareDocument();
    return document != nullptr;
}

//==============================================================================

void Drawable::prepareDocument()
{
    if (document == nullptr)
        return;

    document->visit ([this] (SVGData& data)
    {
        std::unordered_set<SVGElement*> preparedElements;

        const auto prepareElements = [this, &data, &preparedElements] (auto& elements)
        {
            for (auto& element : elements)
                if (element != nullptr)
                    prepareElement (data, *element, preparedElements);
        };

        prepareElements (data.elements);

        for (auto& clipPath : data.clipPaths)
            if (clipPath != nullptr)
                prepareElements (clipPath->elements);
        for (auto& mask : data.masks)
            if (mask != nullptr)
                prepareElements (mask->elements);
        for (auto& marker : data.markers)
            if (marker != nullptr)
                prepareElements (marker->elements);
        for (auto& pattern : data.patterns)
            if (pattern != nullptr)
                prepareElements (pattern->elements);
    });
}

void Drawable::prepareElement (SVGData& data, SVGElement& element, std::unordered_set<SVGElement*>& preparedElements)
{
    if (! preparedElements.insert (std::addressof (element)).second)
        return;

    if (element.path.has_value())
    {
        element.preparedPathBounds = element.path->getBounds();

        if (element.strokeDashArray.has_value() && ! element.strokeDashArray->isEmpty())
            element.preparedDashedPath = createDashedPath (*element.path, *element.strokeDashArray, element.strokeDashOffset.value_or (0.0f));

        prepareMarkerPlacements (element);
    }

    prepareTextElement (element);

    if (element.image.has_value() && element.imageBounds.has_value() && element.image->isValid())
    {
        const Rectangle<float> sourceBounds (0.0f,
                                             0.0f,
                                             static_cast<float> (element.image->getWidth()),
                                             static_cast<float> (element.image->getHeight()));
        element.preparedImageBounds = sourceBounds.transformed (
            sourceBounds.getTransformToFit (*element.imageBounds,
                                            element.preserveAspectRatioFitting,
                                            element.preserveAspectRatioJustification));
    }

    const auto prepareGradient = [this, &data, &element] (const std::optional<String>& gradientId,
                                                           std::optional<ColorGradient>& destination)
    {
        if (! gradientId.has_value())
            return;

        const auto gradient = getGradientById (data, *gradientId);
        if (gradient == nullptr)
            return;

        std::optional<Rectangle<float>> bounds;
        if (element.path.has_value())
            bounds = getPathBounds (element);
        else if (element.reference.has_value())
        {
            if (const auto referenced = data.elementsById[*element.reference]; referenced != nullptr && referenced->path.has_value())
                bounds = getPathBounds (*referenced);
        }

        destination = gradient->toColorGradient (bounds ? std::addressof (*bounds) : nullptr);
    };

    prepareGradient (element.fillUrl, element.preparedFillGradient);
    prepareGradient (element.strokeUrl, element.preparedStrokeGradient);

    for (auto& child : element.children)
        if (child != nullptr)
            prepareElement (data, *child, preparedElements);
}

void Drawable::prepareMarkerPlacements (SVGElement& element)
{
    if (! element.path.has_value()
        || (! element.markerStart.has_value() && ! element.markerMid.has_value() && ! element.markerEnd.has_value()))
        return;

    element.preparedMarkers = PathMarkers (*element.path);
}

void Drawable::prepareTextElement (SVGElement& element)
{
    const auto origin = element.getTextOrigin();

    if (! element.text.has_value() || ! origin.has_value() || element.text->isEmpty())
        return;

    auto anchor = AnchoredText::Anchor::start;
    if (element.textAnchor == "middle")
        anchor = AnchoredText::Anchor::middle;
    else if (element.textAnchor == "end")
        anchor = AnchoredText::Anchor::end;

    element.preparedText = AnchoredText (*element.text, resolveFont (element), *origin, anchor, element.letterSpacing.value_or (0.0f));
}

Rectangle<float> Drawable::getPathBounds (const SVGElement& element) const
{
    if (element.preparedPathBounds.has_value())
        return *element.preparedPathBounds;

    return element.path.has_value() ? element.path->getBounds() : Rectangle<float>();
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
            g.setTransform (data.transform.followedBy (g.getTransform()));

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
        auto finalTransform = finalBounds.getTransformToFit (targetArea,
                                                             data.rootHasPreserveAspectRatio ? data.rootPreserveAspectRatioFitting : fitting,
                                                             data.rootHasPreserveAspectRatio ? data.rootPreserveAspectRatioJustification : justification);

        // The drawable is fitted inside the transform the Graphics already has
        if (! finalTransform.isIdentity())
            g.setTransform (finalTransform.followedBy (g.getTransform()));

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
        if (auto filter = getFilterById (data, *element.filterUrl))
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

    if (element.viewBox && (element.viewportBounds || element.viewportSize))
    {
        auto viewport = element.viewportBounds != std::nullopt
                          ? Rectangle<float> (0.0f, 0.0f, element.viewportBounds->getWidth(), element.viewportBounds->getHeight())
                          : Rectangle<float> (0.0f, 0.0f, element.viewportSize->getWidth(), element.viewportSize->getHeight());

        auto viewportTransform = element.viewBox->getTransformToFit (viewport, element.preserveAspectRatioFitting, element.preserveAspectRatioJustification);
        if (element.tagName == "svg" && element.viewportBounds)
        {
            g.setClipPath (*element.viewportBounds);
            viewportTransform = viewportTransform.followedBy (AffineTransform::translation (element.viewportBounds->getX(), element.viewportBounds->getY()));
        }

        if (! viewportTransform.isIdentity())
            g.setTransform (viewportTransform.followedBy (g.getTransform()));
    }
    else if (element.tagName == "svg" && element.viewportBounds)
    {
        g.setClipPath (*element.viewportBounds);
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
                    clipObjectBounds = getPathBounds (element);
                else if (element.reference)
                {
                    if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
                        clipObjectBounds = getPathBounds (*refElement);
                }
                else if (element.imageBounds)
                {
                    clipObjectBounds = *element.imageBounds;
                }
            }

            const auto clipBounds = clipObjectBounds.value_or (Rectangle<float>());

            // If the clipPath itself has a nested clip-path, apply it first (intersection)
            if (clipPath->clipPathUrl)
            {
                if (auto nestedClipPath = getClipPathById (data, *clipPath->clipPathUrl))
                {
                    auto nestedClipShape = buildClipShape (*nestedClipPath, clipBounds);
                    if (! nestedClipShape.isEmpty())
                    {
                        g.setClipPath (nestedClipShape);
                        hasClipping = true;
                    }
                }
            }

            // Apply the clipPath's own elements (intersects with nested clip if present)
            auto clipShape = buildClipShape (*clipPath, clipBounds);
            if (! clipShape.isEmpty())
            {
                g.setClipPath (clipShape);
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
                    maskObjectBounds = getPathBounds (element);
                else if (element.reference)
                {
                    if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
                        maskObjectBounds = getPathBounds (*refElement);
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
                g.setClipPath (combinedMaskPath);
        }
    }

    const auto instancedShape = getInstancedShape (data, element);

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
        if (element.preparedFillGradient.has_value())
        {
            g.setFillColorGradient (*element.preparedFillGradient);
            isFillDefined = true;
        }
        else if (auto gradient = getGradientById (data, *element.fillUrl))
        {
            auto resolvedGradient = gradient;
            std::optional<Rectangle<float>> gradientBounds;

            if (element.path)
                gradientBounds = getPathBounds (element);
            else if (element.reference)
            {
                if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
                    gradientBounds = getPathBounds (*refElement);
            }

            ColorGradient colorGradient = resolvedGradient->toColorGradient (gradientBounds ? std::addressof (*gradientBounds) : nullptr);

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
                                                             << " bounds: " << getPathBounds (element).toString()
                                                             << " clip: " << (hasClipping ? "true" : "false"));
                    g.fillPath (*element.path);
                }
            }
            else
            {
                YUP_DRAWABLE_LOG ("Filling path - tag: " << element.tagName
                                                         << " id: " << (element.id ? *element.id : "none")
                                                         << " bounds: " << getPathBounds (element).toString()
                                                         << " clip: " << (hasClipping ? "true" : "false"));
                g.fillPath (*element.path);
            }
        }
        else if (element.reference && instancedShape == nullptr)
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
        if (element.preparedStrokeGradient.has_value())
        {
            g.setStrokeColorGradient (*element.preparedStrokeGradient);
            isStrokeDefined = true;
        }
        else if (auto gradient = getGradientById (data, *element.strokeUrl))
        {
            auto resolvedGradient = gradient;
            std::optional<Rectangle<float>> gradientBounds;

            if (element.path)
                gradientBounds = getPathBounds (element);
            else if (element.reference)
            {
                if (auto refElement = data.elementsById[*element.reference]; refElement != nullptr && refElement->path)
                    gradientBounds = getPathBounds (*refElement);
            }

            ColorGradient colorGradient = resolvedGradient->toColorGradient (gradientBounds ? std::addressof (*gradientBounds) : nullptr);
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
    g.setStrokeMiterLimit (element.strokeMiterLimit.value_or (4.0f));

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
            if (element.strokeDashArray.has_value() && element.preparedDashedPath.has_value())
            {
                pathToStroke = std::addressof (*element.preparedDashedPath);
            }
            else
            {
                dashedPath = createDashedPath (*pathToStroke, *currentStrokeDashArray, currentStrokeDashOffset);
                pathToStroke = std::addressof (*dashedPath);
            }
        }

        if (pathToStroke != nullptr)
        {
            YUP_DRAWABLE_LOG ("Stroking path - tag: " << element.tagName
                                                      << " id: " << (element.id ? *element.id : "none")
                                                      << " bounds: " << pathToStroke->getBounds().toString());
            g.strokePath (*pathToStroke);
        }
        else if (element.reference && instancedShape == nullptr)
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
                    if (refElement->strokeDashArray.has_value() && refElement->preparedDashedPath.has_value())
                    {
                        referencePathToStroke = std::addressof (*refElement->preparedDashedPath);
                    }
                    else
                    {
                        dashedReferencePath = createDashedPath (*referencePathToStroke, *referenceStrokeDashArray, referenceStrokeDashOffset);
                        referencePathToStroke = std::addressof (*dashedReferencePath);
                    }
                }

                g.strokePath (*referencePathToStroke);

                if (refElement->localTransform)
                    g.setTransform (savedTransform);
            }
        }
    }

    if (element.path && (element.markerStart || element.markerMid || element.markerEnd))
    {
        const float sw = element.strokeWidth.value_or (1.0f);

        if (element.markerStart)
        {
            if (auto marker = getMarkerById (data, *element.markerStart))
            {
                for (auto placement : element.preparedMarkers.getStarts())
                {
                    if (marker->orientAutoStartReverse)
                        placement.angle += MathConstants<float>::pi;

                    paintMarker (g, data, *marker, sw, placement, visitingElements, recursionDepth);
                }
            }
        }

        if (element.markerMid)
        {
            if (auto marker = getMarkerById (data, *element.markerMid))
            {
                for (const auto& placement : element.preparedMarkers.getMids())
                    paintMarker (g, data, *marker, sw, placement, visitingElements, recursionDepth);
            }
        }

        if (element.markerEnd)
        {
            if (auto marker = getMarkerById (data, *element.markerEnd))
            {
                for (const auto& placement : element.preparedMarkers.getEnds())
                    paintMarker (g, data, *marker, sw, placement, visitingElements, recursionDepth);
            }
        }
    }

    if (instancedShape != nullptr)
    {
        paintElement (g, data, *instancedShape, isFillDefined && ! element.noFill, isStrokeDefined && ! element.noStroke, currentColor, visitingElements, currentStrokeDashArray, currentStrokeDashOffset, recursionDepth + 1);
    }
    else if (element.reference)
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
                    auto viewBoxTransform = refElement->viewBox->getTransformToFit (viewport, refElement->preserveAspectRatioFitting, refElement->preserveAspectRatioJustification);
                    if (! viewBoxTransform.isIdentity())
                        g.setTransform (viewBoxTransform.followedBy (g.getTransform()));
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
                            const PathMarkers::Placement& placement,
                            std::unordered_set<const SVGElement*>& visitingElements,
                            int recursionDepth)
{
    const auto savedState = g.saveState();

    PathMarkers::Shape shape;
    shape.reference = { marker.refX, marker.refY };
    shape.size = { marker.markerWidth, marker.markerHeight };
    shape.viewBox = marker.viewBox;
    shape.scalesWithStrokeWidth = marker.markerUnits == SVGMarker::StrokeWidth;

    if (marker.orient)
        shape.angle = degreesToRadians (*marker.orient);

    g.setTransform (PathMarkers::getTransform (placement, shape, strokeWidth).followedBy (g.getTransform()));

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
    g.setClipPath (shape);

    // The tiles are placed in the coordinates of the shape: each transform goes inside the ones before it
    if (! pattern.patternTransform.isIdentity())
        g.setTransform (pattern.patternTransform.followedBy (g.getTransform()));

    const auto shapeBounds = shape.getBounds();
    const float startX = std::floor ((shapeBounds.getX() - originX) / tileW) * tileW + originX;
    const float startY = std::floor ((shapeBounds.getY() - originY) / tileH) * tileH + originY;

    for (float tileY = startY; tileY < shapeBounds.getBottom(); tileY += tileH)
    {
        for (float tileX = startX; tileX < shapeBounds.getRight(); tileX += tileW)
        {
            const auto savedTileState = g.saveState();

            g.setTransform (AffineTransform::translation (tileX, tileY).followedBy (g.getTransform()));

            if (pattern.viewBox
                && pattern.viewBox->getWidth() > 0.0f
                && pattern.viewBox->getHeight() > 0.0f)
            {
                const float scaleX = tileW / pattern.viewBox->getWidth();
                const float scaleY = tileH / pattern.viewBox->getHeight();
                g.setTransform (AffineTransform::scaling (scaleX, scaleY)
                                    .translated (-pattern.viewBox->getX(), -pattern.viewBox->getY())
                                    .followedBy (g.getTransform()));
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
    if (element.preparedText.isEmpty())
        return;

    g.fillFittedText (element.preparedText.getStyledText(), element.preparedText.getBounds());
}

//==============================================================================

void Drawable::renderImageElement (Graphics& g, const SVGElement& element)
{
    if (! element.image.has_value()
        || ! element.image->isValid()
        || ! element.imageBounds.has_value()
        || ! element.preparedImageBounds.has_value())
        return;

    const auto savedState = g.saveState();
    g.setClipPath (*element.imageBounds);
    g.drawImage (*element.image, *element.preparedImageBounds);
}

//==============================================================================

Path Drawable::createDashedPath (const Path& source, const Array<float>& dashArray, float dashOffset) const
{
    return source.createDashed ({ dashArray.getRawDataPointer(), static_cast<size_t> (dashArray.size()) }, dashOffset);
}

} // namespace yup
