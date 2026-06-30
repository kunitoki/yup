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
    Reads image data from PPM, PGM, and PBM formatted streams.

    Supports all six Netpbm magic numbers:
    - P1: ASCII bitmap (PBM)
    - P2: ASCII grayscale (PGM)
    - P3: ASCII colour (PPM)
    - P4: binary bitmap (PBM)
    - P5: binary grayscale (PGM)
    - P6: binary colour (PPM)

    The pixel data is normalised to 8-bit per channel using the maxval declared
    in the file header. Bitmap formats (P1/P4) are decoded as Grayscale images
    where white maps to 255 and black maps to 0.

    @see ImageFormatReader, PpmImageFormatWriter, PpmImageFormat
*/
class YUP_API PpmImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the PPM/PGM/PBM header from the stream.

        Width, height, and pixelFormat are populated during construction. If the
        header cannot be parsed (wrong magic, non-positive dimensions, etc.) the
        width and height are left at zero and readImage() will return an invalid Image.

        @param stream  The source stream. This object takes ownership.
    */
    explicit PpmImageFormatReader (InputStream* stream);

    //==============================================================================
    /** Decodes the full image from the input stream.

        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

private:
    //==============================================================================
    int magic = 0;  /**< Numeric suffix of the magic: 1-6. */
    int maxval = 1; /**< Maximum sample value declared in the header (1 for bitmaps). */

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PpmImageFormatReader)
};

//==============================================================================
/**
    Writes image data to PPM or PGM streams using binary encoding.

    Grayscale images are written as P5 (binary PGM) and RGB / RGBA images are
    written as P6 (binary PPM). Alpha is discarded when writing RGB data.

    @see ImageFormatWriter, PpmImageFormatReader, PpmImageFormat
*/
class YUP_API PpmImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream and desired pixel format.

        @param stream  The destination stream. This object takes ownership.
        @param fmt     The pixel format to write. Grayscale produces P5, anything
                       else (RGB/RGBA) produces P6.
    */
    PpmImageFormatWriter (OutputStream* stream, PixelFormat fmt);

    //==============================================================================
    /** Encodes and writes the image to the output stream.

        @returns true if the image was written successfully, false otherwise.
    */
    bool writeImage (const Image& image) override;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PpmImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the PPM, PGM, and PBM Netpbm image formats.

    This format does not use any third-party library: reading and writing are
    implemented directly against the Netpbm specification. The format is
    uncompressed and lossless.

    Extensions handled: .ppm, .pgm, .pbm

    @see ImageFormat, PpmImageFormatReader, PpmImageFormatWriter
*/
class YUP_API PpmImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a PpmImageFormat instance. */
    PpmImageFormat();

    //==============================================================================
    /** Returns "PPM/PGM/PBM Image". */
    const String& getFormatName() const override;

    /** Returns {".ppm", ".pgm", ".pbm"} for both reading and writing. */
    Array<String> getFileExtensions (Mode mode) const override;

    /** Returns true if the stream starts with a Netpbm magic token P1 through P6. */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a PpmImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a PpmImageFormatWriter for the given stream and pixel format. */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::Grayscale, PixelFormat::RGB}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PpmImageFormat)
};

} // namespace yup
