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

namespace
{

//==============================================================================

struct CoreAudioOutputStreamState
{
    OutputStream* stream = nullptr;
    int64 size = 0;
};

struct CoreAudioInputStreamState
{
    InputStream* stream = nullptr;
    int64 size = -1;
};

static OSStatus coreAudioInputReadProc (void* inClientData,
                                        SInt64 inPosition,
                                        UInt32 requestCount,
                                        void* buffer,
                                        UInt32* actualCount)
{
    auto* state = static_cast<CoreAudioInputStreamState*> (inClientData);
    if (state == nullptr || state->stream == nullptr || inPosition < 0)
    {
        if (actualCount != nullptr)
            *actualCount = 0;

        return kAudioFileUnspecifiedError;
    }

    auto* stream = state->stream;
    if (! stream->setPosition (inPosition))
        return kAudioFileOperationNotSupportedError;

    const auto bytesRead = stream->read (buffer, (int) requestCount);
    const auto clampedRead = (bytesRead > 0 ? (UInt32) bytesRead : 0u);

    if (actualCount != nullptr)
        *actualCount = clampedRead;

    if (state->size < 0 && clampedRead > 0)
    {
        const auto endPosition = (int64) inPosition + (int64) clampedRead;
        if (endPosition > state->size)
            state->size = endPosition;
    }

    return noErr;
}

static SInt64 coreAudioInputGetSizeProc (void* inClientData)
{
    auto* state = static_cast<CoreAudioInputStreamState*> (inClientData);
    if (state == nullptr || state->stream == nullptr)
        return 0;

    if (state->size < 0)
        state->size = state->stream->getTotalLength();

    return state->size > 0 ? state->size : 0;
}

static OSStatus coreAudioReadProc (void* inClientData,
                                   SInt64 inPosition,
                                   UInt32 requestCount,
                                   void* buffer,
                                   UInt32* actualCount)
{
    ignoreUnused (inClientData, inPosition, requestCount, buffer);

    if (actualCount != nullptr)
        *actualCount = 0;

    return kAudioFileOperationNotSupportedError;
}

static OSStatus coreAudioWriteProc (void* inClientData,
                                    SInt64 inPosition,
                                    UInt32 requestCount,
                                    const void* buffer,
                                    UInt32* actualCount)
{
    auto* state = static_cast<CoreAudioOutputStreamState*> (inClientData);
    if (state == nullptr || state->stream == nullptr || inPosition < 0)
    {
        if (actualCount != nullptr)
            *actualCount = 0;

        return kAudioFileUnspecifiedError;
    }

    auto* stream = state->stream;
    const auto targetPosition = (int64) inPosition;

    if (targetPosition > state->size)
    {
        if (! stream->setPosition (state->size))
            return kAudioFileOperationNotSupportedError;

        const auto gap = (size_t) (targetPosition - state->size);
        if (gap > 0 && ! stream->writeRepeatedByte (0, gap))
            return kAudioFileUnspecifiedError;

        state->size = targetPosition;
    }

    if (! stream->setPosition (targetPosition))
        return kAudioFileOperationNotSupportedError;

    if (requestCount > 0 && ! stream->write (buffer, requestCount))
        return kAudioFileUnspecifiedError;

    const auto endPosition = targetPosition + (int64) requestCount;
    if (endPosition > state->size)
        state->size = endPosition;

    if (actualCount != nullptr)
        *actualCount = requestCount;

    return noErr;
}

static SInt64 coreAudioGetSizeProc (void* inClientData)
{
    auto* state = static_cast<CoreAudioOutputStreamState*> (inClientData);
    return state != nullptr ? state->size : 0;
}

static OSStatus coreAudioSetSizeProc (void* inClientData, SInt64 inSize)
{
    auto* state = static_cast<CoreAudioOutputStreamState*> (inClientData);
    if (state == nullptr || state->stream == nullptr || inSize < 0)
        return kAudioFileUnspecifiedError;

    if (inSize == state->size)
        return noErr;

    if (inSize < state->size)
    {
        if (! state->stream->setPosition (inSize))
            return kAudioFileOperationNotSupportedError;

        state->size = inSize;
        return noErr;
    }

    if (! state->stream->setPosition (state->size))
        return kAudioFileOperationNotSupportedError;

    const auto gap = (size_t) (inSize - state->size);
    if (gap > 0 && ! state->stream->writeRepeatedByte (0, gap))
        return kAudioFileUnspecifiedError;

    state->size = inSize;
    return noErr;
}

class AppleCoreAudioFormatReader : public AudioFormatReader
{
public:
    explicit AppleCoreAudioFormatReader (InputStream* sourceStream);
    ~AppleCoreAudioFormatReader() override;

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override;

private:
    bool openFromStream (InputStream* sourceStream);
    void close();

    ExtAudioFileRef audioFile = nullptr;
    AudioStreamBasicDescription inputFormat = {};
    AudioStreamBasicDescription clientFormat = {};
    SInt64 headerFrames = 0;
    bool isOpen = false;
    CoreAudioInputStreamState streamState;

    HeapBlock<float> tempBuffer;
    size_t tempBufferFrames = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppleCoreAudioFormatReader)
};

class AppleCoreAudioFormatWriter : public AudioFormatWriter
{
public:
    AppleCoreAudioFormatWriter (OutputStream* destStream,
                                double sampleRate,
                                int numberOfChannels,
                                int bitsPerSample,
                                const StringPairArray& metadataValues,
                                int qualityOptionIndex);

    ~AppleCoreAudioFormatWriter() override;

    bool write (const float* const* samplesToWrite, int numSamples) override;
    bool flush() override;

private:
    void close();

    ExtAudioFileRef audioFile = nullptr;
    AudioStreamBasicDescription clientFormat = {};
    AudioStreamBasicDescription fileFormat = {};
    bool isOpen = false;
    CoreAudioOutputStreamState streamState;

    HeapBlock<float> tempBuffer;
    size_t tempBufferFrames = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppleCoreAudioFormatWriter)
};

AppleCoreAudioFormatReader::AppleCoreAudioFormatReader (InputStream* sourceStream)
    : AudioFormatReader (sourceStream, "CoreAudio file")
{
    isOpen = openFromStream (sourceStream);
}

AppleCoreAudioFormatReader::~AppleCoreAudioFormatReader()
{
    close();
}

void AppleCoreAudioFormatReader::close()
{
    if (audioFile != nullptr)
    {
        ExtAudioFileDispose (audioFile);
        audioFile = nullptr;
    }

    isOpen = false;
}

bool AppleCoreAudioFormatReader::openFromStream (InputStream* sourceStream)
{
    if (sourceStream == nullptr)
        return false;

    streamState.stream = sourceStream;
    streamState.size = sourceStream->getTotalLength();

    AudioFileID audioFileId = nullptr;
    auto err = AudioFileOpenWithCallbacks (&streamState,
                                           coreAudioInputReadProc,
                                           nullptr,
                                           coreAudioInputGetSizeProc,
                                           nullptr,
                                           0,
                                           &audioFileId);
    if (err == noErr)
        err = ExtAudioFileWrapAudioFileID (audioFileId, false, &audioFile);

    if (err != noErr && audioFileId != nullptr)
        AudioFileClose (audioFileId);

    if (err != noErr || audioFile == nullptr)
    {
        close();
        return false;
    }

    UInt32 size = sizeof (inputFormat);
    if (ExtAudioFileGetProperty (audioFile,
                                 kExtAudioFileProperty_FileDataFormat,
                                 &size,
                                 &inputFormat)
        != noErr)
    {
        close();
        return false;
    }

    clientFormat = {};
    clientFormat.mFormatID = kAudioFormatLinearPCM;
    clientFormat.mSampleRate = inputFormat.mSampleRate;
    clientFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
    clientFormat.mFormatFlags = kAudioFormatFlagIsFloat
                              | kAudioFormatFlagIsPacked
                              | kAudioFormatFlagsNativeEndian;
    clientFormat.mBitsPerChannel = sizeof (float) * 8;
    clientFormat.mFramesPerPacket = 1;
    clientFormat.mBytesPerFrame = clientFormat.mChannelsPerFrame * sizeof (float);
    clientFormat.mBytesPerPacket = clientFormat.mBytesPerFrame * clientFormat.mFramesPerPacket;

    size = sizeof (clientFormat);
    if (ExtAudioFileSetProperty (audioFile,
                                 kExtAudioFileProperty_ClientDataFormat,
                                 size,
                                 &clientFormat)
        != noErr)
    {
        close();
        return false;
    }

    SInt64 totalFrameCount = 0;
    size = sizeof (totalFrameCount);
    if (ExtAudioFileGetProperty (audioFile,
                                 kExtAudioFileProperty_FileLengthFrames,
                                 &size,
                                 &totalFrameCount)
        != noErr)
    {
        close();
        return false;
    }

    AudioConverterRef converter = nullptr;
    UInt32 converterSize = sizeof (converter);
    if (ExtAudioFileGetProperty (audioFile,
                                 kExtAudioFileProperty_AudioConverter,
                                 &converterSize,
                                 &converter)
            == noErr
        && converter != nullptr)
    {
        AudioConverterPrimeInfo primeInfo = {};
        UInt32 primeSize = sizeof (primeInfo);

        if (AudioConverterGetProperty (converter,
                                       kAudioConverterPrimeInfo,
                                       &primeSize,
                                       &primeInfo)
            == noErr)
        {
            headerFrames = primeInfo.leadingFrames;
        }
    }

    sampleRate = clientFormat.mSampleRate;
    bitsPerSample = (int) clientFormat.mBitsPerChannel;
    lengthInSamples = totalFrameCount;
    numChannels = (int) clientFormat.mChannelsPerFrame;
    usesFloatingPointData = true;

    return true;
}

bool AppleCoreAudioFormatReader::readSamples (float* const* destChannels,
                                              int numDestChannels,
                                              int startOffsetInDestBuffer,
                                              int64 startSampleInFile,
                                              int numSamples)
{
    if (! isOpen || audioFile == nullptr)
        return false;

    if (numSamples <= 0)
        return true;

    if (ExtAudioFileSeek (audioFile, startSampleInFile + headerFrames) != noErr)
        return false;

    const auto numChannelsToRead = jmin (numDestChannels, numChannels);
    if (numChannelsToRead <= 0)
        return false;

    HeapBlock<float*> offsetDestChannels;
    offsetDestChannels.malloc (numDestChannels);

    for (int ch = 0; ch < numDestChannels; ++ch)
        offsetDestChannels[ch] = destChannels[ch] + startOffsetInDestBuffer;

    const int maxFramesPerRead = 4096;
    int remainingFrames = numSamples;
    int totalFramesRead = 0;

    while (remainingFrames > 0)
    {
        const int framesToRead = jmin (remainingFrames, maxFramesPerRead);
        const size_t neededFrames = (size_t) framesToRead;

        if (neededFrames > tempBufferFrames)
        {
            tempBufferFrames = neededFrames;
            tempBuffer.allocate ((size_t) numChannels * tempBufferFrames, false);
        }

        AudioBufferList bufferList {};
        bufferList.mNumberBuffers = 1;
        bufferList.mBuffers[0].mNumberChannels = (UInt32) numChannels;
        bufferList.mBuffers[0].mDataByteSize = (UInt32) (framesToRead * numChannels * (int) sizeof (float));
        bufferList.mBuffers[0].mData = tempBuffer.getData();

        UInt32 framesRead = (UInt32) framesToRead;
        const auto err = ExtAudioFileRead (audioFile, &framesRead, &bufferList);

        if (err != noErr)
            return false;

        if (framesRead == 0)
            break;

        using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { tempBuffer.getData(), numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numChannelsToRead },
                                        (int) framesRead);

        for (int ch = numChannelsToRead; ch < numDestChannels; ++ch)
        {
            if (offsetDestChannels[ch] != nullptr)
                zeromem (offsetDestChannels[ch], sizeof (float) * framesRead);
        }

        for (int ch = 0; ch < numDestChannels; ++ch)
        {
            if (offsetDestChannels[ch] != nullptr)
                offsetDestChannels[ch] += framesRead;
        }

        totalFramesRead += (int) framesRead;
        remainingFrames -= (int) framesRead;
    }

    return totalFramesRead > 0;
}

AppleCoreAudioFormatWriter::AppleCoreAudioFormatWriter (OutputStream* destStream,
                                                        double sampleRate,
                                                        int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex)
    : AudioFormatWriter (destStream, "CoreAudio file", sampleRate, numberOfChannels, bitsPerSample)
{
    ignoreUnused (metadataValues);

    if (destStream == nullptr || numberOfChannels < 1 || numberOfChannels > 2 || sampleRate <= 0.0)
        return;

    if (bitsPerSample != 32)
        return;

    AudioFileTypeID fileType = kAudioFileAAC_ADTSType;

    fileFormat = {};
    fileFormat.mSampleRate = sampleRate;
    fileFormat.mFormatID = kAudioFormatMPEG4AAC;
    fileFormat.mChannelsPerFrame = (UInt32) numberOfChannels;

    UInt32 size = sizeof (fileFormat);
    if (AudioFormatGetProperty (kAudioFormatProperty_FormatInfo, 0, nullptr, &size, &fileFormat) != noErr)
        return;

    streamState.stream = destStream;
    streamState.size = destStream->getPosition();

    AudioFileID audioFileId = nullptr;
    auto err = AudioFileInitializeWithCallbacks (&streamState,
                                                 coreAudioReadProc,
                                                 coreAudioWriteProc,
                                                 coreAudioGetSizeProc,
                                                 coreAudioSetSizeProc,
                                                 fileType,
                                                 &fileFormat,
                                                 kAudioFileFlags_EraseFile,
                                                 &audioFileId);
    if (err == noErr)
        err = ExtAudioFileWrapAudioFileID (audioFileId, true, &audioFile);

    if (err != noErr && audioFileId != nullptr)
        AudioFileClose (audioFileId);

    if (err != noErr || audioFile == nullptr)
    {
        close();
        return;
    }

    clientFormat = {};
    clientFormat.mFormatID = kAudioFormatLinearPCM;
    clientFormat.mSampleRate = sampleRate;
    clientFormat.mChannelsPerFrame = (UInt32) numberOfChannels;
    clientFormat.mFormatFlags = kAudioFormatFlagIsFloat
                              | kAudioFormatFlagIsPacked
                              | kAudioFormatFlagsNativeEndian;
    clientFormat.mBitsPerChannel = sizeof (float) * 8;
    clientFormat.mFramesPerPacket = 1;
    clientFormat.mBytesPerFrame = clientFormat.mChannelsPerFrame * sizeof (float);
    clientFormat.mBytesPerPacket = clientFormat.mBytesPerFrame * clientFormat.mFramesPerPacket;

    size = sizeof (clientFormat);
    err = ExtAudioFileSetProperty (audioFile,
                                   kExtAudioFileProperty_ClientDataFormat,
                                   size,
                                   &clientFormat);
    if (err != noErr)
    {
        close();
        return;
    }

    AudioConverterRef converter = nullptr;
    UInt32 converterSize = sizeof (converter);
    if (ExtAudioFileGetProperty (audioFile,
                                 kExtAudioFileProperty_AudioConverter,
                                 &converterSize,
                                 &converter)
            == noErr
        && converter != nullptr)
    {
        int targetBitrate = 128000 * numberOfChannels;

        if (qualityOptionIndex > 0)
        {
            const int clampedQuality = jlimit (0, 100, qualityOptionIndex);
            const int minBitrate = 64000 * numberOfChannels;
            const int maxBitrate = 256000 * numberOfChannels;
            targetBitrate = minBitrate + ((maxBitrate - minBitrate) * clampedQuality / 100);
        }

        UInt32 bitrateProperty = (UInt32) targetBitrate;
        AudioConverterSetProperty (converter,
                                   kAudioConverterEncodeBitRate,
                                   sizeof (bitrateProperty),
                                   &bitrateProperty);
    }

    isOpen = true;
}

AppleCoreAudioFormatWriter::~AppleCoreAudioFormatWriter()
{
    close();
}

void AppleCoreAudioFormatWriter::close()
{
    if (audioFile != nullptr)
    {
        ExtAudioFileDispose (audioFile);
        audioFile = nullptr;
    }

    isOpen = false;
}

bool AppleCoreAudioFormatWriter::write (const float* const* samplesToWrite, int numSamples)
{
    if (! isOpen || audioFile == nullptr || numSamples <= 0)
        return false;

    const auto numChannels = getNumChannels();
    const size_t framesNeeded = (size_t) numSamples;

    if (framesNeeded > tempBufferFrames)
    {
        tempBufferFrames = framesNeeded;
        tempBuffer.allocate (tempBufferFrames * (size_t) numChannels, false);
    }

    using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
    using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, numChannels },
                                  AudioData::InterleavedDest<DestFormat> { tempBuffer.getData(), numChannels },
                                  numSamples);

    AudioBufferList bufferList {};
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mNumberChannels = (UInt32) numChannels;
    bufferList.mBuffers[0].mDataByteSize = (UInt32) (numSamples * numChannels * (int) sizeof (float));
    bufferList.mBuffers[0].mData = tempBuffer.getData();

    UInt32 framesToWrite = (UInt32) numSamples;
    const auto err = ExtAudioFileWrite (audioFile, framesToWrite, &bufferList);
    return err == noErr;
}

bool AppleCoreAudioFormatWriter::flush()
{
    if (output != nullptr)
    {
        output->flush();
        return true;
    }

    return false;
}

} // namespace

//==============================================================================
// AppleCoreAudioFormat implementation
AppleCoreAudioFormat::AppleCoreAudioFormat()
    : formatName ("CoreAudio file")
{
}

AppleCoreAudioFormat::~AppleCoreAudioFormat() = default;

const String& AppleCoreAudioFormat::getFormatName() const
{
    return formatName;
}

Array<String> AppleCoreAudioFormat::getFileExtensions() const
{
    return { ".m4a", ".aac", ".mp3", ".mp2" };
}

std::unique_ptr<AudioFormatReader> AppleCoreAudioFormat::createReaderFor (InputStream* sourceStream)
{
    auto reader = std::make_unique<AppleCoreAudioFormatReader> (sourceStream);

    if (reader->sampleRate > 0 && reader->numChannels > 0)
        return reader;

    return nullptr;
}

std::unique_ptr<AudioFormatWriter> AppleCoreAudioFormat::createWriterFor (OutputStream* streamToWriteTo,
                                                                          double sampleRate,
                                                                          int numberOfChannels,
                                                                          int bitsPerSample,
                                                                          const StringPairArray& metadataValues,
                                                                          int qualityOptionIndex)
{
    if (streamToWriteTo == nullptr)
        return nullptr;

    if (numberOfChannels < 1 || numberOfChannels > 2)
        return nullptr;

    if (sampleRate <= 0.0)
        return nullptr;

    if (bitsPerSample != 32)
        return nullptr;

    return std::make_unique<AppleCoreAudioFormatWriter> (streamToWriteTo,
                                                         sampleRate,
                                                         numberOfChannels,
                                                         bitsPerSample,
                                                         metadataValues,
                                                         qualityOptionIndex);
}

Array<int> AppleCoreAudioFormat::getPossibleBitDepths() const
{
    return { 32 };
}

Array<int> AppleCoreAudioFormat::getPossibleSampleRates() const
{
    return { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 192000 };
}

} // namespace yup
