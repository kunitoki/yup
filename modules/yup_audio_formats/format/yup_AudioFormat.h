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

class AudioFormatReader;
class AudioFormatWriter;

//==============================================================================
/**
    Abstract base class for audio format implementations.

    This class serves as the foundation for all audio file format handlers within
    the YUP library. Each concrete implementation represents a specific audio file
    format (such as WAV, FLAC, or MP3) and provides the necessary functionality to
    create reader and writer objects for parsing and writing files in that particular
    format.

    The AudioFormat class defines a common interface for:
    - Identifying supported file extensions
    - Creating format-specific readers and writers
    - Querying format capabilities (sample rates, bit depths, channel configurations)
    - Handling format-specific metadata and quality settings

    Subclasses must implement all pure virtual methods to provide format-specific
    behavior. The AudioFormatManager typically manages instances of AudioFormat
    subclasses to provide a unified interface for handling multiple audio formats
    in an application.

    @see AudioFormatReader, AudioFormatWriter, AudioFormatManager

    @tags{Audio}
*/
class YUP_API AudioFormat
{
public:
    /** Destructor. */
    virtual ~AudioFormat() = default;

    /** Returns the descriptive name of this audio format.

        @returns A string containing the human-readable name of the format (e.g., "Wave file", "FLAC Audio")
    */
    virtual const String& getFormatName() const = 0;

    /** Returns the file extensions associated with this format.

        @returns An array of file extensions (including the dot) that this format can handle
                 (e.g., {".wav", ".wave"} for WAV format)
    */
    virtual Array<String> getFileExtensions() const = 0;

    /** Tests whether this format can handle files with the given file extension.

        This method provides a convenient way to check if a file can be processed by this format
        based on its extension, without needing to attempt to open the file.

        @param file The file to test for compatibility

        @returns true if this format can potentially handle the file, false otherwise
    */
    virtual bool canHandleFile (const File& file) const;

    /** Creates a reader object capable of parsing audio data from the given stream.

        This method attempts to create a format-specific reader for the provided input stream.
        The reader will be configured with the appropriate parameters extracted from the stream's
        audio data (sample rate, channels, bit depth, etc.).

        @param sourceStream The input stream containing audio data to be read. The AudioFormat
                            takes ownership of this stream if successful.

        @returns A unique pointer to an AudioFormatReader if successful, nullptr if the stream
                 cannot be parsed by this format
    */
    virtual std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* sourceStream) = 0;

    /** Creates a writer object capable of writing audio data to the given stream.

        This method creates a format-specific writer configured with the specified audio parameters.
        The writer will encode audio data according to the format's specifications and write it
        to the provided output stream.

        @param streamToWriteTo     The output stream where audio data will be written
        @param sampleRate          The sample rate of the audio data (e.g., 44100, 48000)
        @param numberOfChannels    The number of audio channels (1 for mono, 2 for stereo, etc.)
        @param bitsPerSample       The bit depth for each sample (e.g., 16, 24, 32)
        @param metadataValues      A collection of metadata key-value pairs to embed in the file
        @param qualityOptionIndex  Index into the quality options array for compressed formats

        @returns A unique pointer to an AudioFormatWriter if successful, nullptr if the
                 parameters are not supported by this format
    */
    virtual std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* streamToWriteTo,
                                                                double sampleRate,
                                                                int numberOfChannels,
                                                                int bitsPerSample,
                                                                const StringPairArray& metadataValues,
                                                                int qualityOptionIndex) = 0;

    /** Returns the set of bit depths that this format supports for writing.

        Different audio formats support different bit depths. This method allows clients
        to query which bit depths are available before attempting to create a writer.

        @returns An array of supported bit depths in bits per sample (e.g., {8, 16, 24, 32})
    */
    virtual Array<int> getPossibleBitDepths() const = 0;

    /** Returns the set of sample rates that this format supports for writing.

        Audio formats may have limitations on supported sample rates. This method provides
        a way to discover these limitations before attempting to create a writer.

        @returns An array of supported sample rates in Hz (e.g., {44100, 48000, 96000})
    */
    virtual Array<int> getPossibleSampleRates() const = 0;

    /** Returns true if this format supports writing mono (single-channel) audio files.

        @returns true if mono files can be written, false otherwise
    */
    virtual bool canDoMono() const = 0;

    /** Returns true if this format supports writing stereo (two-channel) audio files.

        @returns true if stereo files can be written, false otherwise
    */
    virtual bool canDoStereo() const = 0;

    /** Returns true if this format supports compression with variable quality settings.

        Formats like MP3, OGG Vorbis, and FLAC support different compression levels or quality
        settings. Uncompressed formats like WAV typically return false.

        @returns true if the format supports quality options, false for uncompressed formats
    */
    virtual bool isCompressed() const { return false; }

    /** Returns a list of quality option descriptions for compressed formats.

        For compressed formats that support multiple quality levels, this method returns
        human-readable descriptions of the available quality options. The index of the
        desired quality can be passed to createWriterFor().

        @returns An array of quality descriptions (e.g., {"Low", "Medium", "High"}) or
                 empty array for formats that don't support quality options
    */
    virtual StringArray getQualityOptions() const { return {}; }
};

} // namespace yup