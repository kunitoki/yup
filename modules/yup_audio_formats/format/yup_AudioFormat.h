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
    Base class for audio format descriptions.

    This class represents a type of audio file format, and can create
    reader and writer objects for parsing and writing that format.
*/
class YUP_API AudioFormat
{
public:
    /** Destructor. */
    virtual ~AudioFormat() = default;

    /** Returns the name of this format. */
    virtual const String& getFormatName() const = 0;

    /** Returns the file extensions associated with this format. */
    virtual Array<String> getFileExtensions() const = 0;

    /** Tests whether this format can handle the given file extension. */
    virtual bool canHandleFile (const File& file) const;

    /** Creates a reader for this format. */
    virtual std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* sourceStream) = 0;

    /** Creates a writer for this format. */
    virtual std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* streamToWriteTo,
                                                                double sampleRate,
                                                                int numberOfChannels,
                                                                int bitsPerSample,
                                                                const StringPairArray& metadataValues,
                                                                int qualityOptionIndex) = 0;

    /** Returns a set of bit depths that the format can write. */
    virtual Array<int> getPossibleBitDepths() const = 0;

    /** Returns a set of sample rates that the format can write. */
    virtual Array<int> getPossibleSampleRates() const = 0;

    /** Returns true if this format can write files with multiple channels. */
    virtual bool canDoMono() const = 0;

    /** Returns true if this format can write stereo files. */
    virtual bool canDoStereo() const = 0;

    /** Returns true if the format can encode at different qualities. */
    virtual bool isCompressed() const { return false; }

    /** Returns a list of different qualities that can be used when writing. */
    virtual StringArray getQualityOptions() const { return {}; }
};

} // namespace yup