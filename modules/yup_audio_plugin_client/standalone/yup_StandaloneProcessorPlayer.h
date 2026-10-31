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

#pragma once

namespace yup
{

//==============================================================================
/** Drives an AudioProcessor from an audio device in the standalone plugin wrapper.

    Device channels are clamped to the processor bus layout: missing inputs are
    fed silence and device outputs the processor doesn't have are cleared. Device
    blocks larger than the prepared block size are split into sub-blocks. MIDI
    reaching the collector, from MIDI input devices or an on-screen keyboard, is
    delivered to the processor with sample offsets relative to each sub-block.

    Nothing is allocated on the audio thread once the player is prepared.
*/
class StandaloneProcessorPlayer final : public AudioIODeviceCallback
{
public:
    //==============================================================================
    /** Creates a player.

        @param processorToPlay  the processor to drive, which must outlive the player
        @param midiOutputSource optional manager whose default MIDI output receives the
                                MIDI produced by the processor
    */
    explicit StandaloneProcessorPlayer (AudioProcessor& processorToPlay, AudioDeviceManager* midiOutputSource = nullptr)
        : processor (processorToPlay)
        , deviceManager (midiOutputSource)
    {
        // MIDI devices and the on-screen keyboard can deliver before the device starts.
        midiCollector.reset (44100.0);
    }

    //==============================================================================
    /** Returns the collector that feeds MIDI to the processor. */
    MidiMessageCollector& getMidiMessageCollector() noexcept { return midiCollector; }

    //==============================================================================
    /** Prepares the processor and every buffer for the given configuration.

        Called by audioDeviceAboutToStart(), never while the audio callback is running.
    */
    void prepare (double sampleRate, int maximumBlockSize)
    {
        release();

        preparedBlockSize = jmax (1, maximumBlockSize);
        numProcessorInputs = getTotalAudioInputChannels (processor);
        numProcessorOutputs = getTotalAudioOutputChannels (processor);

        processor.AudioProcessorBase::setPlaybackConfiguration (static_cast<float> (sampleRate), preparedBlockSize);
        processor.prepareToPlay (AudioSpec (static_cast<float> (sampleRate),
                                            preparedBlockSize,
                                            jmax (numProcessorInputs, numProcessorOutputs)));

        inputBuffer.setSize (numProcessorInputs, preparedBlockSize);
        workBuffer.setSize (jmax (1, numProcessorInputs, numProcessorOutputs), preparedBlockSize);

        inputChannels.assign (inputBuffer.getArrayOfReadPointers(),
                              inputBuffer.getArrayOfReadPointers() + inputBuffer.getNumChannels());
        workChannels.assign (workBuffer.getArrayOfWritePointers(),
                             workBuffer.getArrayOfWritePointers() + workBuffer.getNumChannels());

        buildBusViews();

        midiBuffer.ensureSize (midiStorageBytes);
        parameterChanges.reserve (getDefaultParameterChangeCapacity (processor));

        midiCollector.reset (sampleRate);
        midiCollector.ensureStorageAllocated (midiStorageBytes);

#if YUP_EMSCRIPTEN
        // Web MIDI sends wait on the main thread, which the audio worklet must never do.
        sendsMidi = false;
#else
        sendsMidi = processor.producesMidi();
#endif
        prepared = true;
    }

    /** Releases the processor resources if it was prepared. */
    void release()
    {
        if (! prepared)
            return;

        prepared = false;
        processor.releaseResources();
    }

    //==============================================================================
    /** Processes one device block with MIDI that is already positioned within it.

        The device callback drains the collector and calls this. Outputs are silenced
        while the processor is suspended or its process lock is held elsewhere.
    */
    void process (const float* const* inputChannelData,
                  int numInputChannels,
                  float* const* outputChannelData,
                  int numOutputChannels,
                  int numSamples,
                  const MidiBuffer& incomingMidi)
    {
        const auto lock = CriticalSection::ScopedTryLockType (processor.getProcessLock());

        if (! prepared || ! lock.isLocked() || processor.isSuspended())
        {
            clearOutputs (outputChannelData, numOutputChannels, 0, numSamples);
            return;
        }

        for (int start = 0; start < numSamples; start += preparedBlockSize)
        {
            processSubBlock (inputChannelData,
                             numInputChannels,
                             outputChannelData,
                             numOutputChannels,
                             start,
                             jmin (preparedBlockSize, numSamples - start),
                             incomingMidi);
        }
    }

    //==============================================================================
    /** @internal */
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const AudioIODeviceCallbackContext&) override
    {
        if (numSamples <= 0)
            return;

        // Drained once per device block: the collector places events by the time elapsed since its last call.
        deviceMidi.clear();
        midiCollector.removeNextBlockOfMessages (deviceMidi, numSamples);

        process (inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples, deviceMidi);
    }

    /** @internal */
    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        deviceMidi.ensureSize (midiStorageBytes);

        prepare (device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
    }

    /** @internal */
    void audioDeviceStopped() override
    {
        release();
    }

private:
    //==============================================================================
    void buildBusViews()
    {
        inputViews.clear();
        outputViews.clear();

        int channel = 0;
        for (const auto& bus : processor.getBusLayout().getInputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            inputViews.emplace_back (inputChannels.data() + channel, bus.getNumChannels(), bus.getRole());
            channel += bus.getNumChannels();
        }

        channel = 0;
        for (const auto& bus : processor.getBusLayout().getOutputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            outputViews.emplace_back (workChannels.data() + channel, bus.getNumChannels(), bus.getRole());
            channel += bus.getNumChannels();
        }
    }

    void processSubBlock (const float* const* inputChannelData,
                          int numInputChannels,
                          float* const* outputChannelData,
                          int numOutputChannels,
                          int start,
                          int numSamples,
                          const MidiBuffer& incomingMidi)
    {
        for (int ch = 0; ch < numProcessorInputs; ++ch)
        {
            auto* destination = inputBuffer.getWritePointer (ch);

            if (ch < numInputChannels && inputChannelData[ch] != nullptr)
                FloatVectorOperations::copy (destination, inputChannelData[ch] + start, numSamples);
            else
                FloatVectorOperations::clear (destination, numSamples);
        }

        for (int ch = 0; ch < static_cast<int> (workChannels.size()); ++ch)
        {
            if (ch < numProcessorInputs)
                FloatVectorOperations::copy (workChannels[ch], inputChannels[ch], numSamples);
            else
                FloatVectorOperations::clear (workChannels[ch], numSamples);
        }

        midiBuffer.clear();
        midiBuffer.addEvents (incomingMidi, start, numSamples, -start);
        parameterChanges.clear();

        AudioBuffer<float> audio (workChannels.data(), static_cast<int> (workChannels.size()), numSamples);

        AudioProcessContext<float> context {
            audio,
            midiBuffer,
            parameterChanges,
            nullptr,
            { inputViews.data(), inputViews.size() },
            { outputViews.data(), outputViews.size() }
        };

        processor.processBlock (context);

        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            auto* destination = outputChannelData[ch];
            if (destination == nullptr)
                continue;

            if (ch < numProcessorOutputs)
                FloatVectorOperations::copy (destination + start, workChannels[ch], numSamples);
            else
                FloatVectorOperations::clear (destination + start, numSamples);
        }

        if (sendsMidi && deviceManager != nullptr && ! midiBuffer.isEmpty())
        {
            if (auto* midiOutput = deviceManager->getDefaultMidiOutput())
                midiOutput->sendBlockOfMessagesNow (midiBuffer);
        }
    }

    static void clearOutputs (float* const* outputChannelData, int numOutputChannels, int start, int numSamples)
    {
        for (int ch = 0; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch] != nullptr)
                FloatVectorOperations::clear (outputChannelData[ch] + start, numSamples);
    }

    //==============================================================================
    static constexpr size_t midiStorageBytes = 16384;

    AudioProcessor& processor;
    AudioDeviceManager* deviceManager = nullptr;

    MidiMessageCollector midiCollector;
    MidiBuffer deviceMidi;
    MidiBuffer midiBuffer;
    ParameterChangeBuffer parameterChanges;

    AudioBuffer<float> inputBuffer;
    AudioBuffer<float> workBuffer;
    std::vector<const float*> inputChannels;
    std::vector<float*> workChannels;
    std::vector<AudioBusBufferView<const float>> inputViews;
    std::vector<AudioBusBufferView<float>> outputViews;

    int preparedBlockSize = 0;
    int numProcessorInputs = 0;
    int numProcessorOutputs = 0;
    bool sendsMidi = false;
    bool prepared = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandaloneProcessorPlayer)
};

} // namespace yup
