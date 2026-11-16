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

    In `.yui` markup this component is registered under the `Image` type name:

    @code
    Image {
        source: "bg.png";
        fitting: scaleToFit;
        justification: center;
        imageOpacity: 0.9;
    }
    @endcode

    @see Image, Drawable, Fitting, Justification

    @tags{GUI}
*/
class YUP_API ImageComponent : public Component
{
public:
    //==============================================================================
    /** Creates an empty ImageComponent.

        @param componentID  Optional identifier for this component.
    */
    explicit ImageComponent (StringRef componentID = {});

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
    /** Loads an image or SVG file from @p newSource (absolute or relative to the working directory). */
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
    Rectangle<float> computeFittedBounds (Rectangle<float> sourceBounds, Rectangle<float> targetArea) const noexcept;

    String source;
    Image image;
    std::shared_ptr<Drawable> drawable;
    Fitting fitting = Fitting::scaleToFit;
    Justification justification = Justification::center;
    float imageOpacity = 1.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageComponent)
};

} // namespace yup
