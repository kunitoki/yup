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
    AudioFormat implementation for Ogg Opus audio files.

    This format provides read support for .opus files encoded with the Opus codec
    in an Ogg container. Decoding is performed using libopus and all samples are
    converted to floating point.

    @see AudioFormat, AudioFormatReader, AudioFormatWriter

    @tags{Audio}
*/
class YUP_API OpusAudioFormat : public AudioFormat
{
public:
    OpusAudioFormat();
    ~OpusAudioFormat() override;

    const String& getFormatName() const override;
    Array<String> getFileExtensions() const override;

    std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* sourceStream) override;
    std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* streamToWriteTo,
                                                        double sampleRate,
                                                        int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    Array<int> getPossibleBitDepths() const override;
    Array<int> getPossibleSampleRates() const override;

    bool canDoMono() const override { return true; }

    bool canDoStereo() const override { return true; }

    bool isCompressed() const override { return true; }

private:
    String formatName;
};

} // namespace yup
