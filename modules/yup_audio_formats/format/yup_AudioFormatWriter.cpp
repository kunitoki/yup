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

AudioFormatWriter::AudioFormatWriter (OutputStream* destStream,
                                      const String& formatName_,
                                      double rate,
                                      int numberOfChannels_,
                                      int bits)
    : output (destStream)
    , formatName (formatName_)
    , sampleRate (rate)
    , numChannels (numberOfChannels_)
    , bitsPerSample (bits)
    , isFloatingPointFormat (bits == 32)
{
}

AudioFormatWriter::~AudioFormatWriter()
{
}

bool AudioFormatWriter::flush()
{
    return true;
}

bool AudioFormatWriter::writeFromAudioReader (AudioFormatReader& reader,
                                              int64 startSample,
                                              int64 numSamplesToRead)
{
    const auto bufferSize = 16384;
    const auto maxChans = jmin ((int) numChannels, (int) reader.numChannels);

    HeapBlock<float> tempBuffer (bufferSize * maxChans, true);
    HeapBlock<float*> channels (maxChans, false);

    for (int i = 0; i < maxChans; ++i)
        channels[i] = tempBuffer.getData() + i * bufferSize;

    while (numSamplesToRead > 0)
    {
        const auto numThisTime = jmin (numSamplesToRead, (int64) bufferSize);

        if (! reader.read (channels.getData(), maxChans, startSample, (int) numThisTime))
            return false;

        if (! write (channels.getData(), (int) numThisTime))
            return false;

        startSample += numThisTime;
        numSamplesToRead -= numThisTime;
    }

    return true;
}

bool AudioFormatWriter::writeFromAudioSource (AudioSource& source,
                                              int numSamplesToRead,
                                              int samplesPerBlock)
{
    AudioBuffer<float> tempBuffer (numChannels, samplesPerBlock);

    while (numSamplesToRead > 0)
    {
        const auto numThisTime = jmin (numSamplesToRead, samplesPerBlock);

        AudioSourceChannelInfo info;
        info.buffer = &tempBuffer;
        info.startSample = 0;
        info.numSamples = numThisTime;

        source.getNextAudioBlock (info);

        if (! writeFromFloatArrays (tempBuffer.getArrayOfReadPointers(), numChannels, numThisTime))
            return false;

        numSamplesToRead -= numThisTime;
    }

    return true;
}

bool AudioFormatWriter::writeFromAudioSampleBuffer (const AudioBuffer<float>& source,
                                                    int startSample,
                                                    int numSamples)
{
    const auto numSourceChannels = source.getNumChannels();
    const auto numSamplesClamped = jmin (numSamples, source.getNumSamples() - startSample);

    if (numSamplesClamped <= 0)
        return true;

    HeapBlock<const float*> channels (numChannels);

    // Map source channels to writer channels
    for (int i = 0; i < (int) numChannels; ++i)
    {
        if (i < numSourceChannels)
            channels[i] = source.getReadPointer (i, startSample);
        else
            channels[i] = nullptr; // Will be filled with zeros
    }

    return writeFromFloatArrays (channels, numChannels, numSamplesClamped);
}

bool AudioFormatWriter::writeFromFloatArrays (const float* const* channels,
                                              int numChannelsToWrite,
                                              int numSamples)
{
    if (numSamples <= 0)
        return true;

    numChannelsToWrite = jmin (numChannelsToWrite, (int) numChannels);

    // Create temp buffer with proper channel layout for write method
    HeapBlock<float> tempBuffer (numSamples * numChannels, true);
    HeapBlock<float*> floatChannels (numChannels, false);

    for (int i = 0; i < (int) numChannels; ++i)
        floatChannels[i] = tempBuffer.getData() + i * numSamples;

    // Copy float data to temporary channels
    for (int i = 0; i < numChannelsToWrite; ++i)
    {
        if (channels[i] != nullptr)
        {
            FloatVectorOperations::copy (floatChannels[i], channels[i], numSamples);
        }
        else
        {
            FloatVectorOperations::clear (floatChannels[i], numSamples);
        }
    }

    // Clear any remaining channels
    for (int i = numChannelsToWrite; i < (int) numChannels; ++i)
        FloatVectorOperations::clear (floatChannels[i], numSamples);

    return write (floatChannels.getData(), numSamples);
}

//==============================================================================
// ThreadedWriter implementation
class AudioFormatWriter::ThreadedWriter::ThreadedWriterHelper : public TimeSliceClient
{
public:
    ThreadedWriterHelper (std::unique_ptr<AudioFormatWriter> writer_, int numSamplesToBuffer)
        : writer (std::move (writer_))
        , fifo (numSamplesToBuffer)
        , tempBuffer (writer->getNumChannels(), numSamplesToBuffer)
        , fifoBuffer (numSamplesToBuffer * writer->getNumChannels())
    {
    }

    ~ThreadedWriterHelper() override
    {
        flushAllData();
    }

    bool write (const float* const* data, int numSamples)
    {
        const ScopedLock sl (lock);

        if (hasFinished || ! writer)
            return false;

        const auto scope = fifo.write (numSamples);

        if (scope.blockSize1 + scope.blockSize2 < numSamples)
            return false;

        const int numChannels = tempBuffer.getNumChannels();
        const int bufferSize = (int) fifoBuffer.size() / numChannels;

        int offset = 0;

        if (scope.blockSize1 > 0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                FloatVectorOperations::copy (fifoBuffer.data() + (scope.startIndex1 + ch * bufferSize),
                                             data[ch],
                                             scope.blockSize1);
            }
            offset = scope.blockSize1;
        }

        if (scope.blockSize2 > 0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                FloatVectorOperations::copy (fifoBuffer.data() + (scope.startIndex2 + ch * bufferSize),
                                             data[ch] + offset,
                                             scope.blockSize2);
            }
        }

        return true;
    }

    void finish()
    {
        const ScopedLock sl (lock);
        hasFinished = true;
    }

    bool isRunning() const
    {
        const ScopedLock sl (lock);
        return ! hasFinished || fifo.getNumReady() > 0;
    }

    int useTimeSlice() override
    {
        const int numReady = fifo.getNumReady();

        if (numReady == 0)
            return hasFinished ? -1 : 10;

        const auto numToWrite = jmin (numReady, tempBuffer.getNumSamples());
        const auto scope = fifo.read (numToWrite);

        const int numChannels = tempBuffer.getNumChannels();
        const int bufferSize = (int) fifoBuffer.size() / numChannels;

        int offset = 0;

        if (scope.blockSize1 > 0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                FloatVectorOperations::copy (tempBuffer.getWritePointer (ch),
                                             fifoBuffer.data() + (scope.startIndex1 + ch * bufferSize),
                                             scope.blockSize1);
            }
            offset = scope.blockSize1;
        }

        if (scope.blockSize2 > 0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                FloatVectorOperations::copy (tempBuffer.getWritePointer (ch) + offset,
                                             fifoBuffer.data() + (scope.startIndex2 + ch * bufferSize),
                                             scope.blockSize2);
            }
        }

        if (! writer->writeFromFloatArrays (tempBuffer.getArrayOfReadPointers(),
                                            tempBuffer.getNumChannels(),
                                            numToWrite))
        {
            hasFinished = true;
            return -1;
        }

        return 0;
    }

private:
    void flushAllData()
    {
        while (fifo.getNumReady() > 0 && writer != nullptr)
        {
            const auto numToWrite = jmin (fifo.getNumReady(), tempBuffer.getNumSamples());
            const auto scope = fifo.read (numToWrite);

            const int numChannels = tempBuffer.getNumChannels();
            const int bufferSize = (int) fifoBuffer.size() / numChannels;

            int offset = 0;

            if (scope.blockSize1 > 0)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    FloatVectorOperations::copy (tempBuffer.getWritePointer (ch),
                                                 fifoBuffer.data() + (scope.startIndex1 + ch * bufferSize),
                                                 scope.blockSize1);
                }
                offset = scope.blockSize1;
            }

            if (scope.blockSize2 > 0)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    FloatVectorOperations::copy (tempBuffer.getWritePointer (ch) + offset,
                                                 fifoBuffer.data() + (scope.startIndex2 + ch * bufferSize),
                                                 scope.blockSize2);
                }
            }

            writer->writeFromFloatArrays (tempBuffer.getArrayOfReadPointers(),
                                          tempBuffer.getNumChannels(),
                                          numToWrite);
        }
    }

    std::unique_ptr<AudioFormatWriter> writer;
    AbstractFifo fifo;
    AudioBuffer<float> tempBuffer;
    std::vector<float> fifoBuffer;
    mutable CriticalSection lock;
    bool hasFinished = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThreadedWriterHelper)
};

AudioFormatWriter::ThreadedWriter::ThreadedWriter (std::unique_ptr<AudioFormatWriter> writer,
                                                   TimeSliceThread& backgroundThread,
                                                   int numSamplesToBuffer)
    : helper (std::make_unique<ThreadedWriterHelper> (std::move (writer), numSamplesToBuffer))
{
    backgroundThread.addTimeSliceClient (helper.get());
}

AudioFormatWriter::ThreadedWriter::~ThreadedWriter()
{
    helper->finish();
    waitForThreadToFinish();
}

bool AudioFormatWriter::ThreadedWriter::isThreadRunning() const
{
    return helper->isRunning();
}

bool AudioFormatWriter::ThreadedWriter::write (const float* const* data, int numSamples)
{
    return helper->write (data, numSamples);
}

void AudioFormatWriter::ThreadedWriter::waitForThreadToFinish()
{
    while (helper->isRunning())
        Thread::sleep (1);
}

//==============================================================================
void AudioFormatWriter::WriteHelper::write (void* destData, const void* sourceData, int numSamples, int destBytesPerSample, bool isFloatingPoint, bool isLittleEndian) noexcept
{
    if (isFloatingPoint)
    {
        if (destBytesPerSample == 4)
        {
            WriteHelper::writeFloat32 (destData, sourceData, numSamples, isLittleEndian);
        }
        else if (destBytesPerSample == 8)
        {
            WriteHelper::writeFloat64 (destData, sourceData, numSamples, isLittleEndian);
        }
        else
        {
            jassertfalse; // Unsupported floating-point size
        }
    }
    else
    {
        if (destBytesPerSample == 1)
        {
            WriteHelper::writeInt8 (destData, sourceData, numSamples);
        }
        else if (destBytesPerSample == 2)
        {
            WriteHelper::writeInt16 (destData, sourceData, numSamples, isLittleEndian);
        }
        else if (destBytesPerSample == 3)
        {
            WriteHelper::writeInt24 (destData, sourceData, numSamples, isLittleEndian);
        }
        else if (destBytesPerSample == 4)
        {
            WriteHelper::writeInt32 (destData, sourceData, numSamples, isLittleEndian);
        }
        else
        {
            jassertfalse; // Unsupported bit depth
        }
    }
}

void AudioFormatWriter::WriteHelper::writeInt8 (void* dest, const void* src, int numSamples) noexcept
{
    const auto* source = static_cast<const float*> (src);
    auto* destination = static_cast<char*> (dest);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto clampedValue = jlimit (-1.0f, 1.0f, source[i]);
        const auto value = static_cast<int> (clampedValue * 127.0f) + 128;
        destination[i] = (char) jlimit (0, 255, value);
    }
}

void AudioFormatWriter::WriteHelper::writeInt16 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const float*> (src);
    auto* destination = static_cast<uint16*> (dest);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto clampedValue = jlimit (-1.0f, 1.0f, source[i]);
            const auto intValue = static_cast<int> (clampedValue * 32767.0f);
            destination[i] = ByteOrder::swapIfBigEndian ((uint16) intValue);
        }
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto clampedValue = jlimit (-1.0f, 1.0f, source[i]);
            const auto intValue = static_cast<int> (clampedValue * 32767.0f);
            destination[i] = ByteOrder::swapIfLittleEndian ((uint16) intValue);
        }
    }
}

void AudioFormatWriter::WriteHelper::writeInt24 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const float*> (src);
    auto* destination = static_cast<uint8*> (dest);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto clampedValue = jlimit (-1.0f, 1.0f, source[i]);
            const auto sample = static_cast<int> (clampedValue * 8388607.0f);
            destination[i * 3] = (uint8) sample;
            destination[i * 3 + 1] = (uint8) (sample >> 8);
            destination[i * 3 + 2] = (uint8) (sample >> 16);
        }
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto clampedValue = jlimit (-1.0f, 1.0f, source[i]);
            const auto sample = static_cast<int> (clampedValue * 8388607.0f);
            destination[i * 3] = (uint8) (sample >> 16);
            destination[i * 3 + 1] = (uint8) (sample >> 8);
            destination[i * 3 + 2] = (uint8) sample;
        }
    }
}

void AudioFormatWriter::WriteHelper::writeInt32 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const float*> (src);
    auto* destination = static_cast<uint32*> (dest);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto clampedValue = jlimit (-1.0f, 1.0f, source[i]);
            const auto intValue = static_cast<uint32> (clampedValue * 2147483647.0f);
            destination[i] = ByteOrder::swapIfBigEndian (intValue);
        }
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto clampedValue = jlimit (-1.0f, 1.0f, source[i]);
            const auto intValue = static_cast<uint32> (clampedValue * 2147483647.0f);
            destination[i] = ByteOrder::swapIfLittleEndian (intValue);
        }
    }
}

void AudioFormatWriter::WriteHelper::writeFloat32 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const float*> (src);
    auto* destination = static_cast<float*> (dest);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
            destination[i] = ByteOrder::swapIfBigEndian (source[i]);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            destination[i] = ByteOrder::swapIfLittleEndian (source[i]);
    }
}

void AudioFormatWriter::WriteHelper::writeFloat64 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept
{
    const auto* source = static_cast<const double*> (src);
    auto* destination = static_cast<double*> (dest);

    if (littleEndian)
    {
        for (int i = 0; i < numSamples; ++i)
            destination[i] = ByteOrder::swapIfBigEndian (source[i]);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
            destination[i] = ByteOrder::swapIfLittleEndian (source[i]);
    }
}

} // namespace yup
