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
    Reads image data from TGA (Truevision TARGA) formatted streams.

    Supports the following variants:
    - Type 1:  Uncompressed, color-mapped (paletted)
    - Type 2:  Uncompressed, true-color (RGB / RGBA)
    - Type 3:  Uncompressed, grayscale
    - Type 9:  RLE-compressed, color-mapped
    - Type 10: RLE-compressed, true-color
    - Type 11: RLE-compressed, grayscale

    Pixel depths of 8, 16, 24, and 32 bits are supported. 16-bit images use
    RGB555 encoding. Paletted images are expanded to RGB. Image origin
    (top/bottom, left/right) is handled via the descriptor byte.

    Width, height, and pixelFormat are set during construction. If the header
    cannot be parsed (invalid image type or pixel depth), width and height
    are left at zero and readImage() returns an invalid Image.

    @see ImageFormatReader, TgaImageFormatWriter, TgaImageFormat
*/
class YUP_API TgaImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the TGA header from the stream.

        Width, height, pixelFormat, dpiX, and dpiY are populated during
        construction. If the header is invalid, width and height remain zero
        and readImage() returns an invalid Image.

        @param stream  The source stream. This object takes ownership.
        @param options Options controlling metadata extraction.
    */
    explicit TgaImageFormatReader (InputStream* stream, const ImageFormat::Options& options = {});

    //==============================================================================
    /** Decodes the full image from the input stream.

        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

private:
    //==============================================================================
    uint8 idLength = 0;
    uint8 colorMapType = 0;
    uint8 imageType = 0;
    uint16 firstEntryIndex = 0;
    uint16 colorMapLength = 0;
    uint8 colorMapEntrySize = 0;
    uint16 xOrigin = 0;
    uint16 yOrigin = 0;
    uint16 imageWidth = 0;
    uint16 imageHeight = 0;
    uint8 pixelDepth = 0;
    uint8 descriptor = 0;
    std::vector<uint32> palette;
    bool leftRight = false;
    bool topDown = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TgaImageFormatReader)
};

//==============================================================================
/**
    Writes image data to TGA (Truevision TARGA) formatted streams.

    RGB images are written as 24-bit true-color (type 2 or 10).
    RGBA images are written as 32-bit true-color (type 2 or 10).
    The writer can optionally use RLE compression (type 10).

    Images are always written top-down. A standard TGA footer with the
    "TRUEVISION-XFILE" signature is appended for compatibility.

    @see ImageFormatWriter, TgaImageFormatReader, TgaImageFormat
*/
class YUP_API TgaImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream, pixel format, and compression option.

        @param stream  The destination stream. This object takes ownership.
        @param fmt     The pixel format to write. RGB produces 24-bit output,
                       RGBA produces 32-bit output.
        @param useRLE  If true, use RLE compression (image type 10); otherwise
                       write uncompressed (image type 2).
    */
    TgaImageFormatWriter (OutputStream* stream, PixelFormat fmt, bool useRLE);

    //==============================================================================
    /** Encodes and writes the image to the output stream.

        @returns true if the image was written successfully, false otherwise.
    */
    bool writeImage (const Image& image) override;

private:
    //==============================================================================
    bool useRLE;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TgaImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the TGA (Truevision TARGA) image format.

    This format does not use any third-party library: reading and writing are
    implemented directly against the TGA specification. Both uncompressed and
    RLE-compressed variants are supported.

    Extensions handled: .tga, .icb, .vda, .vst

    The quality option index selects compression:
    - 0: Uncompressed (image type 2)
    - 1: RLE compressed (image type 10)

    @see ImageFormat, TgaImageFormatReader, TgaImageFormatWriter
*/
class YUP_API TgaImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a TgaImageFormat instance. */
    TgaImageFormat();

    //==============================================================================
    /** Returns "TGA Image". */
    const String& getFormatName() const override;

    /** Returns {".tga", ".icb", ".vda", ".vst"} for both reading and writing. */
    StringArray getFileExtensions (Mode mode) const override;

    /** Returns true if the stream contains a valid TGA image type byte. */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a TgaImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream,
                                                        const Options& options = {}) override;

    /** Creates a TgaImageFormatWriter for the given stream and pixel format.

        The qualityOptionIndex controls compression:
        - 0 = uncompressed (type 2)
        - 1 = RLE compressed (type 10)
    */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::RGB, PixelFormat::RGBA}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

    //==============================================================================
    /** Returns false — TGA defaults to uncompressed output, though RLE is available. */
    bool isCompressed() const override { return false; }

    /** Returns {"Uncompressed", "RLE Compressed"} — quality option 1 enables RLE. */
    StringArray getQualityOptions() const override;

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TgaImageFormat)
};

} // namespace yup
