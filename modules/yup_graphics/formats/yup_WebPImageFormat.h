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

struct WebPAnimEncoder; // forward declare (defined in webp/mux.h)

namespace yup
{

//==============================================================================
/**
    Reads image data from WebP formatted streams using libwebp.

    Decodes both lossless and lossy WebP images, including animated WebP.
    Supports RGBA and RGB output pixel formats. Width, height, and pixelFormat
    are populated during construction via WebPGetInfo / WebPDemux.

    For animated images, frame metadata (count, loop, per-frame delays) is
    available immediately after construction via the animation API. Frame
    decoding uses WebPDemux + WebPDecode for random access with manual
    compositing of frame sub-rectangles.

    @see ImageFormatReader, WebPImageFormatWriter, WebPImageFormat
*/
class YUP_API WebPImageFormatReader : public ImageFormatReader
{
public:
    //==============================================================================
    /** Constructs the reader and parses the WebP header from the stream.

        Reads the entire stream into memory. For animated WebP, uses WebPDemux
        to extract canvas dimensions, frame count, loop count, and per-frame
        metadata. For static images, falls back to WebPGetFeatures.

        Takes ownership of the stream.

        @param stream  The source stream. This object takes ownership.
        @param options Options controlling metadata extraction.
    */
    explicit WebPImageFormatReader (InputStream* stream, const ImageFormat::Options& options = {});

    //==============================================================================
    /** Decodes the full image from the buffered WebP data.

        For animated images, returns frame 0 as a fully composited Image.
        @returns The decoded Image, or a default-constructed (invalid) Image on failure.
    */
    Image readImage() override;

    //==============================================================================
    /** Decodes and returns a single animation frame by index.

        Sequential access (0, 1, 2, …) reuses the internal canvas (O(1) per frame).
        Backward seeks re-composite from frame 0.
        @param frameIndex  Zero-based frame index.
        @returns The decoded frame Image, or an invalid Image if frameIndex is out of range.
    */
    Image readFrame (int frameIndex) override;

    /** Decodes a frame directly into an existing Image, avoiding allocation when possible.
        If dest has the correct width, height, and PixelFormat::RGBA, decodes directly
        into dest's raw pixel buffer — zero allocation.
        @param frameIndex  Zero-based frame index.
        @param dest        Image to decode into (may be reallocated).
        @returns true on success.
    */
    bool readFrame (int frameIndex, Image& dest) override;

    /** Returns true when the WebP contains more than one frame. */
    bool isAnimated() const override;

    /** Returns the number of frames in the WebP animation. */
    int getFrameCount() const override;

    /** Returns the loop count from the ANIM chunk.
        0 = loop infinitely; 1 = play once (also returned for non-animated images).
    */
    int getLoopCount() const override;

    /** Returns the display duration for a frame in milliseconds. */
    int getFrameDelayMs (int frameIndex) const override;

private:
    //==============================================================================
    struct FrameInfo
    {
        std::vector<uint8_t> fragmentData;
        int xOffset = 0;
        int yOffset = 0;
        int frameWidth = 0;
        int frameHeight = 0;
        int durationMs = 0;
        int disposeMethod = 0; // WEBP_MUX_DISPOSE_NONE (0) or WEBP_MUX_DISPOSE_BACKGROUND (1)
        int blendMethod = 0;   // WEBP_MUX_BLEND (0) or WEBP_MUX_NO_BLEND (1)
    };

    void compositeFrame (int frameIndex);
    void resetCanvas();

    std::vector<uint8_t> fileData;
    std::vector<FrameInfo> frames;
    Image canvas;
    int loopCount = 1;
    int lastRenderedFrame = -1;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebPImageFormatReader)
};

//==============================================================================
/**
    Writes image data to WebP formatted streams using libwebp.

    Supports lossless (qualityIndex 0) and lossy encoding at quality levels 90,
    80, 60, and 40 (qualityIndex 1–4). Both RGBA and RGB pixel formats are supported.

    Also supports animated WebP output via the beginAnimation() / writeFrame() /
    endAnimation() sequence, using WebPAnimEncoder internally.

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

    /** Destructor — flushes any open animation state. */
    ~WebPImageFormatWriter() override;

    //==============================================================================
    /** Encodes and writes a single-frame WebP to the output stream.
        @returns true on success.
    */
    bool writeImage (const Image& image) override;

    //==============================================================================
    /** Returns true — WebP supports animated output. */
    bool supportsAnimation() const override { return true; }

    /** Begins an animated WebP sequence.

        Must be called before writeFrame(). Initialises the WebPAnimEncoder
        with the animation canvas dimensions, background color, and loop count.

        @param loopCount  0 = loop infinitely; 1 = play once; N = play N times.
        @returns true on success.
    */
    bool beginAnimation (int loopCount = 0) override;

    /** Encodes and appends one animation frame.

        Must be called between beginAnimation() and endAnimation(). Each call
        converts the frame to WebPPicture format and adds it to the encoder
        with the given timestamp.

        @param frame    Frame image to encode (RGBA expected).
        @param delayMs  Display duration for this frame in milliseconds.
        @returns true on success.
    */
    bool writeFrame (const Image& frame, int delayMs) override;

    /** Finalises the animated WebP sequence.

        Assembles all frames into a single WebP bitstream and writes it to
        the output stream.

        @returns true on success.
    */
    bool endAnimation() override;

private:
    //==============================================================================
    int qualityIndex = 0;

    struct WebPAnimEncoderDeleter
    {
        void operator() (::WebPAnimEncoder* p) const noexcept;
    };

    std::unique_ptr<::WebPAnimEncoder, WebPAnimEncoderDeleter> animEncoder;
    int animWidth = 0;
    int animHeight = 0;
    int animTimestampMs = 0;
    int animLoopCount = 0;

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
    StringArray getFileExtensions (Mode mode) const override;

    /** Returns true if the stream starts with the RIFF/WEBP header. */
    bool canHandleStream (InputStream& stream, Mode mode) const override;

    //==============================================================================
    /** Creates a WebPImageFormatReader for the given stream. */
    std::unique_ptr<ImageFormatReader> createReaderFor (InputStream* sourceStream,
                                                        const Options& options = {}) override;

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
