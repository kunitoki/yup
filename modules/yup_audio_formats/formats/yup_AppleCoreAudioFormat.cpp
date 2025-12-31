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
static AudioFileTypeID toAudioFileTypeID (AppleCoreAudioFormat::StreamKind kind)
{
    using StreamKind = AppleCoreAudioFormat::StreamKind;

    switch (kind)
    {
        case StreamKind::kAiff:
            return kAudioFileAIFFType;
        case StreamKind::kAifc:
            return kAudioFileAIFCType;
        case StreamKind::kWave:
            return kAudioFileWAVEType;
        case StreamKind::kSoundDesigner2:
            return kAudioFileSoundDesigner2Type;
        case StreamKind::kNext:
            return kAudioFileNextType;
        case StreamKind::kMp4:
            return kAudioFileMPEG4Type;
        case StreamKind::kMp3:
            return kAudioFileMP3Type;
        case StreamKind::kMp2:
            return kAudioFileMP2Type;
        case StreamKind::kMp1:
            return kAudioFileMP1Type;
        case StreamKind::kAc3:
            return kAudioFileAC3Type;
        case StreamKind::kAacAdts:
            return kAudioFileAAC_ADTSType;
        case StreamKind::kM4a:
            return kAudioFileM4AType;
        case StreamKind::kM4b:
            return kAudioFileM4BType;
        case StreamKind::kCaf:
            return kAudioFileCAFType;
        case StreamKind::k3gp:
            return kAudioFile3GPType;
        case StreamKind::k3gp2:
            return kAudioFile3GP2Type;
        case StreamKind::kAmr:
            return kAudioFileAMRType;
        case StreamKind::kNone:
            break;
    }

    return {};
}

static Array<String> getStringInfo (AudioFilePropertyID property, UInt32 size, void* data)
{
    Array<String> extensionsArray;

    CFArrayRef extensions = nullptr;
    UInt32 sizeOfArray = sizeof (extensions);

    if (AudioFileGetGlobalInfo (property, size, data, &sizeOfArray, &extensions) != noErr || extensions == nullptr)
        return extensionsArray;

    const auto numValues = CFArrayGetCount (extensions);

    for (CFIndex i = 0; i < numValues; ++i)
        extensionsArray.add ("." + String::fromCFString ((CFStringRef) CFArrayGetValueAtIndex (extensions, i)));

    CFRelease (extensions);
    return extensionsArray;
}

static Array<String> findFileExtensionsForCoreAudioCodec (AudioFileTypeID type)
{
    return getStringInfo (kAudioFileGlobalInfo_ExtensionsForType, sizeof (AudioFileTypeID), &type);
}

static Array<String> findFileExtensionsForCoreAudioCodecs()
{
    return getStringInfo (kAudioFileGlobalInfo_AllExtensions, 0, nullptr);
}

static bool isEncodingStreamKindSupported (AppleCoreAudioFormat::StreamKind kind)
{
    using StreamKind = AppleCoreAudioFormat::StreamKind;

    switch (kind)
    {
        case StreamKind::kAacAdts:
        case StreamKind::kMp4:
        case StreamKind::kM4a:
        case StreamKind::kM4b:
        case StreamKind::kCaf:
        case StreamKind::k3gp:
        case StreamKind::k3gp2:
            return true;
        case StreamKind::kNone:
        default:
            break;
    }

    return false;
}

struct CoreAudioFormatMetadata
{
    static uint32 chunkName (const char* const name) noexcept
    {
        return ByteOrder::bigEndianInt (name);
    }

    struct FileHeader
    {
        explicit FileHeader (InputStream& input)
        {
            fileType = (uint32) input.readIntBigEndian();
            fileVersion = (uint16) input.readShortBigEndian();
            fileFlags = (uint16) input.readShortBigEndian();
        }

        uint32 fileType = 0;
        uint16 fileVersion = 0;
        uint16 fileFlags = 0;
    };

    struct ChunkHeader
    {
        explicit ChunkHeader (InputStream& input)
        {
            chunkType = (uint32) input.readIntBigEndian();
            chunkSize = (int64) input.readInt64BigEndian();
        }

        uint32 chunkType = 0;
        int64 chunkSize = 0;
    };

    static StringPairArray parseUserDefinedChunk (InputStream& input, int64 size)
    {
        StringPairArray infoStrings;
        const auto originalPosition = input.getPosition();

        uint8 uuid[16] = {};
        input.read (uuid, sizeof (uuid));

        if (memcmp (uuid, "\x29\x81\x92\x73\xB5\xBF\x4A\xEF\xB7\x8D\x62\xD1\xEF\x90\xBB\x2C", 16) == 0)
        {
            const auto numEntries = (uint32) input.readIntBigEndian();

            for (uint32 i = 0; i < numEntries && input.getPosition() < originalPosition + size; ++i)
                infoStrings.set (input.readString(), input.readString());
        }

        input.setPosition (originalPosition + size);
        return infoStrings;
    }

    static void findTempoEvents (MidiFile& midiFile, StringPairArray& midiMetadata)
    {
        MidiMessageSequence tempoEvents;
        midiFile.findAllTempoEvents (tempoEvents);

        const auto numTempoEvents = tempoEvents.getNumEvents();
        MemoryOutputStream tempoSequence;

        for (int i = 0; i < numTempoEvents; ++i)
        {
            if (auto* holder = tempoEvents.getEventPointer (i))
            {
                auto& midiMessage = holder->message;
                if (midiMessage.isTempoMetaEvent())
                {
                    const auto tempoSecondsPerQuarterNote = midiMessage.getTempoSecondsPerQuarterNote();
                    if (tempoSecondsPerQuarterNote > 0.0)
                    {
                        const auto tempo = 60.0 / tempoSecondsPerQuarterNote;

                        if (i == 0)
                            midiMetadata.set (AppleCoreAudioFormat::tempo, String (tempo));

                        if (numTempoEvents > 1)
                            tempoSequence << String (tempo) << ',' << tempoEvents.getEventTime (i) << ';';
                    }
                }
            }
        }

        if (tempoSequence.getDataSize() > 0)
            midiMetadata.set ("tempo sequence", tempoSequence.toUTF8());
    }

    static void findTimeSigEvents (MidiFile& midiFile, StringPairArray& midiMetadata)
    {
        MidiMessageSequence timeSigEvents;
        midiFile.findAllTimeSigEvents (timeSigEvents);

        const auto numTimeSigEvents = timeSigEvents.getNumEvents();
        MemoryOutputStream timeSigSequence;

        for (int i = 0; i < numTimeSigEvents; ++i)
        {
            int numerator = 0;
            int denominator = 0;
            timeSigEvents.getEventPointer (i)->message.getTimeSignatureInfo (numerator, denominator);

            String timeSigString;
            timeSigString << numerator << '/' << denominator;

            if (i == 0)
                midiMetadata.set (AppleCoreAudioFormat::timeSig, timeSigString);

            if (numTimeSigEvents > 1)
                timeSigSequence << timeSigString << ',' << timeSigEvents.getEventTime (i) << ';';
        }

        if (timeSigSequence.getDataSize() > 0)
            midiMetadata.set ("time signature sequence", timeSigSequence.toUTF8());
    }

    static void findKeySigEvents (MidiFile& midiFile, StringPairArray& midiMetadata)
    {
        MidiMessageSequence keySigEvents;
        midiFile.findAllKeySigEvents (keySigEvents);

        const auto numKeySigEvents = keySigEvents.getNumEvents();
        MemoryOutputStream keySigSequence;

        static const char* majorKeys[] = { "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#" };
        static const char* minorKeys[] = { "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#" };

        for (int i = 0; i < numKeySigEvents; ++i)
        {
            auto& message (keySigEvents.getEventPointer (i)->message);
            const auto key = jlimit (0, 14, message.getKeySignatureNumberOfSharpsOrFlats() + 7);
            const auto isMajor = message.isKeySignatureMajorKey();

            String keySigString (isMajor ? majorKeys[key] : minorKeys[key]);
            if (! isMajor)
                keySigString << 'm';

            if (i == 0)
                midiMetadata.set (AppleCoreAudioFormat::keySig, keySigString);

            if (numKeySigEvents > 1)
                keySigSequence << keySigString << ',' << keySigEvents.getEventTime (i) << ';';
        }

        if (keySigSequence.getDataSize() > 0)
            midiMetadata.set ("key signature sequence", keySigSequence.toUTF8());
    }

    static StringPairArray parseMidiChunk (InputStream& input, int64 size)
    {
        const auto originalPosition = input.getPosition();

        MemoryBlock midiBlock;
        input.readIntoMemoryBlock (midiBlock, (ssize_t) size);
        MemoryInputStream midiInputStream (midiBlock, false);

        StringPairArray midiMetadata;
        MidiFile midiFile;

        if (midiFile.readFrom (midiInputStream))
        {
            midiMetadata.set (AppleCoreAudioFormat::midiDataBase64, midiBlock.toBase64Encoding());
            findTempoEvents (midiFile, midiMetadata);
            findTimeSigEvents (midiFile, midiMetadata);
            findKeySigEvents (midiFile, midiMetadata);
        }

        input.setPosition (originalPosition + size);
        return midiMetadata;
    }

    static StringPairArray parseInformationChunk (InputStream& input)
    {
        StringPairArray infoStrings;
        const auto numEntries = (uint32) input.readIntBigEndian();

        for (uint32 i = 0; i < numEntries; ++i)
            infoStrings.set (input.readString(), input.readString());

        return infoStrings;
    }

    static bool read (InputStream& input, StringPairArray& metadataValues)
    {
        const auto originalPos = input.getPosition();

        const FileHeader cafFileHeader (input);
        const bool isCafFile = cafFileHeader.fileType == chunkName ("caff");

        if (isCafFile)
        {
            while (! input.isExhausted())
            {
                const ChunkHeader chunkHeader (input);

                if (chunkHeader.chunkType == chunkName ("uuid"))
                    metadataValues.addArray (parseUserDefinedChunk (input, chunkHeader.chunkSize));
                else if (chunkHeader.chunkType == chunkName ("midi"))
                    metadataValues.addArray (parseMidiChunk (input, chunkHeader.chunkSize));
                else if (chunkHeader.chunkType == chunkName ("info"))
                    metadataValues.addArray (parseInformationChunk (input));
                else if (chunkHeader.chunkType == chunkName ("data"))
                {
                    if (chunkHeader.chunkSize == -1)
                        break;

                    input.setPosition (input.getPosition() + chunkHeader.chunkSize);
                }
                else
                {
                    input.setPosition (input.getPosition() + chunkHeader.chunkSize);
                }
            }
        }

        input.setPosition (originalPos);
        return isCafFile;
    }
};

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
    AppleCoreAudioFormatReader (InputStream* sourceStream, AppleCoreAudioFormat::StreamKind kind);
    ~AppleCoreAudioFormatReader() override;

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override;
    AudioChannelSet getChannelLayout() override;

private:
    bool openFromStream (InputStream* sourceStream);
    void close();

    ExtAudioFileRef audioFile = nullptr;
    AudioFileID audioFileId = nullptr;
    AudioStreamBasicDescription inputFormat = {};
    AudioStreamBasicDescription clientFormat = {};
    SInt64 headerFrames = 0;
    bool isOpen = false;
    CoreAudioInputStreamState streamState;

    HeapBlock<float> tempBuffer;
    size_t tempBufferFrames = 0;
    AudioChannelSet channelSet;
    HeapBlock<int> channelMap;
    bool hasChannelMap = false;
    AppleCoreAudioFormat::StreamKind streamKind = AppleCoreAudioFormat::StreamKind::kNone;

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
                                int qualityOptionIndex,
                                AppleCoreAudioFormat::StreamKind kind);

    ~AppleCoreAudioFormatWriter() override;

    bool write (const float* const* samplesToWrite, int numSamples) override;
    bool flush() override;

private:
    void close();

    ExtAudioFileRef audioFile = nullptr;
    AudioFileID audioFileId = nullptr;
    AudioStreamBasicDescription clientFormat = {};
    AudioStreamBasicDescription fileFormat = {};
    bool isOpen = false;
    CoreAudioOutputStreamState streamState;

    HeapBlock<float> tempBuffer;
    size_t tempBufferFrames = 0;
    AppleCoreAudioFormat::StreamKind streamKind = AppleCoreAudioFormat::StreamKind::kNone;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppleCoreAudioFormatWriter)
};

AppleCoreAudioFormatReader::AppleCoreAudioFormatReader (InputStream* sourceStream, AppleCoreAudioFormat::StreamKind kind)
    : AudioFormatReader (sourceStream, "CoreAudio file")
{
    streamKind = kind;
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

    if (audioFileId != nullptr)
    {
        AudioFileClose (audioFileId);
        audioFileId = nullptr;
    }

    isOpen = false;
}

bool AppleCoreAudioFormatReader::openFromStream (InputStream* sourceStream)
{
    if (sourceStream == nullptr)
        return false;

    CoreAudioFormatMetadata::read (*sourceStream, metadataValues);

    streamState.stream = sourceStream;
    streamState.size = sourceStream->getTotalLength();

    audioFileId = nullptr;
    const auto fileTypeHint = toAudioFileTypeID (streamKind);
    auto err = AudioFileOpenWithCallbacks (&streamState,
                                           coreAudioInputReadProc,
                                           nullptr,
                                           coreAudioInputGetSizeProc,
                                           nullptr,
                                           fileTypeHint,
                                           &audioFileId);
    if (err == noErr)
        err = ExtAudioFileWrapAudioFileID (audioFileId, false, &audioFile);

    if (err != noErr && audioFileId != nullptr)
    {
        AudioFileClose (audioFileId);
        audioFileId = nullptr;
    }

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

    UInt32 sizeOfLayout = 0;
    UInt32 isWritable = 0;
    if (AudioFileGetPropertyInfo (audioFileId, kAudioFilePropertyChannelLayout, &sizeOfLayout, &isWritable) == noErr
        && sizeOfLayout >= (sizeof (AudioChannelLayout) - sizeof (AudioChannelDescription)))
    {
        HeapBlock<uint8> layoutData;
        layoutData.malloc (sizeOfLayout);
        auto* layout = reinterpret_cast<AudioChannelLayout*> (layoutData.getData());

        if (AudioFileGetProperty (audioFileId, kAudioFilePropertyChannelLayout, &sizeOfLayout, layout) == noErr)
        {
            auto fileLayout = CoreAudioLayouts::fromCoreAudio (*layout);
            if (fileLayout.size() == numChannels)
            {
                channelSet = fileLayout;
                auto caOrder = CoreAudioLayouts::getCoreAudioLayoutChannels (*layout);
                if (caOrder.size() == numChannels)
                {
                    channelMap.malloc (numChannels);
                    hasChannelMap = true;

                    for (int i = 0; i < numChannels; ++i)
                    {
                        const auto idx = channelSet.getChannelIndexForType (caOrder.getReference (i));
                        channelMap[i] = isPositiveAndBelow (idx, numChannels) ? idx : i;
                    }
                }
            }
        }
    }

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

    HeapBlock<float*> destWritePointers;
    destWritePointers.malloc (numDestChannels);

    for (int ch = 0; ch < numDestChannels; ++ch)
        destWritePointers[ch] = destChannels[ch] + startOffsetInDestBuffer;

    HeapBlock<float*> mappedDestChannels;
    mappedDestChannels.malloc (numChannels);

    HeapBlock<char> channelWritten;
    channelWritten.malloc (numDestChannels);

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

        zeromem (channelWritten.getData(), (size_t) numDestChannels);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto targetIndex = hasChannelMap ? channelMap[ch] : ch;
            if (isPositiveAndBelow (targetIndex, numDestChannels))
            {
                mappedDestChannels[ch] = destWritePointers[targetIndex];
                channelWritten[targetIndex] = 1;
            }
            else
            {
                mappedDestChannels[ch] = nullptr;
            }
        }

        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { tempBuffer.getData(), numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { mappedDestChannels.getData(), numChannels },
                                        (int) framesRead);

        for (int ch = 0; ch < numDestChannels; ++ch)
        {
            if (destWritePointers[ch] != nullptr)
            {
                if (channelWritten[ch] == 0)
                    zeromem (destWritePointers[ch], sizeof (float) * framesRead);

                destWritePointers[ch] += framesRead;
            }
        }

        totalFramesRead += (int) framesRead;
        remainingFrames -= (int) framesRead;
    }

    return totalFramesRead > 0;
}

AudioChannelSet AppleCoreAudioFormatReader::getChannelLayout()
{
    if (channelSet.size() == numChannels)
        return channelSet;

    return AudioFormatReader::getChannelLayout();
}

AppleCoreAudioFormatWriter::AppleCoreAudioFormatWriter (OutputStream* destStream,
                                                        double sampleRate,
                                                        int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex,
                                                        AppleCoreAudioFormat::StreamKind kind)
    : AudioFormatWriter (destStream, "CoreAudio file", sampleRate, numberOfChannels, bitsPerSample)
{
    ignoreUnused (metadataValues);
    streamKind = kind;

    if (destStream == nullptr || numberOfChannels < 1 || sampleRate <= 0.0)
        return;

    if (bitsPerSample != 32)
        return;

    AudioFileTypeID fileType = kAudioFileAAC_ADTSType;
    if (streamKind != AppleCoreAudioFormat::StreamKind::kNone)
    {
        if (! isEncodingStreamKindSupported (streamKind))
            return;

        const auto kindType = toAudioFileTypeID (streamKind);
        if (kindType != 0)
            fileType = kindType;
    }

    fileFormat = {};
    fileFormat.mSampleRate = sampleRate;
    fileFormat.mFormatID = kAudioFormatMPEG4AAC;
    fileFormat.mChannelsPerFrame = (UInt32) numberOfChannels;

    UInt32 size = sizeof (fileFormat);
    if (AudioFormatGetProperty (kAudioFormatProperty_FormatInfo, 0, nullptr, &size, &fileFormat) != noErr)
        return;

    streamState.stream = destStream;
    streamState.size = destStream->getPosition();

    audioFileId = nullptr;
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
    {
        AudioFileClose (audioFileId);
        audioFileId = nullptr;
    }

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

    if (audioFileId != nullptr)
    {
        AudioFileClose (audioFileId);
        audioFileId = nullptr;
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
const char* const AppleCoreAudioFormat::midiDataBase64 = "midiDataBase64";
const char* const AppleCoreAudioFormat::tempo = "tempo";
const char* const AppleCoreAudioFormat::timeSig = "time signature";
const char* const AppleCoreAudioFormat::keySig = "key signature";

AppleCoreAudioFormat::AppleCoreAudioFormat()
    : AppleCoreAudioFormat (StreamKind::kNone)
{
}

AppleCoreAudioFormat::AppleCoreAudioFormat (StreamKind kind)
    : formatName ("CoreAudio")
    , streamKind (kind)
{
}

AppleCoreAudioFormat::~AppleCoreAudioFormat() = default;

const String& AppleCoreAudioFormat::getFormatName() const
{
    return formatName;
}

Array<String> AppleCoreAudioFormat::getFileExtensions() const
{
    if (streamKind != StreamKind::kNone)
    {
        const auto extensions = findFileExtensionsForCoreAudioCodec (toAudioFileTypeID (streamKind));
        if (! extensions.isEmpty())
            return extensions;
    }

    const auto extensions = findFileExtensionsForCoreAudioCodecs();
    if (! extensions.isEmpty())
        return extensions;

    return { ".wav", ".aiff", ".aif", ".aifc", ".wav", ".sd2", ".au", ".snd", ".mp4", ".mp3", ".mp2", ".mp1", ".ac3", ".aac", ".m4a", ".m4b", ".caf", ".3gp", ".3g2", ".amr" };
}

std::unique_ptr<AudioFormatReader> AppleCoreAudioFormat::createReaderFor (InputStream* sourceStream)
{
    auto reader = std::make_unique<AppleCoreAudioFormatReader> (sourceStream, streamKind);

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

    if (numberOfChannels < 1)
        return nullptr;

    if (sampleRate <= 0.0)
        return nullptr;

    if (streamKind != StreamKind::kNone && ! isEncodingStreamKindSupported (streamKind))
        return nullptr;

    return std::make_unique<AppleCoreAudioFormatWriter> (streamToWriteTo,
                                                         sampleRate,
                                                         numberOfChannels,
                                                         bitsPerSample,
                                                         metadataValues,
                                                         qualityOptionIndex,
                                                         streamKind);
}

Array<int> AppleCoreAudioFormat::getPossibleBitDepths() const
{
    return {};
}

Array<int> AppleCoreAudioFormat::getPossibleSampleRates() const
{
    return { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 192000 };
}

} // namespace yup
