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

//==============================================================================
// Simple WAV/RF64 Audio Format Reader

class WAVAudioFormatReader : public AudioFormatReader
{
public:
    WAVAudioFormatReader (InputStream* sourceStream)
        : stream (sourceStream)
        , sampleRate (0)
        , numChannels (0)
        , bitsPerSample (0)
        , totalSamples (0)
        , dataOffset (0)
        , isRF64 (false)
    {
        if (stream != nullptr && parseHeader())
        {
            // Successfully parsed
        }
        else
        {
            stream = nullptr; // Mark as invalid
        }
    }

    double getSampleRate() const override { return sampleRate; }

    int getNumChannels() const override { return numChannels; }

    int64 getTotalSamples() const override { return totalSamples; }

    bool readSamples (AudioSampleBuffer& buffer, int64 startSampleInFile, int64 numSamples) override
    {
        if (stream == nullptr || buffer.getNumChannels() != numChannels)
            return false;

        const int samplesToRead = static_cast<int> (jmin (static_cast<int64> (numSamples), static_cast<int64> (buffer.getNumSamples())));
        const int frameSize = numChannels * (bitsPerSample / 8);
        const int64 byteOffset = dataOffset + startSampleInFile * frameSize;

        if (! stream->setPosition (byteOffset))
            return false;

        const int bufferSize = samplesToRead * frameSize;
        MemoryBlock tempBuffer (bufferSize);
        const int bytesRead = stream->read (tempBuffer.getData(), bufferSize);

        if (bytesRead != bufferSize)
            return false;

        // Use AudioData for proper conversion based on bit depth
        switch (bitsPerSample)
        {
            case 8:
                convertFromInterleavedSource<AudioData::UInt8> (tempBuffer.getData(), buffer, samplesToRead);
                break;

            case 16:
                convertFromInterleavedSource<AudioData::Int16> (tempBuffer.getData(), buffer, samplesToRead);
                break;

            case 24:
                convertFromInterleavedSource<AudioData::Int24> (tempBuffer.getData(), buffer, samplesToRead);
                break;

            case 32:
                convertFromInterleavedSource<AudioData::Float32> (tempBuffer.getData(), buffer, samplesToRead);
                break;

            default:
                return false;
        }

        return true;
    }

    bool isValidFile() const { return stream != nullptr; }

private:
    InputStream* stream;
    double sampleRate;
    int numChannels;
    int bitsPerSample;
    int64 totalSamples;
    int64 dataOffset;
    bool isRF64;

    template <typename SourceFormat>
    void convertFromInterleavedSource (void* sourceData, AudioSampleBuffer& buffer, int samplesToRead)
    {
        using SourceType = AudioData::Pointer<SourceFormat, AudioData::LittleEndian, AudioData::Interleaved, AudioData::Const>;
        using DestType = AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::NonConst>;

        SourceType source (sourceData, numChannels);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            DestType dest (buffer.getWritePointer (channel));
            SourceType channelSource (static_cast<const char*> (sourceData) + channel * SourceFormat::bytesPerSample, numChannels);
            dest.convertSamples (channelSource, samplesToRead);
        }
    }

    bool parseHeader()
    {
        char header[12];
        if (stream->read (header, 12) != 12)
            return false;

        // Check for RIFF or RF64
        if (memcmp (header, "RIFF", 4) == 0)
            isRF64 = false;
        else if (memcmp (header, "RF64", 4) == 0)
            isRF64 = true;
        else
            return false;

        if (memcmp (header + 8, "WAVE", 4) != 0)
            return false;

        // Simple implementation: look for fmt and data chunks
        bool foundFmt = false;
        bool foundData = false;

        while (! stream->isExhausted() && (! foundFmt || ! foundData))
        {
            char chunkHeader[8];
            if (stream->read (chunkHeader, 8) != 8)
                break;

            uint32 chunkSize = ByteOrder::littleEndianInt (chunkHeader + 4);
            String chunkName (chunkHeader, 4);

            if (chunkName == "fmt ")
            {
                if (chunkSize >= 16)
                {
                    char fmtData[16];
                    if (stream->read (fmtData, 16) == 16)
                    {
                        uint16 audioFormat = ByteOrder::littleEndianShort (fmtData);
                        numChannels = ByteOrder::littleEndianShort (fmtData + 2);
                        sampleRate = ByteOrder::littleEndianInt (fmtData + 4);
                        bitsPerSample = ByteOrder::littleEndianShort (fmtData + 14);

                        // Support PCM format with 8, 16, 24, or 32 bits
                        if (audioFormat == 1 && numChannels > 0 && sampleRate > 0 && (bitsPerSample == 8 || bitsPerSample == 16 || bitsPerSample == 24 || bitsPerSample == 32))
                            foundFmt = true;
                    }

                    // Skip any remaining format data
                    if (chunkSize > 16)
                        stream->skipNextBytes (chunkSize - 16);
                }
            }
            else if (chunkName == "data")
            {
                dataOffset = stream->getPosition();
                totalSamples = chunkSize / (numChannels * (bitsPerSample / 8));
                foundData = true;
                break; // Don't read the data content
            }
            else
            {
                // Skip unknown chunk
                stream->skipNextBytes (chunkSize);
            }

            // Ensure even alignment
            if (chunkSize & 1)
                stream->readByte();
        }

        return foundFmt && foundData;
    }
};

//==============================================================================
// Simple WAV Audio Format Writer

class WAVAudioFormatWriter : public AudioFormatWriter
{
public:
    WAVAudioFormatWriter (OutputStream* destStream, int sampleRate, int numChannels, int bitsPerSample)
        : stream (destStream)
        , sampleRate (sampleRate)
        , numChannels (numChannels)
        , bitsPerSample (bitsPerSample)
        , samplesWritten (0)
        , finalized (false)
    {
        if (stream != nullptr)
            writeHeader();
    }

    ~WAVAudioFormatWriter() override
    {
        if (! finalized)
            finalize();
    }

    bool writeSamples (const AudioSampleBuffer& buffer, int numSamples) override
    {
        if (numSamples == 0)
            return true;

        if (stream == nullptr || buffer.getNumChannels() != numChannels)
            return false;

        // Use AudioData for proper conversion based on bit depth
        switch (bitsPerSample)
        {
            case 8:
                return convertToInterleavedDest<AudioData::UInt8> (buffer, numSamples);

            case 16:
                return convertToInterleavedDest<AudioData::Int16> (buffer, numSamples);

            case 24:
                return convertToInterleavedDest<AudioData::Int24> (buffer, numSamples);

            case 32:
                return convertToInterleavedDest<AudioData::Float32> (buffer, numSamples);

            default:
                return false;
        }
    }

    bool finalize() override
    {
        if (finalized || stream == nullptr)
            return finalized;

        finalized = true;

        // Update header with final sizes
        int64 dataSize = samplesWritten * numChannels * (bitsPerSample / 8);
        int64 fileSize = 36 + dataSize; // Basic WAV header size + data

        // Update RIFF chunk size
        stream->setPosition (4);
        stream->writeInt (static_cast<uint32> (fileSize - 8));

        // Update data chunk size
        stream->setPosition (40); // Skip RIFF header + fmt chunk
        stream->writeInt (static_cast<uint32> (dataSize));

        stream->flush();
        return true;
    }

private:
    OutputStream* stream;
    int sampleRate;
    int numChannels;
    int bitsPerSample;
    int64 samplesWritten;
    bool finalized;

    template <typename DestFormat>
    bool convertToInterleavedDest (const AudioSampleBuffer& buffer, int numSamples)
    {
        using SourceType = AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const>;
        using DestType = AudioData::Pointer<DestFormat, AudioData::LittleEndian, AudioData::Interleaved, AudioData::NonConst>;

        const int frameSize = numChannels * DestFormat::bytesPerSample;
        const int bufferSize = numSamples * frameSize;
        MemoryBlock tempBuffer (bufferSize);

        DestType dest (tempBuffer.getData(), numChannels);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            SourceType source (buffer.getReadPointer (channel));
            DestType channelDest (static_cast<char*> (tempBuffer.getData()) + channel * DestFormat::bytesPerSample, numChannels);
            channelDest.convertSamples (source, numSamples);
        }

        bool success = stream->write (tempBuffer.getData(), bufferSize);
        if (success)
            samplesWritten += numSamples;

        return success;
    }

    void writeHeader()
    {
        // Write RIFF header
        stream->write ("RIFF", 4);
        stream->writeInt (0); // Placeholder for file size
        stream->write ("WAVE", 4);

        // Write fmt chunk
        stream->write ("fmt ", 4);
        stream->writeInt (16);  // Format chunk size
        stream->writeShort (1); // PCM format
        stream->writeShort (static_cast<short> (numChannels));
        stream->writeInt (sampleRate);
        stream->writeInt (sampleRate * numChannels * (bitsPerSample / 8));           // Byte rate
        stream->writeShort (static_cast<short> (numChannels * (bitsPerSample / 8))); // Block align
        stream->writeShort (static_cast<short> (bitsPerSample));

        // Write data chunk header
        stream->write ("data", 4);
        stream->writeInt (0); // Placeholder for data size
    }
};

//==============================================================================
// WAVAudioFormat implementation

WAVAudioFormat::WAVAudioFormat()
{
}

String WAVAudioFormat::getFormatName() const
{
    return "WAV/RF64";
}

StringArray WAVAudioFormat::getSupportedFileExtensions() const
{
    StringArray extensions;
    extensions.add (".wav");
    extensions.add (".rf64");
    return extensions;
}

bool WAVAudioFormat::canHandleFile (const File& filePath) const
{
    return filePath.existsAsFile() && (filePath.hasFileExtension (".wav") || filePath.hasFileExtension (".rf64"));
}

Array<int> WAVAudioFormat::getSupportedBitsPerSample() const
{
    Array<int> bitsPerSample;
    bitsPerSample.add (8);  // 8-bit unsigned PCM
    bitsPerSample.add (16); // 16-bit signed PCM
    bitsPerSample.add (24); // 24-bit signed PCM
    bitsPerSample.add (32); // 32-bit signed PCM
    return bitsPerSample;
}

Array<int> WAVAudioFormat::getSupportedSampleRates() const
{
    Array<int> sampleRates;
    sampleRates.add (8000);
    sampleRates.add (11025);
    sampleRates.add (16000);
    sampleRates.add (22050);
    sampleRates.add (44100);
    sampleRates.add (48000);
    sampleRates.add (88200);
    sampleRates.add (96000);
    sampleRates.add (176400);
    sampleRates.add (192000);
    return sampleRates;
}

std::unique_ptr<AudioFormatReader> WAVAudioFormat::createReaderFor (InputStream* stream)
{
    if (stream == nullptr)
        return nullptr;

    auto reader = std::make_unique<WAVAudioFormatReader> (stream);
    return reader->isValidFile() ? std::move (reader) : nullptr;
}

std::unique_ptr<AudioFormatWriter> WAVAudioFormat::createWriterFor (OutputStream* stream,
                                                                    int sampleRate,
                                                                    int numChannels,
                                                                    int bitsPerSample)
{
    if (stream == nullptr || sampleRate <= 0 || numChannels <= 0)
        return nullptr;

    // Check if the bit depth is supported
    Array<int> supportedBits = getSupportedBitsPerSample();
    if (! supportedBits.contains (bitsPerSample))
        return nullptr;

    return std::make_unique<WAVAudioFormatWriter> (stream, sampleRate, numChannels, bitsPerSample);
}

} // namespace yup
