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

class ImageFormatReader;
class ImageFormatWriter;

//==============================================================================
/**
    Abstract base class for image format implementations.

    This class serves as the foundation for all image file format handlers within
    the YUP library. Each concrete implementation represents a specific image file
    format (such as BMP, PPM, PNG, or WebP) and provides the necessary functionality
    to create reader and writer objects for parsing and writing files in that particular
    format.

    Subclasses must implement all pure virtual methods to provide format-specific
    behaviour. The ImageFormatManager typically manages instances of ImageFormat
    subclasses to provide a unified interface for handling multiple image formats
    in an application.

    @see ImageFormatReader, ImageFormatWriter, ImageFormatManager
*/
class YUP_API ImageFormat
{
public:
    //==============================================================================
    /** Mode of operation. */
    enum Mode
    {
        forReading,
        forWriting
    };

    //==============================================================================
    /**
        Controls which metadata categories are extracted during image decoding.

        By default (both flags false), no metadata is collected — the reader only
        decodes pixel data and DPI. Set one or both flags to opt in to specific
        categories of metadata.

        @see ImageMetadata, ImageFormatReader, Image::loadFromData
    */
    struct Options
    {
        Options()
            : parseMetadata (false)
            , parseRawChunks (false)
        {
        }

        Options& withMetadata (bool parseMetadata)
        {
            this->parseMetadata = parseMetadata;
            return *this;
        }

        Options& withRawChunks (bool parseRawChunks)
        {
            this->parseRawChunks = parseRawChunks;
            return *this;
        }

        /** When true, text key-value pairs (title, author, comment, etc.) are extracted.
            Populates ImageMetadata::textEntries. */
        bool parseMetadata;

        /** When true, raw binary chunks (EXIF, ICC profiles, XMP, custom chunks) are extracted.
            Populates ImageMetadata::rawChunks. */
        bool parseRawChunks;
    };

    //==============================================================================
    /** Destructor. */
    virtual ~ImageFormat() = default;

    //==============================================================================
    /** Returns the descriptive name of this image format.

        @returns A string containing the human-readable name of the format (e.g., "PNG Image", "WebP Image")
    */
    virtual const String& getFormatName() const = 0;

    /** Returns the file extensions associated with this format for the given mode.

        @param mode  Whether to return extensions for reading or writing
        @returns An array of file extensions (including the dot) that this format can handle
                 in the specified mode (e.g., {".png"} for PNG format)
    */
    virtual StringArray getFileExtensions (Mode mode) const = 0;

    /** Tests whether this format can handle the given file based on its extension.

        The default implementation compares the file's extension (lowercased) against
        each entry returned by getFileExtensions(mode). Subclasses may override this
        to perform deeper inspection of the file contents.

        @param file  The file to test for compatibility
        @param mode  Whether to check for reading or writing compatibility
        @returns true if this format can potentially handle the file, false otherwise
    */
    virtual bool canHandleFile (const File& file, Mode mode) const;

    /** Returns true if this format can decode the stream content by inspecting magic bytes.

        The default implementation returns false. Subclasses that support stream-based
        detection should override this, read the minimum number of header bytes needed,
        seek the stream back to position 0, and return the result of the magic comparison.

        @param stream  The input stream to inspect. Must be seekable; position is reset to 0 before returning.
        @param mode    Whether to check for reading or writing compatibility
        @returns true if this format recognises the stream's header signature, false otherwise
    */
    virtual bool canHandleStream (InputStream& stream, Mode mode) const;

    //==============================================================================
    /** Creates a reader object capable of decoding image data from the given stream.

        Attempts to create a format-specific reader for the provided input stream.
        The reader will extract image dimensions, pixel format, and metadata from
        the stream header before any pixel data is decoded.

        @param sourceStream  The input stream containing image data to be read. The
                             ImageFormat takes ownership of this stream on success.
        @param options       Controls which metadata categories are extracted during
                             decoding. Defaults to no metadata extraction.
        @returns A unique pointer to an ImageFormatReader if successful, nullptr if
                 the stream cannot be parsed by this format
    */
    virtual std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream,
                                                                const Options& options = {}) = 0;

    /** Creates a writer object capable of encoding image data to the given stream.

        Creates a format-specific writer configured with the specified pixel format and
        quality settings. The writer will encode pixel data according to the format's
        specification and write the result to the provided output stream.

        @param destStream         The output stream where encoded image data will be written
        @param pixelFormat        The pixel format of the source image data to be encoded
        @param metadataValues     A collection of metadata key-value pairs to embed in the file
        @param qualityOptionIndex Index into the quality options array for compressed formats;
                                  ignored for lossless or uncompressed formats
        @returns A unique pointer to an ImageFormatWriter if successful, nullptr if the
                 parameters are not supported by this format
    */
    virtual std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                                PixelFormat pixelFormat,
                                                                const StringPairArray& metadataValues,
                                                                int qualityOptionIndex) = 0;

    //==============================================================================
    /** Returns the pixel formats this format can produce on read or consume on write.

        Clients should query this before creating a reader or writer to ensure the
        desired pixel layout is supported. If the format natively supports only a
        subset of pixel formats, the reader or writer may perform an internal conversion.

        @returns An array of PixelFormat values indicating what this format supports
    */
    virtual Array<PixelFormat> getPossiblePixelFormats() const = 0;

    //==============================================================================
    /** Returns true if this format uses lossy or lossless compression.

        Uncompressed formats (e.g., BMP, PPM) return false. Compressed formats such
        as PNG (lossless) or WebP (lossy or lossless) return true.

        @returns true if the format applies any form of compression to image data
    */
    virtual bool isCompressed() const { return false; }

    /** Returns quality option descriptions for compressed formats.

        For formats that support multiple quality levels (e.g., WebP with different
        quality percentages), this method returns human-readable labels for each level.
        The index of the desired quality is passed to createWriterFor().

        @returns An array of quality descriptions (e.g., {"Low", "Medium", "High"}) or
                 an empty array for formats that do not support quality options
    */
    virtual StringArray getQualityOptions() const { return {}; }
};

} // namespace yup
