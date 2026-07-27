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

#if YUP_IMAGE_FORMAT_JPEG

namespace yup
{

//==============================================================================
/**
    Reads image data from JPEG formatted streams using libjpeg-turbo.

    Supports 8-bit grayscale and RGB JPEG images. Width, height, pixelFormat,
    dpiX, and dpiY are populated during construction. If the header cannot be
    decoded, width and height remain zero and readImage() returns an invalid Image.

    @see ImageFormatReader, JpegImageFormatWriter, JpegImageFormat
*/
class YUP_API JpegImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the JPEG header from the stream.

        @param stream  The source stream. This object takes ownership.
        @param options Options controlling metadata extraction.
    */
    explicit JpegImageFormatReader (InputStream* stream, const ImageFormat::Options& options = {});

    //==============================================================================
    /** Decodes the full image from the input stream.

        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

private:
    //==============================================================================
    std::vector<uint8> fileData;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JpegImageFormatReader)
};

//==============================================================================
/**
    Writes image data to JPEG formatted streams using libjpeg-turbo.

    Supports grayscale, RGB, and RGBA input images. RGBA input is encoded as RGB
    because JPEG has no alpha channel.

    @see ImageFormatWriter, JpegImageFormatReader, JpegImageFormat
*/
class YUP_API JpegImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream, pixel format, and quality option.

        @param stream        The destination stream. This object takes ownership.
        @param fmt           The requested source pixel format.
        @param qualityIndex  Index into JpegImageFormat::getQualityOptions().
    */
    JpegImageFormatWriter (OutputStream* stream, PixelFormat fmt, int qualityIndex);

    //==============================================================================
    /** Encodes and writes the image to the output stream.

        @returns true if the image was written successfully, false otherwise.
    */
    bool writeImage (const Image& image) override;

private:
    //==============================================================================
    int qualityIndex = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JpegImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the JPEG image format.

    Uses libjpeg-turbo for decoding and encoding baseline-compatible JPEG files.
    Extensions handled: .jpg, .jpeg, .jpe

    @see ImageFormat, JpegImageFormatReader, JpegImageFormatWriter
*/
class YUP_API JpegImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a JpegImageFormat instance. */
    JpegImageFormat();

    //==============================================================================
    /** Returns "JPEG Image". */
    const String& getFormatName() const override;

    /** Returns {".jpg", ".jpeg", ".jpe"} for both reading and writing. */
    StringArray getFileExtensions (Mode mode) const override;

    /** Returns true if the stream starts with a JPEG SOI marker. */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    /** Returns the available lossy JPEG quality presets. */
    StringArray getQualityOptions() const override;

    //==============================================================================
    /** Creates a JpegImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream,
                                                        const Options& options = {}) override;

    /** Creates a JpegImageFormatWriter for the given stream and pixel format. */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::Grayscale, PixelFormat::RGB, PixelFormat::RGBA}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

    /** Returns true because JPEG uses lossy compression. */
    bool isCompressed() const override { return true; }

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JpegImageFormat)
};

} // namespace yup

#endif // YUP_IMAGE_FORMAT_JPEG
