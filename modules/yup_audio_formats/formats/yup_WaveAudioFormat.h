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
/**
    Wave audio format implementation using dr_wav.
*/
class YUP_API WaveAudioFormat : public AudioFormat
{
public:
    /** Constructor. */
    WaveAudioFormat();

    /** Destructor. */
    ~WaveAudioFormat() override;

    /** Returns the name of this format. */
    const String& getFormatName() const override;

    /** Returns the file extensions associated with this format. */
    Array<String> getFileExtensions() const override;

    /** Creates a reader for this format. */
    std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a writer for this format. */
    std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* streamToWriteTo,
                                                        double sampleRate,
                                                        unsigned int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    /** Returns a set of bit depths that the format can write. */
    Array<int> getPossibleBitDepths() const override;

    /** Returns a set of sample rates that the format can write. */
    Array<int> getPossibleSampleRates() const override;

    /** Returns true if this format can write files with multiple channels. */
    bool canDoMono() const override { return true; }

    /** Returns true if this format can write stereo files. */
    bool canDoStereo() const override { return true; }

private:
    String formatName;
};

//==============================================================================
/**
    Wave audio format reader implementation.
*/
class WaveAudioFormatReader : public AudioFormatReader
{
public:
    /** Constructor. */
    WaveAudioFormatReader (InputStream* sourceStream);

    /** Destructor. */
    ~WaveAudioFormatReader() override;

    /** Reads samples from the file. */
    bool readSamples (int* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveAudioFormatReader)
};

//==============================================================================
/**
    Wave audio format writer implementation.
*/
class WaveAudioFormatWriter : public AudioFormatWriter
{
public:
    /** Constructor. */
    WaveAudioFormatWriter (OutputStream* destStream,
                           double sampleRate,
                           unsigned int numberOfChannels,
                           unsigned int bitsPerSample,
                           const StringPairArray& metadataValues);

    /** Destructor. */
    ~WaveAudioFormatWriter() override;

    /** Writes samples to the file. */
    bool write (const int** samplesToWrite, int numSamples) override;

    /** Flushes any pending data. */
    bool flush() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveAudioFormatWriter)
};

} // namespace yup