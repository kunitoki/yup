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

namespace yup
{

static int calcBufferStreamBufferSize (int requestedSize, InputStream* source) noexcept
{
    // You need to supply a real stream when creating a BufferedInputStream
    jassert (source != nullptr);

    requestedSize = jmax (256, requestedSize);
    auto sourceSize = source->getTotalLength();

    if (sourceSize >= 0 && sourceSize < requestedSize)
        return jmax (32, (int) sourceSize);

    return requestedSize;
}

//==============================================================================
BufferedInputStream::BufferedInputStream (InputStream* sourceStream, int size, bool takeOwnership)
    : source (sourceStream, takeOwnership)
    , bufferedRange (sourceStream->getPosition(), sourceStream->getPosition())
    , position (bufferedRange.getStart())
    , bufferLength (calcBufferStreamBufferSize (size, sourceStream))
{
    buffer.malloc (bufferLength);
}

BufferedInputStream::BufferedInputStream (InputStream& sourceStream, int size)
    : BufferedInputStream (&sourceStream, size, false)
{
}

BufferedInputStream::~BufferedInputStream() = default;

//==============================================================================
char BufferedInputStream::peekByte()
{
    if (! ensureBuffered())
        return 0;

    return position < lastReadPos ? buffer[(int) (position - bufferedRange.getStart())] : 0;
}

int64 BufferedInputStream::getTotalLength()
{
    return source->getTotalLength();
}

int64 BufferedInputStream::getPosition()
{
    return position;
}

bool BufferedInputStream::setPosition (int64 newPosition)
{
    position = jmax ((int64) 0, newPosition);
    return true;
}

bool BufferedInputStream::isExhausted()
{
    return position >= lastReadPos && source->isExhausted();
}

bool BufferedInputStream::ensureBuffered()
{
    auto bufferEndOverlap = lastReadPos - bufferOverlap;

    if (position < bufferedRange.getStart() || position >= bufferEndOverlap)
    {
        int bytesRead = 0;

        if (position < lastReadPos
            && position >= bufferEndOverlap
            && position >= bufferedRange.getStart())
        {
            auto bytesToKeep = (int) (lastReadPos - position);
            memmove (buffer, buffer + (int) (position - bufferedRange.getStart()), (size_t) bytesToKeep);

            bytesRead = source->read (buffer + bytesToKeep,
                                      (int) (bufferLength - bytesToKeep));

            if (bytesRead < 0)
                return false;

            lastReadPos += bytesRead;
            bytesRead += bytesToKeep;
        }
        else
        {
            if (! source->setPosition (position))
                return false;

            bytesRead = (int) source->read (buffer, (size_t) bufferLength);

            if (bytesRead < 0)
                return false;

            lastReadPos = position + bytesRead;
        }

        bufferedRange = Range<int64> (position, lastReadPos);

        while (bytesRead < bufferLength)
            buffer[bytesRead++] = 0;
    }

    return true;
}

int BufferedInputStream::read (void* destBuffer, const int maxBytesToRead)
{
    const auto initialPosition = position;

    const auto getBufferedRange = [this]
    {
        return bufferedRange;
    };

    const auto readFromReservoir = [this, &destBuffer, &initialPosition] (const Range<int64> rangeToRead)
    {
        memcpy (static_cast<char*> (destBuffer) + (rangeToRead.getStart() - initialPosition),
                buffer + (rangeToRead.getStart() - bufferedRange.getStart()),
                (size_t) rangeToRead.getLength());
    };

    const auto fillReservoir = [this] (int64 requestedStart)
    {
        position = requestedStart;
        ensureBuffered();
    };

    const auto remaining = Reservoir::doBufferedRead (Range<int64> (position, position + maxBytesToRead),
                                                      getBufferedRange,
                                                      readFromReservoir,
                                                      fillReservoir);

    const auto bytesRead = maxBytesToRead - remaining.getLength();
    position = remaining.getStart();
    return (int) bytesRead;
}

String BufferedInputStream::readString()
{
    if (position >= bufferedRange.getStart()
        && position < lastReadPos)
    {
        auto maxChars = (int) (lastReadPos - position);
        auto* src = buffer + (int) (position - bufferedRange.getStart());

        for (int i = 0; i < maxChars; ++i)
        {
            if (src[i] == 0)
            {
                position += i + 1;
                return String::fromUTF8 (src, i);
            }
        }
    }

    return InputStream::readString();
}

} // namespace yup
