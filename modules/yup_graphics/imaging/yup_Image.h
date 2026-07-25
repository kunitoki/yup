/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

class Color;
class GraphicsContext;
class GpuTexture;

//==============================================================================
/**
    Represents an image using ImagePixelData.

    Provides methods to manipulate and access pixel data through ImagePixelData.

    @tags{Core}
*/
class Image
{
public:
    //==============================================================================
    /** Default constructor. Creates an empty image. */
    Image() = default;

    /** Constructs an image with specified dimensions and pixel format.
        @param w        The width of the image in pixels.
        @param h        The height of the image in pixels.
        @param fmt      The pixel format of the image.
    */
    Image (int w, int h, PixelFormat fmt = PixelFormat::RGBA);

    /** Copy constructor.
        Shares the pixel data and metadata via reference counting.
    */
    Image (const Image& other);

    /** Move constructor. */
    Image (Image&& other) noexcept;

    /** Copy assignment operator.
        Shares the pixel data and metadata via reference counting.
    */
    Image& operator= (const Image& other);

    /** Move assignment operator. */
    Image& operator= (Image&& other) noexcept;

    /** Destructor. */
    ~Image() = default;

    //==============================================================================
    /** Returns true if the image contains valid bitmap data. */
    bool isValid() const noexcept;

    //==============================================================================
    /** Returns the width of the image in pixels. */
    int getWidth() const noexcept;

    /** Returns the height of the image in pixels. */
    int getHeight() const noexcept;

    /** Returns the pixel format of the image. */
    PixelFormat getPixelFormat() const noexcept;

    /** Returns the pixel stride of the image. */
    int getPixelStride() const noexcept;

    //==============================================================================
    /** Sets the pixel at (x, y) with the specified ARGB color.

        PixelFormat controls the raw byte layout used in storage. The color
        value passed here is always packed as `0xAARRGGBB`.

        @param x        The x-coordinate of the pixel.
        @param y        The y-coordinate of the pixel.
        @param color    The ARGB color value to set.
    */
    void setPixel (int x, int y, uint32 color);

    /** Sets the pixel at (x, y) with the specified color. */
    void setPixelColor (int x, int y, Color color);

    /** Gets the pixel color at (x, y) as an ARGB value.

        @param x    The x-coordinate of the pixel.
        @param y    The y-coordinate of the pixel.
        @return     The ARGB color value of the pixel.
    */
    uint32 getPixel (int x, int y) const;

    /** Gets the pixel color at (x, y). */
    Color getPixelColor (int x, int y) const;

    /** Fills the entire image with the specified ARGB color.

        @param color    The ARGB color value to fill the image with.
    */
    void fill (uint32 color);

    /** Fills the entire image with the specified color. */
    void fillColor (Color color);

    /** Clears the image by setting all pixels to zero. */
    void clear();

    /** Returns a const reference to ImagePixelData. */
    const ImagePixelData& getPixelData() const noexcept;

    /** Returns a mutable reference to ImagePixelData. */
    ImagePixelData& getPixelData() noexcept;

    /** Returns a pointer to the raw pixel data. */
    Span<const uint8> getRawData() const noexcept;

    /** Returns a mutable pointer to the raw pixel data. */
    Span<uint8> getRawData() noexcept;

    //==============================================================================
    /** Duplicate the image.

        This will duplicate the image on the CPU and copy the metadata reference
        (metadata is shared via ref-counting). It won't duplicate the GPU texture
        if it exists.

        Use this method to create a new image with the same pixel data.

        @return A new Image object that is a duplicate of the current image.
    */
    Image duplicate() const;

    //==============================================================================
    /** Returns the metadata attached to this image, or nullptr if none. */
    const ImageMetadata::Ptr& getMetadata() const noexcept { return metadata; }

    /** Returns true if this image has metadata attached. */
    bool hasMetadata() const noexcept { return metadata != nullptr; }

    /** Attaches metadata to this image (replaces any existing metadata). */
    void setMetadata (ImageMetadata::Ptr newMetadata) { metadata = std::move (newMetadata); }

    //==============================================================================
    /** Loads an image from raw data.

        @param imageData    The raw image data.
        @param options      Controls which metadata categories are extracted.
                            Defaults to no metadata extraction.
        @return             A ResultValue containing the loaded image or an error message.
    */
    static ResultValue<Image> loadFromData (Span<const uint8> imageData,
                                            const ImageFormat::Options& options = {});

    /** Creates an Image that wraps an existing GPU Texture without allocating new ImagePixelData.

        The returned Image has no CPU-side pixel data. It is suitable for passing to
        Graphics::drawImage(); CPU pixel access (getPixel, getRawData) will jassert.

        Returns an empty (invalid) Image if tex is null or invalid.

        @param tex  A GPU texture obtained from GpuCanvas::asTexture().
    */
    static Image fromTexture (GpuTexture::Ptr tex);

    /** Creates an Image from a GpuTarget, reading back pixels into CPU memory.

        Equivalent to fromTexture(target.asTexture()) followed by target.readPixels().
        Returns an empty Image on failure.

        @param target  A GPU render target whose contents will be read back.
    */
    static Image fromTarget (GpuTarget& target);

    //==============================================================================
    /** Creates a texture on the GPU for the image if it doesn't already exist.

        @param context    The graphics context to use for texture creation.
        @return           True if the texture was created or already exists, false otherwise.
    */
    bool createTextureIfNotPresent (GraphicsContext& context) const;

    /** Invalidates the existing texture, forcing a new texture to be created on the next request. */
    void invalidateTexture();

    //==============================================================================
    /** Get the GPU texture associated with this image, or nullptr if no texture exists. */
    GpuTexture::Ptr getGpuTexture() const;

    /** Sets the GPU texture, replacing any existing GPU backing. */
    void setGpuTexture (GpuTexture::Ptr tex);

private:
    friend class Graphics;

    //==============================================================================
    rive::rcp<rive::gpu::Texture> getTexture() const;

    //==============================================================================
    ImagePixelData::Ptr pixelData;
    mutable GpuTexture::Ptr gpuTexture;
    ImageMetadata::Ptr metadata;
};

} // namespace yup
