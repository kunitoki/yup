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

class AudioProcessorEditor;

//==============================================================================
/**
    Base class for all audio processors.

    @see AudioProcessorEditor
*/
class JUCE_API AudioProcessor
{
public:
    //==============================================================================

    /** Constructs an AudioProcessor. */
    AudioProcessor (StringRef name, AudioBusLayout busLayout);

    /** Destructs an AudioProcessor. */
    virtual ~AudioProcessor();

    //==============================================================================

    Span<const AudioParameter::Ptr> getParameters() const { return parameters; }

    void addParameter (AudioParameter::Ptr parameter);

    //==============================================================================

    /** Returns the bus layout. */
    const AudioBusLayout& getBusLayout() const noexcept { return busLayout; }

    /** Returns the number of audio outputs. */
    int getNumAudioOutputs() const;

    /** Returns the number of audio inputs. */
    int getNumAudioInputs() const;

    //==============================================================================

    /** Prepares the processor for playback. */
    virtual void prepareToPlay (float sampleRate, int maxBlockSize) = 0;

    /** Releases resources. */
    virtual void releaseResources() = 0;

    /** Processes a block of audio. */
    virtual void processBlock (AudioSampleBuffer& audioBuffer, MidiBuffer& midiBuffer) = 0;

    /** Flushes the processor. */
    virtual void flush() {}

    //==============================================================================

    /** Returns true if the processor has an editor. */
    virtual bool hasEditor() const = 0;

    /** Creates an editor for the processor. */
    virtual AudioProcessorEditor* createEditor() { return nullptr; }

private:
    String processorName;
    std::vector<AudioParameter::Ptr> parameters;
    std::unordered_map<String, AudioParameter::Ptr> parameterMap;
    AudioBusLayout busLayout;
};

} // namespace yup
