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
    AudioFormat implementation for Ogg Vorbis audio files.

    This format provides read and write support for .ogg files containing Vorbis
    audio streams. Decoding and encoding are handled through libvorbis, with
    samples exposed as floating point values.

    @see AudioFormat, AudioFormatReader, AudioFormatWriter

    @tags{Audio}
*/
class YUP_API OggVorbisAudioFormat : public AudioFormat
{
public:
    /** Constructs a new OggVorbisAudioFormat instance. */
    OggVorbisAudioFormat();

    /** Destructor. */
    ~OggVorbisAudioFormat() override;

    /** Returns the descriptive name of this format. */
    const String& getFormatName() const override;

    /** Returns the file extensions that this format can handle. */
    Array<String> getFileExtensions (Mode handleMode) const override;

    /** Creates a reader for decoding Ogg Vorbis audio data from the provided stream. */
    std::unique_ptr<AudioFormatReader> createReaderFor (InputStream* sourceStream) override;

    /** Creates a writer for encoding audio data to Ogg Vorbis format. */
    std::unique_ptr<AudioFormatWriter> createWriterFor (OutputStream* streamToWriteTo,
                                                        double sampleRate,
                                                        int numberOfChannels,
                                                        int bitsPerSample,
                                                        const StringPairArray& metadataValues,
                                                        int qualityOptionIndex) override;

    /** Returns the bit depths supported by this Ogg Vorbis format implementation. */
    Array<int> getPossibleBitDepths() const override;

    /** Returns the sample rates supported by this Ogg Vorbis format implementation. */
    Array<int> getPossibleSampleRates() const override;

    /** Indicates whether this format can handle mono audio. */
    bool canDoMono() const override { return true; }

    /** Indicates whether this format can handle stereo audio. */
    bool canDoStereo() const override { return true; }

    /** Indicates that this format is compressed. */
    bool isCompressed() const override { return true; }

    /** Returns the available quality options for Vorbis encoding. */
    StringArray getQualityOptions() const override;

private:
    String formatName;
};

} // namespace yup
