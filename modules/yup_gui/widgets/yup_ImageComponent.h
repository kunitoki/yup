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

#pragma once

namespace yup
{

//==============================================================================
/**
    A component that displays a raster @ref Image or an SVG @ref Drawable fitted into its bounds.

    As every widget it is painted by its style: the one set on it, or else the one the theme sets
    for the type, which fits, aligns and fades the image as the component is set to.

    In `.yui` markup this component is registered under the `Image` type name:

    @code
    Image {
        source: "bg.png";
        fitting: scaleToFit;
        justification: center;
        imageOpacity: 0.9;
    }
    @endcode

    The file a `source` names is read through the document the component is in (see
    ComponentDocument::readAssetFor): from a folder, embedded resources or the unsaved text of an
    editor. It is read again when the document tells that it changed, and a source that can't be
    loaded is listed with the problems of the document. In no document the file is read as it is:
    an absolute path, or a path relative to the working directory.

    @see Image, Drawable, Fitting, Justification, ComponentDocument

    @tags{GUI}
*/
class YUP_API ImageComponent : public Component
    , private ComponentDocument::Listener
{
public:
    //==============================================================================
    /** Creates an empty ImageComponent. */
    ImageComponent();

    /** Creates an empty ImageComponent.

        @param componentID  Identifier for this component.
    */
    explicit ImageComponent (StringRef componentID);

    /** Destructor. */
    ~ImageComponent() override;

    //==============================================================================
    /** Sets a raster image to display, clearing any SVG drawable. */
    void setImage (Image newImage);

    /** Returns the raster image currently set, or an invalid Image when none is loaded. */
    const Image& getImage() const noexcept { return image; }

    /** Sets an SVG drawable to display, clearing any raster image. */
    void setDrawable (std::shared_ptr<Drawable> newDrawable);

    /** Returns the SVG drawable currently set, or nullptr when none is loaded. */
    const std::shared_ptr<Drawable>& getDrawable() const noexcept { return drawable; }

    //==============================================================================
    /** Loads an image, SVG file, or `data:image/...;base64,...` URI from @p newSource.

        A file is read through the document the component is in at that time, so set the source
        once the component is where it stays.
    */
    void setSource (const String& newSource);

    /** Returns the source path last passed to setSource(). */
    const String& getSource() const noexcept { return source; }

    //==============================================================================
    /** Sets how the image or drawable scales within the component bounds. */
    void setFitting (Fitting newFitting);

    /** Returns how the image or drawable scales within the component bounds. */
    Fitting getFitting() const noexcept { return fitting; }

    /** Sets how the fitted image or drawable is aligned inside the component bounds. */
    void setJustification (Justification newJustification);

    /** Returns how the fitted image or drawable is aligned inside the component bounds. */
    Justification getJustification() const noexcept { return justification; }

    /** Sets the opacity applied when painting the image, clamped to [0, 1]. */
    void setImageOpacity (float newOpacity);

    /** Returns the opacity applied when painting the image. */
    float getImageOpacity() const noexcept { return imageOpacity; }

    //==============================================================================
    /** Returns the static type info describing the Image type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;

private:
    void reloadSource();
    void reportSourceProblem();
    void documentLoaded (ComponentDocument&) override;
    void documentAssetChanged (ComponentDocument&, const String& path) override;

    String source;
    String sourceProblem;
    Image image;
    std::shared_ptr<Drawable> drawable;
    Fitting fitting = Fitting::scaleToFit;
    Justification justification = Justification::center;
    float imageOpacity = 1.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageComponent)
};

} // namespace yup
