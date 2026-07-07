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
// Internal helpers (file-scope)
//==============================================================================

namespace
{

/** Skips whitespace bytes (space, tab, CR, LF) and comment lines starting with '#'. */
static void skipWhitespaceAndComments (InputStream& stream)
{
    for (;;)
    {
        if (stream.isExhausted())
            return;

        const char c = stream.readByte();

        if (c == '#')
        {
            // Skip until end of line.
            for (;;)
            {
                if (stream.isExhausted())
                    return;

                const char lc = stream.readByte();

                if (lc == '\n' || lc == '\r')
                    break;
            }
        }
        else if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            // Keep consuming whitespace.
            continue;
        }
        else
        {
            // Not whitespace / comment - push back by seeking one byte back.
            stream.setPosition (stream.getPosition() - 1);
            return;
        }
    }
}

/** Reads an ASCII non-negative integer from the stream. Returns -1 on error. */
static int readAsciiInt (InputStream& stream)
{
    skipWhitespaceAndComments (stream);

    if (stream.isExhausted())
        return -1;

    int value = 0;
    bool gotDigit = false;

    for (;;)
    {
        if (stream.isExhausted())
            break;

        const char c = stream.readByte();

        if (c >= '0' && c <= '9')
        {
            value = value * 10 + (c - '0');
            gotDigit = true;
        }
        else
        {
            // End of number - put the non-digit character back.
            stream.setPosition (stream.getPosition() - 1);
            break;
        }
    }

    return gotDigit ? value : -1;
}

/** Normalises a sample value to [0, 255] given the file's maxval. */
static uint8 normalise (int sample, int maxval)
{
    if (maxval <= 0 || maxval == 255)
        return static_cast<uint8> (sample);

    return static_cast<uint8> ((static_cast<int> (sample) * 255) / maxval);
}

} // namespace

//==============================================================================
// PpmImageFormatReader
//==============================================================================

PpmImageFormatReader::PpmImageFormatReader (InputStream* stream)
    : ImageFormatReader (stream, "PPM/PGM/PBM Image")
{
    if (input == nullptr)
        return;

    // Read and validate magic "Pn".
    char p = input->readByte();
    char n = input->readByte();

    if (p != 'P' || n < '1' || n > '6')
        return;

    magic = n - '0';

    // Read width and height.
    const int w = readAsciiInt (*input);
    const int h = readAsciiInt (*input);

    if (w <= 0 || h <= 0)
        return;

    width = w;
    height = h;

    // Read maxval for non-bitmap types.
    if (magic == 1 || magic == 4)
    {
        // Bitmap: one mandatory whitespace byte after height, then data.
        maxval = 1;

        // Consume the single mandatory whitespace separating header from data.
        if (! input->isExhausted())
            input->readByte();
    }
    else
    {
        maxval = readAsciiInt (*input);

        if (maxval <= 0 || maxval > 65535)
        {
            width = 0;
            height = 0;
            return;
        }

        // Consume the single mandatory whitespace separating header from data.
        if (! input->isExhausted())
            input->readByte();
    }

    // Determine pixel format.
    switch (magic)
    {
        case 3:
        case 6:
            pixelFormat = PixelFormat::RGB;
            break;

        default: // P1, P2, P4, P5
            pixelFormat = PixelFormat::Grayscale;
            break;
    }
}

Image PpmImageFormatReader::readImage()
{
    if (input == nullptr || width <= 0 || height <= 0)
        return {};

    Image image (width, height, pixelFormat);

    switch (magic)
    {
        //----------------------------------------------------------------------
        case 1: // ASCII bitmap - '0' = white (255), '1' = black (0)
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    skipWhitespaceAndComments (*input);

                    if (input->isExhausted())
                        return {};

                    const char c = input->readByte();
                    const uint8 gray = (c == '1') ? 0u : 255u;
                    image.setPixel (x, y, 0xff000000u | (static_cast<uint32> (gray) << 16) | (static_cast<uint32> (gray) << 8) | static_cast<uint32> (gray));
                }
            }

            break;
        }

        //----------------------------------------------------------------------
        case 2: // ASCII grayscale
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const int sample = readAsciiInt (*input);

                    if (sample < 0)
                        return {};

                    const uint8 gray = normalise (sample, maxval);
                    image.setPixel (x, y, 0xff000000u | (static_cast<uint32> (gray) << 16) | (static_cast<uint32> (gray) << 8) | static_cast<uint32> (gray));
                }
            }

            break;
        }

        //----------------------------------------------------------------------
        case 3: // ASCII RGB
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const int r = readAsciiInt (*input);
                    const int g = readAsciiInt (*input);
                    const int b = readAsciiInt (*input);

                    if (r < 0 || g < 0 || b < 0)
                        return {};

                    image.setPixel (x, y, 0xff000000u | (static_cast<uint32> (normalise (r, maxval)) << 16) | (static_cast<uint32> (normalise (g, maxval)) << 8) | static_cast<uint32> (normalise (b, maxval)));
                }
            }

            break;
        }

        //----------------------------------------------------------------------
        case 4: // Binary bitmap - each row is packed into ceil(width/8) bytes
        {
            const int rowBytes = (width + 7) / 8;

            for (int y = 0; y < height; ++y)
            {
                for (int byteIndex = 0; byteIndex < rowBytes; ++byteIndex)
                {
                    if (input->isExhausted())
                        return {};

                    const uint8 packedByte = static_cast<uint8> (input->readByte());

                    for (int bit = 7; bit >= 0; --bit)
                    {
                        const int x = byteIndex * 8 + (7 - bit);

                        if (x >= width)
                            break;

                        // In P4, bit 1 = black (0), bit 0 = white (255).
                        const uint8 gray = ((packedByte >> bit) & 1u) ? 0u : 255u;
                        image.setPixel (x, y, 0xff000000u | (static_cast<uint32> (gray) << 16) | (static_cast<uint32> (gray) << 8) | static_cast<uint32> (gray));
                    }
                }
            }

            break;
        }

        //----------------------------------------------------------------------
        case 5: // Binary grayscale
        {
            const bool is16Bit = (maxval > 255);

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    if (input->isExhausted())
                        return {};

                    int sample = 0;

                    if (is16Bit)
                    {
                        const uint8 hi = static_cast<uint8> (input->readByte());
                        const uint8 lo = static_cast<uint8> (input->readByte());
                        sample = (static_cast<int> (hi) << 8) | static_cast<int> (lo);
                    }
                    else
                    {
                        sample = static_cast<uint8> (input->readByte());
                    }

                    const uint8 gray = normalise (sample, maxval);
                    image.setPixel (x, y, 0xff000000u | (static_cast<uint32> (gray) << 16) | (static_cast<uint32> (gray) << 8) | static_cast<uint32> (gray));
                }
            }

            break;
        }

        //----------------------------------------------------------------------
        case 6: // Binary RGB
        {
            const bool is16Bit = (maxval > 255);

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    if (input->isExhausted())
                        return {};

                    int r = 0, g = 0, b = 0;

                    if (is16Bit)
                    {
                        const uint8 rhi = static_cast<uint8> (input->readByte());
                        const uint8 rlo = static_cast<uint8> (input->readByte());
                        r = (static_cast<int> (rhi) << 8) | static_cast<int> (rlo);

                        const uint8 ghi = static_cast<uint8> (input->readByte());
                        const uint8 glo = static_cast<uint8> (input->readByte());
                        g = (static_cast<int> (ghi) << 8) | static_cast<int> (glo);

                        const uint8 bhi = static_cast<uint8> (input->readByte());
                        const uint8 blo = static_cast<uint8> (input->readByte());
                        b = (static_cast<int> (bhi) << 8) | static_cast<int> (blo);
                    }
                    else
                    {
                        r = static_cast<uint8> (input->readByte());
                        g = static_cast<uint8> (input->readByte());
                        b = static_cast<uint8> (input->readByte());
                    }

                    image.setPixel (x, y, 0xff000000u | (static_cast<uint32> (normalise (r, maxval)) << 16) | (static_cast<uint32> (normalise (g, maxval)) << 8) | static_cast<uint32> (normalise (b, maxval)));
                }
            }

            break;
        }

        default:
            return {};
    }

    return image;
}

//==============================================================================
// PpmImageFormatWriter
//==============================================================================

PpmImageFormatWriter::PpmImageFormatWriter (OutputStream* stream, PixelFormat fmt)
    : ImageFormatWriter (stream, "PPM/PGM/PBM Image", fmt)
{
}

bool PpmImageFormatWriter::writeImage (const Image& image)
{
    if (output == nullptr || ! image.isValid())
        return false;

    const int w = image.getWidth();
    const int h = image.getHeight();
    const bool isGray = (getPixelFormat() == PixelFormat::Grayscale);

    // Write binary PGM (P5) or binary PPM (P6) header.
    const char* magic = isGray ? "P5\n" : "P6\n";

    if (! output->write (magic, 3))
        return false;

    // Write "width height\n255\n".
    const String header = String (w) + " " + String (h) + "\n255\n";

    if (! output->write (header.toRawUTF8(), static_cast<size_t> (header.length())))
        return false;

    // Write pixel data row by row.
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const uint32 argb = image.getPixel (x, y);
            const uint8 r = static_cast<uint8> ((argb >> 16) & 0xffu);
            const uint8 g = static_cast<uint8> ((argb >> 8) & 0xffu);
            const uint8 b = static_cast<uint8> (argb & 0xffu);

            if (isGray)
            {
                const uint8 gray = static_cast<uint8> ((static_cast<uint32> (r) * 77u
                                                        + static_cast<uint32> (g) * 150u
                                                        + static_cast<uint32> (b) * 29u)
                                                       >> 8);

                if (! output->write (&gray, 1))
                    return false;
            }
            else
            {
                const uint8 rgb[3] = { r, g, b };

                if (! output->write (rgb, 3))
                    return false;
            }
        }
    }

    return true;
}

//==============================================================================
// PpmImageFormat
//==============================================================================

PpmImageFormat::PpmImageFormat()
    : formatName ("PPM/PGM/PBM Image")
{
}

const String& PpmImageFormat::getFormatName() const
{
    return formatName;
}

Array<String> PpmImageFormat::getFileExtensions (Mode /*mode*/) const
{
    return { ".ppm", ".pgm", ".pbm" };
}

bool PpmImageFormat::canHandleStream (InputStream& stream, Mode /*mode*/) const
{
    uint8 sig[2] = {};
    stream.read (sig, 2);
    stream.setPosition (0);
    return sig[0] == 'P' && sig[1] >= '1' && sig[1] <= '6';
}

std::unique_ptr<ImageFormatReader> PpmImageFormat::createReaderFor (InputStream* sourceStream)
{
    return std::make_unique<PpmImageFormatReader> (sourceStream);
}

std::unique_ptr<ImageFormatWriter> PpmImageFormat::createWriterFor (OutputStream* destStream,
                                                                    PixelFormat pixelFormat,
                                                                    const StringPairArray& /*metadataValues*/,
                                                                    int /*qualityOptionIndex*/)
{
    return std::make_unique<PpmImageFormatWriter> (destStream, pixelFormat);
}

Array<PixelFormat> PpmImageFormat::getPossiblePixelFormats() const
{
    return { PixelFormat::Grayscale, PixelFormat::RGB };
}

} // namespace yup
