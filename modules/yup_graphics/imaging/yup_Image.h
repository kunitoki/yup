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

namespace yup
{

class Color;
class GraphicsContext;

//==============================================================================
/** Supported raw pixel byte formats. */
enum class PixelFormat
{
    Grayscale, /**< 8-bit grayscale luminance. */
    RGB,       /**< 24-bit red, green, blue byte order. */
    RGBA       /**< 32-bit red, green, blue, alpha byte order. */
};

//==============================================================================
/**
    Represents bitmap pixel data with accessors.

    Supports different pixel formats and provides methods to manipulate individual pixels.

    @tags{Core}
*/
class YUP_API BitmapData : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<BitmapData>;

    //==============================================================================
    /** Default constructor. Creates empty bitmap data. */
    BitmapData() = default;

    /** Constructs bitmap data with specified dimensions and pixel format.
 
        @param w        The width of the bitmap in pixels.
        @param h        The height of the bitmap in pixels.
        @param fmt      The pixel format of the bitmap.
    */
    BitmapData (int w, int h, PixelFormat fmt)
        : width (w)
        , height (h)
        , format (fmt)
    {
        if (w <= 0 || h <= 0)
            throw std::invalid_argument ("Width and Height must be positive integers.");

        pixelStride = getBytesPerPixel (fmt);
        lineStride = static_cast<size_t> (w) * static_cast<size_t> (pixelStride);
        totalSizeBytes = static_cast<size_t> (h) * lineStride;
        pixelBuffer = std::make_unique<uint8[]> (totalSizeBytes);
    }

    /** Constructs bitmap data with specified dimensions and pixel format.
 
        @param w            The width of the bitmap in pixels.
        @param h            The height of the bitmap in pixels.
        @param fmt          The pixel format of the bitmap.
        @param pixelData    The pixel data bytes.
    */
    BitmapData (int w, int h, PixelFormat fmt, Span<const uint8> pixelData)
        : width (w)
        , height (h)
        , format (fmt)
    {
        if (w <= 0 || h <= 0)
            throw std::invalid_argument ("Width and Height must be positive integers.");

        pixelStride = getBytesPerPixel (fmt);
        lineStride = static_cast<size_t> (w) * static_cast<size_t> (pixelStride);
        totalSizeBytes = static_cast<size_t> (h) * lineStride;
        pixelBuffer = std::make_unique<uint8[]> (totalSizeBytes);

        std::memcpy (pixelBuffer.get(), pixelData.data(), totalSizeBytes);
    }

    /** Constructs bitmap data with specified dimensions and pixel format.
 
        @param w            The width of the bitmap in pixels.
        @param h            The height of the bitmap in pixels.
        @param fmt          The pixel format of the bitmap.
        @param pixelData    The pixel data bytes.
    */
    BitmapData (int w, int h, PixelFormat fmt, std::unique_ptr<const uint8[]> pixelData)
        : width (w)
        , height (h)
        , format (fmt)
    {
        if (w <= 0 || h <= 0)
            throw std::invalid_argument ("Width and Height must be positive integers.");

        pixelStride = getBytesPerPixel (fmt);
        lineStride = static_cast<size_t> (w) * static_cast<size_t> (pixelStride);
        totalSizeBytes = static_cast<size_t> (h) * lineStride;
        pixelBuffer = std::unique_ptr<uint8[]> (const_cast<uint8*> (pixelData.release()));
    }

    /** Copy constructor. */
    BitmapData (const BitmapData& other) = delete;

    /** Move constructor. */
    BitmapData (BitmapData&& other) noexcept
        : width (std::exchange (other.width, 0))
        , height (std::exchange (other.height, 0))
        , format (other.format)
        , pixelStride (std::exchange (other.pixelStride, 0))
        , lineStride (std::exchange (other.lineStride, 0))
        , totalSizeBytes (std::exchange (other.totalSizeBytes, 0))
        , pixelBuffer (std::exchange (other.pixelBuffer, {}))
    {
    }

    /** Copy assignment operator. */
    BitmapData& operator= (const BitmapData& other) = delete;

    /** Move assignment operator. */
    BitmapData& operator= (BitmapData&& other) noexcept
    {
        if (this != &other)
        {
            width = std::exchange (other.width, 0);
            height = std::exchange (other.height, 0);
            format = other.format;
            pixelStride = std::exchange (other.pixelStride, 0);
            lineStride = std::exchange (other.lineStride, 0);
            totalSizeBytes = std::exchange (other.totalSizeBytes, 0);
            pixelBuffer = std::exchange (other.pixelBuffer, {});
        }

        return *this;
    }

    /** Destructor. */
    ~BitmapData() = default;

    //==============================================================================
    /** Returns the width of the bitmap in pixels. */
    int getWidth() const noexcept
    {
        return width;
    }

    /** Returns the height of the bitmap in pixels. */
    int getHeight() const noexcept
    {
        return height;
    }

    /** Returns the pixel format of the bitmap. */
    PixelFormat getPixelFormat() const noexcept
    {
        return format;
    }

    /** Returns the pixel stride. */
    int getPixelStride() const noexcept
    {
        return pixelStride;
    }

    //==============================================================================
    /** Sets the pixel at (x, y) with the specified ARGB color.

        PixelFormat controls the raw byte layout used in storage. The color
        value passed here is always packed as `0xAARRGGBB`.

        @param x        The x-coordinate of the pixel.
        @param y        The y-coordinate of the pixel.
        @param color    The ARGB color value to set.
    */
    void setPixel (int x, int y, uint32 color)
    {
        validateCoordinates (x, y);

        auto* pixel = getPixelPointerUnchecked (x, y);

        switch (format)
        {
            case PixelFormat::Grayscale:
                pixel[0] = argbToLuminance (color);
                break;

            case PixelFormat::RGB:
                pixel[0] = getRedFromARGB (color);
                pixel[1] = getGreenFromARGB (color);
                pixel[2] = getBlueFromARGB (color);
                break;

            case PixelFormat::RGBA:
                writeARGBAsRGBA (color, pixel);
                break;

            default:
                throw std::runtime_error ("Unsupported pixel format.");
        }
    }

    /** Sets the pixel at (x, y) with the specified color. */
    void setPixelColor (int x, int y, Color color);

    /** Gets the pixel color at (x, y) as an ARGB value.

        PixelFormat controls the raw byte layout used in storage. The returned
        color value is always packed as `0xAARRGGBB`.

        @param x    The x-coordinate of the pixel.
        @param y    The y-coordinate of the pixel.
        @return     The ARGB color value of the pixel.
    */
    uint32 getPixel (int x, int y) const
    {
        validateCoordinates (x, y);

        const auto* pixel = getPixelPointerUnchecked (x, y);

        switch (format)
        {
            case PixelFormat::Grayscale:
            {
                const auto value = static_cast<uint32> (pixel[0]);
                return 0xff000000u | (value << 16) | (value << 8) | value;
            }

            case PixelFormat::RGB:
                return 0xff000000u
                     | (static_cast<uint32> (pixel[0]) << 16)
                     | (static_cast<uint32> (pixel[1]) << 8)
                     | static_cast<uint32> (pixel[2]);

            case PixelFormat::RGBA:
                return readRGBAAsARGB (pixel);

            default:
                throw std::runtime_error ("Unsupported pixel format.");
        }
    }

    /** Gets the pixel color at (x, y). */
    Color getPixelColor (int x, int y) const;

    /** Fills the entire bitmap with the specified ARGB color.

        @param color    The ARGB color value to fill the bitmap with.
    */
    void fill (uint32 color)
    {
        if (totalSizeBytes == 0)
            return;

        switch (format)
        {
            case PixelFormat::Grayscale:
            {
                uint8 gray = argbToLuminance (color);

                std::memset (pixelBuffer.get(), gray, totalSizeBytes);

                break;
            }

            case PixelFormat::RGB:
            {
                const uint8 r = getRedFromARGB (color);
                const uint8 g = getGreenFromARGB (color);
                const uint8 b = getBlueFromARGB (color);

                const auto numPixels = static_cast<size_t> (width) * static_cast<size_t> (height);
                auto* pixel = pixelBuffer.get();

                for (size_t i = 0; i < numPixels; ++i)
                {
                    pixel[0] = r;
                    pixel[1] = g;
                    pixel[2] = b;
                    pixel += 3;
                }

                break;
            }

            case PixelFormat::RGBA:
            {
                const auto numPixels = static_cast<size_t> (width) * static_cast<size_t> (height);
                const auto rgba = packARGBAsRGBABytes (color);
                auto* pixel = pixelBuffer.get();

                for (size_t i = 0; i < numPixels; ++i)
                {
                    std::memcpy (pixel, &rgba, sizeof (rgba));
                    pixel += 4;
                }

                break;
            }

            default:
                throw std::runtime_error ("Unsupported pixel format.");
        }
    }

    /** Fills the entire bitmap with the specified color. */
    void fillColor (Color color);

    /** Clears the bitmap by setting all pixels to zero. */
    void clear()
    {
        if (totalSizeBytes == 0)
            return;

        std::fill (pixelBuffer.get(), pixelBuffer.get() + totalSizeBytes, 0);
    }

    /** Returns a pointer to the raw pixel data. */
    Span<const uint8> getRawData() const noexcept
    {
        return { pixelBuffer.get(), totalSizeBytes };
    }

    /** Returns a mutable pointer to the raw pixel data. */
    Span<uint8> getRawData() noexcept
    {
        return { pixelBuffer.get(), totalSizeBytes };
    }

private:
    //==============================================================================
    /** Returns the number of bytes per pixel for the given format. */
    static int getBytesPerPixel (PixelFormat fmt)
    {
        switch (fmt)
        {
            case PixelFormat::Grayscale:
                return 1;

            case PixelFormat::RGB:
                return 3;

            case PixelFormat::RGBA:
                return 4;

            default:
                throw std::runtime_error ("Unsupported pixel format.");
        }
    }

    static uint8 getAlphaFromARGB (uint32 color) noexcept
    {
        return static_cast<uint8> ((color >> 24) & 0xFF);
    }

    static uint8 getRedFromARGB (uint32 color) noexcept
    {
        return static_cast<uint8> ((color >> 16) & 0xFF);
    }

    static uint8 getGreenFromARGB (uint32 color) noexcept
    {
        return static_cast<uint8> ((color >> 8) & 0xFF);
    }

    static uint8 getBlueFromARGB (uint32 color) noexcept
    {
        return static_cast<uint8> (color & 0xFF);
    }

    static uint8 argbToLuminance (uint32 color) noexcept
    {
        const auto red = static_cast<uint32> (getRedFromARGB (color));
        const auto green = static_cast<uint32> (getGreenFromARGB (color));
        const auto blue = static_cast<uint32> (getBlueFromARGB (color));

        return static_cast<uint8> ((red * 54u + green * 183u + blue * 19u + 128u) >> 8);
    }

    static uint32 packARGBAsRGBABytes (uint32 color) noexcept
    {
        return ByteOrder::swapIfBigEndian (ByteOrder::makeInt (
            getRedFromARGB (color),
            getGreenFromARGB (color),
            getBlueFromARGB (color),
            getAlphaFromARGB (color)));
    }

    static uint32 unpackRGBABytesAsARGB (uint32 rgba) noexcept
    {
        return (rgba & 0xff000000u)
             | ((rgba & 0x000000ffu) << 16)
             | (rgba & 0x0000ff00u)
             | ((rgba & 0x00ff0000u) >> 16);
    }

    static void writeARGBAsRGBA (uint32 color, uint8* pixel) noexcept
    {
        const auto rgba = packARGBAsRGBABytes (color);
        std::memcpy (pixel, &rgba, sizeof (rgba));
    }

    static uint32 readRGBAAsARGB (const uint8* pixel) noexcept
    {
        return unpackRGBABytesAsARGB (ByteOrder::littleEndianInt (pixel));
    }

    uint8* getPixelPointerUnchecked (int x, int y) noexcept
    {
        return pixelBuffer.get()
             + static_cast<size_t> (y) * lineStride
             + static_cast<size_t> (x) * static_cast<size_t> (pixelStride);
    }

    const uint8* getPixelPointerUnchecked (int x, int y) const noexcept
    {
        return pixelBuffer.get()
             + static_cast<size_t> (y) * lineStride
             + static_cast<size_t> (x) * static_cast<size_t> (pixelStride);
    }

    /** Validates the (x, y) coordinates. */
    void validateCoordinates (int x, int y) const
    {
        if (static_cast<unsigned int> (x) >= static_cast<unsigned int> (width)
            || static_cast<unsigned int> (y) >= static_cast<unsigned int> (height))
        {
            throw std::out_of_range ("Pixel coordinates out of range.");
        }
    }

    int width = 0;
    int height = 0;
    PixelFormat format = PixelFormat::RGBA;
    int pixelStride = 4;
    size_t lineStride = 0;
    size_t totalSizeBytes = 0;
    std::unique_ptr<uint8[]> pixelBuffer;
};

//==============================================================================
/**
    Represents an image using BitmapData.

    Provides methods to manipulate and access pixel data through BitmapData.

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

    /** Copy constructor. */
    Image (const Image& other);

    /** Move constructor. */
    Image (Image&& other) noexcept;

    /** Copy assignment operator. */
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

    /** Returns a const reference to BitmapData. */
    const BitmapData& getBitmapData() const noexcept;

    /** Returns a mutable reference to BitmapData. */
    BitmapData& getBitmapData() noexcept;

    /** Returns a pointer to the raw pixel data. */
    Span<const uint8> getRawData() const noexcept;

    /** Returns a mutable pointer to the raw pixel data. */
    Span<uint8> getRawData() noexcept;

    //==============================================================================
    /** Duplicate the image.
     
        This will just duplicate the image on the CPU and won't duplicate the GPU texture if it exists.
        
        Use this method to create a new image with the same pixel data.

        @return A new Image object that is a duplicate of the current image.
    */
    Image duplicate() const;

    //==============================================================================
    /** Loads an image from raw data.

        @param imageData    The raw image data.
        @return             A ResultValue containing the loaded image or an error message.
    */
    static ResultValue<Image> loadFromData (Span<const uint8> imageData);

    //==============================================================================
    /** Creates a texture on the GPU for the image if it doesn't already exist.

        @param context    The graphics context to use for texture creation.
        @return           True if the texture was created or already exists, false otherwise.
    */
    bool createTextureIfNotPresent (GraphicsContext& context) const;

    /** Invalidates the existing texture, forcing a new texture to be created on the next request. */
    void invalidateTexture();

    //==============================================================================
    /** @internal Sets the GPU texture directly, bypassing the BitmapData upload path. */
    void adoptTexture (rive::rcp<rive::gpu::Texture> t);

    /** @internal Returns the GPU texture associated with this image, or nullptr if no texture exists. */
    rive::rcp<rive::gpu::Texture> getTexture() const;

private:
    //==============================================================================
    BitmapData::Ptr bitmapData;
    mutable rive::rcp<rive::gpu::Texture> texture;
};

} // namespace yup
