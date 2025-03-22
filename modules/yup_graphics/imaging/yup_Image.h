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

class GraphicsContext;

//==============================================================================
/** Supported pixel formats. */
enum class PixelFormat
{
    Grayscale, /**< 8-bit grayscale. */
    RGB,       /**< 24-bit RGB. */
    RGBA       /**< 32-bit RGBA. */
};

//==============================================================================
/**
    Represents bitmap pixel data with accessors.

    Supports different pixel formats and provides methods to manipulate individual pixels.

    @tags{Core}
*/
class JUCE_API BitmapData : public ReferenceCountedObject
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

        pixelBuffer = std::make_unique<uint8[]> (getTotalSizeBytes());
    }

    BitmapData (int w, int h, PixelFormat fmt, std::unique_ptr<const uint8[]> pixelData)
        : width (w)
        , height (h)
        , format (fmt)
    {
        if (w <= 0 || h <= 0)
            throw std::invalid_argument ("Width and Height must be positive integers.");

        pixelBuffer = std::unique_ptr<uint8[]> (const_cast<uint8*> (pixelData.release()));
    }

    /** Copy constructor. */
    BitmapData (const BitmapData& other) = delete;

    /** Move constructor. */
    BitmapData (BitmapData&& other) noexcept
        : width (std::exchange (other.width, 0))
        , height (std::exchange (other.height, 0))
        , format (other.format)
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
        return static_cast<int> (getBytesPerPixel (format));
    }

    //==============================================================================
    /** Sets the pixel at (x, y) with the specified color.
        @param x        The x-coordinate of the pixel.
        @param y        The y-coordinate of the pixel.
        @param color    The color value to set.
    */
    void setPixel (int x, int y, uint32_t color)
    {
        validateCoordinates (x, y);

        size_t index = (y * width + x) * getBytesPerPixel (format);

        switch (format)
        {
            case PixelFormat::Grayscale:
                pixelBuffer[index] = static_cast<uint8> (color & 0xFF);
                break;

            case PixelFormat::RGB:
                pixelBuffer[index] = static_cast<uint8> ((color >> 16) & 0xFF);    // R
                pixelBuffer[index + 1] = static_cast<uint8> ((color >> 8) & 0xFF); // G
                pixelBuffer[index + 2] = static_cast<uint8> (color & 0xFF);        // B
                break;

            case PixelFormat::RGBA:
                pixelBuffer[index] = static_cast<uint8> ((color >> 24) & 0xFF);     // R
                pixelBuffer[index + 1] = static_cast<uint8> ((color >> 16) & 0xFF); // G
                pixelBuffer[index + 2] = static_cast<uint8> ((color >> 8) & 0xFF);  // B
                pixelBuffer[index + 3] = static_cast<uint8> (color & 0xFF);         // A
                break;

            default:
                throw std::runtime_error ("Unsupported pixel format.");
        }
    }

    /** Gets the pixel color at (x, y).
        @param x    The x-coordinate of the pixel.
        @param y    The y-coordinate of the pixel.
        @return     The color value of the pixel.
    */
    uint32_t getPixel (int x, int y) const
    {
        validateCoordinates (x, y);

        size_t index = (y * width + x) * getBytesPerPixel (format);
        uint32_t color = 0;

        switch (format)
        {
            case PixelFormat::Grayscale:
                return pixelBuffer[index];

            case PixelFormat::RGB:
                return (pixelBuffer[index] << 16) | (pixelBuffer[index + 1] << 8) | pixelBuffer[index + 2];

            case PixelFormat::RGBA:
                return (pixelBuffer[index] << 24) | (pixelBuffer[index + 1] << 16) | (pixelBuffer[index + 2] << 8) | pixelBuffer[index + 3];

            default:
                throw std::runtime_error ("Unsupported pixel format.");
        }

        return 0;
    }

    /** Fills the entire bitmap with the specified color.
        @param color    The color value to fill the bitmap with.
    */
    void fill (uint32_t color)
    {
        size_t totalBytes = getTotalSizeBytes();

        switch (format)
        {
            case PixelFormat::Grayscale:
            {
                uint8 gray = static_cast<uint8> (color & 0xFF);

                std::memset (pixelBuffer.get(), gray, totalBytes);

                break;
            }

            case PixelFormat::RGB:
            {
                uint8 r = static_cast<uint8> ((color >> 16) & 0xFF);
                uint8 g = static_cast<uint8> ((color >> 8) & 0xFF);
                uint8 b = static_cast<uint8> (color & 0xFF);

                for (int i = 0; i < width * height; ++i)
                {
                    pixelBuffer[i * 3] = r;
                    pixelBuffer[i * 3 + 1] = g;
                    pixelBuffer[i * 3 + 2] = b;
                }

                break;
            }

            case PixelFormat::RGBA:
            {
                uint8 r = static_cast<uint8> ((color >> 24) & 0xFF);
                uint8 g = static_cast<uint8> ((color >> 16) & 0xFF);
                uint8 b = static_cast<uint8> ((color >> 8) & 0xFF);
                uint8 a = static_cast<uint8> (color & 0xFF);

                for (int i = 0; i < width * height; ++i)
                {
                    pixelBuffer[i * 4] = r;
                    pixelBuffer[i * 4 + 1] = g;
                    pixelBuffer[i * 4 + 2] = b;
                    pixelBuffer[i * 4 + 3] = a;
                }

                break;
            }

            default:
                throw std::runtime_error ("Unsupported pixel format.");
        }
    }

    /** Clears the bitmap by setting all pixels to zero. */
    void clear()
    {
        std::fill (pixelBuffer.get(), pixelBuffer.get() + getTotalSizeBytes(), 0);
    }

    /** Returns a pointer to the raw pixel data. */
    Span<const uint8> getRawData() const noexcept
    {
        return { pixelBuffer.get(), getTotalSizeBytes() };
    }

    /** Returns a mutable pointer to the raw pixel data. */
    Span<uint8> getRawData() noexcept
    {
        return { pixelBuffer.get(), getTotalSizeBytes() };
    }

private:
    //==============================================================================
    /** Returns the number of bytes per pixel for the given format. */
    static size_t getBytesPerPixel (PixelFormat fmt)
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

    size_t getTotalSizeBytes() const
    {
        return width * height * getBytesPerPixel (format);
    }

    /** Validates the (x, y) coordinates. */
    void validateCoordinates (int x, int y) const
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            throw std::out_of_range ("Pixel coordinates out of range.");
    }

    int width = 0;
    int height = 0;
    PixelFormat format = PixelFormat::RGB;
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
    /** Sets the pixel at (x, y) with the specified color.
        @param x        The x-coordinate of the pixel.
        @param y        The y-coordinate of the pixel.
        @param color    The color value to set.
    */
    void setPixel (int x, int y, uint32_t color);

    /** Gets the pixel color at (x, y).
        @param x    The x-coordinate of the pixel.
        @param y    The y-coordinate of the pixel.
        @return     The color value of the pixel.
    */
    uint32_t getPixel (int x, int y) const;

    /** Fills the entire image with the specified color.
        @param color    The color value to fill the image with.
    */
    void fill (uint32_t color);

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
    /*
    Image duplicate() const;
    */

    bool createTextureIfNotPresent (GraphicsContext& context) const;

    rive::rcp<rive::gpu::Texture> getTexture() const;

    //==============================================================================

    static ResultValue<Image> loadFromData (Span<const uint8> imageData);

private:
    //==============================================================================
    BitmapData::Ptr bitmapData;
    mutable rive::rcp<rive::gpu::Texture> texture;
};

} // namespace yup
