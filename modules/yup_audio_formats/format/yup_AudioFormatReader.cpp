/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

AudioFormatReader::AudioFormatReader (InputStream* sourceStream, const String& formatName_)
    : formatName (formatName_)
    , input (sourceStream)
{
}

bool AudioFormatReader::read (float* const* destChannels, int numDestChannels, int64 startSampleInSource, int numSamplesToRead)
{
    if (numSamplesToRead <= 0)
        return true;

    const auto numChannelsToRead = jmin (numDestChannels, (int) numChannels);

    if (numChannelsToRead == 0)
        return true;

    // Since readSamples now uses float, we can read directly into destChannels
    if (! readSamples (destChannels, numChannelsToRead, 0, startSampleInSource, numSamplesToRead))
        return false;

    // Clear any remaining channels
    for (int i = numChannelsToRead; i < numDestChannels; ++i)
        if (destChannels[i] != nullptr)
            FloatVectorOperations::clear (destChannels[i], numSamplesToRead);

    return true;
}

bool AudioFormatReader::read (int* const* destChannels, int numDestChannels, int64 startSampleInSource, int numSamplesToRead, bool fillLeftoverChannelsWithCopies)
{
    if (numSamplesToRead <= 0)
        return true;

    const auto numChannelsToRead = jmin (numDestChannels, (int) numChannels);

    if (numChannelsToRead == 0)
        return true;

    // Create temporary float buffers and read into them
    HeapBlock<float> tempBuffer (numChannelsToRead * numSamplesToRead, true);
    HeapBlock<float*> floatChans (numChannelsToRead, false);

    for (int i = 0; i < numChannelsToRead; ++i)
        floatChans[i] = tempBuffer.getData() + i * numSamplesToRead;

    if (! readSamples (floatChans.getData(), numChannelsToRead, 0, startSampleInSource, numSamplesToRead))
        return false;

    // Convert float to int
    for (int i = 0; i < numChannelsToRead; ++i)
        if (destChannels[i] != nullptr)
            FloatVectorOperations::convertFloatToFixed (destChannels[i], floatChans[i], 0x7fffffff, numSamplesToRead);

    if (numChannelsToRead < numDestChannels)
    {
        if (fillLeftoverChannelsWithCopies && numChannelsToRead > 0)
        {
            // Duplicate the existing channels to fill the rest
            for (int i = numChannelsToRead; i < numDestChannels; ++i)
                if (destChannels[i] != nullptr)
                    memcpy (destChannels[i], destChannels[i % numChannelsToRead], sizeof (int) * (size_t) numSamplesToRead);
        }
        else
        {
            // Clear the remaining channels
            for (int i = numChannelsToRead; i < numDestChannels; ++i)
                if (destChannels[i] != nullptr)
                    zeromem (destChannels[i], sizeof (int) * (size_t) numSamplesToRead);
        }
    }

    return true;
}

bool AudioFormatReader::read (AudioBuffer<float>* buffer,
                              int startSampleInDestBuffer,
                              int numSamples,
                              int64 readerStartSample,
                              bool useReaderLeftChan,
                              bool useReaderRightChan)
{
    if (buffer == nullptr)
        return false;

    const auto numCh = buffer->getNumChannels();

    if (numSamples <= 0 || numCh == 0)
        return true;

    // Determine what we actually can and should read
    const bool canReadLeft = useReaderLeftChan;
    const bool canReadRight = useReaderRightChan && (numChannels >= 2);

    // Early exit if nothing to read
    if (! canReadLeft && ! canReadRight)
    {
        buffer->clear (startSampleInDestBuffer, numSamples);
        return true;
    }

    // Allocate temporary float buffer on heap to avoid stack overflow
    const int numChannelsToRead = (canReadLeft ? 1 : 0) + (canReadRight ? 1 : 0);
    HeapBlock<float> tempBuffer ((size_t) (numSamples * numChannelsToRead), true);

    // Set up channel pointers for readSamples
    float* chans[2] = { nullptr, nullptr };

    if (canReadLeft && canReadRight)
    {
        chans[0] = tempBuffer.getData();
        chans[1] = tempBuffer.getData() + numSamples;
    }
    else if (canReadLeft)
    {
        chans[0] = tempBuffer.getData();
    }
    else // canReadRight only
    {
        chans[1] = tempBuffer.getData();
    }

    // Read the raw samples
    if (! readSamples (chans, 2, 0, readerStartSample, numSamples))
        return false;

    // Distribute to output channels (no conversion needed, already float)
    if (canReadLeft && canReadRight && numCh >= 2)
    {
        // Stereo in, stereo out - direct mapping
        if (chans[0] != nullptr)
            FloatVectorOperations::copy (buffer->getWritePointer (0, startSampleInDestBuffer), chans[0], numSamples);
        if (chans[1] != nullptr)
            FloatVectorOperations::copy (buffer->getWritePointer (1, startSampleInDestBuffer), chans[1], numSamples);

        // Copy pattern to any additional output channels
        for (int ch = 2; ch < numCh; ++ch)
            buffer->copyFrom (ch, startSampleInDestBuffer, *buffer, ch % 2, startSampleInDestBuffer, numSamples);
    }
    else if (canReadLeft && canReadRight && numCh == 1)
    {
        // Stereo in, mono out - mix both channels
        auto* dest = buffer->getWritePointer (0, startSampleInDestBuffer);
        if (chans[0] != nullptr && chans[1] != nullptr)
        {
            FloatVectorOperations::copyWithMultiply (dest, chans[0], 0.5f, numSamples);
            FloatVectorOperations::addWithMultiply (dest, chans[1], 0.5f, numSamples);
        }
        else if (chans[0] != nullptr)
        {
            FloatVectorOperations::copy (dest, chans[0], numSamples);
        }
        else if (chans[1] != nullptr)
        {
            FloatVectorOperations::copy (dest, chans[1], numSamples);
        }
    }
    else
    {
        // Single channel to all outputs
        const float* sourceData = canReadLeft ? chans[0] : chans[1];
        if (sourceData != nullptr)
        {
            for (int ch = 0; ch < numCh; ++ch)
                FloatVectorOperations::copy (buffer->getWritePointer (ch, startSampleInDestBuffer), sourceData, numSamples);
        }
    }

    return true;
}

void AudioFormatReader::readMaxLevels (int64 startSample, int64 numSamples, Range<float>* results, int numChannelsToRead)
{
    numChannelsToRead = jmin (numChannelsToRead, (int) numChannels);

    HeapBlock<float> tempBuffer (numChannelsToRead * 4096, true);
    HeapBlock<float*> chans (numChannelsToRead, false);

    for (int i = 0; i < numChannelsToRead; ++i)
    {
        chans[i] = tempBuffer + i * 4096;
        results[i] = Range<float>();
    }

    while (numSamples > 0)
    {
        const auto numThisTime = jmin (numSamples, (int64) 4096);

        if (! readSamples (chans, numChannelsToRead, 0, startSample, (int) numThisTime))
            break;

        for (int i = 0; i < numChannelsToRead; ++i)
        {
            Range<float> r;
            r.setStart (chans[i][0]);
            r.setEnd (chans[i][0]);

            for (int j = 1; j < (int) numThisTime; ++j)
            {
                const auto sample = chans[i][j];
                r = r.getUnionWith (sample);
            }

            results[i] = results[i].getUnionWith (r);
        }

        startSample += numThisTime;
        numSamples -= numThisTime;
    }

    // Results are already in float format [-1.0, 1.0], no conversion needed
}

void AudioFormatReader::readMaxLevels (int64 startSample, int64 numSamples, float& lowestLeft, float& highestLeft, float& lowestRight, float& highestRight)
{
    Range<float> levels[2];
    readMaxLevels (startSample, numSamples, levels, 2);

    lowestLeft = levels[0].getStart();
    highestLeft = levels[0].getEnd();
    lowestRight = levels[1].getStart();
    highestRight = levels[1].getEnd();
}

int64 AudioFormatReader::searchForLevel (int64 startSample,
                                         int64 numSamplesToSearch,
                                         double magnitudeRangeMinimum,
                                         double magnitudeRangeMaximum,
                                         int minimumConsecutiveSamples)
{
    if (numSamplesToSearch <= 0)
        return -1;

    const auto magnitudeRangeMin = (float) magnitudeRangeMinimum;
    const auto magnitudeRangeMax = (float) magnitudeRangeMaximum;
    const auto bufferSize = 4096;
    HeapBlock<float> tempBuffer (bufferSize * 2, true); // Stereo buffer

    float* chans[2] = { tempBuffer.getData(), tempBuffer.getData() + bufferSize };
    int consecutiveSamples = 0;
    bool lastSampleWasInRange = false;

    while (numSamplesToSearch > 0)
    {
        const auto numThisTime = jmin (numSamplesToSearch, (int64) bufferSize);

        if (! readSamples (chans, 2, 0, startSample, (int) numThisTime))
            break;

        for (int i = 0; i < (int) numThisTime; ++i)
        {
            bool isInRange = false;

            for (int ch = 0; ch < (int) numChannels; ++ch)
            {
                const auto sample = std::abs (chans[ch][i]);
                if (sample >= magnitudeRangeMin && sample <= magnitudeRangeMax)
                {
                    isInRange = true;
                    break;
                }
            }

            if (isInRange)
            {
                if (lastSampleWasInRange)
                {
                    if (++consecutiveSamples >= minimumConsecutiveSamples)
                        return startSample + i - (consecutiveSamples - 1);
                }
                else
                {
                    consecutiveSamples = 1;
                    lastSampleWasInRange = true;
                }
            }
            else
            {
                consecutiveSamples = 0;
                lastSampleWasInRange = false;
            }
        }

        startSample += numThisTime;
        numSamplesToSearch -= numThisTime;
    }

    return -1;
}

AudioChannelSet AudioFormatReader::getChannelLayout()
{
    if (numChannels == 1)
        return AudioChannelSet::mono();
    if (numChannels == 2)
        return AudioChannelSet::stereo();

    return AudioChannelSet::discreteChannels ((int) numChannels);
}

//==============================================================================
void AudioFormatReader::ReadHelper::read (void* destData, const void* sourceData, int numSamples, int srcBytesPerSample, bool isFloatingPoint, bool isLittleEndian) noexcept
{
    if (isFloatingPoint)
    {
        if (srcBytesPerSample == 4)
        {
            ReadHelper::readFloat32 ((float*) destData, sourceData, numSamples, isLittleEndian);
        }
        else if (srcBytesPerSample == 8)
        {
            ReadHelper::readFloat64 ((float*) destData, sourceData, numSamples, isLittleEndian);
        }
        else
        {
            jassertfalse; // Unsupported floating-point size
        }
    }
    else
    {
        if (srcBytesPerSample == 1)
        {
            ReadHelper::readInt8 ((int*) destData, sourceData, numSamples);
        }
        else if (srcBytesPerSample == 2)
        {
            ReadHelper::readInt16 ((int*) destData, sourceData, numSamples, isLittleEndian);
        }
        else if (srcBytesPerSample == 3)
        {
            ReadHelper::readInt24 ((int*) destData, sourceData, numSamples, isLittleEndian);
        }
        else if (srcBytesPerSample == 4)
        {
            ReadHelper::readInt32 ((int*) destData, sourceData, numSamples, isLittleEndian);
        }
        else
        {
            jassertfalse; // Unsupported bit depth
        }
    }
}

void AudioFormatReader::ReadHelper::readInt8 (int* dest, const void* src, int numSamples) noexcept
{
    const auto* source = static_cast<const char*> (src);

    for (int i = 0; i < numSamples; ++i)
        dest[i] = ((int) source[i]) << 24;
}

void AudioFormatReader::ReadHelper::readInt16 (int* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const uint16*> (src);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
            dest[i] = (int) ByteOrder::swapIfBigEndian (source[i]) << 16;
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            dest[i] = (int) ByteOrder::swapIfLittleEndian (source[i]) << 16;
    }
}

void AudioFormatReader::ReadHelper::readInt24 (int* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const uint8*> (src);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = (((int) source[i * 3 + 2]) << 24)
                    | (((int) source[i * 3 + 1]) << 16)
                    | (((int) source[i * 3]) << 8);
        }
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = (((int) source[i * 3]) << 24)
                    | (((int) source[i * 3 + 1]) << 16)
                    | (((int) source[i * 3 + 2]) << 8);
        }
    }
}

void AudioFormatReader::ReadHelper::readInt32 (int* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const uint32*> (src);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
            dest[i] = (int) ByteOrder::swapIfBigEndian (source[i]);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            dest[i] = (int) ByteOrder::swapIfLittleEndian (source[i]);
    }
}

void AudioFormatReader::ReadHelper::readFloat32 (float* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const float*> (src);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
            dest[i] = ByteOrder::swapIfBigEndian (source[i]);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            dest[i] = ByteOrder::swapIfLittleEndian (source[i]);
    }
}

void AudioFormatReader::ReadHelper::readFloat64 (float* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const double*> (src);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
            dest[i] = (float) ByteOrder::swapIfBigEndian (source[i]);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            dest[i] = (float) ByteOrder::swapIfLittleEndian (source[i]);
    }
}

} // namespace yup
