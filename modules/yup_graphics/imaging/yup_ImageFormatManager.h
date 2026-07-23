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
/** Enumeration of built-in image format types for use with registerDefaultFormats(). */
enum class ImageFormatType
{
    bmp = 1 << 0,
    ppm = 1 << 1,
    png = 1 << 2,
    jpeg = 1 << 3,
    webp = 1 << 4,
    gif = 1 << 5,
    tga = 1 << 6,
    tiff = 1 << 7,
    all = ~0
};

YUP_DECLARE_SCOPED_ENUM_BITWISE_OPERATORS (ImageFormatType)

//==============================================================================
/**
    Central registry and factory for image format handlers.

    ImageFormatManager maintains a collection of registered ImageFormat implementations
    and provides convenient methods for creating readers and writers based on file extension.

    Example usage:
    @code
    ImageFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor (imageFile);
    if (reader != nullptr)
    {
        auto image = reader->readImage();
        // use image...
    }
    @endcode

    @see ImageFormat, ImageFormatReader, ImageFormatWriter
*/
class YUP_API ImageFormatManager
{
public:
    //==============================================================================
    /** Constructs an empty manager with no registered formats. */
    ImageFormatManager();

    //==============================================================================
    /** Registers all built-in image format implementations.

        The specific formats registered depend on compile-time configuration:
        - BMP is always registered (no external dependency)
        - PPM/PGM/PBM is always registered (no external dependency)
        - TGA is always registered (no external dependency)
        - PNG requires libpng (YUP_IMAGE_FORMAT_PNG)
        - JPEG requires libjpeg-turbo (YUP_IMAGE_FORMAT_JPEG)
        - WebP requires libwebp (YUP_IMAGE_FORMAT_WEBP)
        - GIF requires libgif (YUP_IMAGE_FORMAT_GIF)

        @param types  Bitmask of ImageFormatType values to register. Defaults to all.
    */
    void registerDefaultFormats (ImageFormatType types = ImageFormatType::all);

    /** Registers a custom image format implementation.
        The manager takes ownership of the format object.

        @param format  A unique pointer to the ImageFormat implementation to register.
    */
    void registerFormat (std::unique_ptr<ImageFormat> format);

    //==============================================================================
    /** Returns all file extensions for all currently registered formats.

        Collects extensions from both reading and writing modes, deduplicating
        so each extension appears only once.

        @returns An Array of lowercase file extension strings (e.g. ".bmp", ".tga").
    */
    StringArray getFormatFileExtensions() const;

    //==============================================================================
    /** Creates an appropriate reader for the given file, based on file extension.

        @param file     The image file to create a reader for.
        @param options  Controls which metadata categories are extracted during decoding.
        @returns A valid reader, or nullptr if no registered format handles this file.
    */
    std::unique_ptr<ImageFormatReader> createReaderFor (const File& file,
                                                        const ImageFormat::Options& options = {});

    /** Creates a reader for raw stream data, detecting the format by magic bytes.

        Iterates over all registered formats and calls canHandleStream() on each.
        The first format that recognises the stream header wins. The stream must
        support setPosition() so each candidate can reset the read position to 0.

        Ownership of the stream is always transferred to this method. On success
        the returned reader owns the stream; on failure (nullptr returned) the
        stream is deleted by this method.

        @param stream   The input stream to detect and read. Ownership is always
                        consumed by this method regardless of the return value.
        @param options  Controls which metadata categories are extracted during decoding.
        @returns A valid reader on success, or nullptr if no registered format
                 recognises the stream header.
    */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* stream,
                                                        const ImageFormat::Options& options = {});

    //==============================================================================
    /** Creates an appropriate writer for the given file, based on file extension.

        @param file               The destination file where image data will be written.
        @param pixelFormat        The pixel format of the source image data to encode.
        @param metadataValues     Metadata key-value pairs to embed in the file.
        @param qualityOptionIndex Index into the quality options for compressed formats.
        @returns A valid writer, or nullptr if no registered format handles this file.
    */
    std::unique_ptr<ImageFormatWriter> createWriterFor (const File& file,
                                                        PixelFormat pixelFormat = PixelFormat::RGBA,
                                                        const StringPairArray& metadataValues = {},
                                                        int qualityOptionIndex = 0);

private:
    std::vector<std::unique_ptr<ImageFormat>> formats;
};

} // namespace yup
