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

namespace juce
{

namespace
{

template <class T>
using hasAudioSampleFrameSamplesPerChannel = decltype(T::samplesPerChannel);

template <class T>
int getNumSamplesPerChannel (const T& frame)
{
    if constexpr (isDetected<hasAudioSampleFrameSamplesPerChannel, T>)
        return frame.samplesPerChannel;
    else
        return 128;
}

} // namespace

//==============================================================================
class AudioWorkletAudioIODevice final : public AudioIODevice
{
public:
    AudioWorkletAudioIODevice()
        : AudioIODevice (AudioWorkletAudioIODevice::audioWorkletTypeName,
                         AudioWorkletAudioIODevice::audioWorkletTypeName)
    {
        context = emscripten_create_audio_context (0);
    }

    ~AudioWorkletAudioIODevice()
    {
        close();

        emscripten_destroy_audio_context (context);
    }

    //==============================================================================
    StringArray getOutputChannelNames() override
    {
        StringArray result;

        for (int i = 1; i <= actualNumberOfOutputs; i++)
            result.add ("Out #" + String (i));

        return result;
    }

    StringArray getInputChannelNames() override
    {
        StringArray result;

        for (int i = 1; i <= actualNumberOfInputs; i++)
            result.add ("In #" + String (i));

        return result;
    }

    Array<double> getAvailableSampleRates() override
    {
        return { getDefaultSampleRate() };
    }

    double getDefaultSampleRate()
    {
        int outputSampleRate = EM_ASM_INT ({
            return emscriptenGetAudioObject ($0).sampleRate;
        }, context);

        if (outputSampleRate == 0)
            outputSampleRate = 44100;

        return static_cast<double> (outputSampleRate);
    }

    Array<int> getAvailableBufferSizes() override
    {
        return { getDefaultBufferSize() };
    }

    int getDefaultBufferSize() override
    {
        return 128;
    }

    //==============================================================================
    String open (const BigInteger& inputChannels,
                 const BigInteger& outputChannels,
                 double sampleRate,
                 int bufferSizeSamples) override
    {
        if (sampleRate != getDefaultSampleRate() || bufferSizeSamples != getDefaultBufferSize())
        {
            lastError = "Browser audio outputs only support 44.1 kHz sample rate and 128 samples buffer size.";
            return lastError;
        }

        auto numIns = getNumContiguousSetBits (inputChannels);
        auto numOuts = getNumContiguousSetBits (outputChannels);
        actualNumberOfInputs = jmax (numIns, 1);
        actualNumberOfOutputs = jmax (numOuts, 1);
        actualSampleRate = sampleRate;
        actualBufferSize = (uint32) bufferSizeSamples;

        channelInBuffer.calloc (actualNumberOfInputs);
        channelOutBuffer.calloc (actualNumberOfOutputs);

        isDeviceOpen = true;
        isRunning = false;
        callback = nullptr;
        underruns = 0;

        emscripten_start_wasm_audio_worklet_thread_async (
            context, audioThreadStack, sizeof (audioThreadStack), &audioThreadInitializedCallback, this);

        return {};
    }

    void close() override
    {
        stop();

        if (isDeviceOpen)
        {
            EM_ASM ({
                emscriptenGetAudioObject ($0).disconnect();
            }, audioWorkletNode);
            audioWorkletNode = {};

            isDeviceOpen = false;
            callback = nullptr;
            underruns = 0;

            actualBufferSize = 0;
            actualNumberOfInputs = 0;
            actualNumberOfOutputs = 0;
            actualSampleRate = 44100.0;
            actualBufferSize = 128u;

            channelInBuffer.free();
            channelOutBuffer.free();
        }
    }

    bool isOpen() override { return isDeviceOpen; }

    void start (AudioIODeviceCallback* newCallback) override
    {
        if (! isDeviceOpen)
            return;

        if (isRunning)
        {
            if (newCallback != callback)
            {
                if (newCallback != nullptr)
                    newCallback->audioDeviceAboutToStart (this);

                {
                    ScopedLock lock (callbackLock);
                    std::swap (callback, newCallback);
                }

                if (newCallback != nullptr)
                    newCallback->audioDeviceStopped();
            }
        }
        else
        {
            callback = newCallback;
            isRunning = emscripten_audio_context_state (context) == AUDIO_CONTEXT_STATE_RUNNING;

            if (! isRunning && hasBeenActivatedAlreadyByUser)
            {
                emscripten_resume_audio_context_sync (context);
                isRunning = emscripten_audio_context_state (context) == AUDIO_CONTEXT_STATE_RUNNING;
            }

            firstCallback = true;

            if (callback != nullptr)
            {
                if (isRunning)
                    callback->audioDeviceAboutToStart (this);
            }
        }
    }

    void stop() override
    {
        AudioIODeviceCallback* oldCallback = nullptr;

        if (callback != nullptr)
        {
            ScopedLock lock (callbackLock);
            std::swap (callback, oldCallback);
        }

        isRunning = false;

        EM_ASM ({
            emscriptenGetAudioObject ($0).suspend();
        }, context);

        if (oldCallback != nullptr)
            oldCallback->audioDeviceStopped();
    }

    bool isPlaying() override { return isRunning; }

    String getLastError() override { return lastError; }

    //==============================================================================
    int getCurrentBufferSizeSamples() override { return (int) actualBufferSize; }

    double getCurrentSampleRate() override { return actualSampleRate; }

    int getCurrentBitDepth() override { return 16; }

    BigInteger getActiveOutputChannels() const override
    {
        BigInteger b;
        b.setRange (0, actualNumberOfOutputs, true);
        return b;
    }

    BigInteger getActiveInputChannels() const override
    {
        BigInteger b;
        b.setRange (0, actualNumberOfInputs, true);
        return b;
    }

    int getOutputLatencyInSamples() override
    {
        return 0;
    }

    int getInputLatencyInSamples() override
    {
        return 0;
    }

    int getXRunCount() const noexcept override { return underruns; }

    //==============================================================================
    static const char* const audioWorkletTypeName;

private:
    //==============================================================================

    void audioThreadInitialized()
    {
        WebAudioWorkletProcessorCreateOptions opts =
        {
            .name = audioWorkletTypeName
        };

        emscripten_create_wasm_audio_worklet_processor_async (
            context, &opts, &audioWorkletProcessorCreatedCallback, this);
    }

    void audioWorkletProcessorCreated()
    {
        int outputChannelCounts[1] = { actualNumberOfOutputs };
        EmscriptenAudioWorkletNodeCreateOptions options =
        {
            .numberOfInputs = actualNumberOfInputs,
            .numberOfOutputs = 1,
            .outputChannelCounts = outputChannelCounts
        };

        // Create node
        audioWorkletNode = emscripten_create_wasm_audio_worklet_node (
            context, audioWorkletTypeName, &options, renderAudioCallback, this);

        // Connect it to audio context destination
        // emscripten_audio_node_connect (audioWorkletNode, context, 0, 0);
        EM_ASM ({
            emscriptenGetAudioObject ($0).connect (emscriptenGetAudioObject ($1).destination);
        }, audioWorkletNode, context);

        emscripten_set_click_callback ("canvas", reinterpret_cast<void*> (this), 0, canvasClickCallback);
    }

    EM_BOOL renderAudio (int numInputs, const AudioSampleFrame* inputs,
                         int numOutputs, AudioSampleFrame* outputs,
                         int numParams, const AudioParamFrame* params)
    {
        const int samplesPerChannel = [&]
        {
            if (numOutputs > 0)
                return getNumSamplesPerChannel (outputs[0]);

            else if (numInputs > 0)
                return getNumSamplesPerChannel (inputs[0]);

            return 128;
        }();

        // check for xruns
        calculateXruns (samplesPerChannel);

        ScopedLock lock (callbackLock);

        if (callback != nullptr)
        {
            // Setup channelInBuffers
            for (int ch = 0; ch < actualNumberOfInputs; ++ch)
                channelInBuffer[ch] = &(inputs[ch].data[0]);

            // Setup channelOutBuffers (assume a single worklet output)
            for (int ch = 0; ch < actualNumberOfOutputs; ++ch)
                channelOutBuffer[ch] = &(outputs[0].data[ch * samplesPerChannel]);

            callback->audioDeviceIOCallbackWithContext (channelInBuffer.getData(),
                                                        actualNumberOfInputs,
                                                        channelOutBuffer.getData(),
                                                        actualNumberOfOutputs,
                                                        samplesPerChannel,
                                                        {});

            audioFramesElapsed += samplesPerChannel;
        }

        return EM_TRUE; // keep going !
    }

    void canvasClick()
    {
        if (emscripten_audio_context_state (context) != AUDIO_CONTEXT_STATE_RUNNING)
        {
            emscripten_resume_audio_context_sync (context);

            isRunning = true;
            hasBeenActivatedAlreadyByUser = true;

            ScopedLock lock (callbackLock);

            if (callback != nullptr)
                callback->audioDeviceAboutToStart (this);
        }
    }

    //==============================================================================

    static void audioThreadInitializedCallback (EMSCRIPTEN_WEBAUDIO_T audioContext, EM_BOOL success, void* userData)
    {
        if (! success) return; // Check browser console in a debug build for detailed errors

        static_cast<AudioWorkletAudioIODevice*> (userData)->audioThreadInitialized();
    }

    static void audioWorkletProcessorCreatedCallback (EMSCRIPTEN_WEBAUDIO_T audioContext, EM_BOOL success, void* userData)
    {
        if (! success) return; // Check browser console in a debug build for detailed errors

        static_cast<AudioWorkletAudioIODevice*> (userData)->audioWorkletProcessorCreated();
    }

    static EM_BOOL renderAudioCallback (int numInputs, const AudioSampleFrame* inputs,
                                        int numOutputs, AudioSampleFrame* outputs,
                                        int numParams, const AudioParamFrame* params,
                                        void* userData)
    {
        return static_cast<AudioWorkletAudioIODevice*> (userData)->renderAudio (numInputs, inputs, numOutputs, outputs, numParams, params);
    }

    static EM_BOOL canvasClickCallback (int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData)
    {
        static_cast<AudioWorkletAudioIODevice*> (userData)->canvasClick();

        return EM_FALSE;
    }

    //==============================================================================
    uint64_t expectedElapsedAudioSamples = 0;
    uint64_t audioFramesElapsed = 0;
    int underruns = 0;
    bool firstCallback = false;

    void calculateXruns (uint32_t numSamples)
    {
        if (audioFramesElapsed > expectedElapsedAudioSamples && ! firstCallback)
            ++underruns;

        firstCallback = false;
        expectedElapsedAudioSamples = audioFramesElapsed + numSamples;
    }

    //==============================================================================
    static int getNumContiguousSetBits (const BigInteger& value) noexcept
    {
        int bit = 0;

        while (value[bit])
            ++bit;

        return bit;
    }

    //==============================================================================
    EMSCRIPTEN_WEBAUDIO_T context{};
    EMSCRIPTEN_AUDIO_WORKLET_NODE_T audioWorkletNode{};

    bool isDeviceOpen = false;
    bool isRunning = false;
    bool hasBeenActivatedAlreadyByUser = false;

    CriticalSection callbackLock;
    AudioIODeviceCallback* callback = nullptr;

    String lastError;
    uint32 actualBufferSize = 0;
    int actualNumberOfInputs = 0, actualNumberOfOutputs = 0;
    double actualSampleRate = 44100.0;

    HeapBlock<const float*> channelInBuffer;
    HeapBlock<float*> channelOutBuffer;

    alignas(16) uint8 audioThreadStack[4096] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioWorkletAudioIODevice)
};

const char* const AudioWorkletAudioIODevice::audioWorkletTypeName = "Audio Worklet";

//==============================================================================
struct AudioWorkletAudioIODeviceType final : public AudioIODeviceType
{
    AudioWorkletAudioIODeviceType()
        : AudioIODeviceType ("AudioWorklet")
    {
    }

    StringArray getDeviceNames (bool) const override { return StringArray (AudioWorkletAudioIODevice::audioWorkletTypeName); }

    void scanForDevices() override {}

    int getDefaultDeviceIndex (bool) const override { return 0; }

    int getIndexOfDevice (AudioIODevice* device, bool) const override { return device != nullptr ? 0 : -1; }

    bool hasSeparateInputsAndOutputs() const override { return false; }

    AudioIODevice* createDevice (const String& outputName, const String& inputName) override
    {
        if (outputName == AudioWorkletAudioIODevice::audioWorkletTypeName || inputName == AudioWorkletAudioIODevice::audioWorkletTypeName)
            return new AudioWorkletAudioIODevice();

        return nullptr;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioWorkletAudioIODeviceType)
};

} // namespace juce
