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
    Abstract base class for writing audio sample data to formatted audio streams.

    AudioFormatWriter provides a standardized interface for encoding and writing audio data
    to various audio file formats. Each concrete implementation handles the specific encoding
    requirements of a particular format (such as WAV, FLAC, or MP3), while accepting
    floating-point sample data through a unified API.

    Key features:
    - Accepts floating-point samples for consistent input format
    - Handles format-specific encoding and bit depth conversion internally
    - Supports multi-channel audio output with proper interleaving
    - Provides metadata embedding capabilities where supported by the format
    - Offers both direct sample writing and high-level convenience methods
    - Includes threaded writing support for background processing

    The writer is configured during construction with essential parameters like sample rate,
    channel count, and bit depth. These parameters determine how the floating-point input
    samples are encoded into the target format's specific representation.

    Format-specific implementations are typically created through AudioFormat::createWriterFor(),
    which validates parameters against format capabilities and instantiates the appropriate
    writer with proper configuration.

    @see AudioFormat, AudioFormatReader, AudioFormatManager

    @tags{Audio}
*/
class YUP_API AudioFormatWriter
{
public:
    /** Destructor. */
    virtual ~AudioFormatWriter();

    /** Returns a descriptive name identifying the audio format being written.

        This method provides a human-readable description of the format that this writer
        is designed to produce, such as "Wave file", "FLAC Audio", or "MP3 Audio".

        @returns A reference to the format name string
    */
    const String& getFormatName() const noexcept { return formatName; }

    /** Writes floating-point audio sample data to the output stream.

        This is the primary method for encoding and writing audio samples to the stream.
        The floating-point samples (typically in the range ±1.0) are converted to the
        appropriate format-specific encoding and bit depth as configured during construction.

        @param samplesToWrite   An array of pointers to float arrays, one per channel.
                               The number of pointers must match the channel count specified
                               during writer creation. Each array must contain at least
                               numSamples valid sample values.
        @param numSamples       The number of samples to write from each channel array.
                               Must be greater than 0.

        @returns true if the samples were successfully encoded and written to the stream,
                false if an encoding error occurred or if the stream write failed
    */
    virtual bool write (const float* const* samplesToWrite, int numSamples) = 0;

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
        static void write (void* destData, const void* sourceData, int numSamples, int destBytesPerSample, bool isFloatingPoint, bool isLittleEndian) noexcept;

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
                       int numberOfChannels,
                       int bitsPerSample);

    /** The output stream for use by subclasses. */
    std::unique_ptr<OutputStream> output;

private:
    String formatName;
    double sampleRate;
    int numChannels;
    int bitsPerSample;
    bool isFloatingPointFormat;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFormatWriter)
};

} // namespace yup
