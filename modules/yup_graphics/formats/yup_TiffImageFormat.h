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

#if YUP_IMAGE_FORMAT_TIFF

namespace yup
{

//==============================================================================
/**
    Reads image data from TIFF formatted streams using libtiff.

    Decodes TIFF images supporting Grayscale, RGB, and RGBA pixel formats.
    Width, height, pixelFormat, dpiX, and dpiY are populated during construction
    via TIFFGetField.

    If the header cannot be decoded, width and height remain zero and readImage()
    returns an invalid Image.

    @see ImageFormatReader, TiffImageFormatWriter, TiffImageFormat
*/
class YUP_API TiffImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the TIFF header from the stream.

        Width, height, pixelFormat, dpiX, dpiY, and metadataValues are populated
        during construction. If parsing fails, width and height remain zero.

        @param stream  The source stream. This object takes ownership.
        @param options Options controlling metadata extraction.
    */
    explicit TiffImageFormatReader (InputStream* stream, const ImageFormat::Options& options = {});

    //==============================================================================
    /** Decodes the full image from the input stream.

        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

private:
    //==============================================================================
    void parseHeader();

    std::vector<uint8_t> fileData;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TiffImageFormatReader)
};

//==============================================================================
/**
    Writes image data to TIFF formatted streams using libtiff.

    Supports writing Grayscale, RGB, and RGBA images with Deflate (ZIP) compression.

    @see ImageFormatWriter, TiffImageFormatReader, TiffImageFormat
*/
class YUP_API TiffImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream and desired pixel format.

        @param stream  The destination stream. This object takes ownership.
        @param fmt     The pixel format to write. Determines the TIFF photometric
                       interpretation and samples per pixel.
    */
    TiffImageFormatWriter (OutputStream* stream, PixelFormat fmt);

    //==============================================================================
    /** Encodes and writes the image to the output stream.

        @returns true if the image was written successfully, false otherwise.
    */
    bool writeImage (const Image& image) override;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TiffImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the TIFF (Tag Image File Format) image format.

    Uses libtiff for decoding and encoding. Supports Grayscale, RGB, and RGBA
    pixel formats with Deflate (ZIP) compression.

    Extensions handled: .tiff, .tif

    @see ImageFormat, TiffImageFormatReader, TiffImageFormatWriter
*/
class YUP_API TiffImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a TiffImageFormat instance. */
    TiffImageFormat();

    //==============================================================================
    /** Returns "TIFF Image". */
    const String& getFormatName() const override;

    /** Returns {".tiff", ".tif"} for both reading and writing. */
    StringArray getFileExtensions (Mode mode) const override;

    /** Returns true if the stream starts with the TIFF byte-order signature (II or MM). */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a TiffImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream,
                                                        const Options& options = {}) override;

    /** Creates a TiffImageFormatWriter for the given stream and pixel format. */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::Grayscale, PixelFormat::RGB, PixelFormat::RGBA}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

    /** Returns true because TIFF uses Deflate compression. */
    bool isCompressed() const override { return true; }

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TiffImageFormat)
};

} // namespace yup

#endif // YUP_IMAGE_FORMAT_TIFF
