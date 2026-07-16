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

#if YUP_IMAGE_FORMAT_GIF

namespace yup
{

//==============================================================================
/**
    Reads image data from GIF formatted streams using giflib.

    Supports both single-frame and animated GIF87a/GIF89a files. On construction
    the entire stream is read into memory and DGifSlurp() parses all frames.
    Width, height, frame count, loop count, and per-frame delays are available
    immediately after construction via the animation API.

    Sequential frame access (0, 1, 2, …) is O(1) per frame thanks to an internal
    composited canvas. Backward seeks re-composite from frame 0.

    @see ImageFormatReader, GifImageFormatWriter, GifImageFormat
*/
class YUP_API GifImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses all GIF frames from the stream.

        Reads the full stream into memory, opens a GIF decoder via memory callback,
        and calls DGifSlurp() to parse all frames. Width and height are set to the
        GIF logical screen dimensions. Takes ownership of the stream.

        @param stream  The source stream. This object takes ownership.
    */
    explicit GifImageFormatReader (InputStream* stream);

    //==============================================================================
    /** Decodes and returns frame 0 as a fully composited Image.
        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

    /** Decodes and returns a single animation frame by index.
        Sequential access (0, 1, 2, …) reuses the internal canvas (O(1) per frame).
        Backward seeks re-composite from frame 0.
        @param frameIndex  Zero-based frame index.
        @returns The decoded frame Image, or an invalid Image if frameIndex is out of range.
    */
    Image readFrame (int frameIndex) override;

    /** Decodes a frame directly into an existing Image, avoiding allocation when possible.
        If dest has the correct width, height, and PixelFormat::RGBA, decodes directly
        into dest's raw pixel buffer - zero allocation.
        @param frameIndex  Zero-based frame index.
        @param dest        Image to decode into (may be reallocated).
        @returns true on success.
    */
    bool readFrame (int frameIndex, Image& dest) override;

    //==============================================================================
    /** Returns true when the GIF contains more than one frame. */
    bool isAnimated() const override;

    /** Returns the number of frames in the GIF (GifFile->ImageCount). */
    int getFrameCount() const override;

    /** Returns the loop count from the NETSCAPE2.0 extension.
        0 = loop infinitely; 1 = play once (also returned when the block is absent).
    */
    int getLoopCount() const override;

    /** Returns the display duration for a frame in milliseconds (GCE delay × 10). */
    int getFrameDelayMs (int frameIndex) const override;

private:
    //==============================================================================
    struct GifDeleter
    {
        void operator() (GifFileType* p) const noexcept
        {
            if (p != nullptr)
            {
                int err = 0;
                DGifCloseFile (p, &err);
            }
        }
    };

    void compositeFrame (int frameIndex);
    void resetCanvas();

    std::vector<uint8_t> fileData;
    std::unique_ptr<GifFileType, GifDeleter> gifFile;

    Image canvas;
    Image previousCanvas;
    int lastRenderedFrame = -1;

    int loopCount = 1;
    std::vector<int> frameDelaysMs;
    std::vector<int> disposalMethods;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GifImageFormatReader)
};

//==============================================================================
/**
    Writes image data to GIF formatted streams using giflib.

    Supports both single-frame output (writeImage()) and animated GIF output
    via the beginAnimation() / writeFrame() / endAnimation() sequence.
    All output uses GIF89a format. Palette quantization is performed per frame
    via GifQuantizeBuffer (fixed 256-color palette).

    @see ImageFormatWriter, GifImageFormatReader, GifImageFormat
*/
class YUP_API GifImageFormatWriter : public ImageFormatWriter
{
public:
    //==============================================================================
    /** Constructs the writer for the given stream and pixel format.

        @param stream  The destination stream. This object takes ownership.
        @param fmt     The pixel format to expect from source images (RGBA is used).
    */
    GifImageFormatWriter (OutputStream* stream, PixelFormat fmt);

    /** Destructor - flushes any open animation state. */
    ~GifImageFormatWriter() override;

    //==============================================================================
    /** Encodes and writes a single-frame GIF89a to the output stream.
        @returns true on success.
    */
    bool writeImage (const Image& image) override;

    //==============================================================================
    /** Returns true - GIF supports animated output. */
    bool supportsAnimation() const override { return true; }

    /** Begins an animated GIF sequence.
        Writes the GIF89a header, global colour table, and (if loopCount != 1)
        the NETSCAPE2.0 loop extension block.
        @param loopCount  0 = loop infinitely; 1 = play once; N = play N times.
        @returns true on success.
    */
    bool beginAnimation (int loopCount = 0) override;

    /** Encodes and appends one animation frame.
        Quantizes to 256-colour palette, writes a Graphic Control Extension (GCE),
        and appends the image descriptor and raster data.
        @param frame    Frame image to encode (RGBA expected).
        @param delayMs  Display duration in milliseconds (rounded to centiseconds).
        @returns true on success.
    */
    bool writeFrame (const Image& frame, int delayMs) override;

    /** Finalises the animated GIF by writing the trailer byte (0x3B).
        @returns true on success.
    */
    bool endAnimation() override;

private:
    //==============================================================================
    struct GifDeleter
    {
        void operator() (GifFileType* p) const noexcept
        {
            if (p != nullptr)
            {
                int err = 0;
                EGifCloseFile (p, &err);
            }
        }
    };

    bool writeFrameInternal (GifFileType* gif, const Image& frame, int delayMs, bool writeScreenDesc, int globalWidth, int globalHeight);

    std::unique_ptr<GifFileType, GifDeleter> animGif;
    int animWidth = 0;
    int animHeight = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GifImageFormatWriter)
};

//==============================================================================
/**
    ImageFormat implementation for the GIF image format.

    Uses giflib for both reading and writing. Supports GIF87a and GIF89a,
    including animated GIFs. Output is always GIF89a.

    Extensions handled: .gif (both read and write)

    @see ImageFormat, GifImageFormatReader, GifImageFormatWriter
*/
class YUP_API GifImageFormat : public ImageFormat
{
public:
    //==============================================================================
    /** Constructs a GifImageFormat instance. */
    GifImageFormat();

    //==============================================================================
    /** Returns "GIF Image". */
    const String& getFormatName() const override;

    /** Returns {".gif"} for both reading and writing. */
    Array<String> getFileExtensions (Mode mode) const override;

    /** Returns true if the first 6 bytes of the stream equal "GIF87a" or "GIF89a". */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a GifImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a GifImageFormatWriter for the given stream and pixel format. */
    std::unique_ptr<ImageFormatWriter> createWriterFor (OutputStream* destStream,
                                                        PixelFormat pixelFormat,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    //==============================================================================
    /** Returns {PixelFormat::RGBA}. */
    Array<PixelFormat> getPossiblePixelFormats() const override;

    /** Returns true because GIF uses LZW compression. */
    bool isCompressed() const override { return true; }

    /** Returns {} - GIF palette quantization is fixed (no quality options). */
    StringArray getQualityOptions() const override { return {}; }

private:
    //==============================================================================
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GifImageFormat)
};

} // namespace yup

#endif // YUP_IMAGE_FORMAT_GIF
