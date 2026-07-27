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
    Base class for all audio domains processors.

    The AudioProcessorBase class is the base class for all audio processing domain modules
    in the framework. It provides a common interface for processing parameters, handling
    presets, dealing with latency and tails, suspending and resuming processing, and
    communicating with hosts.

    @see AudioProcessorEditor
*/
class YUP_API AudioProcessorBase
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

        /** Returns a copy of these details with the program change flag set. */
        ChangeDetails withProgramChanged (bool shouldBeProgramChanged) const noexcept
        {
            auto copy = *this;
            copy.programChanged = shouldBeProgramChanged;
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

        /** True when the program/preset has changed. */
        bool programChanged = false;
    };

    //==============================================================================
    /** Receives processor-level change notifications. */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when a processor-level property changes. */
        virtual void audioProcessorChanged (AudioProcessorBase* processor, const ChangeDetails& details) = 0;
    };

    //==============================================================================
    /** Constructs an AudioProcessorBase. */
    AudioProcessorBase (StringRef name);

    /** Destructs an AudioProcessorBase. */
    virtual ~AudioProcessorBase();

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
    /** Returns the critical section used to protect the audio processing code. */
    CriticalSection& getProcessLock() { return processLock; }

    /** Returns true if the processor is currently suspended. */
    bool isSuspended() const;

    /** Suspends or resumes the processor. */
    virtual void suspendProcessing (bool shouldSuspend);

    /** RAII helper class to automatically suspend and resume processing.

        @code
        {
            AudioProcessorBase::ScopedProcessSuspension suspension (*this);
            // processing is suspended within this scope
        }
        // processing is resumed here
    */
    struct ScopedProcessSuspension
    {
        /** Constructs a ScopedProcessSuspension and suspends processing. */
        explicit ScopedProcessSuspension (AudioProcessorBase& processor, bool startSuspended = true)
            : processor (processor)
        {
            if (startSuspended)
                setSuspended();
        }

        /** Destructs a ScopedProcessSuspension and resumes processing. */
        ~ScopedProcessSuspension()
        {
            if (wasSuspended)
                processor.suspendProcessing (false);
        }

        /** Manually suspends processing if it wasn't already suspended in the constructor. */
        void setSuspended()
        {
            if (wasSuspended)
                return;

            processor.suspendProcessing (true);
            wasSuspended = true;
        }

    private:
        AudioProcessorBase& processor;
        bool wasSuspended = false;
    };

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
    /** Returns true when this processor supports structured DataTree state.

        Processors that return true can use the default binary state transport,
        which serializes the DataTree state as XML into a MemoryBlock.
    */
    virtual bool supportsDataTreeState() const noexcept { return false; }

    /** Loads a preset from a structured DataTree.

        The default implementation returns a failure. Override this together with
        saveStateIntoDataTree() and supportsDataTreeState() for processors that
        want XML-readable state while still using MemoryBlock transport in plugin
        wrappers.

        @param state The structured state to load.

        @return The result of the operation.
    */
    virtual Result loadStateFromDataTree (const DataTree& state);

    /** Saves the current state into a structured DataTree.

        The implementation should assign a valid DataTree to @p state.

        @param state The structured state destination.

        @return The result of the operation.
    */
    virtual Result saveStateIntoDataTree (DataTree& state);

    /** Loads a preset from a memory block.

        The default implementation is available to processors that return true
        from supportsDataTreeState(): it parses XML from the memory block and
        forwards the resulting DataTree to loadStateFromDataTree(). Processors
        that need opaque binary state should override this method.

        @param memoryBlock The memory block to load the state from.

        @return The result of the operation.
    */
    virtual Result loadStateFromMemory (const MemoryBlock& memoryBlock);

    /** Saves the current state as a memory block.

        The default implementation is available to processors that return true
        from supportsDataTreeState(): it calls saveStateIntoDataTree() and writes
        the resulting XML into the memory block. Processors that need opaque
        binary state should override this method.

        @param memoryBlock The memory block to save the state to.

        @return The result of the operation.
    */
    virtual Result saveStateIntoMemory (MemoryBlock& memoryBlock);

    //==============================================================================
    /** @internal Used by plugin wrappers. */
    virtual void setPlaybackConfiguration (float sampleRate, int samplesPerBlock);

private:
    String processorName;

    std::vector<AudioParameter::Ptr> parameters;
    std::unordered_map<String, AudioParameter::Ptr> parameterMap;
    std::unordered_map<uint32, AudioParameter::Ptr> parameterHostIDMap;
    ListenerList<Listener, Array<Listener*, CriticalSection>> listeners;

    float sampleRate = 44100.0f;
    int samplesPerBlock = 1024;
    std::atomic<int> latencySamples { 0 };

    CriticalSection processLock;
    std::atomic<int> processIsSuspended { 0 };
    std::atomic<bool> offlineProcessing { false };
};

} // namespace yup
