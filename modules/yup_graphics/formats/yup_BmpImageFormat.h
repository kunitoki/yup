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
    Reads image data from BMP (Windows Bitmap) formatted streams.

    Supports the following variants:
    - BITMAPINFOHEADER (40 bytes), BITMAPV4HEADER (108 bytes), BITMAPV5HEADER (124 bytes)
    - 1, 4, 8-bit paletted images (expanded to RGB)
    - 16-bit RGB555 images
    - 24-bit BGR images (converted to RGB)
    - 32-bit BGRA images (converted to RGBA)
    - RLE4 and RLE8 compressed images

    Width, height, and pixelFormat are set during construction. If the header
    cannot be parsed (wrong signature or unsupported header), width and height
    are left at zero and readImage() returns an invalid Image.

    @see ImageFormatReader, BmpImageFormatWriter, BmpImageFormat
*/
class YUP_API BmpImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the BMP file and info headers from the stream.

        Width, height, pixelFormat, dpiX, and dpiY are populated during construction.
        If the header is invalid, width and height remain zero and readImage() returns
        an invalid Image.

        @param stream  The source stream. This object takes ownership.
        @param options Options controlling metadata extraction.
    */
    explicit BmpImageFormatReader (InputStream* stream, const ImageFormat::Options& options = {});

    //==============================================================================
    /** Decodes the full image from the input stream.

        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

private:
    //==============================================================================
    uint32 pixelDataOffset = 0;  /**< Byte offset from start of file to pixel data. */
    uint16 bitCount = 0;         /**< Bits per pixel: 1, 4, 8, 16, 24, or 32. */
    uint32 compression = 0;      /**< Compression type: 0=BI_RGB, 1=BI_RLE8, 2=BI_RLE4, 3=BI_BITFIELDS. */
    bool topDown = false;        /**< True if rows are stored top-to-bottom (negative height in file). */
    std::vector<uint32> palette; /**< ARGB palette entries for paletted images. */

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BmpImageFormatReader)
};

//==============================================================================
/**
    Writes image data to BMP (Windows Bitmap) formatted streams.

    RGB images are written as 24-bit BGR. RGBA images are written as 32-bit BGRA.
    The file uses a standard BITMAPINFOHEADER (40 bytes) with no compression.

    @see ImageFormatWriter, BmpImageFormatReader, BmpImageFormat
*/
class YUP_API BmpImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream and desired pixel format.

        @param stream  The destination stream. This object takes ownership.
        @param fmt     The pixel format to write. RGB produces 24-bit output,
                       RGBA produces 32-bit output.
    */
    BmpImageFormatWriter (OutputStream* stream, PixelFormat fmt);

    //==============================================================================
    /** Encodes and writes the image to the output stream.

        @returns true if the image was written successfully, false otherwise.
    */
    bool writeImage (const Image& image) override;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BmpImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the BMP (Windows Bitmap) image format.

    This format does not use any third-party library: reading and writing are
    implemented directly against the BMP specification. The format is
    uncompressed (for writing) and supports RLE-compressed reading.

    Extensions handled: .bmp

    @see ImageFormat, BmpImageFormatReader, BmpImageFormatWriter
*/
class YUP_API BmpImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a BmpImageFormat instance. */
    BmpImageFormat();

    //==============================================================================
    /** Returns "BMP Image". */
    const String& getFormatName() const override;

    /** Returns {".bmp"} for both reading and writing. */
    StringArray getFileExtensions (Mode mode) const override;

    /** Returns true if the stream starts with the BMP magic bytes "BM". */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a BmpImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream,
                                                        const Options& options = {}) override;

    /** Creates a BmpImageFormatWriter for the given stream and pixel format. */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::RGB, PixelFormat::RGBA}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BmpImageFormat)
};

} // namespace yup
