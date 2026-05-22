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
#define YUP_DRAWABLE_LOGGING 1
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
    YUP_DRAWABLE_LOG ("parseSVG(file, options) - file: " << svgFile.getFullPathName()
                                                         << " baseDirectory: " << options.baseDirectory.getFullPathName()
                                                         << " allowDataImages: " << (options.allowDataImages ? "true" : "false")
                                                         << " allowLocalImages: " << (options.allowLocalImages ? "true" : "false")
                                                         << " hasImageResolver: " << (options.imageResolver ? "true" : "false")
                                                         << " hasFontResolver: " << (options.fontResolver ? "true" : "false"));

    clear();

    parseOptions = options;
    if (parseOptions.baseDirectory.getFullPathName().isEmpty())
        parseOptions.baseDirectory = svgFile.getParentDirectory();

    XmlDocument svgDoc (svgFile);
    std::unique_ptr<XmlElement> svgRoot (svgDoc.getDocumentElement());

    return parseDocument (std::move (svgRoot));
}

bool Drawable::parseSVG (StringRef svgText, const ParseOptions& options)
{
    YUP_DRAWABLE_LOG ("parseSVG(text, options) - length: " << String (svgText.text).length()
                                                           << " baseDirectory: " << options.baseDirectory.getFullPathName()
                                                           << " allowDataImages: " << (options.allowDataImages ? "true" : "false")
                                                           << " allowLocalImages: " << (options.allowLocalImages ? "true" : "false")
                                                           << " hasImageResolver: " << (options.imageResolver ? "true" : "false")
                                                           << " hasFontResolver: " << (options.fontResolver ? "true" : "false"));

    clear();

    parseOptions = options;

    XmlDocument svgDoc (String (svgText.text));
    std::unique_ptr<XmlElement> svgRoot (svgDoc.getDocumentElement());

    return parseDocument (std::move (svgRoot));
}

bool Drawable::parseDocument (std::unique_ptr<XmlElement> svgRoot)
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
            viewBox.setX (coords.getReference (0).getFloatValue());
            viewBox.setY (coords.getReference (1).getFloatValue());
            viewBox.setWidth (coords.getReference (2).getFloatValue());
            viewBox.setHeight (coords.getReference (3).getFloatValue());
        }
        else
        {
            YUP_DRAWABLE_LOG ("Invalid root viewBox - expected 4 coordinates, got: " << coords.size() << " value: " << view);
        }
    }

    auto width = svgRoot->getFloatAttribute ("width");
    size.setWidth (width == 0.0f ? viewBox.getWidth() : width);

    auto height = svgRoot->getFloatAttribute ("height");
    size.setHeight (height == 0.0f ? viewBox.getHeight() : height);

    // ViewBox transform is now calculated at render-time based on actual target area
    YUP_DRAWABLE_LOG ("Parse complete - viewBox: " << viewBox.toString() << " size: " << size.getWidth() << "x" << size.getHeight());

    std::function<void (const XmlElement&)> collectStyleElements = [&] (const XmlElement& xml)
    {
        if (xml.hasTagName ("style"))
            parseStyleElement (xml);

        for (auto* child = xml.getFirstChildElement(); child != nullptr; child = child->getNextElement())
            collectStyleElements (*child);
    };

    collectStyleElements (*svgRoot);

    auto result = parseElement (*svgRoot, true, {});

    if (result)
    {
        bounds = calculateBounds();
        YUP_DRAWABLE_LOG ("parseDocument result - success: true"
                          << " topLevelElements: " << elements.size()
                          << " ids: " << elementsById.size()
                          << " gradients: " << gradients.size()
                          << " filters: " << filters.size()
                          << " clipPaths: " << clipPaths.size()
                          << " cssRules: " << cssRules.size()
                          << " bounds: " << bounds.toString()
                          << " rootHasFill: " << (rootHasFill ? "true" : "false")
                          << " rootHasStroke: " << (rootHasStroke ? "true" : "false")
                          << " rootFillColor: " << (rootFillColor ? rootFillColor->toString() : "none")
                          << " rootStrokeColor: " << (rootStrokeColor ? rootStrokeColor->toString() : "none"));
    }
    else
    {
        YUP_DRAWABLE_LOG ("parseDocument result - success: false");
    }

    return result;
}

//==============================================================================

void Drawable::clear()
{
    YUP_DRAWABLE_LOG ("clear - previous topLevelElements: " << elements.size()
                                                            << " ids: " << elementsById.size()
                                                            << " gradients: " << gradients.size()
                                                            << " filters: " << filters.size()
                                                            << " clipPaths: " << clipPaths.size()
                                                            << " cssRules: " << cssRules.size());

    viewBox = { 0.0f, 0.0f, 0.0f, 0.0f };
    size = { 0.0f, 0.0f };
    bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
    transform = AffineTransform::identity();

    elements.clear();
    elementsById.clear();
    gradients.clear();
    gradientsById.clear();
    filters.clear();
    filtersById.clear();
    clipPaths.clear();
    clipPathsById.clear();
    cssRules.clear();

    // Reset root element's default presentation attributes to SVG defaults
    rootHasFill = true;    // SVG default fill is black
    rootHasStroke = false; // SVG default stroke is none
    rootFillColor = std::nullopt;
    rootStrokeColor = std::nullopt;
}

//==============================================================================

Rectangle<float> Drawable::getBounds() const
{
    return bounds;
}

//==============================================================================

void Drawable::paint (Graphics& g)
{
    YUP_DRAWABLE_LOG ("paint - bounds: " << bounds.toString()
                                         << " viewBox: " << viewBox.toString()
                                         << " size: " << size.getWidth() << "x" << size.getHeight()
                                         << " topLevelElements: " << elements.size()
                                         << " drawableTransform: " << transform.toString()
                                         << " graphicsTransform: " << g.getTransform().toString()
                                         << " rootHasFill: " << (rootHasFill ? "true" : "false")
                                         << " rootHasStroke: " << (rootHasStroke ? "true" : "false"));

    const auto savedState = g.saveState();

    g.setStrokeWidth (1.0f);

    // Set default fill color based on root SVG element or SVG default (black)
    if (rootFillColor)
        g.setFillColor (*rootFillColor);
    else
        g.setFillColor (Colors::black);

    // Set default stroke color if root SVG element specified one
    if (rootStrokeColor)
        g.setStrokeColor (*rootStrokeColor);

    if (! transform.isIdentity())
    {
        YUP_DRAWABLE_LOG ("paint - applying drawable transform: " << transform.toString());
        g.addTransform (transform);
    }

    // Pass root element's fill/stroke state to top-level elements
    for (const auto& element : elements)
        paintElement (g, *element, rootHasFill, rootHasStroke, rootFillColor.value_or (Colors::black));
}

void Drawable::paint (Graphics& g, const Rectangle<float>& targetArea, Fitting fitting, Justification justification)
{
    YUP_DRAWABLE_LOG ("Fitted paint called - bounds: " << bounds.toString() << " targetArea: " << targetArea.toString());

    if (bounds.isEmpty())
    {
        YUP_DRAWABLE_LOG ("Fitted paint skipped - drawable bounds are empty");
        return;
    }

    const auto savedState = g.saveState();

    auto finalBounds = viewBox.isEmpty() ? bounds : viewBox;
    auto finalTransform = calculateTransformForTarget (finalBounds, targetArea, fitting, justification);
    YUP_DRAWABLE_LOG ("Fitted paint transform - sourceBounds: " << finalBounds.toString()
                                                                << " transform: " << finalTransform.toString()
                                                                << " graphicsTransformBefore: " << g.getTransform().toString()
                                                                << " topLevelElements: " << elements.size());
    if (! finalTransform.isIdentity())
        g.addTransform (finalTransform);

    g.setStrokeWidth (1.0f);

    // Set default fill color based on root SVG element or SVG default (black)
    if (rootFillColor)
        g.setFillColor (*rootFillColor);
    else
        g.setFillColor (Colors::black);

    // Set default stroke color if root SVG element specified one
    if (rootStrokeColor)
        g.setStrokeColor (*rootStrokeColor);

    // Pass root element's fill/stroke state to top-level elements
    for (const auto& element : elements)
        paintElement (g, *element, rootHasFill, rootHasStroke, rootFillColor.value_or (Colors::black));
}

//==============================================================================

void Drawable::paintElement (Graphics& g, const Element& element, bool hasParentFillEnabled, bool hasParentStrokeEnabled, Color currentColor, int recursionDepth)
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

    const auto transformBeforeElement = g.getTransform();

    YUP_DRAWABLE_LOG ("paintElement - tag: " << element.tagName
                                             << " id: " << (element.id ? *element.id : "none")
                                             << " depth: " << recursionDepth
                                             << " hasPath: " << (element.path ? "true" : "false")
                                             << " pathBounds: " << (element.path ? element.path->getBounds().toString() : "none")
                                             << " hasTransform: " << (element.transform ? "true" : "false")
                                             << " hasReference: " << (element.reference ? "true" : "false")
                                             << " reference: " << (element.reference ? *element.reference : "none")
                                             << " filter: " << element.filterUrl.value_or (String ("none"))
                                             << " children: " << element.children.size()
                                             << " parentFill: " << (hasParentFillEnabled ? "true" : "false")
                                             << " parentStroke: " << (hasParentStrokeEnabled ? "true" : "false")
                                             << " noFill: " << (element.noFill ? "true" : "false")
                                             << " noStroke: " << (element.noStroke ? "true" : "false")
                                             << " graphicsTransform: " << g.getTransform().toString()
                                             << " opacity: " << g.getOpacity());

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
    {
        YUP_DRAWABLE_LOG ("Applying opacity - tag: " << element.tagName
                                                     << " elementOpacity: " << *element.opacity
                                                     << " graphicsOpacityBefore: " << g.getOpacity());
        g.setOpacity (g.getOpacity() * (*element.opacity));
        YUP_DRAWABLE_LOG ("After opacity - graphicsOpacity: " << g.getOpacity());
    }

    if (element.filterUrl)
    {
        YUP_DRAWABLE_LOG ("Resolving filter - tag: " << element.tagName << " filter: " << *element.filterUrl);

        if (auto filter = resolveFilter (getFilterById (*element.filterUrl)))
        {
            if (filter->gaussianBlurStdDeviation)
            {
                const auto feather = jmax (g.getFeather(), *filter->gaussianBlurStdDeviation);
                YUP_DRAWABLE_LOG ("Applying GaussianBlur filter as feather - id: " << *element.filterUrl
                                                                                   << " stdDeviation: " << *filter->gaussianBlurStdDeviation
                                                                                   << " previousFeather: " << g.getFeather()
                                                                                   << " appliedFeather: " << feather);
                g.setFeather (feather);
            }
            else
            {
                YUP_DRAWABLE_LOG ("Filter resolved without supported primitives - id: " << *element.filterUrl);
            }
        }
        else
        {
            YUP_DRAWABLE_LOG ("Filter not found - id: " << *element.filterUrl);
        }
    }

    if (element.viewBox && element.viewportSize)
    {
        Rectangle<float> viewport (0.0f, 0.0f, element.viewportSize->getWidth(), element.viewportSize->getHeight());
        auto viewBoxTransform = calculateTransformForTarget (*element.viewBox, viewport, element.preserveAspectRatioFitting, element.preserveAspectRatioJustification);
        YUP_DRAWABLE_LOG ("Applying nested viewBox - tag: " << element.tagName
                                                            << " viewBox: " << element.viewBox->toString()
                                                            << " viewport: " << viewport.toString()
                                                            << " transform: " << viewBoxTransform.toString());
        if (! viewBoxTransform.isIdentity())
            g.addTransform (viewBoxTransform);
    }

    // Apply clipping path if specified
    bool hasClipping = false;
    if (element.clipPathUrl)
    {
        YUP_DRAWABLE_LOG ("Resolving clip path - tag: " << element.tagName << " clipPath: " << *element.clipPathUrl);

        if (auto clipPath = getClipPathById (*element.clipPathUrl))
        {
            std::optional<Rectangle<float>> clipObjectBounds;

            if (clipPath->units == ClipPath::ObjectBoundingBox)
            {
                if (element.path)
                    clipObjectBounds = element.path->getBounds();
                else if (element.reference)
                {
                    if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
                        clipObjectBounds = refElement->path->getBounds();
                }
                else if (element.imageBounds)
                {
                    clipObjectBounds = *element.imageBounds;
                }

                YUP_DRAWABLE_LOG ("Clip path uses objectBoundingBox units - id: " << *element.clipPathUrl
                                                                                  << " hasObjectBounds: " << (clipObjectBounds ? "true" : "false")
                                                                                  << " objectBounds: " << (clipObjectBounds ? clipObjectBounds->toString() : String ("none")));
            }

            // Create a combined path from all clip path elements
            Path combinedClipPath;
            bool clipUsesNonZeroWinding = true;

            for (const auto& clipElement : clipPath->elements)
            {
                if (clipElement->path)
                {
                    if (! clipElement->path->isUsingNonZeroWinding())
                        clipUsesNonZeroWinding = false;

                    AffineTransform clipTransform = clipElement->transform.value_or (AffineTransform::identity());

                    if (clipPath->units == ClipPath::ObjectBoundingBox)
                    {
                        if (! clipObjectBounds || clipObjectBounds->isEmpty())
                        {
                            YUP_DRAWABLE_LOG ("Skipping objectBoundingBox clip path without bounds - id: " << *element.clipPathUrl);
                            continue;
                        }

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

                const auto transformDuringClip = g.getTransform();
                if (element.transform)
                {
                    YUP_DRAWABLE_LOG ("Applying clip path in pre-element transform space - id: " << *element.clipPathUrl
                                                                                                 << " clipTransform: " << transformBeforeElement.toString()
                                                                                                 << " elementTransform: " << element.transform->toString()
                                                                                                 << " drawTransform: " << transformDuringClip.toString());
                    g.setTransform (transformBeforeElement);
                }

                g.setClipPath (combinedClipPath);

                if (element.transform)
                    g.setTransform (transformDuringClip);

                hasClipping = true;
                YUP_DRAWABLE_LOG ("Applied clip path - id: " << *element.clipPathUrl
                                                             << " elements: " << clipPath->elements.size()
                                                             << " fillRule: " << (clipUsesNonZeroWinding ? "nonzero" : "evenodd")
                                                             << " bounds: " << combinedClipPath.getBounds().toString());
            }
            else
            {
                YUP_DRAWABLE_LOG ("Clip path resolved but empty - id: " << *element.clipPathUrl
                                                                        << " elements: " << clipPath->elements.size());
            }
        }
        else
        {
            YUP_DRAWABLE_LOG ("Clip path not found - id: " << *element.clipPathUrl);
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
        YUP_DRAWABLE_LOG ("Fill color selected - tag: " << element.tagName << " color: " << fillColor.toString());
    }
    else if (element.fillCurrentColor)
    {
        Color fillColor = currentColor;
        if (element.fillOpacity)
            fillColor = fillColor.withMultipliedAlpha (*element.fillOpacity);
        g.setFillColor (fillColor);
        isFillDefined = true;
        YUP_DRAWABLE_LOG ("Fill currentColor selected - tag: " << element.tagName << " color: " << fillColor.toString());
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
        YUP_DRAWABLE_LOG ("Fill inherited - tag: " << element.tagName);
    }
    else
    {
        YUP_DRAWABLE_LOG ("No fill selected - tag: " << element.tagName);
    }

    if (isFillDefined && ! element.noFill)
    {
        if (element.path)
        {
            // SVG spec: fill is applied to both closed and unclosed paths
            // For unclosed paths, an implicit line connects the last point to the first point
            // TODO: Apply fill-rule when Graphics class supports it
            // if (element.fillRule)
            //     g.setFillRule (*element.fillRule == "evenodd" ? FillRule::EvenOdd : FillRule::NonZero);
            YUP_DRAWABLE_LOG ("Filling path - tag: " << element.tagName
                                                     << " id: " << (element.id ? *element.id : "none")
                                                     << " bounds: " << element.path->getBounds().toString()
                                                     << " clip: " << (hasClipping ? "true" : "false"));
            g.fillPath (*element.path);
        }
        else if (element.reference)
        {
            if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
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

                    YUP_DRAWABLE_LOG ("Rendering use element - reference: " << *element.reference);
                    YUP_DRAWABLE_LOG ("Use element transform: " << (element.transform ? element.transform->toString() : "none"));
                    YUP_DRAWABLE_LOG ("Referenced element local transform: " << (refElement->localTransform ? refElement->localTransform->toString() : "none"));
                    YUP_DRAWABLE_LOG ("Graphics transform during use fill: " << g.getTransform().toString());

                    // For <use> elements, apply only the referenced element's own transform (not inherited parents)
                    const auto savedTransform = g.getTransform();
                    if (refElement->localTransform)
                        g.setTransform (refElement->localTransform->followedBy (savedTransform));

                    // SVG spec: fill is applied to both closed and unclosed paths
                    g.fillPath (*refElement->path);

                    if (refElement->localTransform)
                        g.setTransform (savedTransform);
                }
                else
                {
                    YUP_DRAWABLE_LOG ("Use fill skipped - reference disables fill: " << *element.reference);
                }
            }
            else
            {
                YUP_DRAWABLE_LOG ("Use fill skipped - missing reference/path: " << *element.reference);
            }
        }
        else if (element.text && element.textPosition)
        {
            YUP_DRAWABLE_LOG ("Rendering text - tag: " << element.tagName
                                                       << " textLength: " << element.text->length()
                                                       << " position: " << element.textPosition->toString());
            renderTextElement (g, element);
        }
        else if ((element.imageHref || element.image) && element.imageBounds)
        {
            YUP_DRAWABLE_LOG ("Rendering image - tag: " << element.tagName
                                                        << " href: " << (element.imageHref ? *element.imageHref : "embedded")
                                                        << " bounds: " << element.imageBounds->toString()
                                                        << " alreadyLoaded: " << (element.image ? "true" : "false"));
            renderImageElement (g, element);
        }
        else
        {
            YUP_DRAWABLE_LOG ("Fill branch had nothing to render - tag: " << element.tagName
                                                                          << " hasText: " << (element.text ? "true" : "false")
                                                                          << " hasImage: " << ((element.imageHref || element.image) ? "true" : "false"));
        }
    }
    else
    {
        YUP_DRAWABLE_LOG ("Fill skipped - tag: " << element.tagName
                                                 << " isFillDefined: " << (isFillDefined ? "true" : "false")
                                                 << " noFill: " << (element.noFill ? "true" : "false"));
    }

    // Setup stroke
    if (element.strokeColor)
    {
        Color strokeColor = *element.strokeColor;
        if (element.strokeOpacity)
            strokeColor = strokeColor.withMultipliedAlpha (*element.strokeOpacity);
        g.setStrokeColor (strokeColor);
        isStrokeDefined = true;
        YUP_DRAWABLE_LOG ("Stroke color selected - tag: " << element.tagName << " color: " << strokeColor.toString());
    }
    else if (element.strokeCurrentColor)
    {
        Color strokeColor = currentColor;
        if (element.strokeOpacity)
            strokeColor = strokeColor.withMultipliedAlpha (*element.strokeOpacity);
        g.setStrokeColor (strokeColor);
        isStrokeDefined = true;
        YUP_DRAWABLE_LOG ("Stroke currentColor selected - tag: " << element.tagName << " color: " << strokeColor.toString());
    }
    else if (element.strokeUrl)
    {
        YUP_DRAWABLE_LOG ("Looking for stroke gradient with ID: " << *element.strokeUrl);
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
            YUP_DRAWABLE_LOG ("Applied gradient to stroke");
        }
        else
        {
            YUP_DRAWABLE_LOG ("Stroke gradient not found for ID: " << *element.strokeUrl);
        }
    }
    else if (hasParentStrokeEnabled)
    {
        // Inherit parent stroke - don't change graphics state, just mark as defined
        isStrokeDefined = true;
        YUP_DRAWABLE_LOG ("Stroke inherited - tag: " << element.tagName);
    }
    else
    {
        YUP_DRAWABLE_LOG ("No stroke selected - tag: " << element.tagName);
    }

    if (element.strokeJoin)
    {
        YUP_DRAWABLE_LOG ("Applying stroke join - tag: " << element.tagName);
        g.setStrokeJoin (*element.strokeJoin);
    }

    if (element.strokeCap)
    {
        YUP_DRAWABLE_LOG ("Applying stroke cap - tag: " << element.tagName);
        g.setStrokeCap (*element.strokeCap);
    }

    if (element.strokeWidth)
    {
        YUP_DRAWABLE_LOG ("Applying stroke width - tag: " << element.tagName << " width: " << *element.strokeWidth);
        g.setStrokeWidth (*element.strokeWidth);
    }

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
            YUP_DRAWABLE_LOG ("Stroke dash array parsed - tag: " << element.tagName
                                                                 << " count: " << dashArray.size()
                                                                 << " offset: " << element.strokeDashOffset.value_or (0.0f));
        }
    }

    bool referenceDefinesStroke = false;
    if (! isStrokeDefined && element.reference)
    {
        if (auto refElement = elementsById[*element.reference]; refElement != nullptr)
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
            YUP_DRAWABLE_LOG ("Created dashed stroke path - tag: " << element.tagName
                                                                   << " sourceBounds: " << element.path->getBounds().toString()
                                                                   << " dashedBounds: " << dashedPath->getBounds().toString());
        }

        if (pathToStroke != nullptr)
        {
            YUP_DRAWABLE_LOG ("Stroking path - tag: " << element.tagName
                                                      << " id: " << (element.id ? *element.id : "none")
                                                      << " bounds: " << pathToStroke->getBounds().toString()
                                                      << " referenceDefinesStroke: " << (referenceDefinesStroke ? "true" : "false"));
            g.strokePath (*pathToStroke);
        }
        else if (element.reference)
        {
            if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
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
            else
            {
                YUP_DRAWABLE_LOG ("Use stroke skipped - missing reference/path: " << *element.reference);
            }
        }
        else
        {
            YUP_DRAWABLE_LOG ("Stroke branch had no path/reference - tag: " << element.tagName);
        }
    }
    else
    {
        YUP_DRAWABLE_LOG ("Stroke skipped - tag: " << element.tagName
                                                   << " isStrokeDefined: " << (isStrokeDefined ? "true" : "false")
                                                   << " referenceDefinesStroke: " << (referenceDefinesStroke ? "true" : "false")
                                                   << " noStroke: " << (element.noStroke ? "true" : "false"));
    }

    if (element.reference)
    {
        if (auto refElement = elementsById[*element.reference]; refElement != nullptr && ! refElement->children.empty())
        {
            const auto savedTransform = g.getTransform();
            if (refElement->localTransform)
                g.setTransform (refElement->localTransform->followedBy (savedTransform));

            if (refElement->viewBox)
            {
                auto viewportSizeToUse = element.viewportSize.value_or (refElement->viewportSize.value_or (Size<float> (refElement->viewBox->getWidth(), refElement->viewBox->getHeight())));
                Rectangle<float> viewport (0.0f, 0.0f, viewportSizeToUse.getWidth(), viewportSizeToUse.getHeight());
                auto viewBoxTransform = calculateTransformForTarget (*refElement->viewBox, viewport, refElement->preserveAspectRatioFitting, refElement->preserveAspectRatioJustification);
                YUP_DRAWABLE_LOG ("Applying referenced viewBox - reference: " << *element.reference
                                                                              << " viewBox: " << refElement->viewBox->toString()
                                                                              << " viewport: " << viewport.toString()
                                                                              << " transform: " << viewBoxTransform.toString());
                if (! viewBoxTransform.isIdentity())
                    g.addTransform (viewBoxTransform);
            }

            YUP_DRAWABLE_LOG ("Rendering referenced children - reference: " << *element.reference
                                                                            << " childCount: " << refElement->children.size()
                                                                            << " transform: " << g.getTransform().toString());

            for (const auto& childElement : refElement->children)
                paintElement (g, *childElement, isFillDefined && ! element.noFill, isStrokeDefined && ! element.noStroke, currentColor, recursionDepth + 1);

            g.setTransform (savedTransform);
        }
        else
        {
            YUP_DRAWABLE_LOG ("No referenced children rendered - reference: " << *element.reference);
        }
    }

    for (const auto& childElement : element.children)
    {
        YUP_DRAWABLE_LOG ("Rendering child element - current graphics transform: " << g.getTransform().toString());
        // Pass fill/stroke state to children, but respect explicit "none" values
        // If this element has fill="none", children should not inherit fill
        paintElement (g, *childElement, isFillDefined && ! element.noFill, isStrokeDefined && ! element.noStroke, currentColor, recursionDepth + 1);
    }

    // paintDebugElement (g, element);
}

//==============================================================================

bool Drawable::parseElement (const XmlElement& element, bool parentIsRoot, AffineTransform currentTransform, Element* parent)
{
    Element::Ptr e = new Element;
    bool isRootElement = element.hasTagName ("svg");
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
        elementsById.set (id, e);
    }

    const float inheritedFontSize = parent != nullptr && parent->fontSize ? *parent->fontSize : 12.0f;
    const float viewportWidth = viewBox.getWidth() > 0.0f ? viewBox.getWidth() : (size.getWidth() > 0.0f ? size.getWidth() : 100.0f);
    const float viewportHeight = viewBox.getHeight() > 0.0f ? viewBox.getHeight() : (size.getHeight() > 0.0f ? size.getHeight() : 100.0f);
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

        // Apply fill-rule after parsing style
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

        // Handle x,y positioning for use elements (SVG spec requirement)
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
        auto cx = parseLengthAttribute (element, "cx", 0.0f, inheritedFontSize, viewportWidth);
        auto cy = parseLengthAttribute (element, "cy", 0.0f, inheritedFontSize, viewportHeight);
        auto rx = parseLengthAttribute (element, "rx", 0.0f, inheritedFontSize, viewportDiagonal);
        auto ry = parseLengthAttribute (element, "ry", 0.0f, inheritedFontSize, viewportDiagonal);

        auto path = Path();
        path.addCenteredEllipse (cx, cy, rx, ry);
        e->path = std::move (path);

        YUP_DRAWABLE_LOG ("Parsed ellipse - id: " << element.getStringAttribute ("id", "none")
                                                  << " cx: " << cx
                                                  << " cy: " << cy
                                                  << " rx: " << rx
                                                  << " ry: " << ry
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
                                                 << " cx: " << cx
                                                 << " cy: " << cy
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
                                               << " x: " << x
                                               << " y: " << y
                                               << " width: " << width
                                               << " height: " << height
                                               << " rx: " << rx
                                               << " ry: " << ry
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
                                               << " x1: " << x1
                                               << " y1: " << y1
                                               << " x2: " << x2
                                               << " y2: " << y2
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
        YUP_DRAWABLE_LOG ("Parsing text font inputs - tag: " << e->tagName
                                                             << " id: " << element.getStringAttribute ("id", "none")
                                                             << " inheritedFontSize: " << inheritedFontSize
                                                             << " parentFontFamily: " << (parent != nullptr && parent->fontFamily ? *parent->fontFamily : "none")
                                                             << " parentFontSize: " << (parent != nullptr && parent->fontSize ? String (*parent->fontSize) : String ("none"))
                                                             << " rawFontFamily: " << element.getStringAttribute ("font-family")
                                                             << " rawFontSize: " << element.getStringAttribute ("font-size")
                                                             << " rawFontWeight: " << element.getStringAttribute ("font-weight")
                                                             << " rawFontStyle: " << element.getStringAttribute ("font-style")
                                                             << " rawFontStretch: " << element.getStringAttribute ("font-stretch")
                                                             << " rawTextAnchor: " << element.getStringAttribute ("text-anchor")
                                                             << " rawDominantBaseline: " << element.getStringAttribute ("dominant-baseline")
                                                             << " rawAlignmentBaseline: " << element.getStringAttribute ("alignment-baseline")
                                                             << " rawBaselineShift: " << element.getStringAttribute ("baseline-shift"));

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
                                                << " rawTextLength: " << text.length()
                                                << " rawText: " << text
                                                << " textLength: " << (e->text ? e->text->length() : 0)
                                                << " text: " << (e->text ? *e->text : String())
                                                << " position: " << e->textPosition->toString());

        if (auto xList = element.getStringAttribute ("x"); xList.isNotEmpty())
        {
            e->textX = parseLengthList (xList, inheritedFontSize, viewportWidth);
            YUP_DRAWABLE_LOG ("Parsed text x list - tag: " << e->tagName << " count: " << e->textX->size() << " first: " << (e->textX->isEmpty() ? 0.0f : e->textX->getFirst()));
        }
        if (auto yList = element.getStringAttribute ("y"); yList.isNotEmpty())
        {
            e->textY = parseLengthList (yList, inheritedFontSize, viewportHeight);
            YUP_DRAWABLE_LOG ("Parsed text y list - tag: " << e->tagName << " count: " << e->textY->size() << " first: " << (e->textY->isEmpty() ? 0.0f : e->textY->getFirst()));
        }
        if (auto dxList = element.getStringAttribute ("dx"); dxList.isNotEmpty())
        {
            e->textDx = parseLengthList (dxList, inheritedFontSize, viewportWidth);
            YUP_DRAWABLE_LOG ("Parsed text dx list - tag: " << e->tagName << " count: " << e->textDx->size() << " first: " << (e->textDx->isEmpty() ? 0.0f : e->textDx->getFirst()));
        }
        if (auto dyList = element.getStringAttribute ("dy"); dyList.isNotEmpty())
        {
            e->textDy = parseLengthList (dyList, inheritedFontSize, viewportHeight);
            YUP_DRAWABLE_LOG ("Parsed text dy list - tag: " << e->tagName << " count: " << e->textDy->size() << " first: " << (e->textDy->isEmpty() ? 0.0f : e->textDy->getFirst()));
        }

        String fontFamily = element.getStringAttribute ("font-family");
        if (fontFamily.isNotEmpty())
        {
            e->fontFamily = fontFamily;
            YUP_DRAWABLE_LOG ("Parsed text font-family attribute: " << fontFamily);
        }

        float fontSize = parseLengthAttribute (element, "font-size", 0.0f, inheritedFontSize, inheritedFontSize);
        if (fontSize > 0.0f)
        {
            e->fontSize = fontSize;
            YUP_DRAWABLE_LOG ("Parsed text font-size attribute: " << fontSize);
        }
        else if (element.getStringAttribute ("font-size").isNotEmpty())
        {
            YUP_DRAWABLE_LOG ("Text font-size attribute ignored - parsed non-positive value: " << fontSize);
        }

        String textAnchor = element.getStringAttribute ("text-anchor");
        if (textAnchor.isNotEmpty())
        {
            e->textAnchor = textAnchor;
            YUP_DRAWABLE_LOG ("Parsed text-anchor attribute: " << textAnchor);
        }

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);

        YUP_DRAWABLE_LOG ("Parsed text font result - tag: " << e->tagName
                                                            << " id: " << element.getStringAttribute ("id", "none")
                                                            << " fontFamily: " << e->fontFamily.value_or (String ("none"))
                                                            << " fontSize: " << e->fontSize.value_or (0.0f)
                                                            << " textAnchor: " << e->textAnchor.value_or (String ("none"))
                                                            << " letterSpacing: " << e->letterSpacing.value_or (0.0f)
                                                            << " wordSpacing: " << e->wordSpacing.value_or (0.0f)
                                                            << " unsupportedFontWeight: " << element.getStringAttribute ("font-weight")
                                                            << " unsupportedFontStyle: " << element.getStringAttribute ("font-style"));
    }
    else if (element.hasTagName ("image"))
    {
        auto x = parseLengthAttribute (element, "x", 0.0f, inheritedFontSize, viewportWidth);
        auto y = parseLengthAttribute (element, "y", 0.0f, inheritedFontSize, viewportHeight);
        auto width = parseLengthAttribute (element, "width", 0.0f, inheritedFontSize, viewportWidth);
        auto height = parseLengthAttribute (element, "height", 0.0f, inheritedFontSize, viewportHeight);

        e->imageBounds = Rectangle<float> (x, y, width, height);

        String href = element.getStringAttribute ("href");
        if (href.isEmpty())
            href = element.getStringAttribute ("xlink:href");

        if (href.isNotEmpty())
        {
            e->imageHref = href;
            e->image = loadImageFromHref (href);
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

        YUP_DRAWABLE_LOG ("Parsed container start - tag: " << e->tagName
                                                           << " id: " << element.getStringAttribute ("id", "none")
                                                           << " hidden: " << (e->hidden ? "true" : "false")
                                                           << " viewBoxAttr: " << element.getStringAttribute ("viewBox"));

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
                                                     << " viewportSize: " << (e->viewportSize ? String (e->viewportSize->getWidth()) + "x" + String (e->viewportSize->getHeight()) : String ("none"))
                                                     << " hidden: " << (e->hidden ? "true" : "false"));
    }
    else if (element.hasTagName ("defs"))
    {
        e->hidden = true;

        YUP_DRAWABLE_LOG ("Parsing defs - id: " << element.getStringAttribute ("id", "none"));

        // Parse definitions like gradients and clip paths
        for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
        {
            if (child->hasTagName ("linearGradient") || child->hasTagName ("radialGradient"))
                parseGradient (*child);
            else if (child->hasTagName ("filter"))
                parseFilter (*child);
            else if (child->hasTagName ("clipPath"))
                parseClipPath (*child);
            else if (child->hasTagName ("style"))
                parseStyleElement (*child);
        }
    }
    else if (element.hasTagName ("style"))
    {
        YUP_DRAWABLE_LOG ("Parsing style element");
        parseStyleElement (element);
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
        if (! e->textAnchor && parent->textAnchor)
            e->textAnchor = parent->textAnchor;
        if (! e->letterSpacing && parent->letterSpacing)
            e->letterSpacing = parent->letterSpacing;
        if (! e->wordSpacing && parent->wordSpacing)
            e->wordSpacing = parent->wordSpacing;
        if (! e->color && parent->color)
            e->color = parent->color;

        YUP_DRAWABLE_LOG ("Inherited text/color state - tag: " << e->tagName
                                                               << " id: " << (e->id ? *e->id : "none")
                                                               << " fontFamily: " << (e->fontFamily ? *e->fontFamily : "none")
                                                               << " fontSize: " << e->fontSize.value_or (0.0f)
                                                               << " textAnchor: " << (e->textAnchor ? *e->textAnchor : "none")
                                                               << " hasColor: " << (e->color ? "true" : "false"));
    }

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->isTextElement())
        {
            YUP_DRAWABLE_LOG ("Skipping XML text child - parent: " << e->tagName);
            continue;
        }

        // Parse gradients and clip paths regardless of whether they're in <defs> or not
        if (child->hasTagName ("linearGradient") || child->hasTagName ("radialGradient"))
            parseGradient (*child);
        else if (child->hasTagName ("filter"))
            parseFilter (*child);
        else if (child->hasTagName ("clipPath"))
            parseClipPath (*child);
        else if (child->hasTagName ("style"))
            parseStyleElement (*child);
        else
        {
            const auto childResult = parseElement (*child, isRootElement, currentTransform, e.get());
            YUP_DRAWABLE_LOG ("Child parse result - parent: " << e->tagName
                                                              << " child: " << child->getTagNameWithoutNamespace()
                                                              << " result: " << (childResult ? "true" : "false"));
        }
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
            YUP_DRAWABLE_LOG ("Resolved tspan position - text: " << childElement->text.value_or (String())
                                                                 << " position: " << childElement->textPosition->toString());
        }
    }

    if (isRootElement)
    {
        // Store root SVG element's default fill/stroke for inheritance by top-level elements
        if (e->fillColor)
        {
            rootFillColor = e->fillColor;
            rootHasFill = true;
        }
        else if (e->noFill)
        {
            rootHasFill = false;
        }

        if (e->strokeColor)
        {
            rootStrokeColor = e->strokeColor;
            rootHasStroke = true;
        }
        else if (e->noStroke)
        {
            rootHasStroke = false;
        }

        YUP_DRAWABLE_LOG ("Root element parsed - rootHasFill: " << (rootHasFill ? "true" : "false")
                                                                << " rootHasStroke: " << (rootHasStroke ? "true" : "false")
                                                                << " rootFillColor: " << (rootFillColor ? rootFillColor->toString() : "none")
                                                                << " rootStrokeColor: " << (rootStrokeColor ? rootStrokeColor->toString() : "none"));

        return true;
    }

    if (parent != nullptr && ! parentIsRoot)
    {
        YUP_DRAWABLE_LOG ("Adding element to parent - tag: " << e->tagName
                                                             << " id: " << (e->id ? *e->id : "none")
                                                             << " parent: " << parent->tagName
                                                             << " parentChildCountBefore: " << parent->children.size());
        parent->children.push_back (std::move (e));
    }
    else
    {
        YUP_DRAWABLE_LOG ("Adding top-level element - tag: " << e->tagName
                                                             << " id: " << (e->id ? *e->id : "none")
                                                             << " topLevelCountBefore: " << elements.size());
        elements.push_back (std::move (e));
    }

    return true;
}

//==============================================================================

void Drawable::parseStyle (const XmlElement& element, const AffineTransform& currentTransform, Element& e)
{
    String styleAttr = element.getStringAttribute ("style");

    YUP_DRAWABLE_LOG ("parseStyle - tag: " << e.tagName
                                           << " id: " << e.id.value_or (String ("none"))
                                           << " fillAttr: " << element.getStringAttribute ("fill")
                                           << " strokeAttr: " << element.getStringAttribute ("stroke")
                                           << " style: " << styleAttr
                                           << " currentTransform: " << currentTransform.toString());

    // Parse presentation attributes first. Author CSS and inline style are applied after
    // this block so they can override presentation attributes.
    String fill = element.getStringAttribute ("fill");
    if (fill.isNotEmpty())
    {
        if (fill != "none")
        {
            String gradientUrl = extractGradientUrl (fill);
            if (gradientUrl.isNotEmpty())
            {
                e.fillUrl = gradientUrl;
                YUP_DRAWABLE_LOG ("Parsed fill gradient URL: " << gradientUrl);
            }
            else if (fill == "currentColor")
            {
                e.fillCurrentColor = true;
                YUP_DRAWABLE_LOG ("Parsed fill currentColor");
            }
            else
            {
                e.fillColor = Color::fromString (fill);
                YUP_DRAWABLE_LOG ("Parsed fill color: " << fill << " -> " << e.fillColor->toString());
            }
        }
        else
        {
            e.noFill = true;
            YUP_DRAWABLE_LOG ("Parsed fill none");
        }
    }

    String stroke = element.getStringAttribute ("stroke");
    if (stroke.isNotEmpty())
    {
        if (stroke != "none")
        {
            String gradientUrl = extractGradientUrl (stroke);
            if (gradientUrl.isNotEmpty())
            {
                e.strokeUrl = gradientUrl;
                YUP_DRAWABLE_LOG ("Parsed stroke gradient URL: " << gradientUrl);
            }
            else if (stroke == "currentColor")
            {
                e.strokeCurrentColor = true;
                YUP_DRAWABLE_LOG ("Parsed stroke currentColor");
            }
            else
            {
                e.strokeColor = Color::fromString (stroke);
                YUP_DRAWABLE_LOG ("Parsed stroke color: " << stroke << " -> " << e.strokeColor->toString());
            }
        }
        else
        {
            e.noStroke = true;
            YUP_DRAWABLE_LOG ("Parsed stroke none");
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
    {
        e.strokeWidth = strokeWidth;
        YUP_DRAWABLE_LOG ("Parsed stroke-width: " << strokeWidth);
    }

    float opacity = element.getFloatAttribute ("opacity", -1.0f);
    if (opacity >= 0.0f && opacity <= 1.0f)
    {
        e.opacity = opacity;
        YUP_DRAWABLE_LOG ("Parsed opacity: " << opacity);
    }

    String clipPath = element.getStringAttribute ("clip-path");
    if (clipPath.isNotEmpty())
    {
        String clipPathUrl = extractGradientUrl (clipPath);
        if (clipPathUrl.isNotEmpty())
        {
            e.clipPathUrl = clipPathUrl;
            YUP_DRAWABLE_LOG ("Parsed clip-path URL: " << clipPathUrl);
        }
    }

    String filter = element.getStringAttribute ("filter");
    if (filter.isNotEmpty())
    {
        if (filter == "none")
        {
            e.filterUrl.reset();
            YUP_DRAWABLE_LOG ("Parsed filter none");
        }
        else if (auto filterUrl = extractUrlId (filter); filterUrl.isNotEmpty())
        {
            e.filterUrl = filterUrl;
            YUP_DRAWABLE_LOG ("Parsed filter URL: " << filterUrl);
        }
        else
        {
            YUP_DRAWABLE_LOG ("Unsupported filter value ignored: " << filter);
        }
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
            {
                e.strokeDashArray = dashes;
                YUP_DRAWABLE_LOG ("Parsed stroke-dasharray count: " << dashes.size());
            }
        }
    }

    // Parse stroke-dashoffset
    String dashOffset = element.getStringAttribute ("stroke-dashoffset");
    if (dashOffset.isNotEmpty())
    {
        e.strokeDashOffset = parseUnit (dashOffset);
        YUP_DRAWABLE_LOG ("Parsed stroke-dashoffset: " << *e.strokeDashOffset << " from: " << dashOffset);
    }

    // Parse fill-opacity
    float fillOpacity = element.getFloatAttribute ("fill-opacity", -1.0f);
    if (fillOpacity >= 0.0f && fillOpacity <= 1.0f)
    {
        e.fillOpacity = fillOpacity;
        YUP_DRAWABLE_LOG ("Parsed fill-opacity: " << fillOpacity);
    }

    // Parse stroke-opacity
    float strokeOpacity = element.getFloatAttribute ("stroke-opacity", -1.0f);
    if (strokeOpacity >= 0.0f && strokeOpacity <= 1.0f)
    {
        e.strokeOpacity = strokeOpacity;
        YUP_DRAWABLE_LOG ("Parsed stroke-opacity: " << strokeOpacity);
    }

    // Parse fill-rule
    String fillRule = element.getStringAttribute ("fill-rule");
    if (fillRule == "evenodd" || fillRule == "nonzero")
    {
        e.fillRule = fillRule;
        YUP_DRAWABLE_LOG ("Parsed fill-rule: " << fillRule);
    }

    String clipRule = element.getStringAttribute ("clip-rule");
    if (clipRule == "evenodd" || clipRule == "nonzero")
    {
        e.clipRule = clipRule;
        YUP_DRAWABLE_LOG ("Parsed clip-rule: " << clipRule);
    }

    String color = element.getStringAttribute ("color");
    if (color.isNotEmpty() && color != "currentColor")
    {
        e.color = Color::fromString (color);
        YUP_DRAWABLE_LOG ("Parsed color: " << color << " -> " << e.color->toString());
    }

    String display = element.getStringAttribute ("display");
    String visibility = element.getStringAttribute ("visibility");
    if (display == "none" || visibility == "hidden" || visibility == "collapse")
    {
        e.hidden = true;
        YUP_DRAWABLE_LOG ("Parsed hidden state - display: " << display << " visibility: " << visibility);
    }

    String fontFamily = element.getStringAttribute ("font-family");
    if (fontFamily.isNotEmpty())
    {
        e.fontFamily = fontFamily;
        YUP_DRAWABLE_LOG ("Parsed font-family: " << fontFamily);
    }

    String fontSize = element.getStringAttribute ("font-size");
    if (fontSize.isNotEmpty())
    {
        e.fontSize = parseUnit (fontSize, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
        YUP_DRAWABLE_LOG ("Parsed font-size: " << *e.fontSize << " from: " << fontSize);
    }

    String letterSpacing = element.getStringAttribute ("letter-spacing");
    if (letterSpacing.isNotEmpty() && letterSpacing != "normal")
    {
        e.letterSpacing = parseUnit (letterSpacing, 0.0f, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
        YUP_DRAWABLE_LOG ("Parsed letter-spacing: " << *e.letterSpacing << " from: " << letterSpacing);
    }

    String wordSpacing = element.getStringAttribute ("word-spacing");
    if (wordSpacing.isNotEmpty() && wordSpacing != "normal")
    {
        e.wordSpacing = parseUnit (wordSpacing, 0.0f, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
        YUP_DRAWABLE_LOG ("Parsed word-spacing: " << *e.wordSpacing << " from: " << wordSpacing);
    }

    if (auto fontWeight = element.getStringAttribute ("font-weight"); fontWeight.isNotEmpty())
        YUP_DRAWABLE_LOG ("Font attribute currently not applied - font-weight: " << fontWeight);

    if (auto fontStyle = element.getStringAttribute ("font-style"); fontStyle.isNotEmpty())
        YUP_DRAWABLE_LOG ("Font attribute currently not applied - font-style: " << fontStyle);

    if (auto fontVariant = element.getStringAttribute ("font-variant"); fontVariant.isNotEmpty())
        YUP_DRAWABLE_LOG ("Font attribute currently not applied - font-variant: " << fontVariant);

    if (auto fontStretch = element.getStringAttribute ("font-stretch"); fontStretch.isNotEmpty())
        YUP_DRAWABLE_LOG ("Font attribute currently not applied - font-stretch: " << fontStretch);

    if (auto fontShorthand = element.getStringAttribute ("font"); fontShorthand.isNotEmpty())
        YUP_DRAWABLE_LOG ("Font shorthand currently not parsed - font: " << fontShorthand);

    if (auto dominantBaseline = element.getStringAttribute ("dominant-baseline"); dominantBaseline.isNotEmpty())
        YUP_DRAWABLE_LOG ("Text baseline attribute currently not applied - dominant-baseline: " << dominantBaseline);

    if (auto alignmentBaseline = element.getStringAttribute ("alignment-baseline"); alignmentBaseline.isNotEmpty())
        YUP_DRAWABLE_LOG ("Text baseline attribute currently not applied - alignment-baseline: " << alignmentBaseline);

    if (auto baselineShift = element.getStringAttribute ("baseline-shift"); baselineShift.isNotEmpty())
        YUP_DRAWABLE_LOG ("Text baseline attribute currently not applied - baseline-shift: " << baselineShift);

    applyStylesheetRules (element, e);

    if (styleAttr.isNotEmpty())
        parseCSSStyle (styleAttr, e);

    YUP_DRAWABLE_LOG ("parseStyle result - tag: " << e.tagName
                                                  << " id: " << e.id.value_or (String ("none"))
                                                  << " hasFillColor: " << (e.fillColor ? "true" : "false")
                                                  << " fillUrl: " << e.fillUrl.value_or (String ("none"))
                                                  << " fillCurrentColor: " << (e.fillCurrentColor ? "true" : "false")
                                                  << " noFill: " << (e.noFill ? "true" : "false")
                                                  << " hasStrokeColor: " << (e.strokeColor ? "true" : "false")
                                                  << " strokeUrl: " << e.strokeUrl.value_or (String ("none"))
                                                  << " strokeCurrentColor: " << (e.strokeCurrentColor ? "true" : "false")
                                                  << " noStroke: " << (e.noStroke ? "true" : "false")
                                                  << " filterUrl: " << e.filterUrl.value_or (String ("none"))
                                                  << " hidden: " << (e.hidden ? "true" : "false"));
}

//==============================================================================

AffineTransform Drawable::parseTransform (const XmlElement& element, const AffineTransform& currentTransform, Element& e)
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
        e.localTransform = result; // Store the local transform separately for use by <use> elements

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

AffineTransform Drawable::parseTransform (const String& transformString)
{
    if (transformString.isEmpty())
        return AffineTransform::identity();

    YUP_DRAWABLE_LOG ("parseTransform(string) - raw: " << transformString);

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

            // Skip whitespace or commas
            while (data.isWhitespace() || *data == ',')
                ++data;
        }

        // Skip the closing parenthesis
        if (*data == ')')
            ++data;

        // Apply the parsed transform
        // SVG transforms are post-multiplied (applied left-to-right), so we use prependedBy
        // to append each new transform on the right: result = result * newTransform
        if (type == "translate" && (params.size() == 1 || params.size() == 2))
        {
            const auto tx = params[0];
            const auto ty = (params.size() == 2) ? params[1] : 0.0f;
            result = result.prependedBy (AffineTransform::translation (tx, ty));
            YUP_DRAWABLE_LOG ("Applied translate transform - tx: " << tx << " ty: " << ty << " result: " << result.toString());
        }
        else if (type == "scale" && (params.size() == 1 || params.size() == 2))
        {
            const auto sx = params[0];
            const auto sy = (params.size() == 2) ? params[1] : params[0];
            result = result.prependedBy (AffineTransform::scaling (sx, sy));
            YUP_DRAWABLE_LOG ("Applied scale transform - sx: " << sx << " sy: " << sy << " result: " << result.toString());
        }
        else if (type == "rotate" && (params.size() == 1 || params.size() == 3))
        {
            if (params.size() == 1)
                result = result.prependedBy (AffineTransform::rotation (degreesToRadians (params[0])));
            else
                result = result.prependedBy (AffineTransform::rotation (degreesToRadians (params[0]), params[1], params[2]));

            YUP_DRAWABLE_LOG ("Applied rotate transform - degrees: " << params[0]
                                                                     << " params: " << params.size()
                                                                     << " result: " << result.toString());
        }
        else if (type == "skewX" && params.size() == 1)
        {
            result = result.prependedBy (AffineTransform::shearing (tanf (degreesToRadians (params[0])), 0.0f));
            YUP_DRAWABLE_LOG ("Applied skewX transform - degrees: " << params[0] << " result: " << result.toString());
        }
        else if (type == "skewY" && params.size() == 1)
        {
            result = result.prependedBy (AffineTransform::shearing (0.0f, tanf (degreesToRadians (params[0]))));
            YUP_DRAWABLE_LOG ("Applied skewY transform - degrees: " << params[0] << " result: " << result.toString());
        }
        else if (type == "matrix" && params.size() == 6)
        {
            result = result.prependedBy (AffineTransform (
                params[0], params[2], params[4], params[1], params[3], params[5]));
            YUP_DRAWABLE_LOG ("Applied matrix transform - result: " << result.toString());
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
    if (gradientUnits.isNotEmpty())
    {
        gradient->hasUnits = true;

        if (gradientUnits == "userSpaceOnUse")
        {
            gradient->units = Gradient::UserSpaceOnUse;
            YUP_DRAWABLE_LOG ("Gradient units: userSpaceOnUse");
        }
        else
        {
            gradient->units = Gradient::ObjectBoundingBox;
            YUP_DRAWABLE_LOG ("Gradient units: objectBoundingBox");
        }
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

    String spreadMethod = element.getStringAttribute ("spreadMethod");
    if (spreadMethod == "pad" || spreadMethod == "reflect" || spreadMethod == "repeat")
    {
        gradient->spreadMethod = spreadMethod;
        gradient->hasSpreadMethod = true;
        YUP_DRAWABLE_LOG ("Gradient spreadMethod: " << spreadMethod);
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
    resolvedGradient->type = gradient->type;
    resolvedGradient->id = gradient->id; // Keep the original ID
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

    if (gradient.type == Gradient::Linear
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

            YUP_DRAWABLE_LOG ("Expanded linear gradient spread - id: " << gradient.id
                                                                       << " spreadMethod: " << gradient.spreadMethod
                                                                       << " repeatStart: " << repeatStart
                                                                       << " repeatEnd: " << repeatEnd
                                                                       << " stopCount: " << colorStops.size());
        }
    }

    if (colorStops.empty())
    {
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
    }

    ColorGradient::Type type = (gradient.type == Gradient::Linear) ? ColorGradient::Linear : ColorGradient::Radial;
    ColorGradient result (type, colorStops);

    YUP_DRAWABLE_LOG ("Created ColorGradient with " << colorStops.size() << " stops");
    return result;
}

//==============================================================================

void Drawable::parseFilter (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
    {
        YUP_DRAWABLE_LOG ("parseFilter skipped - missing id");
        return;
    }

    Filter::Ptr filter = new Filter;
    filter->id = id;

    if (auto href = element.getStringAttribute ("xlink:href"); href.isNotEmpty() && href.startsWith ("#"))
    {
        filter->href = href.substring (1);
        YUP_DRAWABLE_LOG ("Filter references: " << filter->href);
    }

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
            YUP_DRAWABLE_LOG ("Parsed anisotropic GaussianBlur - filter: " << id
                                                                           << " stdDeviationX: " << stdDeviationX
                                                                           << " stdDeviationY: " << stdDeviationY
                                                                           << " featherApproximation: " << stdDeviation);
        }
        else if (values.size() == 1)
        {
            stdDeviation = parseUnit (values[0]);
            YUP_DRAWABLE_LOG ("Parsed GaussianBlur - filter: " << id << " stdDeviation: " << stdDeviation);
        }

        if (stdDeviation > 0.0f)
            filter->gaussianBlurStdDeviation = jmax (filter->gaussianBlurStdDeviation.value_or (0.0f), stdDeviation);
    }

    filters.push_back (filter);
    filtersById.set (id, filter);
    YUP_DRAWABLE_LOG ("parseFilter result - id: " << id
                                                  << " gaussianBlurStdDeviation: " << filter->gaussianBlurStdDeviation.value_or (0.0f));
}

//==============================================================================

Drawable::Filter::Ptr Drawable::getFilterById (const String& id)
{
    return filtersById[id];
}

//==============================================================================

Drawable::Filter::Ptr Drawable::resolveFilter (Filter::Ptr filter)
{
    if (filter == nullptr || filter->href.isEmpty())
        return filter;

    auto referencedFilter = resolveFilter (getFilterById (filter->href));
    if (referencedFilter == nullptr)
    {
        YUP_DRAWABLE_LOG ("Referenced filter not found: " << filter->href);
        return filter;
    }

    Filter::Ptr resolvedFilter = new Filter;
    resolvedFilter->id = filter->id;
    resolvedFilter->href = filter->href;
    resolvedFilter->gaussianBlurStdDeviation = filter->gaussianBlurStdDeviation ? filter->gaussianBlurStdDeviation
                                                                                : referencedFilter->gaussianBlurStdDeviation;

    YUP_DRAWABLE_LOG ("Resolved filter " << filter->id << " from reference " << filter->href);
    return resolvedFilter;
}

//==============================================================================

void Drawable::parseClipPath (const XmlElement& element)
{
    String id = element.getStringAttribute ("id");
    if (id.isEmpty())
    {
        YUP_DRAWABLE_LOG ("parseClipPath skipped - missing id");
        return;
    }

    ClipPath::Ptr clipPath = new ClipPath;
    clipPath->id = id;

    if (element.getStringAttribute ("clipPathUnits") == "objectBoundingBox")
        clipPath->units = ClipPath::ObjectBoundingBox;

    YUP_DRAWABLE_LOG ("parseClipPath - id: " << id
                                             << " units: " << (clipPath->units == ClipPath::ObjectBoundingBox ? "objectBoundingBox" : "userSpaceOnUse"));

    // Parse child elements that make up the clipping path
    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        Element::Ptr clipElement = new Element;
        clipElement->tagName = child->getTagNameWithoutNamespace();

        if (auto childId = child->getStringAttribute ("id"); childId.isNotEmpty())
            clipElement->id = childId;

        if (child->hasTagName ("path"))
        {
            auto path = Path();
            String pathData = child->getStringAttribute ("d");
            if (pathData.isNotEmpty() && path.fromString (pathData))
            {
                clipElement->path = std::move (path);
                YUP_DRAWABLE_LOG ("Parsed clip path child path - id: " << id << " bounds: " << clipElement->path->getBounds().toString());
            }
            else
            {
                YUP_DRAWABLE_LOG ("Clip path child path failed/empty - id: " << id << " pathLength: " << pathData.length());
            }
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
            YUP_DRAWABLE_LOG ("Parsed clip path child rect - id: " << id
                                                                   << " bounds: " << clipElement->path->getBounds().toString());
        }
        else if (child->hasTagName ("circle"))
        {
            auto cx = child->getFloatAttribute ("cx");
            auto cy = child->getFloatAttribute ("cy");
            auto r = child->getFloatAttribute ("r");

            auto path = Path();
            path.addCenteredEllipse (cx, cy, r, r);
            clipElement->path = std::move (path);
            YUP_DRAWABLE_LOG ("Parsed clip path child circle - id: " << id
                                                                     << " bounds: " << clipElement->path->getBounds().toString());
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
            YUP_DRAWABLE_LOG ("Parsed clip path child ellipse - id: " << id
                                                                      << " bounds: " << clipElement->path->getBounds().toString());
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
            YUP_DRAWABLE_LOG ("Parsed clip path child " << child->getTagNameWithoutNamespace()
                                                        << " - id: " << id
                                                        << " coordinateCount: " << coords.size()
                                                        << " bounds: " << clipElement->path->getBounds().toString());
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

    clipPaths.push_back (clipPath);
    clipPathsById.set (id, clipPath);
    YUP_DRAWABLE_LOG ("parseClipPath result - id: " << id << " elementCount: " << clipPath->elements.size());
}

//==============================================================================

Drawable::ClipPath::Ptr Drawable::getClipPathById (const String& id)
{
    return clipPathsById[id];
}

//==============================================================================

void Drawable::parseCSSStyle (const String& styleString, Element& e)
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

void Drawable::applyStyleProperty (StringRef propertyRef, StringRef valueRef, Element& e)
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
        else if (auto url = extractUrlId (value); url.isNotEmpty())
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
        else if (auto url = extractUrlId (value); url.isNotEmpty())
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
        float strokeWidth = parseUnit (value, e.strokeWidth.value_or (1.0f), e.fontSize.value_or (12.0f));
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
        float fontSize = parseUnit (value, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
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
            e.letterSpacing = parseUnit (value, 0.0f, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
    }
    else if (property == "word-spacing")
    {
        if (value != "normal")
            e.wordSpacing = parseUnit (value, 0.0f, e.fontSize.value_or (12.0f), e.fontSize.value_or (12.0f));
    }
    else if (property == "font-weight")
    {
        YUP_DRAWABLE_LOG ("CSS font-weight currently not applied - value: " << value);
    }
    else if (property == "font-style")
    {
        YUP_DRAWABLE_LOG ("CSS font-style currently not applied - value: " << value);
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
        String clipPathUrl = extractUrlId (value);
        if (clipPathUrl.isNotEmpty())
            e.clipPathUrl = clipPathUrl;
    }
    else if (property == "filter")
    {
        if (value == "none")
            e.filterUrl.reset();
        else if (auto filterUrl = extractUrlId (value); filterUrl.isNotEmpty())
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
            auto dashes = parseLengthList (value, e.fontSize.value_or (12.0f), 100.0f);
            if (! dashes.isEmpty())
                e.strokeDashArray = dashes;
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

void Drawable::applyStylesheetRules (const XmlElement& xmlElement, Element& e)
{
    std::vector<const CssRule*> matchedRules;

    for (const auto& rule : cssRules)
    {
        if (matchesCssSelector (xmlElement, rule))
            matchedRules.push_back (std::addressof (rule));
    }

    std::stable_sort (matchedRules.begin(), matchedRules.end(), [] (const CssRule* a, const CssRule* b)
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

void Drawable::parseStyleElement (const XmlElement& element)
{
    auto css = element.getAllSubText();
    int ruleOrder = static_cast<int> (cssRules.size());

    YUP_DRAWABLE_LOG ("parseStyleElement - cssLength: " << css.length() << " existingRules: " << cssRules.size());

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

            CssRule rule;
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
            cssRules.push_back (std::move (rule));
        }
    }

    YUP_DRAWABLE_LOG ("parseStyleElement result - totalRules: " << cssRules.size());
}

bool Drawable::matchesCssSelector (const XmlElement& xmlElement, const CssRule& rule) const
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
    while (unitStart < trimmed.length()
           && (CharacterFunctions::isDigit (trimmed[unitStart])
               || trimmed[unitStart] == '.'
               || trimmed[unitStart] == '-'
               || trimmed[unitStart] == '+'
               || trimmed[unitStart] == 'e'
               || trimmed[unitStart] == 'E'))
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

float Drawable::parseLengthAttribute (const XmlElement& element, StringRef attributeName, float defaultValue, float fontSize, float viewportSize)
{
    auto value = element.getStringAttribute (attributeName);
    if (value.isEmpty())
        return defaultValue;

    const auto result = parseUnit (value, defaultValue, fontSize, viewportSize);
    YUP_DRAWABLE_LOG ("parseLengthAttribute - tag: " << element.getTagNameWithoutNamespace()
                                                     << " attribute: " << String (attributeName.text)
                                                     << " raw: " << value
                                                     << " result: " << result
                                                     << " default: " << defaultValue
                                                     << " fontSize: " << fontSize
                                                     << " viewportSize: " << viewportSize);
    return result;
}

Array<float> Drawable::parseLengthList (const String& value, float fontSize, float viewportSize)
{
    Array<float> result;
    auto tokens = StringArray::fromTokens (value, " ,\t\r\n", "");

    for (const auto& token : tokens)
    {
        if (token.isNotEmpty())
            result.add (parseUnit (token, 0.0f, fontSize, viewportSize));
    }

    YUP_DRAWABLE_LOG ("parseLengthList - raw: " << value
                                                << " count: " << result.size()
                                                << " fontSize: " << fontSize
                                                << " viewportSize: " << viewportSize);
    return result;
}

std::optional<Image> Drawable::loadImageFromHref (const String& href) const
{
    YUP_DRAWABLE_LOG ("loadImageFromHref - href: " << href
                                                   << " baseDirectory: " << parseOptions.baseDirectory.getFullPathName()
                                                   << " allowDataImages: " << (parseOptions.allowDataImages ? "true" : "false")
                                                   << " allowLocalImages: " << (parseOptions.allowLocalImages ? "true" : "false")
                                                   << " hasResolver: " << (parseOptions.imageResolver ? "true" : "false"));

    if (parseOptions.imageResolver)
    {
        if (auto resolved = parseOptions.imageResolver (href, parseOptions.baseDirectory))
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
        if (! parseOptions.allowDataImages)
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
            YUP_DRAWABLE_LOG ("loadImageFromHref - decoded base64 data URI bytes: " << imageData.getSize());
        }
        else
        {
            imageData = MemoryBlock (payload.toRawUTF8(), static_cast<size_t> (payload.getNumBytesAsUTF8()));
            YUP_DRAWABLE_LOG ("loadImageFromHref - copied non-base64 data URI bytes: " << imageData.getSize());
        }
    }
    else
    {
        if (! parseOptions.allowLocalImages || parseOptions.baseDirectory.getFullPathName().isEmpty())
        {
            YUP_DRAWABLE_LOG ("loadImageFromHref skipped - local images disabled or base directory empty");
            return std::nullopt;
        }

        auto imageFile = parseOptions.baseDirectory.getChildFile (href);
        if (! imageFile.existsAsFile() || ! imageFile.loadFileAsData (imageData))
        {
            YUP_DRAWABLE_LOG ("loadImageFromHref failed - local file missing or unreadable: " << imageFile.getFullPathName());
            return std::nullopt;
        }

        YUP_DRAWABLE_LOG ("loadImageFromHref - loaded local file: " << imageFile.getFullPathName()
                                                                    << " bytes: " << imageData.getSize());
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

Font Drawable::resolveFont (const Element& element) const
{
    const auto fontSize = element.fontSize.value_or (12.0f);

    YUP_DRAWABLE_LOG ("resolveFont - tag: " << element.tagName
                                            << " id: " << element.id.value_or (String ("none"))
                                            << " family: " << element.fontFamily.value_or (String ("none"))
                                            << " size: " << fontSize
                                            << " hasResolver: " << (parseOptions.fontResolver ? "true" : "false"));

    if (parseOptions.fontResolver)
    {
        if (auto resolved = parseOptions.fontResolver (element.fontFamily.value_or (String()), fontSize))
        {
            auto result = resolved->withHeight (fontSize);
            YUP_DRAWABLE_LOG ("resolveFont - resolved by custom resolver"
                              << " requestedHeight: " << fontSize
                              << " resolvedHeightBefore: " << resolved->getHeight()
                              << " resultHeight: " << result.getHeight()
                              << " ascent: " << result.getAscent()
                              << " descent: " << result.getDescent()
                              << " weight: " << result.getWeight()
                              << " italic: " << (result.isItalic() ? "true" : "false")
                              << " axisCount: " << result.getNumAxis());
            return result;
        }

        YUP_DRAWABLE_LOG ("resolveFont - custom resolver returned no font");
    }

    auto result = Font().withHeight (fontSize);
    YUP_DRAWABLE_LOG ("resolveFont - using default font"
                      << " resultHeight: " << result.getHeight()
                      << " ascent: " << result.getAscent()
                      << " descent: " << result.getDescent()
                      << " weight: " << result.getWeight()
                      << " italic: " << (result.isItalic() ? "true" : "false")
                      << " axisCount: " << result.getNumAxis());
    return result;
}

void Drawable::renderTextElement (Graphics& g, const Element& element)
{
    if (! element.text || ! element.textPosition || element.text->isEmpty())
    {
        YUP_DRAWABLE_LOG ("renderTextElement skipped - missing text/position"
                          << " tag: " << element.tagName
                          << " hasText: " << (element.text ? "true" : "false")
                          << " hasPosition: " << (element.textPosition ? "true" : "false"));
        return;
    }

    auto position = *element.textPosition;
    YUP_DRAWABLE_LOG ("renderTextElement position inputs - tag: " << element.tagName
                                                                  << " id: " << element.id.value_or (String ("none"))
                                                                  << " basePosition: " << position.toString()
                                                                  << " hasXList: " << (element.textX ? "true" : "false")
                                                                  << " hasYList: " << (element.textY ? "true" : "false")
                                                                  << " hasDxList: " << (element.textDx ? "true" : "false")
                                                                  << " hasDyList: " << (element.textDy ? "true" : "false"));

    if (element.textX && ! element.textX->isEmpty())
    {
        YUP_DRAWABLE_LOG ("renderTextElement applying x list first value: " << element.textX->getFirst());
        position.setX (element.textX->getFirst());
    }
    if (element.textY && ! element.textY->isEmpty())
    {
        YUP_DRAWABLE_LOG ("renderTextElement applying y list first value: " << element.textY->getFirst());
        position.setY (element.textY->getFirst());
    }
    if (element.textDx && ! element.textDx->isEmpty())
    {
        YUP_DRAWABLE_LOG ("renderTextElement applying dx first value: " << element.textDx->getFirst());
        position.setX (position.getX() + element.textDx->getFirst());
    }
    if (element.textDy && ! element.textDy->isEmpty())
    {
        YUP_DRAWABLE_LOG ("renderTextElement applying dy first value: " << element.textDy->getFirst());
        position.setY (position.getY() + element.textDy->getFirst());
    }

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

        YUP_DRAWABLE_LOG ("renderTextElement StyledText setup - maxSize: " << jmax (fontSize, static_cast<float> (element.text->length()) * fontSize * 2.0f)
                                                                           << "x" << (fontSize * 4.0f)
                                                                           << " wrap: noWrap"
                                                                           << " horizontalAlign: left"
                                                                           << " verticalAlign: top"
                                                                           << " appendedLength: " << element.text->length()
                                                                           << " appendedLetterSpacing: " << element.letterSpacing.value_or (0.0f));
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
    {
        YUP_DRAWABLE_LOG ("renderTextElement applying middle anchor - textWidth: " << textWidth);
        textX -= textWidth * 0.5f;
    }
    else if (element.textAnchor == "end")
    {
        YUP_DRAWABLE_LOG ("renderTextElement applying end anchor - textWidth: " << textWidth);
        textX -= textWidth;
    }
    else if (element.textAnchor)
    {
        YUP_DRAWABLE_LOG ("renderTextElement anchor not specially handled - anchor: " << *element.textAnchor);
    }

    Rectangle<float> textBounds (textX,
                                 position.getY() + (ascent * fontSize),
                                 textWidth,
                                 textHeight + bottomPadding);

    YUP_DRAWABLE_LOG ("renderTextElement - text: " << *element.text
                                                   << " position: " << position.toString()
                                                   << " bounds: " << textBounds.toString()
                                                   << " computedTextBounds: " << computedTextBounds.toString()
                                                   << " fontSize: " << fontSize
                                                   << " fontHeight: " << font.getHeight()
                                                   << " fontAscent: " << fontAscent
                                                   << " fontDescent: " << fontDescent
                                                   << " metricsAscentUsed: " << ascent
                                                   << " metricsDescentUsed: " << descent
                                                   << " fontWeight: " << font.getWeight()
                                                   << " fontItalic: " << (font.isItalic() ? "true" : "false")
                                                   << " fontAxisCount: " << font.getNumAxis()
                                                   << " anchor: " << element.textAnchor.value_or (String ("none"))
                                                   << " letterSpacing: " << element.letterSpacing.value_or (0.0f)
                                                   << " wordSpacing: " << element.wordSpacing.value_or (0.0f)
                                                   << " textWidth: " << textWidth
                                                   << " textHeight: " << textHeight
                                                   << " bottomPadding: " << bottomPadding
                                                   << " graphicsTransform: " << g.getTransform().toString());

    YUP_DRAWABLE_LOG ("renderTextElement drawing fitted text - bounds: " << textBounds.toString());
    g.fillFittedText (styledText, textBounds);
}

void Drawable::renderImageElement (Graphics& g, const Element& element)
{
    if (! element.imageBounds)
    {
        YUP_DRAWABLE_LOG ("renderImageElement skipped - missing bounds"
                          << " tag: " << element.tagName
                          << " href: " << element.imageHref.value_or (String ("none")));
        return;
    }

    if (element.image)
    {
        YUP_DRAWABLE_LOG ("renderImageElement - drawing cached image bounds: " << element.imageBounds->toString());
        g.drawImage (*element.image, *element.imageBounds);
        return;
    }

    if (element.imageHref)
    {
        if (auto image = loadImageFromHref (*element.imageHref))
        {
            YUP_DRAWABLE_LOG ("renderImageElement - drawing resolved image href: " << *element.imageHref
                                                                                   << " bounds: " << element.imageBounds->toString());
            g.drawImage (*image, *element.imageBounds);
        }
        else
        {
            YUP_DRAWABLE_LOG ("renderImageElement skipped - image href did not resolve: " << *element.imageHref);
        }
    }
    else
    {
        YUP_DRAWABLE_LOG ("renderImageElement skipped - missing image and href");
    }
}

Path Drawable::createDashedPath (const Path& source, const Array<float>& dashArray, float dashOffset) const
{
    if (dashArray.isEmpty())
    {
        YUP_DRAWABLE_LOG ("createDashedPath - dash array empty, returning source");
        return source;
    }

    Array<float> positiveDashes;
    for (auto dash : dashArray)
    {
        if (dash > 0.0f)
            positiveDashes.add (dash);
    }

    if (positiveDashes.isEmpty())
    {
        YUP_DRAWABLE_LOG ("createDashedPath - no positive dash values, returning source");
        return source;
    }

    Path result;
    float totalPatternLength = 0.0f;
    for (auto dash : positiveDashes)
        totalPatternLength += dash;

    if (totalPatternLength <= 0.0f)
    {
        YUP_DRAWABLE_LOG ("createDashedPath - total pattern length invalid, returning source");
        return source;
    }

    YUP_DRAWABLE_LOG ("createDashedPath - sourceBounds: " << source.getBounds().toString()
                                                          << " dashCount: " << positiveDashes.size()
                                                          << " dashOffset: " << dashOffset
                                                          << " totalPatternLength: " << totalPatternLength);

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

    YUP_DRAWABLE_LOG ("createDashedPath result - bounds: " << result.getBounds().toString()
                                                           << " empty: " << (result.isEmpty() ? "true" : "false"));
    return result;
}

//==============================================================================

Rectangle<float> Drawable::calculateBounds() const
{
    // Use viewBox if available, otherwise use size
    if (! viewBox.isEmpty())
    {
        YUP_DRAWABLE_LOG ("calculateBounds - using viewBox: " << viewBox.toString());
        return viewBox;
    }

    if (size.getWidth() > 0 && size.getHeight() > 0)
    {
        auto sizeBounds = Rectangle<float> (0, 0, size.getWidth(), size.getHeight());
        YUP_DRAWABLE_LOG ("calculateBounds - using size: " << sizeBounds.toString());
        return Rectangle<float> (0, 0, size.getWidth(), size.getHeight());
    }

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

            YUP_DRAWABLE_LOG ("calculateBounds - element tag: " << element->tagName
                                                                << " id: " << element->id.value_or (String ("none"))
                                                                << " bounds: " << pathBounds.toString()
                                                                << " hasTransform: " << (element->transform ? "true" : "false"));

            if (hasValidBounds)
                bounds = bounds.unionWith (pathBounds);
            else
            {
                bounds = pathBounds;
                hasValidBounds = true;
            }
        }
        else
        {
            YUP_DRAWABLE_LOG ("calculateBounds - skipping element without path tag: " << element->tagName
                                                                                      << " id: " << element->id.value_or (String ("none")));
        }
    }

    auto result = hasValidBounds ? bounds : Rectangle<float> (0, 0, 100, 100);
    YUP_DRAWABLE_LOG ("calculateBounds result - hasValidBounds: " << (hasValidBounds ? "true" : "false")
                                                                  << " bounds: " << result.toString());
    return result;
}

//==============================================================================

AffineTransform Drawable::calculateTransformForTarget (const Rectangle<float>& sourceBounds, const Rectangle<float>& targetArea, Fitting fitting, Justification justification) const
{
    if (sourceBounds.isEmpty() || targetArea.isEmpty())
    {
        YUP_DRAWABLE_LOG ("calculateTransformForTarget - empty source or target"
                          << " source: " << sourceBounds.toString()
                          << " target: " << targetArea.toString());
        return AffineTransform::identity();
    }

    float scaleX = targetArea.getWidth() / sourceBounds.getWidth();
    float scaleY = targetArea.getHeight() / sourceBounds.getHeight();
    YUP_DRAWABLE_LOG ("calculateTransformForTarget - source: " << sourceBounds.toString()
                                                               << " target: " << targetArea.toString()
                                                               << " initialScaleX: " << scaleX
                                                               << " initialScaleY: " << scaleY
                                                               << " fitting: " << static_cast<int> (fitting)
                                                               << " justificationFlags: " << static_cast<int> (justification.getFlags()));

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
    auto result = AffineTransform::translation (-sourceBounds.getX(), -sourceBounds.getY())
                      .scaled (scaleX, scaleY)
                      .translated (offsetX, offsetY);

    YUP_DRAWABLE_LOG ("calculateTransformForTarget result - scaleX: " << scaleX
                                                                      << " scaleY: " << scaleY
                                                                      << " scaledWidth: " << scaledWidth
                                                                      << " scaledHeight: " << scaledHeight
                                                                      << " offsetX: " << offsetX
                                                                      << " offsetY: " << offsetY
                                                                      << " transform: " << result.toString());
    return result;
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
    return extractUrlId (value);
}

String Drawable::extractUrlId (const String& value)
{
    // Find the start of the URL
    int urlStart = value.indexOf ("url(");
    if (urlStart == -1)
        return String();

    // Find the end of the URL (first closing parenthesis after the URL start)
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

} // namespace yup
