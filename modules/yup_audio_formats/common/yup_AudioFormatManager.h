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
    Central registry and factory for audio format handlers.

    AudioFormatManager serves as the primary entry point for working with multiple audio
    file formats in a unified way. It maintains a collection of registered AudioFormat
    implementations and provides convenient methods for creating appropriate readers
    and writers based on file extensions or format requirements.

    Key responsibilities:
    - Registry of available audio format implementations
    - Format detection based on file extensions
    - Automatic creation of format-specific readers and writers
    - Centralized management of format capabilities and limitations
    - Support for both built-in and custom audio format plugins

    The manager simplifies audio I/O operations by abstracting away the complexities
    of format-specific handling. Applications typically register the formats they need
    (often using registerDefaultFormats() for common formats) and then use the
    convenience methods to create readers and writers without needing to know the
    specific format implementation details.

    Example usage:
    @code
    AudioFormatManager manager;
    manager.registerDefaultFormats();

    auto reader = manager.createReaderFor(audioFile);
    if (reader != nullptr)
    {
        // Read audio data using the format-appropriate reader
    }
    @endcode

    @see AudioFormat, AudioFormatReader, AudioFormatWriter

    @tags{Audio}
*/
class YUP_API AudioFormatManager
{
public:
    //==============================================================================
    /** Constructs an empty AudioFormatManager with no registered formats.

        After construction, you'll typically want to call registerDefaultFormats()
        or manually register specific formats using registerFormat().
    */
    AudioFormatManager();

    //==============================================================================
    /** Registers all built-in audio format implementations.

        This convenience method automatically registers the standard audio formats
        that are included with the YUP library, such as WAV, potentially FLAC,
        and other commonly-used formats. This is the most common way to initialize
        the manager for typical use cases.

        The specific formats registered may depend on compile-time configuration
        and available dependencies.
    */
    void registerDefaultFormats();

    /** Registers a custom audio format implementation.

        This method allows you to add support for additional audio formats beyond
        the built-in ones. The manager takes ownership of the provided format object
        and will use it for format detection and reader/writer creation.

        @param format A unique pointer to the AudioFormat implementation to register.
                      The manager takes ownership of this object.
    */
    void registerFormat (std::unique_ptr<AudioFormat> format);

    //==============================================================================
    /** Creates an appropriate reader for the specified audio file.

        This method examines the file's extension to determine which registered format
        should handle it, then attempts to create a reader for that format. The file
        is opened and its header is parsed to extract audio properties.

        @param file The audio file to create a reader for. The file must exist and
                    be readable.

        @returns A unique pointer to an AudioFormatReader if a compatible format
                 was found and the file could be parsed successfully, nullptr otherwise.
    */
    std::unique_ptr<AudioFormatReader> createReaderFor (const File& file);

    //==============================================================================
    /** Creates an appropriate writer for the specified audio file with given parameters.

        This method determines which registered format should handle the file based on
        its extension, then creates a writer configured with the specified audio parameters.
        The format's capabilities are validated against the requested parameters.

        @param file The destination file where audio data will be written. Parent
                    directories must exist and be writable.
        @param sampleRate The sample rate for the output audio in Hz (e.g., 44100, 48000).
        @param numChannels The number of audio channels (1 for mono, 2 for stereo, etc.).
        @param bitsPerSample The bit depth for sample encoding (e.g., 16, 24, 32).

        @returns A unique pointer to an AudioFormatWriter if a compatible format was found
                 and supports the specified parameters, nullptr if no suitable format is
                 available or the parameters are not supported.
    */
    std::unique_ptr<AudioFormatWriter> createWriterFor (const File& file,
                                                        int sampleRate,
                                                        int numChannels,
                                                        int bitsPerSample);

private:
    std::vector<std::unique_ptr<AudioFormat>> formats;
};

} // namespace yup
