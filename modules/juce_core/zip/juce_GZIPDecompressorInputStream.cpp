/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "zlib/zlib.h"

namespace juce
{

//==============================================================================
// internal helper object that holds the zlib structures so they don't have to be
// included publicly.
class GZIPDecompressorInputStream::GZIPDecompressHelper
{
public:
    GZIPDecompressHelper (Format f)
    {
        using namespace zlibNamespace;
        zerostruct (stream);
        streamIsValid = (inflateInit2 (&stream, getBitsForFormat (f)) == Z_OK);
        finished = error = ! streamIsValid;
    }

    ~GZIPDecompressHelper()
    {
        if (streamIsValid)
            zlibNamespace::inflateEnd (&stream);
    }

    bool needsInput() const noexcept { return dataSize <= 0; }

    void setInput (uint8* const data_, const size_t size) noexcept
    {
        data = data_;
        dataSize = size;
    }

    int doNextBlock (uint8* const dest, const unsigned int destSize)
    {
        using namespace zlibNamespace;

        if (streamIsValid && data != nullptr && ! finished)
        {
            stream.next_in = data;
            stream.next_out = dest;
            stream.avail_in = (z_uInt) dataSize;
            stream.avail_out = (z_uInt) destSize;

            switch (inflate (&stream, Z_PARTIAL_FLUSH))
            {
                case Z_STREAM_END:
                    finished = true;
                    JUCE_FALLTHROUGH
                case Z_OK:
                    data += dataSize - stream.avail_in;
                    dataSize = (z_uInt) stream.avail_in;
                    return (int) (destSize - stream.avail_out);

                case Z_NEED_DICT:
                    needsDictionary = true;
                    data += dataSize - stream.avail_in;
                    dataSize = (size_t) stream.avail_in;
                    break;

                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    error = true;
                    JUCE_FALLTHROUGH
                default:
                    break;
            }
        }

        return 0;
    }

    static int getBitsForFormat (Format f) noexcept
    {
        switch (f)
        {
            case zlibFormat:
                return MAX_WBITS;
            case deflateFormat:
                return -MAX_WBITS;
            case gzipFormat:
                return MAX_WBITS | 16;
            default:
                jassertfalse;
                break;
        }

        return MAX_WBITS;
    }

    bool finished = true, needsDictionary = false, error = true, streamIsValid = false;

    enum
    {
        gzipDecompBufferSize = 32768
    };

private:
    zlibNamespace::z_stream stream;
    uint8* data = nullptr;
    size_t dataSize = 0;

    JUCE_DECLARE_NON_COPYABLE (GZIPDecompressHelper)
};

//==============================================================================
GZIPDecompressorInputStream::GZIPDecompressorInputStream (InputStream* source, bool deleteSourceWhenDestroyed, Format f, int64 uncompressedLength)
    : sourceStream (source, deleteSourceWhenDestroyed)
    , uncompressedStreamLength (uncompressedLength)
    , format (f)
    , originalSourcePos (source->getPosition())
    , buffer ((size_t) GZIPDecompressHelper::gzipDecompBufferSize)
    , helper (new GZIPDecompressHelper (f))
{
}

GZIPDecompressorInputStream::GZIPDecompressorInputStream (InputStream& source)
    : sourceStream (&source, false)
    , uncompressedStreamLength (-1)
    , format (zlibFormat)
    , originalSourcePos (source.getPosition())
    , buffer ((size_t) GZIPDecompressHelper::gzipDecompBufferSize)
    , helper (new GZIPDecompressHelper (zlibFormat))
{
}

GZIPDecompressorInputStream::~GZIPDecompressorInputStream()
{
}

int64 GZIPDecompressorInputStream::getTotalLength()
{
    return uncompressedStreamLength;
}

int GZIPDecompressorInputStream::read (void* destBuffer, int howMany)
{
    jassert (destBuffer != nullptr && howMany >= 0);

    if (howMany > 0 && ! isEof)
    {
        int numRead = 0;
        auto d = static_cast<uint8*> (destBuffer);

        while (! helper->error)
        {
            auto n = helper->doNextBlock (d, (unsigned int) howMany);
            currentPos += n;

            if (n == 0)
            {
                if (helper->finished || helper->needsDictionary)
                {
                    isEof = true;
                    return numRead;
                }

                if (helper->needsInput())
                {
                    activeBufferSize = sourceStream->read (buffer, (int) GZIPDecompressHelper::gzipDecompBufferSize);

                    if (activeBufferSize > 0)
                    {
                        helper->setInput (buffer, (size_t) activeBufferSize);
                    }
                    else
                    {
                        isEof = true;
                        return numRead;
                    }
                }
            }
            else
            {
                numRead += n;
                howMany -= n;
                d += n;

                if (howMany <= 0)
                    return numRead;
            }
        }
    }

    return 0;
}

bool GZIPDecompressorInputStream::isExhausted()
{
    return helper->error || helper->finished || isEof;
}

int64 GZIPDecompressorInputStream::getPosition()
{
    return currentPos;
}

bool GZIPDecompressorInputStream::setPosition (int64 newPos)
{
    if (newPos < currentPos)
    {
        // to go backwards, reset the stream and start again..
        isEof = false;
        activeBufferSize = 0;
        currentPos = 0;
        helper.reset (new GZIPDecompressHelper (format));

        sourceStream->setPosition (originalSourcePos);
    }

    skipNextBytes (newPos - currentPos);
    return true;
}

} // namespace juce
