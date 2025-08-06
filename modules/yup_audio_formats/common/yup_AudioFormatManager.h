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

/** A class that manages audio formats. */
class YUP_API AudioFormatManager
{
public:
    /** Constructor. */
    AudioFormatManager();

    /** Register the default formats. */
    void registerDefaultFormats();

    /** Register a format.

        @param format The format to register.
    */
    void registerFormat (std::unique_ptr<AudioFormat> format);

    /** Create a reader for a file.

        @param file The file to create a reader for.

        @return A pointer to the reader, or nullptr if the format cannot handle the file.
    */
    std::unique_ptr<AudioFormatReader> createReaderFor (const File& file);

    /** Create a writer for a file.

        @param file The file to create a writer for.
        @param sampleRate The sample rate to use.
        @param numChannels The number of channels to use.
        @param bitsPerSample The number of bits per sample to use.

        @return A pointer to the writer, or nullptr if the format cannot handle the file.
    */
    std::unique_ptr<AudioFormatWriter> createWriterFor (const File& file,
                                                        int sampleRate,
                                                        int numChannels,
                                                        int bitsPerSample);

private:
    std::vector<std::unique_ptr<AudioFormat>> formats;
};

} // namespace yup
