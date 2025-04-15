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

class JUCE_API AudioFormat
{
public:
    virtual ~AudioFormat() = default;

    virtual String getFormatName() const = 0;

    virtual StringArray getSupportedFileExtensions() const = 0;

    virtual bool canHandleFile (const File& filePath) const = 0;

    virtual std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* stream) = 0;

    virtual std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* stream,
                                                                int sampleRate,
                                                                int numChannels,
                                                                int bitsPerSample) = 0;
};

} // namespace yup
