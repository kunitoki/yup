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

    The AudioProcessor class is the base class for all audio processing modules in the framework.
    It provides a common interface for processing audio and MIDI data, managing parameters, and
    communicating with hosts.

    @see AudioProcessorEditor
*/
class YUP_API AudioProcessor
{
public:
    //==============================================================================
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

        /** Returns a copy of these details with the tail length change flag set. */
        ChangeDetails withTailChanged (bool shouldBeTailChanged) const noexcept
        {
            auto copy = *this;
            copy.tailChanged = shouldBeTailChanged;
            return copy;
        }

        /** Returns a copy of these details with the parameter value change flag set. */
        ChangeDetails withParameterValuesChanged (bool shouldBeParameterValuesChanged) const noexcept
        {
            auto copy = *this;
            copy.parameterValuesChanged = shouldBeParameterValuesChanged;
            return copy;
        }

        /** Returns a copy of these details with the parameter metadata change flag set. */
        ChangeDetails withParameterInfoChanged (bool shouldBeParameterInfoChanged) const noexcept
        {
            auto copy = *this;
            copy.parameterInfoChanged = shouldBeParameterInfoChanged;
            return copy;
        }

        /** Returns a copy of these details with the non-parameter state change flag set. */
        ChangeDetails withNonParameterStateChanged (bool shouldBeNonParameterStateChanged) const noexcept
        {
            auto copy = *this;
            copy.nonParameterStateChanged = shouldBeNonParameterStateChanged;
            return copy;
        }

        /** True when the processor latency may have changed. */
        bool latencyChanged = false;

        /** True when the processor tail length may have changed. */
        bool tailChanged = false;

        /** True when one or more parameter values may have changed without a host automation event. */
        bool parameterValuesChanged = false;

        /** True when one or more parameter names, ranges, or display conversions may have changed. */
        bool parameterInfoChanged = false;

        /** True when non-parameter processor state may have changed. */
        bool nonParameterStateChanged = false;
    };

    //==============================================================================
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

    /** Returns a parameter by host-facing automation ID, or nullptr when no such parameter exists. */
    AudioParameter::Ptr getParameterByHostID (uint32 hostParameterID) const;

    /** Returns a parameter index by host-facing automation ID, or -1 when no such parameter exists. */
    int getParameterIndexByHostID (uint32 hostParameterID) const;

    /** Adds a parameter. */
    void addParameter (AudioParameter::Ptr parameter);

    //==============================================================================
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

    // TODO - add support for custom midi inputs and outputs

    //==============================================================================
    /** Prepares the processor for playback.

        getSampleRate() and getSamplesPerBlock() are guaranteed to return the correct
        values when this is called. Subclasses do not need to call the base class.
    */
    virtual void prepareToPlay (float sampleRate, int maxBlockSize) = 0;

    /** Releases resources. */
    virtual void releaseResources() = 0;

    /**
        Primary single-precision processing entry point.

        Override this to process a block of audio and MIDI. The context provides
        sample-accurate parameter automation via @c context.params and the transport
        state via @c context.playHead when available.

        The base-class implementation asserts false so unoverridden processors are
        caught at runtime in debug builds.

        @param context  All per-block inputs: audio, MIDI, parameter changes, and position.
    */
    virtual void processBlock (AudioProcessContext<float>& context) = 0;

    /**
        Double-precision processing entry point.

        Override this and return true from supportsDoublePrecisionProcessing() to
        support 64-bit audio. The default implementation does nothing.

        @param context  All per-block inputs with double-precision audio.
    */
    virtual void processBlock (AudioProcessContext<double>& context) { ignoreUnused (context); }

    /**
        Called by plugin wrappers when the processor is bypassed (single-precision).

        The default implementation routes inputs to outputs, or clears extra outputs.

        @param context  All per-block inputs.
    */
    virtual void processBlockBypassed (AudioProcessContext<float>& context) { ignoreUnused (context); }

    /**
        Called by plugin wrappers when the processor is bypassed (double-precision).

        The default implementation routes inputs to outputs, or clears extra outputs.

        @param context  All per-block inputs.
    */
    virtual void processBlockBypassed (AudioProcessContext<double>& context) { ignoreUnused (context); }

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
    /** Returns the critical section used to protect the audio processing code. */
    CriticalSection& getProcessLock() { return processLock; }

    /** Returns true if the processor is currently suspended. */
    bool isSuspended() const;

    /** Suspends or resumes the processor. */
    virtual void suspendProcessing (bool shouldSuspend);

    //==============================================================================
    /** Returns the current sample rate. */
    float getSampleRate() const { return sampleRate; }

    /** Returns the current block size in samples. */
    int getSamplesPerBlock() const { return samplesPerBlock; }

    //==============================================================================
    /** Returns the number of tail samples. */
    virtual int getTailSamples() { return 0; }

    /** Returns the latency in samples. */
    virtual int getLatencySamples() { return latencySamples.load(); }

    /** Sets the processor latency in samples and notifies listeners when it changes. */
    void setLatencySamples (int newLatencySamples);

    /** Returns the number of simultaneous voices this processor can produce.
        Returns 0 for effects and MIDI-only processors. Override in instruments. */
    virtual int getNumVoices() const { return 0; }

    //==============================================================================
    /** Returns true when the processor is running in offline (non-realtime) mode. */
    bool isOfflineProcessing() const noexcept { return offlineProcessing.load(); }

    /** Called by the plugin wrapper to indicate offline vs. realtime rendering. */
    void setOfflineProcessing (bool offline) { offlineProcessing.store (offline); }

    //==============================================================================
    /** Returns the current preset index. */
    virtual int getCurrentPreset() const noexcept = 0;

    /** Sets the current preset index.

        @param index The index of the preset to select.
    */
    virtual void setCurrentPreset (int index) noexcept = 0;

    /** Returns the number of available user presets. */
    virtual int getNumPresets() const = 0;

    /** Returns the name of a preset by index.
      
        @param index The index of the preset.

        @return The name of the preset.
    */
    virtual String getPresetName (int index) const = 0;

    /** Sets the name of a preset by index.

        @param index The index of the preset.
        @param newName The new name for the preset.
    */
    virtual void setPresetName (int index, StringRef newName) = 0;

    //==============================================================================
    /** Loads a preset from a memory block.

        @param memoryBlock The memory block to load the state from.
        @return The result of the operation.
    */
    virtual Result loadStateFromMemory (const MemoryBlock& memoryBlock) = 0;

    /** Saves the current state as a memory block.

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
    std::unordered_map<uint32, AudioParameter::Ptr> parameterHostIDMap;
    ListenerList<Listener, Array<Listener*, CriticalSection>> listeners;

    AudioBusLayout busLayout;

    float sampleRate = 44100.0f;
    int samplesPerBlock = 1024;
    std::atomic<int> latencySamples { 0 };
    ProcessingPrecision processingPrecision = ProcessingPrecision::singlePrecision;

    CriticalSection processLock;
    std::atomic<bool> processIsSuspended { false };
    std::atomic<bool> offlineProcessing { false };
};

} // namespace yup
