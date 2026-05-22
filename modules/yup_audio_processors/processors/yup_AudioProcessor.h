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
class YUP_API AudioProcessor
{
public:
    /** Details about a processor-level change notification. */
    struct ChangeDetails
    {
        /** Returns a copy of these details with the latency change flag set. */
        ChangeDetails withLatencyChanged (bool shouldBeLatencyChanged) const noexcept
        {
            auto copy = *this;
            copy.latencyChanged = shouldBeLatencyChanged;
            return copy;
        }

        /** True when the processor latency may have changed. */
        bool latencyChanged = false;
    };

    /** Receives processor-level change notifications. */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when a processor-level property changes. */
        virtual void audioProcessorChanged (AudioProcessor* processor, const ChangeDetails& details) = 0;
    };

    //==============================================================================
    /** The floating-point precision used for processBlock() calls. */
    enum class ProcessingPrecision
    {
        singlePrecision,
        doublePrecision
    };

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

    /** Returns a parameter by stable ID, or nullptr when no such parameter exists. */
    AudioParameter::Ptr getParameterByID (StringRef parameterID) const;

    /** Adds a parameter. */
    void addParameter (AudioParameter::Ptr parameter);

    /** Adds a processor-level change listener. */
    void addListener (Listener* listener);

    /** Removes a processor-level change listener. */
    void removeListener (Listener* listener);

    /** Notifies listeners that processor-level details have changed. */
    void updateHostDisplay (ChangeDetails details);

    //==============================================================================

    /** Returns the bus layout. */
    const AudioBusLayout& getBusLayout() const noexcept { return busLayout; }

    /** Returns the number of audio outputs. */
    int getNumAudioOutputs() const;

    /** Returns the number of audio inputs. */
    int getNumAudioInputs() const;

    //==============================================================================

    /** Prepares the processor for playback.

        getSampleRate() and getSamplesPerBlock() are guaranteed to return the correct
        values when this is called. Subclasses do not need to call the base class.
    */
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

    /**
        Processes a block while the processor is bypassed.

        The default implementation leaves audio and MIDI unchanged.

        @param audioBuffer The audio buffer to process.
        @param midiBuffer The MIDI buffer to process.
    */
    virtual void processBlockBypassed (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer);

    /**
        Processes a block while the processor is bypassed.

        The default implementation leaves audio and MIDI unchanged.

        @param audioBuffer The audio buffer to process.
        @param midiBuffer The MIDI buffer to process.
    */
    virtual void processBlockBypassed (AudioBuffer<double>& audioBuffer, MidiBuffer& midiBuffer);

    /** Flushes the processor. */
    virtual void flush() {}

    //==============================================================================

    /** Returns true if this processor implements the double-precision processBlock(). */
    virtual bool supportsDoublePrecisionProcessing() const { return false; }

    /** Sets the preferred processing precision for future processBlock() calls. */
    void setProcessingPrecision (ProcessingPrecision precision);

    /** Returns the current processing precision. */
    ProcessingPrecision getProcessingPrecision() const noexcept { return processingPrecision; }

    /** Returns true when the current processing precision is double precision. */
    bool isUsingDoublePrecision() const noexcept { return processingPrecision == ProcessingPrecision::doublePrecision; }

    //==============================================================================

    CriticalSection& getProcessLock() { return processLock; }

    bool isSuspended() const;

    virtual void suspendProcessing (bool shouldSuspend);

    //==============================================================================

    float getSampleRate() const { return sampleRate; }

    int getSamplesPerBlock() const { return samplesPerBlock; }

    //==============================================================================

    virtual int getTailSamples() { return 0; }

    virtual int getLatencySamples() { return latencySamples.load(); }

    /** Sets the processor latency in samples and notifies listeners when it changes. */
    void setLatencySamples (int newLatencySamples);

    //==============================================================================

    void setPlayHead (AudioPlayHead* playHead);

    AudioPlayHead* getPlayHead() { return playHead; }

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

    //==============================================================================

    /** @internal Used by plugin wrappers. */
    void setPlaybackConfiguration (float sampleRate, int samplesPerBlock);

private:
    String processorName;

    std::vector<AudioParameter::Ptr> parameters;
    std::unordered_map<String, AudioParameter::Ptr> parameterMap;
    ListenerList<Listener, Array<Listener*, CriticalSection>> listeners;

    AudioBusLayout busLayout;

    float sampleRate = 44100.0f;
    int samplesPerBlock = 1024;
    std::atomic<int> latencySamples { 0 };
    ProcessingPrecision processingPrecision = ProcessingPrecision::singlePrecision;

    AudioPlayHead* playHead = nullptr;

    CriticalSection processLock;
    std::atomic<bool> processIsSuspended { false };
};

} // namespace yup
