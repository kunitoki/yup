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
    AudioFormat implementation for MP3 audio files.

    Mp3AudioFormat provides comprehensive support for the MP3 (MPEG-1 Audio Layer III)
    audio format, utilizing the high-performance dr_mp3 library for low-level audio
    data processing. This implementation handles the complexities of the MP3 format
    specification while presenting a clean, easy-to-use interface through the AudioFormat API.

    Supported MP3 features:
    - Multiple bitrates: Variable bitrate (VBR) and constant bitrate (CBR) support
    - Various sample rates: 8kHz to 48kHz (MPEG-1 Layer III)
    - Channel configurations: Mono and stereo
    - Metadata support: ID3v1, ID3v2, APE, Xing, and VBRI tags
    - Seeking support: Both brute-force and seek table-based seeking
    - Frame-level access: Direct access to MP3 frames for advanced applications

    The implementation automatically detects and handles different MP3 variants and
    encoding types, converting all audio data to normalized floating-point samples
    for consistent processing. Special attention has been paid to proper handling
    of delay and padding samples from LAME-encoded files to ensure accurate audio
    reproduction.

    This format is compressed and supports efficient storage with good audio quality,
    making it ideal for applications where file size is a concern while maintaining
    acceptable audio fidelity.

    @see AudioFormat, AudioFormatReader, AudioFormatWriter

    @tags{Audio}
*/
class YUP_API Mp3AudioFormat : public AudioFormat
{
public:
    /** Constructs a new Mp3AudioFormat instance.

        Initializes the format handler with default settings for MP3 file processing.
        The instance is ready to create readers and writers for MP3 files immediately
        after construction.
    */
    Mp3AudioFormat();

    /** Destructor.

        Cleans up any resources used by this format instance. All created readers
        and writers continue to function independently after the format is destroyed.
    */
    ~Mp3AudioFormat() override;

    /** Returns the descriptive name of this format.

        @returns The string "MP3 file" identifying this as an MP3 format handler
    */
    const String& getFormatName() const override;

    /** Returns the file extensions that this format can handle.

        MP3 files typically use the .mp3 extension, though other extensions may be
        supported depending on the application.

        @returns An array containing the supported extensions: ".mp3"
    */
    Array<String> getFileExtensions() const override;

    /** Creates a reader for decoding MP3 audio data from the provided stream.

        This method attempts to parse the MP3 header and create an appropriate reader
        for the specific MP3 variant detected. The reader will handle format-specific
        decoding including VBR/CBR detection, metadata parsing, and sample rate conversion.

        @param sourceStream The input stream containing MP3 audio data. The format
                            takes ownership of this stream if successful.
        @returns A Mp3AudioFormatReader if the stream contains valid MP3 data,
                nullptr if the stream cannot be parsed as an MP3 file
    */
    std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a writer for encoding audio data to MP3 format.

        This method creates an MP3 writer configured for the specified audio parameters.
        Encoding is provided by the Helix MP3 encoder when the hmp3_library module
        is available.

        @param streamToWriteTo     The output stream where MP3 data will be written
        @param sampleRate          The sample rate in Hz (supports 8kHz to 48kHz)
        @param numberOfChannels    The number of audio channels (1-2 channels supported)
        @param bitsPerSample       The bit depth (ignored for MP3, always 16-bit output)
        @param metadataValues      Metadata to embed in the MP3 file (title, artist, etc.)
        @param qualityOptionIndex  Quality setting (0-100, where 100 is highest quality)
        @returns A writer when MP3 encoding is available, otherwise nullptr
    */
    std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* streamToWriteTo,
                                                        double sampleRate,
                                                        int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    /** Returns the bit depths supported by this MP3 format implementation.

        MP3 format supports 16-bit samples for the decoded output.

        @returns An array containing {16} representing the supported
                bit depth in bits per sample
    */
    Array<int> getPossibleBitDepths() const override;

    /** Returns the sample rates supported by this MP3 format implementation.

        MP3 format supports a wide range of sample rates for MPEG-1 Layer III.

        @returns An array of supported sample rates in Hz, ranging from 8000 Hz
                up to 48000 Hz
    */
    Array<int> getPossibleSampleRates() const override;

    /** Returns true indicating that this format supports mono audio files.

        MP3 format fully supports single-channel (mono) audio recording and playback.

        @returns Always true - MP3 format supports mono audio
    */
    bool canDoMono() const override { return true; }

    /** Returns true indicating that this format supports stereo audio files.

        MP3 format fully supports two-channel (stereo) audio recording and playback.

        @returns Always true - MP3 format supports stereo audio
    */
    bool canDoStereo() const override { return true; }

    /** Returns true indicating that this format is compressed.

        MP3 is a lossy compressed audio format.

        @returns Always true - MP3 format is compressed
    */
    bool isCompressed() const override { return true; }

private:
    String formatName;
};

} // namespace yup
