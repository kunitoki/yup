/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
    A PositionableAudioSource that reads from an AudioFormatReader.

    This class wraps an AudioFormatReader, turning it into a PositionableAudioSource
    that can be used with AudioTransportSource for playback of audio files.

    This is the simplest way to read from an audio file: create an AudioFormatReader
    for the file, wrap it in an AudioFormatReaderSource, pass it to an
    AudioTransportSource, and play.

    @see AudioFormatReader, AudioTransportSource, PositionableAudioSource

    @tags{Audio}
*/
class YUP_API AudioFormatReaderSource : public PositionableAudioSource
{
public:
    //==============================================================================
    /** Creates an AudioFormatReaderSource from an AudioFormatReader.

        @param sourceReader                     the reader to use as the source. The
                                                AudioFormatReaderSource will take ownership
                                                of this reader and delete it when no longer needed.
        @param deleteReaderWhenThisIsDeleted    if true, the sourceReader will be deleted
                                                when this object is destroyed
    */
    AudioFormatReaderSource (AudioFormatReader* sourceReader,
                             bool deleteReaderWhenThisIsDeleted);

    /** Creates an AudioFormatReaderSource from a unique_ptr.
        Takes ownership of the reader.
    */
    explicit AudioFormatReaderSource (std::unique_ptr<AudioFormatReader> sourceReader);

    /** Destructor. */
    ~AudioFormatReaderSource() override;

    //==============================================================================
    /** Returns the AudioFormatReader being used as the source. */
    AudioFormatReader* getAudioFormatReader() const noexcept { return reader; }

    //==============================================================================
    /** @internal */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    /** @internal */
    void releaseResources() override;
    /** @internal */
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;

    //==============================================================================
    /** @internal */
    void setNextReadPosition (int64 newPosition) override;
    /** @internal */
    int64 getNextReadPosition() const override;
    /** @internal */
    int64 getTotalLength() const override;
    /** @internal */
    bool isLooping() const override;
    /** @internal */
    void setLooping (bool shouldLoop) override;

private:
    //==============================================================================
    std::unique_ptr<AudioFormatReader> ownedReader;
    AudioFormatReader* reader = nullptr;
    bool deleteReader = false;
    int64 nextReadPosition = 0;
    bool looping = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFormatReaderSource)
};

} // namespace yup
