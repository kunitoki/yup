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
    static inline const Identifier imagePart { "image" };
};

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
                                 .part (ImageComponentIds::imagePart, "The image, fitted and aligned as the component is set to in the area it is given.")
                                 .build();

    return info;
}

//==============================================================================

ImageComponent::ImageComponent()
    : ImageComponent (StringRef {})
{
}

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
    reloadSource();

    notifyPropertyChanged (ImageComponentIds::source);
    repaint();
    reportSourceProblem();
}

void ImageComponent::reloadSource()
{
    image = {};
    drawable.reset();
    sourceProblem.clear();

    const auto trimmed = source.trim();
    if (trimmed.isEmpty())
        return;

    // Followed before reading through it: it tells when the file changes, and when to tell again what is wrong with it
    follow (ComponentDocument::findFor (this));

    const auto take = [this] (auto loaded, auto& shown)
    {
        if (loaded.wasOk())
            shown = std::move (loaded.getReference());
        else
            sourceProblem = loaded.getErrorMessage();
    };

    const bool isText = trimmed.startsWithChar ('<');

    if (isText || (trimmed.endsWithIgnoreCase (".svg") && ! trimmed.startsWithIgnoreCase ("data:image/")))
        take (detail::loadSourceSvg (this, trimmed, isText), drawable);
    else
        take (detail::loadSourceImage (this, trimmed), image);
}

void ImageComponent::reportSourceProblem()
{
    if (sourceProblem.isEmpty())
        return;

    if (auto* document = ComponentDocument::findFor (this))
        document->reportDiagnostic ({ .message = getTypeInfo().getTypeName().toString() + ".source: " + sourceProblem, .property = ImageComponentIds::source }, this);
}

void ImageComponent::documentLoaded (ComponentDocument&)
{
    // A load forgets the problems it was told before, and drops the ones it is told while it runs
    reportSourceProblem();
}

void ImageComponent::documentAssetChanged (ComponentDocument&, const String& path)
{
    if (path != source.trim())
        return;

    reloadSource();
    repaint();
    reportSourceProblem();
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

void ImageComponent::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

} // namespace yup
