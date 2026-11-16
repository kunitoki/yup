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

namespace
{

struct ImageComponentIds
{
    static inline const Identifier source { "source" };
    static inline const Identifier fitting { "fitting" };
    static inline const Identifier justification { "justification" };
    static inline const Identifier imageOpacity { "imageOpacity" };
};

File resolveImageComponentSourceFile (const String& path)
{
    if (path.isEmpty())
        return {};

    if (File::isAbsolutePath (path))
        return File (path);

    return File::getCurrentWorkingDirectory().getChildFile (path);
}

} // namespace

//==============================================================================

const ComponentTypeInfo& ImageComponent::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<ImageComponent> (Identifier ("Image"))
                                 .property (ImageComponentIds::source, &ImageComponent::getSource, &ImageComponent::setSource, String(), "The file path of the raster or SVG image to load.")
                                 .enumeration<Fitting> (
                                     ImageComponentIds::fitting,
                                     { { "scaleToFit", Fitting::scaleToFit },
                                       { "none", Fitting::none },
                                       { "fitWidth", Fitting::fitWidth },
                                       { "fitHeight", Fitting::fitHeight },
                                       { "scaleToFill", Fitting::scaleToFill },
                                       { "fill", Fitting::fill },
                                       { "tile", Fitting::tile },
                                       { "centerCrop", Fitting::centerCrop },
                                       { "centerInside", Fitting::centerInside },
                                       { "stretchWidth", Fitting::stretchWidth },
                                       { "stretchHeight", Fitting::stretchHeight } },
                                     &ImageComponent::getFitting,
                                     &ImageComponent::setFitting,
                                     "How the image scales inside the component bounds.")
                                 .enumeration<Justification::Flags> (
                                     ImageComponentIds::justification,
                                     { { "center", Justification::center },
                                       { "left", Justification::left },
                                       { "right", Justification::right },
                                       { "topLeft", Justification::topLeft },
                                       { "centerTop", Justification::centerTop },
                                       { "topRight", Justification::topRight },
                                       { "centerLeft", Justification::centerLeft },
                                       { "centerRight", Justification::centerRight },
                                       { "bottomLeft", Justification::bottomLeft },
                                       { "centerBottom", Justification::centerBottom },
                                       { "bottomRight", Justification::bottomRight } },
                                     [] (const ImageComponent& c) { return c.getJustification().getFlags(); },
                                     [] (ImageComponent& c, Justification::Flags f) { c.setJustification (f); },
                                     "How the fitted image is aligned within the component bounds.")
                                 .property (ImageComponentIds::imageOpacity, &ImageComponent::getImageOpacity, &ImageComponent::setImageOpacity, 1.0, "The opacity applied when painting the image.")
                                 .build();

    return info;
}

//==============================================================================

ImageComponent::ImageComponent (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);
}

ImageComponent::~ImageComponent() = default;

//==============================================================================

void ImageComponent::setImage (Image newImage)
{
    image = std::move (newImage);
    drawable.reset();
    repaint();
}

void ImageComponent::setDrawable (std::shared_ptr<Drawable> newDrawable)
{
    drawable = std::move (newDrawable);
    image = {};
    repaint();
}

void ImageComponent::setSource (const String& newSource)
{
    if (source == newSource)
        return;

    source = newSource;
    image = {};
    drawable.reset();

    const auto file = resolveImageComponentSourceFile (source);
    if (file.existsAsFile())
    {
        if (file.hasFileExtension (".svg"))
        {
            auto parsed = std::make_shared<Drawable>();
            if (parsed->parseSVG (file))
                drawable = std::move (parsed);
        }
        else
        {
            MemoryBlock block;
            if (file.loadFileAsData (block) && block.getSize() > 0)
            {
                auto loaded = Image::loadFromData ({ static_cast<const uint8*> (block.getData()), block.getSize() });
                if (loaded.wasOk())
                    image = std::move (loaded.getReference());
            }
        }
    }

    notifyPropertyChanged (ImageComponentIds::source);
    repaint();
}

void ImageComponent::setFitting (Fitting newFitting)
{
    if (fitting == newFitting)
        return;

    fitting = newFitting;
    notifyPropertyChanged (ImageComponentIds::fitting);
    repaint();
}

void ImageComponent::setJustification (Justification newJustification)
{
    if (justification == newJustification)
        return;

    justification = newJustification;
    notifyPropertyChanged (ImageComponentIds::justification);
    repaint();
}

void ImageComponent::setImageOpacity (float newOpacity)
{
    const auto clamped = jlimit (0.0f, 1.0f, newOpacity);
    if (approximatelyEqual (imageOpacity, clamped))
        return;

    imageOpacity = clamped;
    notifyPropertyChanged (ImageComponentIds::imageOpacity);
    repaint();
}

//==============================================================================

Rectangle<float> ImageComponent::computeFittedBounds (Rectangle<float> sourceBounds, Rectangle<float> targetArea) const noexcept
{
    if (sourceBounds.isEmpty() || targetArea.isEmpty())
        return {};

    float scaleX = targetArea.getWidth() / sourceBounds.getWidth();
    float scaleY = targetArea.getHeight() / sourceBounds.getHeight();

    switch (fitting)
    {
        case Fitting::none:
        case Fitting::tile:
            scaleX = 1.0f;
            scaleY = 1.0f;
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
    }

    const float scaledWidth = sourceBounds.getWidth() * scaleX;
    const float scaledHeight = sourceBounds.getHeight() * scaleY;

    float x = targetArea.getX();
    float y = targetArea.getY();

    if (justification.testFlags (Justification::horizontalCenter))
        x += (targetArea.getWidth() - scaledWidth) * 0.5f;
    else if (justification.testFlags (Justification::right))
        x += targetArea.getWidth() - scaledWidth;

    if (justification.testFlags (Justification::verticalCenter))
        y += (targetArea.getHeight() - scaledHeight) * 0.5f;
    else if (justification.testFlags (Justification::bottom))
        y += targetArea.getHeight() - scaledHeight;

    return { x, y, scaledWidth, scaledHeight };
}

void ImageComponent::paint (Graphics& g)
{
    if (imageOpacity <= 0.0f)
        return;

    const auto targetArea = getLocalBounds();
    if (targetArea.isEmpty())
        return;

    if (drawable != nullptr)
    {
        const auto savedState = g.saveState();
        if (imageOpacity < 1.0f)
            g.setOpacity (g.getOpacity() * imageOpacity);

        drawable->paint (g, targetArea, fitting, justification);
        return;
    }

    if (! image.isValid())
        return;

    const auto savedState = g.saveState();
    if (imageOpacity < 1.0f)
        g.setOpacity (g.getOpacity() * imageOpacity);

    const Rectangle<float> sourceBounds { 0.0f, 0.0f, static_cast<float> (image.getWidth()), static_cast<float> (image.getHeight()) };

    if (fitting == Fitting::tile)
    {
        g.setClipPath (targetArea);

        for (float y = targetArea.getY(); y < targetArea.getBottom(); y += sourceBounds.getHeight())
        {
            for (float x = targetArea.getX(); x < targetArea.getRight(); x += sourceBounds.getWidth())
                g.drawImage (image, { x, y, sourceBounds.getWidth(), sourceBounds.getHeight() });
        }

        return;
    }

    const auto destBounds = computeFittedBounds (sourceBounds, targetArea);
    if (destBounds.isEmpty())
        return;

    if (! targetArea.contains (destBounds))
        g.setClipPath (targetArea);

    g.drawImage (image, destBounds);
}

} // namespace yup
