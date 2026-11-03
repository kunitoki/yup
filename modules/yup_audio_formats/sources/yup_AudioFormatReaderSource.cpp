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
AudioFormatReaderSource::AudioFormatReaderSource (AudioFormatReader* sourceReader,
                                                  bool deleteReaderWhenThisIsDeleted)
    : reader (sourceReader)
    , deleteReader (deleteReaderWhenThisIsDeleted)
{
    // Allow null reader — source produces silence
}

AudioFormatReaderSource::AudioFormatReaderSource (std::unique_ptr<AudioFormatReader> sourceReader)
    : ownedReader (std::move (sourceReader))
    , reader (ownedReader.get())
    , deleteReader (false)
{
}

AudioFormatReaderSource::~AudioFormatReaderSource()
{
    if (reader != nullptr && deleteReader)
        delete reader;
}

//==============================================================================
int64 AudioFormatReaderSource::getTotalLength() const
{
    if (reader == nullptr)
        return 0;
    return reader->lengthInSamples;
}

void AudioFormatReaderSource::setNextReadPosition (int64 newPosition)
{
    if (newPosition < 0)
        newPosition = 0;

    nextReadPosition = newPosition;
}

int64 AudioFormatReaderSource::getNextReadPosition() const
{
    const auto totalLength = getTotalLength();

    if (looping && totalLength > 0)
        return nextReadPosition % totalLength;

    return nextReadPosition;
}

bool AudioFormatReaderSource::isLooping() const
{
    return looping;
}

void AudioFormatReaderSource::setLooping (bool shouldLoop)
{
    looping = shouldLoop;
}

//==============================================================================
void AudioFormatReaderSource::prepareToPlay (int /*samplesPerBlockExpected*/,
                                             double /*sampleRate*/)
{
}

void AudioFormatReaderSource::releaseResources()
{
}

void AudioFormatReaderSource::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    if (reader == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    const auto totalLength = getTotalLength();

    if (totalLength <= 0)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    if (looping)
    {
        auto position = nextReadPosition % totalLength;
        int destOffset = 0;

        while (destOffset < bufferToFill.numSamples)
        {
            const auto numToRead = static_cast<int> (jmin (static_cast<int64> (bufferToFill.numSamples - destOffset), totalLength - position));

            reader->read (bufferToFill.buffer,
                          bufferToFill.startSample + destOffset,
                          numToRead,
                          position,
                          true,
                          true);

            destOffset += numToRead;
            position += numToRead;

            if (position >= totalLength)
                position = 0;
        }

        nextReadPosition = position;
        return;
    }

    const auto numToRead = static_cast<int> (jlimit (static_cast<int64> (0),
                                                     static_cast<int64> (bufferToFill.numSamples),
                                                     totalLength - nextReadPosition));

    if (numToRead > 0)
    {
        reader->read (bufferToFill.buffer,
                      bufferToFill.startSample,
                      numToRead,
                      nextReadPosition,
                      true,
                      true);
    }

    if (numToRead < bufferToFill.numSamples)
    {
        bufferToFill.buffer->clear (bufferToFill.startSample + numToRead,
                                    bufferToFill.numSamples - numToRead);
    }

    nextReadPosition = jmin (nextReadPosition + bufferToFill.numSamples, totalLength);
}

} // namespace yup
