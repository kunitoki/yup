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
    Holds image metadata extracted from image file formats.

    ImageMetadata is a ref-counted object attached to Image, storing text key-value
    pairs (title, author, comment, etc.), DPI information, and raw binary chunks
    (EXIF profiles, ICC color profiles, XMP packets, and custom format-specific chunks).

    EXIF helpers parse the raw EXIF blob on demand via a self-contained TIFF IFD walker,
    providing convenient access to common tags like orientation, GPS coordinates,
    camera make/model, and creation date.

    Metadata is only extracted when requested via ImageFormat::Options — by default
    no metadata is collected, ensuring zero overhead for users who don't need it.

    @see Image, ImageFormat::Options, ImageFormatReader
*/
class YUP_API ImageMetadata : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<ImageMetadata>;

    //==============================================================================
    /** Well-known raw chunk keys (format-prefixed).

        Readers store binary blobs under these keys; writers look them up to
        re-embed them in the output file.
    */
    static const String kExifChunk;
    static const String kIccChunk;
    static const String kXmpChunk;

    //==============================================================================
    /** Creates an empty metadata object. */
    static Ptr create() { return *new ImageMetadata(); }

    /** Creates an empty metadata object (alias for create()). */
    static Ptr createEmpty() { return create(); }

    //==============================================================================
    /** Horizontal DPI extracted from the image (0.0 if unknown). */
    double dpiX = 0.0;

    /** Vertical DPI extracted from the image (0.0 if unknown). */
    double dpiY = 0.0;

    /** Text key-value pairs (title, author, comment, software, etc.). */
    StringPairArray textEntries;

    /** Raw binary chunks keyed by format-prefixed chunk type.

        Examples: "jpeg/exif", "jpeg/icc", "jpeg/xmp", "png/iCCP", "png/chunk_cuSt",
        "tiff/exif", "tiff/xmp", "webp/EXIF", "webp/ICCP", "gif/xmp".
    */
    std::unordered_map<String, MemoryBlock> rawChunks;

    //==============================================================================
    /** Returns a raw chunk by key, or nullptr if not present. */
    const MemoryBlock* getRawChunk (const String& key) const;

    /** Returns true if any raw chunk is present under the given key. */
    bool hasRawChunk (const String& key) const;

    /** Sets a raw chunk, replacing any existing entry with the same key. */
    void setRawChunk (String key, MemoryBlock data);

    //==============================================================================
    // --- Lazy EXIF helpers (parse from any rawChunks entry whose key ends with "/exif") ---

    /** Returns the EXIF orientation (1-8), or 0 if no EXIF data is present.
        Standard EXIF orientation values:
        - 1 = Normal (0°)
        - 2 = Flipped horizontally
        - 3 = Rotated 180°
        - 4 = Flipped vertically
        - 5 = Rotated 90° CW and flipped horizontally
        - 6 = Rotated 90° CW
        - 7 = Rotated 90° CCW and flipped horizontally
        - 8 = Rotated 90° CCW
    */
    int getOrientation() const;

    /** Returns the creation date string (e.g. "2024:06:15 14:30:00"), or empty if not found. */
    String getCreationDate() const;

    /** Returns the camera make (e.g. "Canon"), or empty if not found. */
    String getCameraMake() const;

    /** Returns the camera model (e.g. "EOS R5"), or empty if not found. */
    String getCameraModel() const;

    /** Returns GPS coordinates as {latitude, longitude}, or {0, 0} if not found. */
    std::pair<double, double> getGpsCoordinates() const;

    /** Returns the image description, or empty if not found. */
    String getImageDescription() const;

    /** Returns the copyright string, or empty if not found. */
    String getCopyright() const;

    /** Returns the software string, or empty if not found. */
    String getSoftware() const;

private:
    //==============================================================================
    ImageMetadata() = default;

    /** Finds the first rawChunks entry whose key ends with "/exif" and returns its data pointer + size. */
    const uint8* findExifData (size_t& size) const;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageMetadata)
};

} // namespace yup
