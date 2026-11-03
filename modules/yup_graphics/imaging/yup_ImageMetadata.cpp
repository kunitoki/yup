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

namespace yup
{

//==============================================================================
const String ImageMetadata::kExifChunk ("exif");
const String ImageMetadata::kIccChunk ("icc");
const String ImageMetadata::kXmpChunk ("xmp");

//==============================================================================
const MemoryBlock* ImageMetadata::getRawChunk (const String& key) const
{
    auto it = rawChunks.find (key);
    return it != rawChunks.end() ? &it->second : nullptr;
}

bool ImageMetadata::hasRawChunk (const String& key) const
{
    return rawChunks.find (key) != rawChunks.end();
}

void ImageMetadata::setRawChunk (String key, MemoryBlock data)
{
    rawChunks.insert_or_assign (std::move (key), std::move (data));
}

//==============================================================================
// EXIF helpers — defined inside ExifIfd namespace below
//==============================================================================

const uint8* ImageMetadata::findExifData (size_t& size) const
{
    for (const auto& [key, chunk] : rawChunks)
    {
        if (key.endsWith ("/exif"))
        {
            size = chunk.getSize();
            return static_cast<const uint8*> (chunk.getData());
        }
    }

    size = 0;
    return nullptr;
}

//==============================================================================
// Lazy EXIF IFD walker
//==============================================================================

namespace ExifIfd
{
// Standard IFD tag IDs
enum Tag : uint16
{
    Orientation = 0x0112,
    DateTime = 0x0132,
    Make = 0x010F,
    Model = 0x0110,
    ImageDescription = 0x010E,
    Copyright = 0x8298,
    Software = 0x0131,
    ExifIFDPointer = 0x8769,
    GPSIFDPointer = 0x8825,
};

enum GPSTag : uint16
{
    GPSLatitudeRef = 0x0001,
    GPSLatitude = 0x0002,
    GPSLongitudeRef = 0x0003,
    GPSLongitude = 0x0004,
};

// TIFF data types
enum Type : uint16
{
    BYTE = 1,
    ASCII = 2,
    SHORT = 3,
    LONG = 4,
    RATIONAL = 5,
    UNDEFINED = 7,
};

//==============================================================================
// Low-level EXIF byte readers
//==============================================================================

static uint16 read16 (const uint8* data, bool bigEndian)
{
    return bigEndian
             ? static_cast<uint16> ((data[0] << 8) | data[1])
             : static_cast<uint16> (data[0] | (data[1] << 8));
}

static uint32 read32 (const uint8* data, bool bigEndian)
{
    return bigEndian
             ? static_cast<uint32> ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3])
             : static_cast<uint32> (data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
}

static bool isRangeValid (size_t offset, size_t length, size_t size)
{
    return offset <= size && length <= size - offset;
}

static String readExifString (const uint8* tiffBase, size_t size, const uint8* data, uint32 length, bool bigEndian)
{
    if (data == nullptr || length == 0 || length > static_cast<uint32> (std::numeric_limits<int>::max()))
        return {};

    if (length > 4)
    {
        auto offset = read32 (data, bigEndian);
        if (! isRangeValid (offset, length, size))
            return {};

        return String::createStringFromData (reinterpret_cast<const char*> (tiffBase + offset),
                                             static_cast<int> (length));
    }

    return String::createStringFromData (reinterpret_cast<const char*> (data),
                                         static_cast<int> (length));
}

static double readExifRational (const uint8* tiffBase, const uint8* data, bool bigEndian)
{
    if (data == nullptr)
        return 0.0;

    auto num = read32 (data, bigEndian);
    auto den = read32 (data + 4, bigEndian);

    if (den == 0)
        return 0.0;

    return static_cast<double> (num) / static_cast<double> (den);
}

static uint32 typeSize (uint16 type)
{
    switch (type)
    {
        case BYTE:
        case ASCII:
        case UNDEFINED:
            return 1;
        case SHORT:
            return 2;
        case LONG:
            return 4;
        case RATIONAL:
            return 8;
        default:
            return 0;
    }
}

struct IFDEntry
{
    uint16 tag;
    uint16 type;
    uint32 count;
    const uint8* data; // points to the 4-byte value/offset field
};

static void walkIFD (const uint8* tiffBase, size_t size, size_t ifdOffset, bool bigEndian, std::function<void (const IFDEntry&)> callback)
{
    if (tiffBase == nullptr || ! isRangeValid (ifdOffset, 2, size))
        return;

    auto numEntries = read16 (tiffBase + ifdOffset, bigEndian);
    if (! isRangeValid (ifdOffset + 2, static_cast<size_t> (numEntries) * 12, size))
        return;

    auto* entry = tiffBase + ifdOffset + 2;

    for (uint16 i = 0; i < numEntries; ++i, entry += 12)
    {
        IFDEntry e;
        e.tag = read16 (entry, bigEndian);
        e.type = read16 (entry + 2, bigEndian);
        e.count = read32 (entry + 4, bigEndian);
        e.data = entry + 8;
        callback (e);
    }
}

static String readAsciiValue (const uint8* tiffBase, size_t size, const IFDEntry& e, bool bigEndian)
{
    return readExifString (tiffBase, size, e.data, e.count, bigEndian);
}

static bool readGPSCoordinate (const uint8* tiffBase, size_t size, const IFDEntry& e, bool bigEndian, double& result)
{
    if (e.type != RATIONAL || e.count < 3)
        return false;

    auto offset = read32 (e.data, bigEndian);
    if (! isRangeValid (offset, 24, size))
        return false;

    auto deg = readExifRational (tiffBase, tiffBase + offset, bigEndian);
    auto min = readExifRational (tiffBase, tiffBase + offset + 8, bigEndian);
    auto sec = readExifRational (tiffBase, tiffBase + offset + 16, bigEndian);
    result = deg + min / 60.0 + sec / 3600.0;
    return true;
}
} // namespace ExifIfd

int ImageMetadata::getOrientation() const
{
    size_t size = 0;
    const auto* data = findExifData (size);
    if (data == nullptr || size < 8)
        return 0;

    // Determine endianness
    bool bigEndian = (data[0] == 'M' && data[1] == 'M');

    // Read TIFF header offset to first IFD
    auto ifdOffset = ExifIfd::read32 (data + 4, bigEndian);
    if (! ExifIfd::isRangeValid (ifdOffset, 2, size))
        return 0;

    int orientation = 0;
    ExifIfd::walkIFD (data, size, ifdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        if (e.tag == ExifIfd::Orientation && e.type == ExifIfd::SHORT)
            orientation = ExifIfd::read16 (e.data, bigEndian);
    });

    return orientation;
}

String ImageMetadata::getCreationDate() const
{
    size_t size = 0;
    const auto* data = findExifData (size);
    if (data == nullptr || size < 8)
        return {};

    bool bigEndian = (data[0] == 'M' && data[1] == 'M');
    auto ifdOffset = ExifIfd::read32 (data + 4, bigEndian);
    if (! ExifIfd::isRangeValid (ifdOffset, 2, size))
        return {};

    String result;
    ExifIfd::walkIFD (data, size, ifdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        if (e.tag == ExifIfd::DateTime && e.type == ExifIfd::ASCII)
            result = ExifIfd::readAsciiValue (data, size, e, bigEndian);
    });

    return result;
}

String ImageMetadata::getCameraMake() const
{
    size_t size = 0;
    const auto* data = findExifData (size);
    if (data == nullptr || size < 8)
        return {};

    bool bigEndian = (data[0] == 'M' && data[1] == 'M');
    auto ifdOffset = ExifIfd::read32 (data + 4, bigEndian);
    if (! ExifIfd::isRangeValid (ifdOffset, 2, size))
        return {};

    String result;
    ExifIfd::walkIFD (data, size, ifdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        if (e.tag == ExifIfd::Make && e.type == ExifIfd::ASCII)
            result = ExifIfd::readAsciiValue (data, size, e, bigEndian);
    });

    return result;
}

String ImageMetadata::getCameraModel() const
{
    size_t size = 0;
    const auto* data = findExifData (size);
    if (data == nullptr || size < 8)
        return {};

    bool bigEndian = (data[0] == 'M' && data[1] == 'M');
    auto ifdOffset = ExifIfd::read32 (data + 4, bigEndian);
    if (! ExifIfd::isRangeValid (ifdOffset, 2, size))
        return {};

    String result;
    ExifIfd::walkIFD (data, size, ifdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        if (e.tag == ExifIfd::Model && e.type == ExifIfd::ASCII)
            result = ExifIfd::readAsciiValue (data, size, e, bigEndian);
    });

    return result;
}

String ImageMetadata::getImageDescription() const
{
    size_t size = 0;
    const auto* data = findExifData (size);
    if (data == nullptr || size < 8)
        return {};

    bool bigEndian = (data[0] == 'M' && data[1] == 'M');
    auto ifdOffset = ExifIfd::read32 (data + 4, bigEndian);
    if (! ExifIfd::isRangeValid (ifdOffset, 2, size))
        return {};

    String result;
    ExifIfd::walkIFD (data, size, ifdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        if (e.tag == ExifIfd::ImageDescription && e.type == ExifIfd::ASCII)
            result = ExifIfd::readAsciiValue (data, size, e, bigEndian);
    });

    return result;
}

String ImageMetadata::getCopyright() const
{
    size_t size = 0;
    const auto* data = findExifData (size);
    if (data == nullptr || size < 8)
        return {};

    bool bigEndian = (data[0] == 'M' && data[1] == 'M');
    auto ifdOffset = ExifIfd::read32 (data + 4, bigEndian);
    if (! ExifIfd::isRangeValid (ifdOffset, 2, size))
        return {};

    String result;
    ExifIfd::walkIFD (data, size, ifdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        if (e.tag == ExifIfd::Copyright && e.type == ExifIfd::ASCII)
            result = ExifIfd::readAsciiValue (data, size, e, bigEndian);
    });

    return result;
}

String ImageMetadata::getSoftware() const
{
    size_t size = 0;
    const auto* data = findExifData (size);
    if (data == nullptr || size < 8)
        return {};

    bool bigEndian = (data[0] == 'M' && data[1] == 'M');
    auto ifdOffset = ExifIfd::read32 (data + 4, bigEndian);
    if (! ExifIfd::isRangeValid (ifdOffset, 2, size))
        return {};

    String result;
    ExifIfd::walkIFD (data, size, ifdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        if (e.tag == ExifIfd::Software && e.type == ExifIfd::ASCII)
            result = ExifIfd::readAsciiValue (data, size, e, bigEndian);
    });

    return result;
}

std::pair<double, double> ImageMetadata::getGpsCoordinates() const
{
    size_t size = 0;
    const auto* data = findExifData (size);
    if (data == nullptr || size < 8)
        return { 0.0, 0.0 };

    bool bigEndian = (data[0] == 'M' && data[1] == 'M');
    auto ifdOffset = ExifIfd::read32 (data + 4, bigEndian);
    if (! ExifIfd::isRangeValid (ifdOffset, 2, size))
        return { 0.0, 0.0 };

    double lat = 0.0, lon = 0.0;
    bool hasLat = false, hasLon = false;
    double latRef = 1.0, lonRef = 1.0;

    // Walk IFD0 for GPS IFD pointer
    std::optional<uint32> gpsIfdOffset;
    ExifIfd::walkIFD (data, size, ifdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        if (e.tag == ExifIfd::GPSIFDPointer && e.type == ExifIfd::LONG)
            gpsIfdOffset = ExifIfd::read32 (e.data, bigEndian);
    });

    if (! gpsIfdOffset.has_value())
        return { 0.0, 0.0 };

    // Walk GPS IFD
    ExifIfd::walkIFD (data, size, *gpsIfdOffset, bigEndian, [&] (const ExifIfd::IFDEntry& e)
    {
        switch (e.tag)
        {
            case ExifIfd::GPSLatitudeRef:
                if (e.type == ExifIfd::ASCII)
                    latRef = (e.data[0] == 'S') ? -1.0 : 1.0;
                break;

            case ExifIfd::GPSLongitudeRef:
                if (e.type == ExifIfd::ASCII)
                    lonRef = (e.data[0] == 'W') ? -1.0 : 1.0;
                break;

            case ExifIfd::GPSLatitude:
                if (ExifIfd::readGPSCoordinate (data, size, e, bigEndian, lat))
                    hasLat = true;
                break;

            case ExifIfd::GPSLongitude:
                if (ExifIfd::readGPSCoordinate (data, size, e, bigEndian, lon))
                    hasLon = true;
                break;
        }
    });

    return { hasLat ? lat * latRef : 0.0, hasLon ? lon * lonRef : 0.0 };
}

} // namespace yup
