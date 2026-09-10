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

    if (totalLength > 0)
    {
        auto samplesAvailable = totalLength - nextReadPosition;

        if (samplesAvailable < bufferToFill.numSamples)
        {
            if (looping)
            {
                auto samplesNeeded = bufferToFill.numSamples;
                auto firstChunk = static_cast<int> (samplesAvailable);
                auto secondChunk = samplesNeeded - firstChunk;

                // Read first chunk from the end of the file
                if (firstChunk > 0)
                {
                    reader->read (bufferToFill.buffer,
                                  bufferToFill.startSample,
                                  firstChunk,
                                  nextReadPosition,
                                  true,
                                  true);
                }

                // Read second chunk from the beginning of the file
                if (secondChunk > 0)
                {
                    reader->read (bufferToFill.buffer,
                                  bufferToFill.startSample + firstChunk,
                                  secondChunk,
                                  0,
                                  true,
                                  true);
                }

                nextReadPosition = secondChunk;
            }
            else
            {
                // Read what's left and clear the rest
                auto numToRead = static_cast<int> (samplesAvailable);

                reader->read (bufferToFill.buffer,
                              bufferToFill.startSample,
                              numToRead,
                              nextReadPosition,
                              true,
                              true);

                bufferToFill.buffer->clear (bufferToFill.startSample + numToRead,
                                            bufferToFill.numSamples - numToRead);

                nextReadPosition = totalLength;
            }
        }
        else
        {
            reader->read (bufferToFill.buffer,
                          bufferToFill.startSample,
                          bufferToFill.numSamples,
                          nextReadPosition,
                          true,
                          true);

            nextReadPosition += bufferToFill.numSamples;
        }
    }
    else
    {
        bufferToFill.clearActiveBufferRegion();
    }
}

} // namespace yup
