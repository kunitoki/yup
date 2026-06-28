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

#if YUP_IMAGE_FORMAT_WEBP

namespace yup
{

//==============================================================================
/**
    Reads image data from WebP formatted streams using libwebp.

    Decodes both lossless and lossy WebP images. Supports RGBA and RGB output
    pixel formats. Width, height, and pixelFormat are populated during construction
    via WebPGetInfo. If the header cannot be decoded, width and height remain zero
    and readImage() returns an invalid Image.

    @see ImageFormatReader, WebPImageFormatWriter, WebPImageFormat
*/
class YUP_API WebPImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the WebP header from the stream.

        Reads the entire stream into memory, then calls WebPGetInfo to extract
        width, height, and determines the pixel format. Takes ownership of the stream.

        @param stream  The source stream. This object takes ownership.
    */
    explicit WebPImageFormatReader (InputStream* stream);

    //==============================================================================
    /** Decodes the full image from the buffered WebP data.

        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

private:
    //==============================================================================
    std::vector<uint8_t> fileData;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebPImageFormatReader)
};

//==============================================================================
/**
    Writes image data to WebP formatted streams using libwebp.

    Supports lossless (qualityIndex 0) and lossy encoding at quality levels 90,
    80, 60, and 40 (qualityIndex 1–4). Both RGBA and RGB pixel formats are supported.

    @see ImageFormatWriter, WebPImageFormatReader, WebPImageFormat
*/
class YUP_API WebPImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream, pixel format, and quality index.

        @param stream        The destination stream. This object takes ownership.
        @param fmt           The pixel format to write (RGB or RGBA).
        @param qualityIndex  0 = lossless; 1 = quality 90; 2 = quality 80;
                             3 = quality 60; 4 = quality 40.
    */
    WebPImageFormatWriter (OutputStream* stream, PixelFormat fmt, int qualityIndex);

    //==============================================================================
    /** Encodes and writes the image to the output stream.

        @returns true if the image was written successfully, false otherwise.
    */
    bool writeImage (const Image& image) override;

private:
    //==============================================================================
    int qualityIndex = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebPImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the WebP image format.

    Uses libwebp for decoding and encoding. Supports both lossless and lossy
    WebP files with RGB and RGBA pixel formats.

    Extensions handled: .webp

    @see ImageFormat, WebPImageFormatReader, WebPImageFormatWriter
*/
class YUP_API WebPImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a WebPImageFormat instance. */
    WebPImageFormat();

    //==============================================================================
    /** Returns "WebP Image". */
    const String& getFormatName() const override;

    /** Returns {".webp"} for both reading and writing. */
    Array<String> getFileExtensions (Mode mode) const override;

    /** Returns true if the stream starts with the RIFF/WEBP header. */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a WebPImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a WebPImageFormatWriter for the given stream, pixel format, and quality. */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::RGB, PixelFormat::RGBA}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

    /** Returns true because WebP uses compression. */
    bool isCompressed() const override { return true; }

    /** Returns quality option strings: Lossless, Quality 90, 80, 60, 40. */
    StringArray getQualityOptions() const override;

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebPImageFormat)
};

} // namespace yup

#endif // YUP_IMAGE_FORMAT_WEBP
