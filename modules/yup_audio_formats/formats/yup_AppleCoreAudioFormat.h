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
    AudioFormat implementation backed by Apple's CoreAudio decoding pipeline.

    This reader uses ExtAudioFile to decode formats supported by CoreAudio (e.g.
    m4a, aac, mp3, mp2) and exposes audio as floating-point samples. AAC encoding
    is supported for .m4a and .aac outputs.

    @see AudioFormat, AudioFormatReader, AudioFormatWriter

    @tags{Audio}
*/
class YUP_API AppleCoreAudioFormat : public AudioFormat
{
public:
    /** Constructs a new AppleCoreAudioFormat instance. */
    AppleCoreAudioFormat();

    /** Destructor. */
    ~AppleCoreAudioFormat() override;

    /** Returns the descriptive name of this format. */
    const String& getFormatName() const override;

    /** Returns the file extensions that this format can handle. */
    Array<String> getFileExtensions() const override;

    /** Creates a reader for decoding CoreAudio-supported audio data. */
    std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a writer for encoding audio data.

        CoreAudio writing is not currently implemented.
    */
    std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* streamToWriteTo,
                                                        double sampleRate,
                                                        int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    /** Returns the bit depths supported by this format implementation. */
    Array<int> getPossibleBitDepths() const override;

    /** Returns the sample rates supported by this format implementation. */
    Array<int> getPossibleSampleRates() const override;

    /** Returns true indicating that this format supports mono audio files. */
    bool canDoMono() const override { return true; }

    /** Returns true indicating that this format supports stereo audio files. */
    bool canDoStereo() const override { return true; }

    /** Returns true indicating that this format is compressed. */
    bool isCompressed() const override { return true; }

private:
    String formatName;
};

} // namespace yup
