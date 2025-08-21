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
    Abstract base class for reading audio sample data from formatted audio streams.

    AudioFormatReader provides a standardized interface for reading audio data from various
    audio file formats. Each concrete implementation handles the specific decoding requirements
    of a particular format (such as WAV, FLAC, or MP3), while presenting a unified API for
    accessing audio samples as floating-point data.

    Key features:
    - Converts all audio data to floating-point samples for consistent processing
    - Supports multi-channel audio with flexible channel mapping
    - Provides metadata extraction capabilities
    - Offers both low-level sample reading and high-level convenience methods
    - Includes level analysis and sample searching functionality

    The reader maintains important audio properties such as sample rate, bit depth, channel count,
    and total length in samples. It also preserves metadata found in the audio file for applications
    that need access to title, artist, album information, and other embedded data.

    Format-specific implementations are typically created through AudioFormat::createReaderFor(),
    which handles the complexities of format detection and appropriate reader instantiation.

    @see AudioFormat, AudioFormatWriter, AudioFormatManager

    @tags{Audio}
*/
class YUP_API AudioFormatReader
{
public:
    /** Destructor. */
    virtual ~AudioFormatReader() = default;

    /** Returns a descriptive name identifying the audio format being read.

        This method provides a human-readable description of the format that this reader
        is designed to handle, such as "Wave file", "FLAC Audio", or "MP3 Audio".

        @returns A reference to the format name string
    */
    const String& getFormatName() const noexcept { return formatName; }

    /** Reads audio sample data from the stream into floating-point arrays.

        This is the primary method for extracting audio samples from the stream. All samples
        are converted to floating-point values in the range approximately ±1.0, regardless
        of the original format's bit depth or encoding.

        @param destChannels             An array of pointers to float arrays, one per channel.
                                        Each array must have space for at least numSamplesToRead samples.
        @param numDestChannels          The number of channel arrays provided in destChannels.
                                        If this is less than the source channel count, only the first
                                        numDestChannels will be read.
        @param startSampleInSource      The zero-based sample position in the source file to begin
                                        reading from. Must be within the range [0, lengthInSamples).
        @param numSamplesToRead         The number of samples to read from each channel.
        @param fillLeftoverChannelsWithCopies   if true, any channels in destChannels above
                                                numChannels will be filled with copies of the
                                                existing channels

        @returns true if the read operation completed successfully, false if an error occurred
                or if the requested range extends beyond the available audio data
    */
    bool read (float* const* destChannels,
               int numDestChannels,
               int64 startSampleInSource,
               int numSamplesToRead,
               bool fillLeftoverChannelsWithCopies = false);

    /** Fills a section of an AudioBuffer from this reader.

        @param buffer                   the buffer to fill
        @param startSampleInDestBuffer  the position in the buffer at which to start writing samples
        @param numSamples               the number of samples to read
        @param readerStartSample        the position in the audio file from which to start reading
        @param useReaderLeftChan        if true, the reader's left channel will be used
        @param useReaderRightChan       if true, the reader's right channel will be used

        @returns true if the operation succeeded
    */
    bool read (AudioBuffer<float>* buffer,
               int startSampleInDestBuffer,
               int numSamples,
               int64 readerStartSample,
               bool useReaderLeftChan,
               bool useReaderRightChan);

    /** Finds the highest and lowest sample levels from a section of the audio stream. */
    virtual void readMaxLevels (int64 startSample, int64 numSamples, Range<float>* results, int numChannelsToRead);

    /** Finds the highest and lowest sample levels from a section of the audio stream. */
    virtual void readMaxLevels (int64 startSample, int64 numSamples, float& lowestLeft, float& highestLeft, float& lowestRight, float& highestRight);

    /** Scans the source looking for a sample whose magnitude is in a specified range.

        @param startSample              the first sample to check
        @param numSamplesToSearch       the number of samples to scan
        @param magnitudeRangeMinimum    the lowest magnitude (absolute) that is considered a match
        @param magnitudeRangeMaximum    the highest magnitude (absolute) that is considered a match
        @param minimumConsecutiveSamples the minimum number of consecutive samples that must be in
                                         the magnitude range for a match to be registered

        @returns the index of the first matching sample, or -1 if none were found
    */
    int64 searchForLevel (int64 startSample,
                          int64 numSamplesToSearch,
                          double magnitudeRangeMinimum,
                          double magnitudeRangeMaximum,
                          int minimumConsecutiveSamples);

    /** Get the channel layout of the audio stream. */
    virtual AudioChannelSet getChannelLayout();

    //==============================================================================
    /** The sample-rate of the stream. */
    double sampleRate = 0;

    /** The number of bits per sample, e.g. 16, 24, 32. */
    int bitsPerSample = 0;

    /** The total number of samples in the audio stream. */
    int64 lengthInSamples = 0;

    /** The total number of channels in the audio stream. */
    int numChannels = 0;

    /** Indicates whether the data is floating-point or fixed. */
    bool usesFloatingPointData = false;

    /** A set of metadata values that the reader has pulled out of the stream. */
    StringPairArray metadataValues;

    /** The input stream, for use by subclasses. */
    std::unique_ptr<InputStream> input;

protected:
    /** Creates an AudioFormatReader object. */
    AudioFormatReader (InputStream* sourceStream, const String& formatName);

    /** Subclasses must implement this method to perform the low-level read operation.

        @param destChannels                     the destination arrays for each channel's samples
        @param numDestChannels                  the number of destination channels
        @param startOffsetInDestBuffer          the offset in the destination buffer to start writing
        @param startSampleInFile                the position to start reading from in the audio file
        @param numSamples                       the number of samples to read

        @returns true if the operation succeeded
    */
    virtual bool readSamples (float* const* destChannels,
                              int numDestChannels,
                              int startOffsetInDestBuffer,
                              int64 startSampleInFile,
                              int numSamples) = 0;

private:
    String formatName;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFormatReader)
};

} // namespace yup
