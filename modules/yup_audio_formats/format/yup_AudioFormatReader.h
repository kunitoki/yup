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
    Reads samples from an audio file stream.

    A subclass that reads a specific type of audio format will be created by
    an AudioFormat object.
*/
class YUP_API AudioFormatReader
{
public:
    /** Destructor. */
    virtual ~AudioFormatReader() = default;

    /** Returns a description of what type of format this is. */
    const String& getFormatName() const noexcept { return formatName; }

    /** Reads samples from the stream.

        @param destChannels             an array of pointers to arrays of floats, into which
                                        the sample data for each channel will be written.
        @param numDestChannels          the number of array pointers in the destChannels array
        @param startSampleInSource      the position in the audio file from which to start reading
        @param numSamplesToRead         the number of samples to read

        @returns true if the operation succeeded
    */
    bool read (float* const* destChannels, int numDestChannels, int64 startSampleInSource, int numSamplesToRead);

    /** Reads samples from the stream.

        @param destChannels                     an array of pointers to arrays of ints, into which
                                                the sample data for each channel will be written
        @param numDestChannels                  the number of array pointers in the destChannels array
        @param startSampleInSource              the position in the audio file from which to start reading
        @param numSamplesToRead                 the number of samples to read
        @param fillLeftoverChannelsWithCopies   if true, any channels in destChannels above
                                                numChannels will be filled with copies of the
                                                existing channels

        @returns true if the operation succeeded
    */
    bool read (int* const* destChannels, int numDestChannels, int64 startSampleInSource, int numSamplesToRead, bool fillLeftoverChannelsWithCopies);

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
    unsigned int bitsPerSample = 0;

    /** The total number of samples in the audio stream. */
    int64 lengthInSamples = 0;

    /** The total number of channels in the audio stream. */
    unsigned int numChannels = 0;

    /** Indicates whether the data is floating-point or fixed. */
    bool usesFloatingPointData = false;

    /** A set of metadata values that the reader has pulled out of the stream. */
    StringPairArray metadataValues;

    /** The input stream, for use by subclasses. */
    std::unique_ptr<InputStream> input;

    //==============================================================================
    /** Used by subclasses to copy data to different formats. */
    struct ReadHelper
    {
        /** Reads samples from a file in the given format. */
        static void read (void* destData, const void* sourceData, int numSamples, int srcBytesPerSample, bool isFloatingPoint, bool isLittleEndian) noexcept;

        /** Reads 8-bit signed samples. */
        static void readInt8 (int* dest, const void* src, int numSamples) noexcept;

        /** Reads 16-bit samples. */
        static void readInt16 (int* dest, const void* src, int numSamples, bool littleEndian) noexcept;

        /** Reads 24-bit samples. */
        static void readInt24 (int* dest, const void* src, int numSamples, bool littleEndian) noexcept;

        /** Reads 32-bit samples. */
        static void readInt32 (int* dest, const void* src, int numSamples, bool littleEndian) noexcept;

        /** Reads 32-bit float samples. */
        static void readFloat32 (float* dest, const void* src, int numSamples, bool littleEndian) noexcept;

        /** Reads 64-bit float samples. */
        static void readFloat64 (float* dest, const void* src, int numSamples, bool littleEndian) noexcept;
    };

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
