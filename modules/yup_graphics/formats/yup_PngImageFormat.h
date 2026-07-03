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

#if YUP_IMAGE_FORMAT_PNG

namespace yup
{

//==============================================================================
/**
    Reads image data from PNG formatted streams using libpng.

    Supports 8-bit and 16-bit (normalised to 8-bit) images in the following
    colour modes: RGBA, RGB, Grayscale+Alpha (expanded to RGBA), Grayscale,
    and Paletted (expanded to RGB or RGBA).

    Width, height, pixelFormat, dpiX, and dpiY are populated during construction.
    tEXt and pHYs metadata chunks are also extracted during construction. If the
    header cannot be decoded, width and height remain zero and readImage() returns
    an invalid Image.

    @see ImageFormatReader, PngImageFormatWriter, PngImageFormat
*/
class YUP_API PngImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the PNG header from the stream.

        Width, height, pixelFormat, dpiX, dpiY, and metadataValues are populated
        during construction. If parsing fails, width and height remain zero.

        @param stream  The source stream. This object takes ownership.
    */
    explicit PngImageFormatReader (InputStream* stream);

    //==============================================================================
    /** Decodes the full image from the input stream.

        The stream is rewound to position 0 before decoding so that the same
        InputStream can be used even after the header has been read.

        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

private:
    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PngImageFormatReader)
};

//==============================================================================
/**
    Writes image data to PNG formatted streams using libpng.

    Supports writing Grayscale (PNG_COLOR_TYPE_GRAY), RGB (PNG_COLOR_TYPE_RGB),
    and RGBA (PNG_COLOR_TYPE_RGBA) images at 8-bit depth with default compression.

    @see ImageFormatWriter, PngImageFormatReader, PngImageFormat
*/
class YUP_API PngImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream and desired pixel format.

        @param stream  The destination stream. This object takes ownership.
        @param fmt     The pixel format to write. Determines the PNG colour type.
    */
    PngImageFormatWriter (OutputStream* stream, PixelFormat fmt);

    //==============================================================================
    /** Encodes and writes the image to the output stream.

        @returns true if the image was written successfully, false otherwise.
    */
    bool writeImage (const Image& image) override;

private:
    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PngImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the PNG (Portable Network Graphics) image format.

    Uses libpng for decoding and encoding. Supports lossless, compressed PNG files
    with RGB, RGBA, and Grayscale pixel formats.

    Extensions handled: .png

    @see ImageFormat, PngImageFormatReader, PngImageFormatWriter
*/
class YUP_API PngImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a PngImageFormat instance. */
    PngImageFormat();

    //==============================================================================
    /** Returns "PNG Image". */
    const String& getFormatName() const override;

    /** Returns {".png"} for both reading and writing. */
    Array<String> getFileExtensions (Mode mode) const override;

    /** Returns true if the stream starts with the 8-byte PNG signature. */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a PngImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a PngImageFormatWriter for the given stream and pixel format. */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::Grayscale, PixelFormat::RGB, PixelFormat::RGBA}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

    /** Returns true because PNG uses zlib compression. */
    bool isCompressed() const override { return true; }

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PngImageFormat)
};

} // namespace yup

#endif // YUP_IMAGE_FORMAT_PNG
