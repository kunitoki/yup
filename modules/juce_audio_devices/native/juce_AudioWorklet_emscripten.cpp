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

//==============================================================================
class AudioWorkletAudioIODevice final : public AudioIODevice
{
public:
    AudioWorkletAudioIODevice()
        : AudioIODevice (AudioWorkletAudioIODevice::audioWorkletTypeName,
                         AudioWorkletAudioIODevice::audioWorkletTypeName)
    {
    }

    ~AudioWorkletAudioIODevice()
    {
        close();
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
        return { 44100.0 };
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
        yup::Logger::outputDebugString (String("open ") + String(sampleRate) + " " + String(bufferSizeSamples));

        if (sampleRate != 44100.0 && sampleRate != 0.0)
        {
            lastError = "Browser audio outputs only support 44.1 kHz sample rate";
            return lastError;
        }

        auto numIns = getNumContiguousSetBits (inputChannels);
        auto numOuts = getNumContiguousSetBits (outputChannels);
        actualNumberOfInputs = jmax (numIns, 2);
        actualNumberOfOutputs = jmax (numOuts, 2);
        actualSampleRate = sampleRate;
        actualBufferSize = (uint32) bufferSizeSamples;

        channelInBuffer.calloc (actualNumberOfInputs);
        channelOutBuffer.calloc (actualNumberOfOutputs);

        isDeviceOpen = false;
        isRunning = false;
        callback = nullptr;
        underruns = 0;

        context = emscripten_create_audio_context (0);

        emscripten_start_wasm_audio_worklet_thread_async (
            context, audioThreadStack, sizeof (audioThreadStack), &audioThreadInitializedCallback, this);

        return {};
    }

    void close() override
    {
        yup::Logger::outputDebugString ("close");

        stop();

        if (isDeviceOpen)
        {
            emscripten_destroy_audio_context (context);

            isDeviceOpen = false;
            callback = nullptr;
            underruns = 0;

            actualBufferSize = 0;
            actualNumberOfInputs = 0;
            actualNumberOfOutputs = 0;
            actualSampleRate = 44100.0;
            actualBufferSize = (uint32) 128;

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
            isRunning = (/*Bela_startAudio()*/0 == 0);

            if (callback != nullptr)
            {
                if (isRunning)
                {
                    callback->audioDeviceAboutToStart (this);
                }
                else
                {
                    lastError = "Bela_StartAudio failed";
                    callback->audioDeviceError (lastError);
                }
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

        // Bela_stopAudio();

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
    { /* TODO */
        return 0;
    }

    int getInputLatencyInSamples() override
    { /* TODO */
        return 0;
    }

    int getXRunCount() const noexcept override { return underruns; }

    //==============================================================================
    static const char* const audioWorkletTypeName;

private:
    //==============================================================================
    /*
    bool setup (BelaContext& context)
    {
        actualBufferSize = context.audioFrames;
        actualNumberOfInputs = (int) (context.audioInChannels + context.analogInChannels);
        actualNumberOfOutputs = (int) (context.audioOutChannels + context.analogOutChannels);
        isBelaOpen = true;
        firstCallback = true;

        ScopedLock lock (callbackLock);

        if (callback != nullptr)
            callback->audioDeviceAboutToStart (this);

        return true;
    }

    void render (BelaContext& context)
    {
        // check for xruns
        calculateXruns (context.audioFramesElapsed, context.audioFrames);

        ScopedLock lock (callbackLock);

        // Check for and process and midi
        for (auto midiInput : MidiInput::Pimpl::midiInputs)
            midiInput->poll();

        if (callback != nullptr)
        {
            jassert (context.audioFrames <= actualBufferSize);
            jassert ((context.flags & BELA_FLAG_INTERLEAVED) == 0);

            using Frames = decltype (context.audioFrames);

            // Setup channelInBuffers
            for (int ch = 0; ch < actualNumberOfInputs; ++ch)
            {
                if (ch < analogChannelStart)
                    channelInBuffer[ch] = &context.audioIn[(Frames) ch * context.audioFrames];
                else
                    channelInBuffer[ch] = &context.analogIn[(Frames) (ch - analogChannelStart) * context.analogFrames];
            }

            // Setup channelOutBuffers
            for (int ch = 0; ch < actualNumberOfOutputs; ++ch)
            {
                if (ch < analogChannelStart)
                    channelOutBuffer[ch] = &context.audioOut[(Frames) ch * context.audioFrames];
                else
                    channelOutBuffer[ch] = &context.analogOut[(Frames) (ch - analogChannelStart) * context.audioFrames];
            }

            callback->audioDeviceIOCallbackWithContext (channelInBuffer.getData(),
                                                        actualNumberOfInputs,
                                                        channelOutBuffer.getData(),
                                                        actualNumberOfOutputs,
                                                        (int) context.audioFrames,
                                                        {});
        }
    }

    void cleanup (BelaContext&)
    {
        ScopedLock lock (callbackLock);

        if (callback != nullptr)
            callback->audioDeviceStopped();
    }
    */

    //==============================================================================
    uint64_t expectedElapsedAudioSamples = 0;
    int underruns = 0;
    bool firstCallback = false;

    void calculateXruns (uint64_t audioFramesElapsed, uint32_t numSamples)
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

    void audioThreadInitialized()
    {
        yup::Logger::outputDebugString ("audioThreadInitialized");

        WebAudioWorkletProcessorCreateOptions opts =
        {
            .name = audioWorkletTypeName,
        };

        emscripten_create_wasm_audio_worklet_processor_async (
            context, &opts, &audioWorkletProcessorCreatedCallback, this);
    }

    void audioWorkletProcessorCreated()
    {
        yup::Logger::outputDebugString ("audioWorkletProcessorCreated");

        int outputChannelCounts[1] = { actualNumberOfOutputs };
        EmscriptenAudioWorkletNodeCreateOptions options =
        {
            .numberOfInputs = actualNumberOfInputs,
            .numberOfOutputs = 1,
            .outputChannelCounts = outputChannelCounts
        };

        // Create node
        EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node (
            context, audioWorkletTypeName, &options, renderAudioCallback, this);

        // Connect it to audio context destination
        //emscripten_audio_node_connect(wasmAudioWorklet, audioContext, 0, 0);
        EM_ASM({
            emscriptenGetAudioObject ($0).connect (emscriptenGetAudioObject ($1).destination)
        }, wasmAudioWorklet, context);

        emscripten_set_click_callback ("canvas", reinterpret_cast<void*> (this), 0, canvasClickCallback);
    }

    EM_BOOL renderAudio (int numInputs, const AudioSampleFrame* inputs,
                         int numOutputs, AudioSampleFrame* outputs,
                         int numParams, const AudioParamFrame* params)
    {
        // check for xruns
        //calculateXruns (context.audioFramesElapsed, context.audioFrames);

        ScopedLock lock (callbackLock);

        if (callback != nullptr)
        {
            int audioFrames = 128;

            // Setup channelInBuffers
            for (int ch = 0; ch < actualNumberOfInputs; ++ch)
                channelInBuffer[ch] = &inputs[ch].data[0];

            // Setup channelOutBuffers (assume a single worklet output)
            for (int ch = 0; ch < actualNumberOfOutputs; ++ch)
                channelOutBuffer[ch] = &outputs[0].data[ch * 128]; // outputs[0].samplesPerChannel / outputs[0].quantumSize

            callback->audioDeviceIOCallbackWithContext (channelInBuffer.getData(),
                                                        actualNumberOfInputs,
                                                        channelOutBuffer.getData(),
                                                        actualNumberOfOutputs,
                                                        audioFrames,
                                                        {});
        }

        return EM_TRUE; // keep going !
    }

    void canvasClick()
    {
        if (emscripten_audio_context_state (context) != AUDIO_CONTEXT_STATE_RUNNING)
            emscripten_resume_audio_context_sync (context);
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
    EMSCRIPTEN_WEBAUDIO_T context{};

    bool isDeviceOpen = false, isRunning = false;

    CriticalSection callbackLock;
    AudioIODeviceCallback* callback = nullptr;

    String lastError;
    uint32_t actualBufferSize = 0;
    int actualNumberOfInputs = 0, actualNumberOfOutputs = 0;
    double actualSampleRate = 44100.0;

    HeapBlock<const float*> channelInBuffer;
    HeapBlock<float*> channelOutBuffer;

    alignas(16) uint8_t audioThreadStack[4096] = {};

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
