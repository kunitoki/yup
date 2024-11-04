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

uint8_t audioThreadStack[4096] = {};

EM_BOOL onCanvasClick(int eventType, const EmscriptenMouseEvent* mouseEvent, void* userData)
{
    EMSCRIPTEN_WEBAUDIO_T audioContext = reinterpret_cast<EMSCRIPTEN_WEBAUDIO_T>(userData);

    if (emscripten_audio_context_state(audioContext) != AUDIO_CONTEXT_STATE_RUNNING)
        emscripten_resume_audio_context_sync(audioContext);

    return EM_FALSE;
}

EM_BOOL generateNoise(int numInputs, const AudioSampleFrame *inputs,
                      int numOutputs, AudioSampleFrame *outputs,
                      int numParams, const AudioParamFrame *params,
                      void* userData)
{
    for(int i = 0; i < numOutputs; ++i)
        for(int j = 0; j < outputs[i].numberOfChannels * 128; ++j)
        // for(int j = 0; j < outputs[i].quantumSize * outputs[i].numberOfChannels; ++j)
        // for(int j = 0; j < outputs[i].samplesPerChannel * outputs[i].numberOfChannels; ++j)
            outputs[i].data[j] = emscripten_random() * 0.2 - 0.1;

    return EM_TRUE; // keep going !
}

void audioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, EM_BOOL success, void* userData)
{
    if (!success) return; // Check browser console in a debug build for detailed errors

    int outputChannelCounts[1] = { 1 };
    EmscriptenAudioWorkletNodeCreateOptions options =
    {
        .numberOfInputs = 0,
        .numberOfOutputs = 1,
        .outputChannelCounts = outputChannelCounts
    };

    // Create node
    EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node (
        audioContext, "yup-processor", &options, &generateNoise, 0);

    // Connect it to audio context destination
    //emscripten_audio_node_connect(wasmAudioWorklet, audioContext, 0, 0);
    EM_ASM({
        emscriptenGetAudioObject($0).connect(emscriptenGetAudioObject($1).destination)
    }, wasmAudioWorklet, audioContext);

    // Resume context on mouse click
    emscripten_set_click_callback("canvas", reinterpret_cast<void*>(audioContext), 0, onCanvasClick);
}

void audioThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, EM_BOOL success, void* userData)
{
    if (!success) return; // Check browser console in a debug build for detailed errors

    WebAudioWorkletProcessorCreateOptions opts =
    {
        .name = "yup-processor",
    };

    emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, &audioWorkletProcessorCreated, 0);
}

//==============================================================================
class AudioWorkletAudioIODevice final : public AudioIODevice
{
public:
    AudioWorkletAudioIODevice()
        : AudioIODevice (AudioWorkletAudioIODevice::audioWorkletTypeName,
                         AudioWorkletAudioIODevice::audioWorkletTypeName)
    {
        context = emscripten_create_audio_context(0);

        emscripten_start_wasm_audio_worklet_thread_async (context, audioThreadStack, sizeof(audioThreadStack), &audioThreadInitialized, 0);
    }

    ~AudioWorkletAudioIODevice()
    {
        //close();
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

    Array<double> getAvailableSampleRates() override { return { 44100.0 }; }

    Array<int> getAvailableBufferSizes() override
    { /* TODO: */
        return { getDefaultBufferSize() };
    }

    int getDefaultBufferSize() override { return 256; /*defaultSettings.periodSize;*/ }

    //==============================================================================
    String open (const BigInteger& inputChannels,
                 const BigInteger& outputChannels,
                 double sampleRate,
                 int bufferSizeSamples) override
    {
        /*
        if (sampleRate != 44100.0 && sampleRate != 0.0)
        {
            lastError = "Bela audio outputs only support 44.1 kHz sample rate";
            return lastError;
        }

        settings = defaultSettings;

        auto numIns = getNumContiguousSetBits (inputChannels);
        auto numOuts = getNumContiguousSetBits (outputChannels);

        // Input and Output channels are numbered as follows
        //
        // 0  .. 1  - audio
        // 2  .. 9  - analog

        if (numIns > 2 || numOuts > 2)
        {
            settings.useAnalog = true;
            settings.numAnalogInChannels = std::max (numIns - 2, 8);
            settings.numAnalogOutChannels = std::max (numOuts - 2, 8);
            settings.uniformSampleRate = true;
        }

        settings.numAudioInChannels = std::max (numIns, 2);
        settings.numAudioOutChannels = std::max (numOuts, 2);

        settings.detectUnderruns = 1;
        settings.setup = setupCallback;
        settings.render = renderCallback;
        settings.cleanup = cleanupCallback;
        settings.interleave = 0;

        if (bufferSizeSamples > 0)
            settings.periodSize = bufferSizeSamples;

        isBelaOpen = false;
        isRunning = false;
        callback = nullptr;
        underruns = 0;

        if (Bela_initAudio (&settings, this) != 0 || ! isBelaOpen)
        {
            lastError = "Bela_initAutio failed";
            return lastError;
        }

        actualNumberOfInputs = jmin (numIns, actualNumberOfInputs);
        actualNumberOfOutputs = jmin (numOuts, actualNumberOfOutputs);

        channelInBuffer.calloc (actualNumberOfInputs);
        channelOutBuffer.calloc (actualNumberOfOutputs);
        */

        return {};
    }

    void close() override
    {
        /*
        stop();

        if (isBelaOpen)
        {
            Bela_cleanupAudio();

            isBelaOpen = false;
            callback = nullptr;
            underruns = 0;

            actualBufferSize = 0;
            actualNumberOfInputs = 0;
            actualNumberOfOutputs = 0;

            channelInBuffer.free();
            channelOutBuffer.free();
        }
        */
    }

    bool isOpen() override { return isBelaOpen; }

    void start (AudioIODeviceCallback* newCallback) override
    {
        /*
        if (! isBelaOpen)
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
            isRunning = (Bela_startAudio() == 0);

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
        */
    }

    void stop() override
    {
        /*
        AudioIODeviceCallback* oldCallback = nullptr;

        if (callback != nullptr)
        {
            ScopedLock lock (callbackLock);
            std::swap (callback, oldCallback);
        }

        isRunning = false;
        Bela_stopAudio();

        if (oldCallback != nullptr)
            oldCallback->audioDeviceStopped();
        */
    }

    bool isPlaying() override { return isRunning; }

    String getLastError() override { return lastError; }

    //==============================================================================
    int getCurrentBufferSizeSamples() override { return (int) actualBufferSize; }

    double getCurrentSampleRate() override { return 44100.0; }

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

    const int analogChannelStart = 2;

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
    /*
    static bool setupCallback (BelaContext* context, void* userData) noexcept { return static_cast<BelaAudioIODevice*> (userData)->setup (*context); }

    static void renderCallback (BelaContext* context, void* userData) noexcept { static_cast<BelaAudioIODevice*> (userData)->render (*context); }

    static void cleanupCallback (BelaContext* context, void* userData) noexcept { static_cast<BelaAudioIODevice*> (userData)->cleanup (*context); }
    */

    //==============================================================================
    EMSCRIPTEN_WEBAUDIO_T context{};

    //BelaInitSettings defaultSettings, settings;
    bool isBelaOpen = false, isRunning = false;

    CriticalSection callbackLock;
    AudioIODeviceCallback* callback = nullptr;

    String lastError;
    uint32_t actualBufferSize = 0;
    int actualNumberOfInputs = 0, actualNumberOfOutputs = 0;

    HeapBlock<const float*> channelInBuffer;
    HeapBlock<float*> channelOutBuffer;

    bool includeAnalogSupport;

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
        // TODO: switching whether to support analog/digital with possible multiple Bela device types?
        if (outputName == AudioWorkletAudioIODevice::audioWorkletTypeName || inputName == AudioWorkletAudioIODevice::audioWorkletTypeName)
            return new AudioWorkletAudioIODevice();

        return nullptr;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioWorkletAudioIODeviceType)
};

} // namespace juce
