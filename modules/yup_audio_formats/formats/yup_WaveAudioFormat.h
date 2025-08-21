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
    AudioFormat implementation for reading and writing WAV audio files.

    WaveAudioFormat provides comprehensive support for the WAV (Waveform Audio File Format)
    audio container format, utilizing the high-performance dr_wav library for low-level
    audio data processing. This implementation handles the complexities of the WAV format
    specification while presenting a clean, easy-to-use interface through the AudioFormat API.

    Supported WAV features:
    - Multiple bit depths: 8-bit, 16-bit, 24-bit, and 32-bit (both integer and float)
    - 64-bit double precision floating-point samples
    - Various encoding types: PCM, IEEE floating-point, A-law, and μ-law companding
    - Full multichannel support (mono, stereo, and surround configurations)
    - Wide range of sample rates from 8kHz to 192kHz
    - Metadata support for embedded title, artist, album, and other information
    - Both little-endian and big-endian byte order handling

    The implementation automatically detects and handles different WAV subtypes and encoding
    formats, converting all audio data to normalized floating-point samples for consistent
    processing. Special attention has been paid to A-law and μ-law formats to ensure proper
    dynamic range and level consistency with PCM formats.

    This format is uncompressed and supports high-quality audio reproduction with no
    generation loss, making it ideal for professional audio applications, digital audio
    workstations, and any scenario where audio fidelity is paramount.

    @see AudioFormat, AudioFormatReader, AudioFormatWriter

    @tags{Audio}
*/
class YUP_API WaveAudioFormat : public AudioFormat
{
public:
    /** Constructs a new WaveAudioFormat instance.
        
        Initializes the format handler with default settings for WAV file processing.
        The instance is ready to create readers and writers for WAV files immediately
        after construction.
    */
    WaveAudioFormat();

    /** Destructor.
        
        Cleans up any resources used by this format instance. All created readers
        and writers continue to function independently after the format is destroyed.
    */
    ~WaveAudioFormat() override;

    /** Returns the descriptive name of this format.
        
        @returns The string "Wave file" identifying this as a WAV format handler
    */
    const String& getFormatName() const override;

    /** Returns the file extensions that this format can handle.
        
        WAV files can have several different extensions depending on their specific
        variant or the application that created them.
        
        @returns An array containing the supported extensions: ".wav", ".wave", and ".bwf"
                (Broadcast Wave Format)
    */
    Array<String> getFileExtensions() const override;

    /** Creates a reader for decoding WAV audio data from the provided stream.
        
        This method attempts to parse the WAV header and create an appropriate reader
        for the specific WAV variant detected. The reader will handle format-specific
        decoding including PCM, floating-point, A-law, and μ-law encodings.
        
        @param sourceStream The input stream containing WAV audio data. The format
                           takes ownership of this stream if successful.
        @returns A WaveAudioFormatReader if the stream contains valid WAV data,
                nullptr if the stream cannot be parsed as a WAV file
    */
    std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a writer for encoding audio data to WAV format.
        
        This method creates a WAV writer configured for the specified audio parameters.
        The writer will encode floating-point input samples to the requested bit depth
        and format the output according to WAV specifications.
        
        @param streamToWriteTo     The output stream where WAV data will be written
        @param sampleRate          The sample rate in Hz (supports 8kHz to 192kHz)
        @param numberOfChannels    The number of audio channels (1-64 channels supported)
        @param bitsPerSample       The bit depth (8, 16, 24, or 32 bits per sample)
        @param metadataValues      Metadata to embed in the WAV file (title, artist, etc.)
        @param qualityOptionIndex  Ignored for WAV format (uncompressed)
        @returns A WaveAudioFormatWriter if the parameters are valid and supported,
                nullptr if the configuration is invalid
    */
    std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* streamToWriteTo,
                                                        double sampleRate,
                                                        int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    /** Returns the bit depths supported by this WAV format implementation.
        
        WAV format supports multiple bit depths, from basic 8-bit samples up to
        high-resolution 32-bit samples for professional audio applications.
        
        @returns An array containing {8, 16, 24, 32} representing the supported
                bit depths in bits per sample
    */
    Array<int> getPossibleBitDepths() const override;

    /** Returns the sample rates supported by this WAV format implementation.
        
        WAV format supports a wide range of sample rates to accommodate different
        audio quality requirements and application domains.
        
        @returns An array of supported sample rates in Hz, ranging from 8000 Hz
                (telephone quality) up to 192000 Hz (high-resolution audio)
    */
    Array<int> getPossibleSampleRates() const override;

    /** Returns true indicating that this format supports mono audio files.
        
        WAV format fully supports single-channel (mono) audio recording and playback.
        
        @returns Always true - WAV format supports mono audio
    */
    bool canDoMono() const override { return true; }

    /** Returns true indicating that this format supports stereo audio files.
        
        WAV format fully supports two-channel (stereo) audio recording and playback.
        
        @returns Always true - WAV format supports stereo audio  
    */
    bool canDoStereo() const override { return true; }

private:
    String formatName;
};

} // namespace yup