/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
    Base class for all spectral processors.

    The SpectralProcessor class is the base class for all spectral processing modules in the framework.
    It provides a common interface for processing spectral bins, managing parameters, and
    communicating with hosts.

    @see AudioProcessorEditor
*/
class YUP_API SpectralProcessor : public DomainProcessor<SpectralProcessContext, SpectralSpec>
{
    using BaseDomainProcessor = DomainProcessor<SpectralProcessContext, SpectralSpec>;

public:
    //==============================================================================
    /** Constructs a SpectralProcessor. */
    SpectralProcessor (StringRef name, AudioBusLayout busLayout);

    /** Destructs a SpectralProcessor. */
    ~SpectralProcessor() override;

    //==============================================================================
    /** Returns the bus layout. */
    const AudioBusLayout& getBusLayout() const noexcept { return busLayout; }

    /** Returns the number of audio outputs. */
    int getNumAudioOutputs() const;

    /** Returns the number of audio inputs. */
    int getNumAudioInputs() const;

    //==============================================================================
    /** Returns true if the processor accepts MIDI input. */
    virtual bool acceptsMidi() const noexcept;

    /** Returns true if the processor produces MIDI output. */
    virtual bool producesMidi() const noexcept;

    //==============================================================================
    /** @internal Used by plugin wrappers. */
    void setPlaybackConfiguration (float sampleRate, int samplesPerBlock) override;

private:
    AudioBusLayout busLayout;
};

} // namespace yup
