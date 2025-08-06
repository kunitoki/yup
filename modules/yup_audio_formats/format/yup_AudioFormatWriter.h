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
    Writes samples to an audio file stream.

    A subclass that writes a specific type of audio format will be created by
    an AudioFormat object.
*/
class YUP_API AudioFormatWriter
{
public:
    /** Destructor. */
    virtual ~AudioFormatWriter();

    /** Returns a description of what type of format this is. */
    const String& getFormatName() const noexcept { return formatName; }

    /** Writes a set of samples to the audio stream.

        @param samplesToWrite   an array of pointers to arrays containing the sample data
                                for each channel to write. The number of channels must
                                match the number of channels that this writer was created with.
        @param numSamples       the number of samples to write

        @returns true if the operation succeeded
    */
    virtual bool write (const int** samplesToWrite, int numSamples) = 0;

    /** Some formats may support a flush operation that makes sure the file
        is in a valid state before carrying on.

        @returns true if the operation succeeded
    */
    virtual bool flush();

    /** Reads a section of samples from an AudioFormatReader, and writes these to the output.

        @param reader               the reader to use as the source
        @param startSample          the sample within the reader to start reading from
        @param numSamplesToRead     the number of samples to read and write

        @returns true if the operation succeeded
    */
    bool writeFromAudioReader (AudioFormatReader& reader,
                               int64 startSample,
                               int64 numSamplesToRead);

    /** Reads some samples from an AudioSource, and writes these to the output.

        @param source               the source to read from
        @param numSamplesToRead     the number of samples to read and write
        @param samplesPerBlock      the maximum number of samples to process in each block

        @returns true if the operation succeeded
    */
    bool writeFromAudioSource (AudioSource& source,
                               int numSamplesToRead,
                               int samplesPerBlock = 2048);

    /** Writes some samples from an AudioBuffer.

        @param source           the buffer to read from
        @param startSample      the sample within the buffer to start reading from
        @param numSamples       the number of samples to read and write

        @returns true if the operation succeeded
    */
    bool writeFromAudioSampleBuffer (const AudioBuffer<float>& source,
                                     int startSample,
                                     int numSamples);

    /** Writes some samples from a set of float data channels.

        @param channels         an array of pointers to arrays of floats containing the
                                sample data for each channel
        @param numChannels      the number of channels to write
        @param numSamples       the number of samples to write

        @returns true if the operation succeeded
    */
    bool writeFromFloatArrays (const float* const* channels,
                               int numChannels,
                               int numSamples);

    /** Returns the sample rate being used. */
    double getSampleRate() const noexcept { return sampleRate; }

    /** Returns the number of channels being written. */
    int getNumChannels() const noexcept { return numChannels; }

    /** Returns the bit-depth of the data being written. */
    int getBitsPerSample() const noexcept { return bitsPerSample; }

    /** Returns true if it's a floating-point format, false if it's fixed-point. */
    bool isFloatingPoint() const noexcept { return isFloatingPointFormat; }

    //==============================================================================
    /** Provides a FIFO for an AudioFormatWriter, allowing you to push incoming
        data into a buffer which will be flushed to disk by a background thread.
    */
    class ThreadedWriter
    {
    public:
        /** Creates a ThreadedWriter for a given writer and buffer size. */
        ThreadedWriter (std::unique_ptr<AudioFormatWriter> writer,
                        TimeSliceThread& backgroundThread,
                        int numSamplesToBuffer);

        /** Destructor. */
        ~ThreadedWriter();

        /** Returns true if there's any data still to be written. */
        bool isThreadRunning() const;

        /** Writes some samples to the FIFO. */
        bool write (const float* const* data, int numSamples);

        /** Tells the thread to finish writing and then stop. */
        void waitForThreadToFinish();

    private:
        class ThreadedWriterHelper;
        std::unique_ptr<ThreadedWriterHelper> helper;
    };

    //==============================================================================
    /** Used by subclasses to copy data to different formats. */
    struct WriteHelper
    {
        /** Writes data in various formats. */
        static void write (void* destData, const void* sourceData,
                           int numSamples, int destBytesPerSample,
                           bool isFloatingPoint, bool isLittleEndian) noexcept;

        /** Writes 8-bit signed samples. */
        static void writeInt8 (void* dest, const void* src, int numSamples) noexcept;

        /** Writes 16-bit samples. */
        static void writeInt16 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept;

        /** Writes 24-bit samples. */
        static void writeInt24 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept;

        /** Writes 32-bit samples. */
        static void writeInt32 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept;

        /** Writes 32-bit float samples. */
        static void writeFloat32 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept;

        /** Writes 64-bit float samples. */
        static void writeFloat64 (void* dest, const void* src, int numSamples, bool littleEndian) noexcept;
    };

protected:
    /** Creates an AudioFormatWriter object. */
    AudioFormatWriter (OutputStream* destStream,
                       const String& formatName,
                       double sampleRate,
                       unsigned int numberOfChannels,
                       unsigned int bitsPerSample);

    /** The output stream for use by subclasses. */
    OutputStream* output;

private:
    String formatName;
    double sampleRate;
    unsigned int numChannels, bitsPerSample;
    bool isFloatingPointFormat;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFormatWriter)
};

} // namespace yup