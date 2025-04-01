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

    /** Returns the name of the processor. */
    String getName() const { return processorName; }

    //==============================================================================

    /** Returns the parameters. */
    Span<const AudioParameter::Ptr> getParameters() const { return parameters; }

    /** Adds a parameter. */
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

    /**
        Processes a block of audio.

        @param audioBuffer The audio buffer to process.
        @param midiBuffer The MIDI buffer to process.
    */
    virtual void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer) = 0;

    /**
        Processes a block of audio.

        @param audioBuffer The audio buffer to process.
        @param midiBuffer The MIDI buffer to process.
    */
    virtual void processBlock (AudioBuffer<double>& audioBuffer, MidiBuffer& midiBuffer) {}

    /** Flushes the processor. */
    virtual void flush() {}

    //==============================================================================

    CriticalSection& getProcessLock() { return processLock; }

    void suspendProcessing (bool shouldSuspend);

    bool isSuspended() const;

    //==============================================================================

    /**
        Returns the current preset index.
    */
    virtual int getCurrentPreset() const noexcept = 0;

    /**
        Sets the current preset index.
    */
    virtual void setCurrentPreset (int index) noexcept = 0;

    /**
        Returns the number of available user presets.
    */
    virtual int getNumPresets() const = 0;

    /**
        Returns the name of a preset by index.
    */
    virtual String getPresetName (int index) const = 0;

    /**
        Returns the name of a preset by index.
    */
    virtual void setPresetName (int index, StringRef newName) = 0;

    //==============================================================================

    /**
        Loads a preset from a memory block.

        @param memoryBlock The memory block to load the state from.
        @return The result of the operation.
    */
    virtual Result loadStateFromMemory (const MemoryBlock& memoryBlock) = 0;

    /**
        Saves the current state as a memory block.

        @param memoryBlock The memory block to save the state to.
        @return The result of the operation.
    */
    virtual Result saveStateIntoMemory (MemoryBlock& memoryBlock) = 0;

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

    CriticalSection processLock;
    bool processIsSuspended = false;
};

} // namespace yup
