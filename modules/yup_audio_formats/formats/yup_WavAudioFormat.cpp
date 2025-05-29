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

        if (!stream->setPosition (byteOffset))
            return false;

        // Simple 16-bit implementation for now
        if (bitsPerSample == 16)
        {
            const int bufferSize = samplesToRead * frameSize;
            MemoryBlock tempBuffer (bufferSize);
            const int bytesRead = stream->read (tempBuffer.getData(), bufferSize);

            if (bytesRead == bufferSize)
            {
                auto* sourceData = static_cast<const int16*> (tempBuffer.getData());

                for (int sample = 0; sample < samplesToRead; ++sample)
                {
                    for (int channel = 0; channel < numChannels; ++channel)
                    {
                        int16 intValue = ByteOrder::littleEndianShort (sourceData + sample * numChannels + channel);
                        float floatValue = static_cast<float> (intValue) / 32768.0f;
                        buffer.getWritePointer (channel)[sample] = floatValue;
                    }
                }
                return true;
            }
        }

        return false;
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

        while (!stream->isExhausted() && (!foundFmt || !foundData))
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

                        if (audioFormat == 1 && numChannels > 0 && sampleRate > 0 && bitsPerSample == 16)
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
        if (!finalized)
            finalize();
    }

    bool writeSamples (const AudioSampleBuffer& buffer, int numSamples) override
    {
        if (stream == nullptr || buffer.getNumChannels() != numChannels)
            return false;

        // Simple 16-bit implementation
        if (bitsPerSample == 16)
        {
            const int frameSize = numChannels * 2;
            const int bufferSize = numSamples * frameSize;
            MemoryBlock tempBuffer (bufferSize);
            auto* destData = static_cast<int16*> (tempBuffer.getData());

            for (int sample = 0; sample < numSamples; ++sample)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    float floatValue = buffer.getReadPointer (channel)[sample];
                    floatValue = jlimit (-1.0f, 1.0f, floatValue);
                    int16 intValue = static_cast<int16> (floatValue * 32767.0f);
                    destData[sample * numChannels + channel] = intValue;
                }
            }

            bool success = stream->write (tempBuffer.getData(), bufferSize);
            if (success)
                samplesWritten += numSamples;

            return success;
        }

        return false;
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

    void writeHeader()
    {
        // Write RIFF header
        stream->write ("RIFF", 4);
        stream->writeInt (0); // Placeholder for file size
        stream->write ("WAVE", 4);

        // Write fmt chunk
        stream->write ("fmt ", 4);
        stream->writeInt (16); // Format chunk size
        stream->writeShort (1); // PCM format
        stream->writeShort (static_cast<short> (numChannels));
        stream->writeInt (sampleRate);
        stream->writeInt (sampleRate * numChannels * (bitsPerSample / 8)); // Byte rate
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
    return filePath.existsAsFile() &&
           (filePath.hasFileExtension (".wav") || filePath.hasFileExtension (".rf64"));
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
    if (stream == nullptr || sampleRate <= 0 || numChannels <= 0 || bitsPerSample != 16)
        return nullptr;

    return std::make_unique<WAVAudioFormatWriter> (stream, sampleRate, numChannels, bitsPerSample);
}

} // namespace yup
