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

#include "../yup_audio_plugin_client.h"

#include "../common/yup_AudioPluginAUHelpers.h"
#include "../common/yup_AudioPluginUtilities.h"

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_AU)
#error "YUP_AUDIO_PLUGIN_ENABLE_AU must be defined"
#endif

#if YUP_MAC
#include <AudioUnitSDK/AUBase.h>
#include <AudioUnitSDK/AUEffectBase.h>
#include <AudioUnitSDK/AUMIDIBase.h>
#include <AudioUnitSDK/ComponentBase.h>
#include <AudioUnitSDK/MusicDeviceBase.h>

#import <AppKit/AppKit.h>
#import <AudioUnit/AUCocoaUIView.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreMIDI/CoreMIDI.h>
#import <Foundation/Foundation.h>

#import <objc/message.h>
#import <objc/runtime.h>

#include <yup_core/native/yup_ObjCHelpers_apple.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

//==============================================================================

extern "C" yup::AudioProcessor* YUP_AUDIO_PLUGIN_CREATE_FUNCTION();

namespace yup
{

class AudioPluginEditorViewAU;

static String describeScopeAndElement (AudioUnitScope scope, AudioUnitElement element)
{
    return "scope=" + String (static_cast<int> (scope)) + ", element=" + String (static_cast<int> (element));
}

static String describeStatus (OSStatus status)
{
    return String (static_cast<int> (status));
}

//==============================================================================

#if YupPlugin_IsSynth
using AudioPluginAUBase = ausdk::MusicDeviceBase;
#else
using AudioPluginAUBase = ausdk::AUEffectBase;
#endif

//==============================================================================

/**
    AUv2 wrapper for a YUP AudioProcessor.

    Supports both effects (AUEffectBase) and instruments (MusicDeviceBase)
    depending on the YupPlugin_IsSynth compile-time setting.
*/
class AudioPluginProcessorAU final
    : public AudioPluginAUBase
    , private AudioParameter::Listener
{
public:
    class AudioPluginPlayHeadAU final : public AudioPlayHead
    {
    public:
        AudioPluginPlayHeadAU (AudioPluginProcessorAU& owner, const AudioTimeStamp* timeStamp)
            : owner (owner)
            , timeStamp (timeStamp)
        {
        }

        bool canControlTransport() override
        {
            return false;
        }

        std::optional<PositionInfo> getPosition() const override
        {
            return owner.createPositionInfo (timeStamp);
        }

    private:
        AudioPluginProcessorAU& owner;
        const AudioTimeStamp* timeStamp = nullptr;
    };

    //==============================================================================

    AudioPluginProcessorAU (AudioComponentInstance component)
#if YupPlugin_IsSynth
        : AudioPluginAUBase (component, 0, 1)
        ,
#else
        : AudioPluginAUBase (component)
        ,
#endif
        componentInstance (component)
    {
        processor.reset (yup::createPluginProcessorForWrapper (yup::AudioPluginWrapperType::au, ::YUP_AUDIO_PLUGIN_CREATE_FUNCTION));

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "created processor instance: wrapper=" << yup::describePointer (this) << ", component=" << yup::describePointer (componentInstance) << ", processor=" << yup::describePointer (processor.get()));

        if (processor == nullptr)
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "createPluginProcessor returned null");
        else
            bypass.attach (*processor);

        addParameterListeners();
        registerInstance (componentInstance, this);
    }

    ~AudioPluginProcessorAU() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "destroying processor instance: wrapper=" << yup::describePointer (this) << ", component=" << yup::describePointer (componentInstance) << ", processor=" << yup::describePointer (processor.get()));

        closeEditorViews();
        removeParameterListeners();
        yup::endActiveParameterGestures (processor.get());

        unregisterInstance (componentInstance);

        processor.reset();
    }

    //==============================================================================

    OSStatus Initialize() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "Initialize requested");

        const auto result = AudioPluginAUBase::Initialize();
        if (result != noErr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "base Initialize failed: status=" << describeStatus (result));
            return result;
        }

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "Initialize failed: processor is null");
            return kAudioUnitErr_FailedInitialization;
        }

#if ! YupPlugin_IsSynth
        // Expose one input element per audio input bus so hosts can route
        // sidechain signals to auxiliary input elements (element 0 is always
        // the main input). Processors with a single input bus keep one element.
        SetNumberOfElements (kAudioUnitScope_Input, static_cast<UInt32> (processor->getNumAudioInputs()));
#endif

        processor->setOfflineProcessing (renderingOffline);
        processor->setPlaybackConfiguration (static_cast<float> (getCurrentSampleRate()),
                                             static_cast<int> (GetMaxFramesPerSlice()));

        midiBuffer.ensureSize (4096);
        midiBuffer.clear();
        renderMidiBuffer.ensureSize (4096);
        renderMidiBuffer.clear();
        paramChangeBuffer.reserve (getDefaultParameterChangeCapacity (*processor));
        emptyParamChangeBuffer.reserve (getDefaultParameterChangeCapacity (*processor));
        audioChannels.reserve (static_cast<size_t> (getTotalAudioOutputChannels (*processor)));

        // Pre-allocate per-bus view storage so the render callback never allocates
        renderViews.prepare (*processor,
                              getTotalAudioInputChannels (*processor),
                              getTotalAudioOutputChannels (*processor));
        auxiliaryInputValid.resize (static_cast<size_t> (processor->getNumAudioInputs()), 0);

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "Initialize completed: sampleRate=" << String (getCurrentSampleRate()) << ", maxFramesPerSlice=" << String (static_cast<int> (GetMaxFramesPerSlice())));

        return noErr;
    }

    void Cleanup() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "Cleanup requested: wrapper=" << yup::describePointer (this) << ", processor=" << yup::describePointer (processor.get()));

        if (processor != nullptr)
            processor->releaseResources();

        AudioPluginAUBase::Cleanup();
    }

    //==============================================================================

    OSStatus GetParameterList (AudioUnitScope inScope,
                               AudioUnitParameterID* outParameterList,
                               UInt32& outNumParameters) override
    {
        if (inScope != kAudioUnitScope_Global || processor == nullptr)
        {
            outNumParameters = 0;
            return kAudioUnitErr_InvalidParameter;
        }

        const auto parameters = processor->getParameters();

        if (outParameterList != nullptr)
        {
            for (size_t i = 0; i < parameters.size(); ++i)
                outParameterList[i] = static_cast<AudioUnitParameterID> (parameters[i]->getHostParameterID());
        }

        outNumParameters = static_cast<UInt32> (parameters.size());
        return noErr;
    }

    OSStatus GetParameterInfo (AudioUnitScope inScope,
                               AudioUnitParameterID inParameterID,
                               AudioUnitParameterInfo& outParameterInfo) override
    {
        if (inScope != kAudioUnitScope_Global || processor == nullptr)
            return kAudioUnitErr_InvalidParameter;

        const auto parameters = processor->getParameters();
        const auto parameterIndex = processor->getParameterIndexByHostID (static_cast<uint32> (inParameterID));
        if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
            return kAudioUnitErr_InvalidParameter;

        const auto& param = parameters[parameterIndex];

        outParameterInfo.flags = makeAUParameterFlags (*param) | kAudioUnitParameterFlag_HasCFNameString;

        outParameterInfo.cfNameString = param->getName().toCFString();
        param->getName().copyToUTF8 (outParameterInfo.name, sizeof (outParameterInfo.name));

        outParameterInfo.unit = makeAUUnit (*param);
        outParameterInfo.minValue = param->getMinimumValue();
        outParameterInfo.maxValue = param->getMaximumValue();
        outParameterInfo.defaultValue = param->getDefaultValue();
        outParameterInfo.clumpID = 0;

        return noErr;
    }

    OSStatus GetParameter (AudioUnitParameterID inID,
                           AudioUnitScope inScope,
                           AudioUnitElement inElement,
                           AudioUnitParameterValue& outValue) override
    {
        if (inScope != kAudioUnitScope_Global || processor == nullptr)
            return kAudioUnitErr_InvalidParameter;

        const auto parameters = processor->getParameters();
        const auto parameterIndex = processor->getParameterIndexByHostID (static_cast<uint32> (inID));
        if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
            return kAudioUnitErr_InvalidParameter;

        outValue = static_cast<AudioUnitParameterValue> (parameters[parameterIndex]->getValue());
        return noErr;
    }

    OSStatus SetParameter (AudioUnitParameterID inID,
                           AudioUnitScope inScope,
                           AudioUnitElement inElement,
                           AudioUnitParameterValue inValue,
                           UInt32 inBufferOffsetInFrames) override
    {
        if (inScope != kAudioUnitScope_Global || processor == nullptr)
            return kAudioUnitErr_InvalidParameter;

        const auto parameters = processor->getParameters();
        const auto parameterIndex = processor->getParameterIndexByHostID (static_cast<uint32> (inID));
        if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
            return kAudioUnitErr_InvalidParameter;

        if (parameters[parameterIndex]->isReadOnly()
            || parameters[parameterIndex]->isPerformingChangeGesture())
        {
            return noErr;
        }

        parameters[parameterIndex]->setValue (static_cast<float> (inValue));

        std::unique_lock<std::mutex> lock (parameterChangeMutex, std::try_to_lock);
        if (lock.owns_lock())
        {
            addParameterChangeByHostParameterID (*processor,
                                                 paramChangeBuffer,
                                                 static_cast<uint32> (inID),
                                                 parameters[parameterIndex]->convertToNormalizedValue (static_cast<float> (inValue)),
                                                 static_cast<int> (inBufferOffsetInFrames));
        }

        return noErr;
    }

    //==============================================================================

    UInt32 SupportedNumChannels (const AUChannelInfo** outInfo) override
    {
        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SupportedNumChannels requested without processor");
            return 0;
        }

        channelInfoCache.clear();

        const auto& busLayout = processor->getBusLayout();

        int inputChannels = 0;
        for (const auto& bus : busLayout.getInputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                inputChannels = std::max (inputChannels, bus.getNumChannels());

        int outputChannels = 0;
        for (const auto& bus : busLayout.getOutputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                outputChannels = std::max (outputChannels, bus.getNumChannels());

        if (inputChannels > 0 || outputChannels > 0)
        {
            AUChannelInfo info;
            info.inChannels = static_cast<SInt16> (inputChannels);
            info.outChannels = static_cast<SInt16> (outputChannels);
            channelInfoCache.push_back (info);
        }

        if (outInfo != nullptr)
            *outInfo = channelInfoCache.data();

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SupportedNumChannels returned " << String (static_cast<int> (channelInfoCache.size())) << " layouts");

        return static_cast<UInt32> (channelInfoCache.size());
    }

    //==============================================================================

    bool SupportsTail() override
    {
        return processor != nullptr && processor->getTailSamples() > 0;
    }

    Float64 GetTailTime() override
    {
        const auto sampleRate = getCurrentSampleRate();
        if (processor == nullptr || sampleRate <= 0.0)
            return 0.0;

        return static_cast<Float64> (processor->getTailSamples()) / sampleRate;
    }

    Float64 GetLatency() override
    {
        const auto sampleRate = getCurrentSampleRate();
        if (processor == nullptr || sampleRate <= 0.0)
            return 0.0;

        return static_cast<Float64> (processor->getLatencySamples()) / sampleRate;
    }

    //==============================================================================

    std::optional<AudioPlayHead::PositionInfo> createPositionInfo (const AudioTimeStamp* timeStamp)
    {
        AudioPlayHead::PositionInfo result;
        bool hasPosition = false;

        if (timeStamp != nullptr && (timeStamp->mFlags & kAudioTimeStampSampleTimeValid) != 0)
        {
            const auto timeInSamples = static_cast<int64_t> (timeStamp->mSampleTime);
            result.setTimeInSamples (timeInSamples);

            const auto sampleRate = getCurrentSampleRate();
            if (sampleRate > 0.0)
                result.setTimeInSeconds (static_cast<double> (timeInSamples) / sampleRate);

            hasPosition = true;
        }

        Float64 currentBeat = 0.0;
        Float64 currentTempo = 0.0;
        if (CallHostBeatAndTempo (&currentBeat, &currentTempo) == noErr)
        {
            result.setPpqPosition (currentBeat);
            result.setBpm (currentTempo);
            hasPosition = true;
        }

        UInt32 deltaSamplesToNextBeat = 0;
        Float32 timeSignatureNumerator = 0.0f;
        UInt32 timeSignatureDenominator = 0;
        Float64 currentMeasureDownBeat = 0.0;
        if (CallHostMusicalTimeLocation (&deltaSamplesToNextBeat,
                                         &timeSignatureNumerator,
                                         &timeSignatureDenominator,
                                         &currentMeasureDownBeat)
            == noErr)
        {
            ignoreUnused (deltaSamplesToNextBeat);
            result.setTimeSignature (AudioPlayHead::TimeSignature {
                static_cast<int> (timeSignatureNumerator),
                static_cast<int> (timeSignatureDenominator) });
            result.setPpqPositionOfLastBarStart (currentMeasureDownBeat);
            hasPosition = true;
        }

        Boolean isPlaying = false;
        Boolean transportStateChanged = false;
        Float64 currentSampleInTimeline = 0.0;
        Boolean isCycling = false;
        Float64 cycleStartBeat = 0.0;
        Float64 cycleEndBeat = 0.0;
        if (CallHostTransportState (&isPlaying,
                                    &transportStateChanged,
                                    &currentSampleInTimeline,
                                    &isCycling,
                                    &cycleStartBeat,
                                    &cycleEndBeat)
            == noErr)
        {
            ignoreUnused (transportStateChanged);
            result.setIsPlaying (isPlaying);
            result.setIsLooping (isCycling);

            if (isCycling)
                result.setLoopPoints (AudioPlayHead::LoopPoints { cycleStartBeat, cycleEndBeat });

            result.setTimeInSamples (static_cast<int64_t> (currentSampleInTimeline));

            const auto sampleRate = getCurrentSampleRate();
            if (sampleRate > 0.0)
                result.setTimeInSeconds (currentSampleInTimeline / sampleRate);

            hasPosition = true;
        }

        return hasPosition ? std::make_optional (result) : std::nullopt;
    }

    //==============================================================================

#if YupPlugin_IsSynth
    // Instrument: render audio and drain the MIDI buffer
    OSStatus RenderBus (AudioUnitRenderActionFlags& ioActionFlags,
                        const AudioTimeStamp& inTimeStamp,
                        UInt32 inBusNumber,
                        UInt32 inNumberFrames) override
    {
        if (processor == nullptr)
            return kAudioUnitErr_NoConnection;

        auto& outputBus = Output (inBusNumber);

        outputBus.PrepareBuffer (inNumberFrames);
        AudioBufferList& outBufList = outputBus.GetBufferList();

        audioChannels.clear();
        for (UInt32 ch = 0; ch < outBufList.mNumberBuffers; ++ch)
            audioChannels.push_back (static_cast<float*> (outBufList.mBuffers[ch].mData));

        AudioSampleBuffer audioBuffer (audioChannels.data(),
                                       static_cast<int> (audioChannels.size()),
                                       0,
                                       static_cast<int> (inNumberFrames));

        {
            AudioPluginPlayHeadAU playHead (*this, &inTimeStamp);

            {
                std::unique_lock<std::mutex> lock (midiMutex, std::try_to_lock);
                if (lock.owns_lock())
                    renderMidiBuffer.swapWith (midiBuffer);
            }

            std::unique_lock<std::mutex> parameterLock (parameterChangeMutex, std::try_to_lock);
            auto& processParamChangeBuffer = parameterLock.owns_lock() ? paramChangeBuffer : emptyParamChangeBuffer;

            AudioProcessContext<float> context { audioBuffer,
                                                 renderMidiBuffer,
                                                 processParamChangeBuffer,
                                                 &playHead };
            processAudioBlock (*processor, context, bypass.isBypassed());

            renderMidiBuffer.clear();
            processParamChangeBuffer.clear();
        }

        return noErr;
    }

    //==============================================================================

    OSStatus HandleMIDIEvent (UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 offsetSampleFrame) override
    {
        const std::lock_guard<std::mutex> lock (midiMutex);

        const uint8_t rawData[3] = {
            static_cast<uint8_t> (status | channel),
            data1,
            data2
        };

        const int numBytes = MidiMessage::getMessageLengthFromFirstByte (rawData[0]);
        midiBuffer.addEvent (rawData, numBytes, static_cast<int> (offsetSampleFrame));

        return noErr;
    }

    [[nodiscard]] bool CanScheduleParameters() const override
    {
        return false;
    }

    bool StreamFormatWritable (AudioUnitScope inScope, AudioUnitElement inElement) override
    {
        return inScope == kAudioUnitScope_Output && inElement == 0;
    }

    OSStatus HandleSysEx (const UInt8* inData, UInt32 inLength) override
    {
        const std::lock_guard<std::mutex> lock (midiMutex);

        if (inData != nullptr && inLength > 0)
            midiBuffer.addEvent (inData, static_cast<int> (inLength), 0);

        return noErr;
    }

#else
    // Effect: pull auxiliary (sidechain) input elements before the base class
    // renders element 0, so their buffers are populated when ProcessBufferLists
    // builds the per-bus input views.
    OSStatus Render (AudioUnitRenderActionFlags& ioActionFlags,
                     const AudioTimeStamp& inTimeStamp,
                     UInt32 inNumberFrames) override
    {
        if (processor != nullptr)
            pullAuxiliaryInputElements (ioActionFlags, inTimeStamp, inNumberFrames);

        return AudioPluginAUBase::Render (ioActionFlags, inTimeStamp, inNumberFrames);
    }

    //==============================================================================

    void pullAuxiliaryInputElements (AudioUnitRenderActionFlags& ioActionFlags,
                                     const AudioTimeStamp& inTimeStamp,
                                     UInt32 inNumberFrames)
    {
        const auto numInputs = static_cast<UInt32> (processor->getNumAudioInputs());
        const auto numElements = Inputs().GetNumberOfElements();

        for (UInt32 element = 1; element < numInputs && element < numElements; ++element)
        {
            auto& input = Input (element);

            // Only connected elements produce audio; disconnected sidechain
            // elements stay silent and are exposed as null-buffer views below
            const auto pullResult = input.IsActive()
                                      ? input.PullInput (ioActionFlags, inTimeStamp, element, inNumberFrames)
                                      : kAudioUnitErr_NoConnection;

            auxiliaryInputValid[static_cast<size_t> (element)] = (pullResult == noErr);
        }
    }

    //==============================================================================

    // Builds one AudioBusBufferView per audio input bus. Element 0 (the main
    // input) is mapped from the passed buffer list; elements 1..n (auxiliary /
    // sidechain inputs) are mapped from their own pulled buffer lists. Views are
    // real-time safe: they reuse pre-allocated channel-pointer storage.
    void buildInputBusViews (const AudioBufferList& mainInBuffer)
    {
        renderViews.inputBusViews.clear();

        int chOffset = 0;
        int audioIdx = 0;

        for (const auto& bus : processor->getBusLayout().getInputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            const int numCh = bus.getNumChannels();
            auto* chPtrs = renderViews.inputChannelPtrStorage.data() + chOffset;

            std::fill (chPtrs, chPtrs + numCh, nullptr);

            if (audioIdx == 0)
            {
                const UInt32 copyCount = std::min (mainInBuffer.mNumberBuffers, static_cast<UInt32> (numCh));
                for (UInt32 ch = 0; ch < copyCount; ++ch)
                    chPtrs[ch] = static_cast<const float*> (mainInBuffer.mBuffers[ch].mData);
            }
            else if (auxiliaryInputValid[static_cast<size_t> (audioIdx)])
            {
                const auto& busBufferList = Input (static_cast<AudioUnitElement> (audioIdx)).GetBufferList();
                const UInt32 copyCount = std::min (busBufferList.mNumberBuffers, static_cast<UInt32> (numCh));
                for (UInt32 ch = 0; ch < copyCount; ++ch)
                    chPtrs[ch] = static_cast<const float*> (busBufferList.mBuffers[ch].mData);
            }

            renderViews.inputBusViews.emplace_back (chPtrs, numCh, bus.getRole());

            chOffset += numCh;
            ++audioIdx;
        }
    }

    // Builds one AudioBusBufferView per audio output bus from the output buffer
    // list, consuming channels sequentially across buses.
    void buildOutputBusViews (const AudioBufferList& outBuffer)
    {
        renderViews.outputBusViews.clear();

        int chOffset = 0;

        for (const auto& bus : processor->getBusLayout().getOutputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            const int numCh = bus.getNumChannels();
            auto* chPtrs = renderViews.outputChannelPtrStorage.data() + chOffset;

            std::fill (chPtrs, chPtrs + numCh, nullptr);

            const auto available = outBuffer.mNumberBuffers > static_cast<UInt32> (chOffset)
                                     ? static_cast<UInt32> (numCh)
                                     : 0u;
            const UInt32 copyCount = std::min (available, outBuffer.mNumberBuffers - static_cast<UInt32> (chOffset));

            for (UInt32 ch = 0; ch < copyCount; ++ch)
                chPtrs[ch] = static_cast<float*> (outBuffer.mBuffers[static_cast<UInt32> (chOffset) + ch].mData);

            renderViews.outputBusViews.emplace_back (chPtrs, numCh, bus.getRole());

            chOffset += numCh;
        }
    }

    // Effect: copy input to output and call processBlock
    OSStatus ProcessBufferLists (AudioUnitRenderActionFlags& ioActionFlags,
                                 const AudioBufferList& inBuffer,
                                 AudioBufferList& outBuffer,
                                 UInt32 inFramesToProcess) override
    {
        if (processor == nullptr)
            return kAudioUnitErr_NoConnection;

        const UInt32 numBuffers = std::min (inBuffer.mNumberBuffers, outBuffer.mNumberBuffers);

        audioChannels.clear();
        for (UInt32 ch = 0; ch < numBuffers; ++ch)
        {
            const auto* in = static_cast<const float*> (inBuffer.mBuffers[ch].mData);
            auto* out = static_cast<float*> (outBuffer.mBuffers[ch].mData);

            if (in != out)
                std::memcpy (out, in, inFramesToProcess * sizeof (float));

            audioChannels.push_back (out);
        }

        AudioSampleBuffer audioBuffer (audioChannels.data(),
                                       static_cast<int> (audioChannels.size()),
                                       0,
                                       static_cast<int> (inFramesToProcess));

        buildInputBusViews (inBuffer);
        buildOutputBusViews (outBuffer);

        AudioPluginPlayHeadAU playHead (*this, nullptr);
        std::unique_lock<std::mutex> parameterLock (parameterChangeMutex, std::try_to_lock);
        auto& processParamChangeBuffer = parameterLock.owns_lock() ? paramChangeBuffer : emptyParamChangeBuffer;

        AudioProcessContext<float> context { audioBuffer,
                                             midiBuffer,
                                             processParamChangeBuffer,
                                             &playHead,
                                             { renderViews.inputBusViews.data(), renderViews.inputBusViews.size() },
                                             { renderViews.outputBusViews.data(), renderViews.outputBusViews.size() } };
        processAudioBlock (*processor, context, bypass.isBypassed());
        midiBuffer.clear();
        processParamChangeBuffer.clear();

        return noErr;
    }
#endif

    //==============================================================================

    OSStatus SaveState (CFPropertyListRef* outData) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState requested");

        if (outData == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState failed: outData is null");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        const auto baseResult = AudioPluginAUBase::SaveState (outData);
        if (baseResult != noErr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState failed: base status=" << describeStatus (baseResult));
            return baseResult;
        }

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState completed without processor state: processor is null");
            return noErr;
        }

        MemoryBlock data;
        const auto result = processor->saveStateIntoMemory (data);
        if (result.failed())
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState completed without processor state: " << result.getErrorMessage());
            return noErr;
        }

        if (*outData != nullptr && CFGetTypeID (*outData) == CFDictionaryGetTypeID())
        {
            NSData* nsData = data.getSize() > 0
                               ? [NSData dataWithBytes:data.getData() length:data.getSize()]
                               : [NSData data];

            auto* stateDictionary = const_cast<CFMutableDictionaryRef> (static_cast<CFDictionaryRef> (*outData));
            CFDictionarySetValue (stateDictionary,
                                  getAUProcessorStateKey(),
                                  (__bridge CFDataRef) nsData);
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState completed with processor state: bytes=" << String (static_cast<int64> (data.getSize())));

        return noErr;
    }

    OSStatus RestoreState (CFPropertyListRef inData) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "RestoreState requested");

        if (inData == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "RestoreState failed: inData is null");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        CFDataRef processorState = nullptr;
        OSStatus baseResult = noErr;

        if (CFGetTypeID (inData) == CFDictionaryGetTypeID())
        {
            processorState = static_cast<CFDataRef> (CFDictionaryGetValue (static_cast<CFDictionaryRef> (inData),
                                                                           getAUProcessorStateKey()));

            if (processorState != nullptr && CFGetTypeID (processorState) != CFDataGetTypeID())
                return kAudioUnitErr_InvalidPropertyValue;

            baseResult = AudioPluginAUBase::RestoreState (inData);
            if (baseResult != noErr)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "RestoreState failed: base status=" << describeStatus (baseResult));
                return baseResult;
            }
        }
        else if (CFGetTypeID (inData) == CFDataGetTypeID())
        {
            processorState = static_cast<CFDataRef> (inData);
        }
        else
        {
            return kAudioUnitErr_InvalidPropertyValue;
        }

        if (processorState == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "RestoreState completed without processor state");
            return baseResult;
        }

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "RestoreState failed: processor is null");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        MemoryBlock data (CFDataGetBytePtr (processorState),
                          static_cast<size_t> (CFDataGetLength (processorState)));

        processor->suspendProcessing (true);
        const auto result = processor->loadStateFromMemory (data);
        const bool ok = result.wasOk();
        processor->suspendProcessing (false);

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "RestoreState " << String (ok ? "completed" : "failed") << ": bytes=" << String (static_cast<int64> (data.getSize())) << (ok ? String() : ", error=" + result.getErrorMessage()));

        return ok ? static_cast<OSStatus> (noErr)
                  : static_cast<OSStatus> (kAudioUnitErr_InvalidPropertyValue);
    }

    //==============================================================================

    OSStatus GetPresets (CFArrayRef* outData) const override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPresets requested");

        if (processor == nullptr || outData == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPresets failed: processor=" << describePointer (processor.get()) << ", outData=" << describePointer (outData));
            return kAudioUnitErr_InvalidPropertyValue;
        }

        const int numPresets = processor->getNumPresets();
        NSMutableArray* presetsArray = [[NSMutableArray alloc] initWithCapacity:numPresets];

        for (int i = 0; i < numPresets; ++i)
        {
            AUPreset preset;
            preset.presetNumber = i;
            preset.presetName = processor->getPresetName (i).toCFString();

            [presetsArray addObject:[NSValue valueWithBytes:&preset objCType:@encode (AUPreset)]];
            CFRelease (preset.presetName);
        }

        *outData = (__bridge_retained CFArrayRef) presetsArray;
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPresets returned " << String (numPresets) << " presets");
        return noErr;
    }

    OSStatus NewFactoryPresetSet (const AUPreset& inNewFactoryPreset) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "NewFactoryPresetSet requested: preset=" << String (static_cast<int> (inNewFactoryPreset.presetNumber)));

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "NewFactoryPresetSet failed: processor is null");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        if (! isPositiveAndBelow (static_cast<int> (inNewFactoryPreset.presetNumber), processor->getNumPresets()))
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "NewFactoryPresetSet failed: preset out of range");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        processor->setCurrentPreset (static_cast<int> (inNewFactoryPreset.presetNumber));
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "NewFactoryPresetSet completed");
        return noErr;
    }

    //==============================================================================

    OSStatus GetPropertyInfo (AudioUnitPropertyID inID,
                              AudioUnitScope inScope,
                              AudioUnitElement inElement,
                              UInt32& outDataSize,
                              bool& outWritable) override
    {
        if (inID == kAudioUnitProperty_OfflineRender)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPropertyInfo OfflineRender requested: " << describeScopeAndElement (inScope, inElement));

            if (inScope != kAudioUnitScope_Global)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPropertyInfo OfflineRender failed: invalid scope");
                return kAudioUnitErr_InvalidScope;
            }

            outDataSize = sizeof (UInt32);
            outWritable = true;
            return noErr;
        }

        if (inID == kAudioUnitProperty_BypassEffect)
        {
            if (inScope != kAudioUnitScope_Global)
                return kAudioUnitErr_InvalidScope;

            outDataSize = sizeof (UInt32);
            outWritable = true;
            return noErr;
        }

        if (inID == kAudioUnitProperty_CocoaUI)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPropertyInfo CocoaUI requested: " << describeScopeAndElement (inScope, inElement));

            if (inScope != kAudioUnitScope_Global)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPropertyInfo CocoaUI failed: invalid scope");
                return kAudioUnitErr_InvalidScope;
            }

            if (processor != nullptr && processor->hasEditor())
            {
                outDataSize = sizeof (AudioUnitCocoaViewInfo);
                outWritable = false;
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPropertyInfo CocoaUI available");
                return noErr;
            }

            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPropertyInfo CocoaUI not available: processor=" << describePointer (processor.get()) << ", hasEditor=" << String (processor != nullptr && processor->hasEditor() ? "true" : "false"));
            return kAudioUnitErr_PropertyNotInUse;
        }

        const auto result = AudioPluginAUBase::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
        if (result != noErr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetPropertyInfo delegated failed: property=" << String (static_cast<int> (inID)) << ", " << describeScopeAndElement (inScope, inElement) << ", status=" << describeStatus (result));
        }

        return result;
    }

    OSStatus GetProperty (AudioUnitPropertyID inID,
                          AudioUnitScope inScope,
                          AudioUnitElement inElement,
                          void* outData) override; // Implemented below (needs ObjC)

    OSStatus SetProperty (AudioUnitPropertyID inID,
                          AudioUnitScope inScope,
                          AudioUnitElement inElement,
                          const void* inData,
                          UInt32 inDataSize) override
    {
        if (inID == kAudioUnitProperty_OfflineRender)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SetProperty OfflineRender requested: " << describeScopeAndElement (inScope, inElement));

            if (inScope != kAudioUnitScope_Global)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SetProperty OfflineRender failed: invalid scope");
                return kAudioUnitErr_InvalidScope;
            }

            if (inData == nullptr || inDataSize < sizeof (UInt32))
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SetProperty OfflineRender failed: inData=" << describePointer (inData) << ", inDataSize=" << String (static_cast<int> (inDataSize)));
                return kAudioUnitErr_InvalidPropertyValue;
            }

            renderingOffline = *static_cast<const UInt32*> (inData) != 0;
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "Offline rendering set to " << String (renderingOffline ? "true" : "false"));

            if (processor != nullptr)
                processor->setOfflineProcessing (renderingOffline);

            return noErr;
        }

        if (inID == kAudioUnitProperty_BypassEffect)
        {
            if (inScope != kAudioUnitScope_Global)
                return kAudioUnitErr_InvalidScope;

            if (inData == nullptr || inDataSize < sizeof (UInt32))
                return kAudioUnitErr_InvalidPropertyValue;

            bypass.setBypassed (*static_cast<const UInt32*> (inData) != 0);
            return noErr;
        }

        const auto result = AudioPluginAUBase::SetProperty (inID, inScope, inElement, inData, inDataSize);
        if (result != noErr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SetProperty delegated failed: property=" << String (static_cast<int> (inID)) << ", " << describeScopeAndElement (inScope, inElement) << ", status=" << describeStatus (result));
        }

        return result;
    }

    //==============================================================================

    AudioProcessor* getProcessor() const { return processor.get(); }

    void ensureWindowingInitialised() { windowing.ensureInitialised(); }

    void registerEditorView (AudioPluginEditorViewAU* view)
    {
        if (view == nullptr)
            return;

        std::lock_guard<std::mutex> lock (editorViewsMutex);
        editorViews.push_back (view);
    }

    void unregisterEditorView (AudioPluginEditorViewAU* view)
    {
        if (view == nullptr)
            return;

        std::lock_guard<std::mutex> lock (editorViewsMutex);
        editorViews.erase (std::remove (editorViews.begin(), editorViews.end(), view), editorViews.end());
    }

    void closeEditorViews();

    static AudioPluginProcessorAU* findInstance (AudioUnit component)
    {
        std::lock_guard<std::mutex> lock (getInstanceRegistryMutex());

        const auto iter = getInstanceRegistry().find (component);
        auto* instance = iter != getInstanceRegistry().end() ? iter->second : nullptr;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "lookup instance: component=" << describePointer (component) << ", instance=" << describePointer (instance) << ", registeredInstances=" << String (static_cast<int> (getInstanceRegistry().size())));

        return instance;
    }

private:
    static std::mutex& getInstanceRegistryMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::unordered_map<AudioUnit, AudioPluginProcessorAU*>& getInstanceRegistry()
    {
        static std::unordered_map<AudioUnit, AudioPluginProcessorAU*> instances;
        return instances;
    }

    static void registerInstance (AudioUnit component, AudioPluginProcessorAU* instance)
    {
        std::lock_guard<std::mutex> lock (getInstanceRegistryMutex());

        getInstanceRegistry()[component] = instance;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "registered instance: component=" << describePointer (component) << ", instance=" << describePointer (instance) << ", registeredInstances=" << String (static_cast<int> (getInstanceRegistry().size())));
    }

    static void unregisterInstance (AudioUnit component)
    {
        std::lock_guard<std::mutex> lock (getInstanceRegistryMutex());

        const auto numRemoved = getInstanceRegistry().erase (component);

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "unregistered instance: component=" << describePointer (component) << ", removed=" << String (static_cast<int> (numRemoved)) << ", registeredInstances=" << String (static_cast<int> (getInstanceRegistry().size())));
    }

    void addParameterListeners()
    {
        removeParameterListeners();

        if (processor == nullptr)
            return;

        for (const auto& parameter : processor->getParameters())
        {
            parameter->addListener (this);
            listenedParameters.push_back (parameter);
        }
    }

    void removeParameterListeners()
    {
        for (auto& parameter : listenedParameters)
            parameter->removeListener (this);

        listenedParameters.clear();
    }

    bool isValidProcessorParameterIndex (int indexInContainer) const
    {
        return processor != nullptr
            && isPositiveAndBelow (indexInContainer, static_cast<int> (processor->getParameters().size()));
    }

    void parameterValueChanged (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        if (! isValidProcessorParameterIndex (indexInContainer) || parameter->isReadOnly())
            return;

        AudioPluginAUBase::SetParameter (static_cast<AudioUnitParameterID> (parameter->getHostParameterID()),
                                         kAudioUnitScope_Global,
                                         0,
                                         static_cast<AudioUnitParameterValue> (parameter->getValue()),
                                         0);

        if (parameter->isBypass())
            PropertyChanged (kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0);
    }

    void parameterGestureBegin (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        ignoreUnused (parameter, indexInContainer);
    }

    void parameterGestureEnd (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        ignoreUnused (parameter, indexInContainer);
    }

    Float64 getCurrentSampleRate()
    {
        return Output (0).GetStreamFormat().mSampleRate;
    }

    ScopedYupInitialiser_GUI scopeInitialiser;
    WrapperWindowing windowing;
    std::unique_ptr<AudioProcessor> processor;

    MidiBuffer midiBuffer;
    MidiBuffer renderMidiBuffer;
    ParameterChangeBuffer paramChangeBuffer;
    ParameterChangeBuffer emptyParamChangeBuffer;
    std::mutex midiMutex;
    std::mutex parameterChangeMutex;
    std::mutex editorViewsMutex;
    std::vector<AUChannelInfo> channelInfoCache;
    std::vector<AudioParameter::Ptr> listenedParameters;
    std::vector<float*> audioChannels;
    std::vector<AudioPluginEditorViewAU*> editorViews;

    // Per-bus view state, pre-allocated in Initialize so the render callback
    // only clears and refills the vectors without allocating
    AudioPluginAURenderViews renderViews;
    std::vector<uint8_t> auxiliaryInputValid; // per-element pull success (effect path)
    AudioUnit componentInstance = nullptr;
    bool renderingOffline = false;
    WrapperBypass bypass;
};

} // namespace yup

//==============================================================================
// Objective-C editor view

namespace yup
{

/** The component placed on the desktop for the host view, holding the editor and its resizable corner on top. */
class AudioPluginEditorAU final
    : public Component
    , private ComponentListener
{
public:
    explicit AudioPluginEditorAU (std::unique_ptr<AudioProcessorEditor> editorToWrap)
        : processorEditor (std::move (editorToWrap))
    {
        jassert (processorEditor != nullptr);

        addAndMakeVisible (*processorEditor);

        if (processorEditor->shouldShowResizableCorner())
        {
            resizableCorner = std::make_unique<ResizableCorner>();
            resizableCorner->constrainSize = [this] (Size<float> size)
            {
                return processorEditor->getConstrainedSize (size.to<int>()).to<float>();
            };
            addAndMakeVisible (*resizableCorner);
        }

        processorEditor->addComponentListener (this);
    }

    ~AudioPluginEditorAU() override
    {
        processorEditor->removeComponentListener (this);
        resizableCorner.reset();
        removeChildComponent (processorEditor.get());
    }

    AudioProcessorEditor& getEditor() const noexcept { return *processorEditor; }

    void resized() override
    {
        processorEditor->setBounds (getLocalBounds());
    }

    void contentScaleChanged (float dpiScale) override
    {
        processorEditor->contentScaleChanged (dpiScale);
    }

private:
    void componentResized (Component& component) override
    {
        const auto editorSize = component.getBounds().getSize();
        if (editorSize != getBounds().getSize())
            setSize (editorSize);
    }

    std::unique_ptr<AudioProcessorEditor> processorEditor;
    std::unique_ptr<ResizableCorner> resizableCorner;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginEditorAU)
};

//==============================================================================
/** The state behind the NSView handed to the host, which owns it through an ivar.

    The NSView class and the view factory class are registered at runtime under unique names, so
    every plugin binary embedding YUP owns distinct Objective-C classes and hosts never resolve
    them from another plugin loaded in the same process. */
class AudioPluginEditorViewAU final : public ComponentListener
{
public:
    AudioPluginEditorViewAU (NSView* view, AudioPluginProcessorAU* processorWrapper)
        : view (view)
        , processorWrapper (processorWrapper)
        , processor (processorWrapper != nullptr ? processorWrapper->getProcessor() : nullptr)
    {
    }

    ~AudioPluginEditorViewAU() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "destroying editor view: view=" << describePointer ((__bridge void*) view) << ", editor=" << describePointer (editorComponent.get()) << ", processor=" << describePointer (processor));

        closeEditorIfNeeded();

        if (processorWrapper != nullptr)
            processorWrapper->unregisterEditorView (this);
    }

    void initialise()
    {
        [view setPostsFrameChangedNotifications:YES];

        if (processorWrapper != nullptr)
            processorWrapper->registerEditorView (this);

        if (processor == nullptr || ! processor->hasEditor())
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "processor has no editor");
            return;
        }

        processorWrapper->ensureWindowingInitialised();

        std::unique_ptr<AudioProcessorEditor> processorEditor (processor->createEditor());

        if (processorEditor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "processor returned null editor");
            return;
        }

        const auto preferredSize = processorEditor->getPreferredSize();
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "created editor: preferredWidth=" << String (preferredSize.getWidth()) << ", preferredHeight=" << String (preferredSize.getHeight()) << ", editor=" << describePointer (processorEditor.get()));

        [view setAutoresizingMask:processorEditor->isResizable() ? (NSViewWidthSizable | NSViewHeightSizable) : NSViewNotSizable];

        editorComponent = std::make_unique<AudioPluginEditorAU> (std::move (processorEditor));
        editorComponent->addComponentListener (this);

        [view setFrameSize:NSMakeSize (preferredSize.getWidth(), preferredSize.getHeight())];
        resizeEditorToBounds();
    }

    void viewDidMoveToWindow()
    {
        if ([view window] != nil)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view moved to window: view=" << describePointer ((__bridge void*) view) << ", window=" << describePointer ((__bridge void*) [view window]) << ", contentView=" << describePointer ((__bridge void*) [[view window] contentView]));

            attachEditorIfNeeded();
            observeHostView();
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view removed from window: view=" << describePointer ((__bridge void*) view));

            stopObservingHostView();
            detachEditorIfNeeded();
        }
    }

    void closeEditorForProcessorDestruction()
    {
        closeEditorIfNeeded();

        processorWrapper = nullptr;
        processor = nullptr;
    }

    void resizeEditorToBounds()
    {
        if (editorComponent == nullptr)
            return;

        const auto bounds = [view bounds];

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "resizing editor to bounds: width=" << String (static_cast<double> (NSWidth (bounds))) << ", height=" << String (static_cast<double> (NSHeight (bounds))) << ", editor=" << describePointer (editorComponent.get()));

        const ScopedValueSetter<bool> scoped (resizingEditorToBounds, true);

        editorComponent->setSize ({ jmax (1.0f, static_cast<float> (NSWidth (bounds))),
                                    jmax (1.0f, static_cast<float> (NSHeight (bounds))) });
    }

    NSSize getConstrainedFrameSize (NSSize size) const
    {
        if (editorComponent == nullptr)
            return size;

        const auto constrained = editorComponent->getEditor().getConstrainedSizeToFit ({ static_cast<int> (size.width), static_cast<int> (size.height) });
        return NSMakeSize (constrained.getWidth(), constrained.getHeight());
    }

    void componentResized (Component&) override
    {
        if (! resizingEditorToBounds)
            resizeViewToEditorSize();
    }

private:
    void attachEditorIfNeeded()
    {
        if (editorComponent == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "attachEditorIfNeeded skipped: editor is null");
            return;
        }

        auto& processorEditor = editorComponent->getEditor();

        if (editorComponent->isOnDesktop())
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "attachEditorIfNeeded skipped: editor is already on desktop");
            return;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "attaching editor to native view: editor=" << describePointer (editorComponent.get()) << ", view=" << describePointer ((__bridge void*) view) << ", window=" << describePointer ((__bridge void*) [view window]));

        resizeEditorToBounds();

        ComponentNative::Flags flags = ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow;

        if (processorEditor.shouldRenderContinuous())
            flags.set (ComponentNative::renderContinuous);

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor native options: renderContinuous=" << String (processorEditor.shouldRenderContinuous() ? "true" : "false") << ", resizable=" << String (processorEditor.isResizable() ? "true" : "false"));

        auto options = ComponentNative::Options()
                           .withFlags (flags)
                           .withResizableWindow (processorEditor.isResizable());

        editorComponent->addToDesktop (options, (__bridge void*) view);
        editorComponent->setVisible (true);
        processorEditor.attachedToNative();

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor attached to native view: isOnDesktop=" << String (editorComponent->isOnDesktop() ? "true" : "false"));
    }

    void detachEditorIfNeeded()
    {
        if (editorComponent == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "detachEditorIfNeeded skipped: editor is null");
            return;
        }

        if (! editorComponent->isOnDesktop())
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "detachEditorIfNeeded skipped: editor is not on desktop");
            return;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "detaching editor from native view: editor=" << describePointer (editorComponent.get()) << ", view=" << describePointer ((__bridge void*) view) << ", window=" << describePointer ((__bridge void*) [view window]));

        endActiveParameterGestures (processor);
        editorComponent->setVisible (false);
        editorComponent->removeFromDesktop();

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor detached from native view: isOnDesktop=" << String (editorComponent->isOnDesktop() ? "true" : "false"));
    }

    /* Some hosts (Reaper) resize the container view on window edge drags without resizing the plugin
       view, so the plugin view follows its superview size like VST3 and CLAP editors follow onSize. */
    void observeHostView()
    {
        stopObservingHostView();

        NSView* hostView = [view superview];
        if (hostView == nil || editorComponent == nullptr || ! editorComponent->getEditor().isResizable())
            return;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "observing host view: hostView=" << describePointer ((__bridge void*) hostView) << ", class=" << String ([NSStringFromClass ([hostView class]) UTF8String]) << ", frame=" << String ([NSStringFromRect ([hostView frame]) UTF8String]) << ", viewFrame=" << String ([NSStringFromRect ([view frame]) UTF8String]) << ", flipped=" << String ([hostView isFlipped] ? "true" : "false") << ", autoresizesSubviews=" << String ([hostView autoresizesSubviews] ? "true" : "false"));

        observedHostView = hostView;
        lastHostViewSize = [hostView bounds].size;

        [hostView setPostsFrameChangedNotifications:YES];

        hostViewObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSViewFrameDidChangeNotification
                                                                             object:hostView
                                                                              queue:nil
                                                                         usingBlock:^(NSNotification*)
                                                                         {
                                                                             hostViewResized();
                                                                         }];
    }

    void stopObservingHostView()
    {
        hostResizeSnapper.cancel();

        if (hostViewObserver != nil)
            [[NSNotificationCenter defaultCenter] removeObserver:hostViewObserver];

        hostViewObserver = nil;
        observedHostView = nil;
    }

    void hostViewResized()
    {
        if (observedHostView == nil || editorComponent == nullptr)
            return;

        const auto hostBounds = [observedHostView bounds];
        if (NSEqualSizes (hostBounds.size, lastHostViewSize))
            return;

        lastHostViewSize = hostBounds.size;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "host view resized: size=" << String ([NSStringFromSize (hostBounds.size) UTF8String]) << ", viewFrame=" << String ([NSStringFromRect ([view frame]) UTF8String]) << ", resizingViewToEditor=" << String (resizingViewToEditor ? "true" : "false"));

        // The host adapting its container to a plugin driven resize, nothing to follow
        if (resizingViewToEditor)
            return;

        fitEditorInsideHostView (getAvailableHostViewSize());
        hostResizeSnapper.hostResized();
    }

    /* Hosts like Waveform place the plugin view inside their container with a border and a title bar, and
       size the container to the view plus those margins. Filling the whole container would make the host grow
       it again by the margins, forever. The view origin gives the leading margins, the trailing ones are
       assumed to match the side margin. */
    NSSize getAvailableHostViewSize() const
    {
        const auto hostSize = [observedHostView bounds].size;
        const auto frame = [view frame];

        const auto sideMargin = jmax (0.0, static_cast<double> (NSMinX (frame)));
        const auto verticalMargins = [observedHostView isFlipped] ? jmax (0.0, static_cast<double> (NSMinY (frame))) + sideMargin : 0.0;

        return NSMakeSize (jmax (1.0, hostSize.width - sideMargin * 2.0),
                           jmax (1.0, hostSize.height - verticalMargins));
    }

    /* Hosts echo plugin view frame changes back to their window, so constraining the view while the user
       drags the window edges makes the window jump between the dragged and the constrained size. The view
       keeps matching the host view, and only the editor inside it is constrained until the drag ends. */
    void fitEditorInsideHostView (NSSize availableSize)
    {
        if (! NSEqualSizes (availableSize, [view frame].size))
        {
            const ScopedValueSetter<bool> scoped (resizingViewToEditor, true);
            ObjCClass<NSView>::sendSuperclassMessage<void> (view, @selector (setFrameSize:), availableSize);
            alignToHostViewTop();
        }

        const auto size = editorComponent->getEditor().getConstrainedSizeToFit ({ static_cast<int> (availableSize.width), static_cast<int> (availableSize.height) });

        const ScopedValueSetter<bool> scoped (resizingEditorToBounds, true);
        editorComponent->setSize (size.to<float>());
    }

    void snapToConstrainedSize()
    {
        if (observedHostView == nil || editorComponent == nullptr)
            return;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "snapping to constrained size: hostSize=" << String ([NSStringFromSize ([observedHostView bounds].size) UTF8String]) << ", availableSize=" << String ([NSStringFromSize (getAvailableHostViewSize()) UTF8String]) << ", viewFrame=" << String ([NSStringFromRect ([view frame]) UTF8String]));

        // Our setFrameSize: override constrains the size, the host then fits its window to the view
        [view setFrameSize:getAvailableHostViewSize()];
        alignToHostViewTop();
    }

    void alignToHostViewTop()
    {
        if (observedHostView == nil || [observedHostView isFlipped])
            return;

        [view setFrameOrigin:NSMakePoint (NSMinX ([view frame]), NSHeight ([observedHostView bounds]) - NSHeight ([view frame]))];
    }

    void closeEditorIfNeeded()
    {
        stopObservingHostView();
        detachEditorIfNeeded();

        endActiveParameterGestures (processor);

        if (editorComponent != nullptr)
            editorComponent->removeComponentListener (this);

        editorComponent.reset();
    }

    void resizeViewToEditorSize()
    {
        if (editorComponent == nullptr || ! editorComponent->getEditor().isResizable())
            return;

        const auto newSize = NSMakeSize (jmax (1.0f, editorComponent->getWidth()),
                                         jmax (1.0f, editorComponent->getHeight()));

        if (NSEqualSizes ([view frame].size, newSize))
            return;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "resizing editor view to editor: width=" << String (static_cast<double> (newSize.width)) << ", height=" << String (static_cast<double> (newSize.height)) << ", editor=" << describePointer (editorComponent.get()));

        const ScopedValueSetter<bool> scoped (resizingViewToEditor, true);

        // Skip our own override, which would resize the editor back to the view
        ObjCClass<NSView>::sendSuperclassMessage<void> (view, @selector (setFrameSize:), newSize);

        if (observedHostView != nil)
            lastHostViewSize = [observedHostView bounds].size;
    }

    friend struct AudioPluginEditorViewAUClass;

    __unsafe_unretained NSView* view = nil;
    AudioPluginProcessorAU* processorWrapper = nullptr;
    AudioProcessor* processor = nullptr;
    std::unique_ptr<AudioPluginEditorAU> editorComponent;
    __weak NSView* observedHostView = nil;
    id hostViewObserver = nil;
    NSSize lastHostViewSize = NSZeroSize;
    bool resizingEditorToBounds = false;
    bool resizingViewToEditor = false;
    HostResizeSnapper hostResizeSnapper { [this]
                                          {
                                              snapToConstrainedSize();
                                          } };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginEditorViewAU)
};

//==============================================================================
struct AudioPluginEditorViewAUClass final : public ObjCClass<NSView>
{
    AudioPluginEditorViewAUClass()
        : ObjCClass<NSView> ("YUPAudioPluginEditorViewAU_")
    {
        addIvar<AudioPluginEditorViewAU*> ("state");

        addMethod (@selector (viewDidMoveToWindow), [] (id self, SEL)
        {
            sendSuperclassMessage<void> (self, @selector (viewDidMoveToWindow));

            if (auto* state = getState (self))
                state->viewDidMoveToWindow();
        });

        addMethod (@selector (setFrameSize:), [] (id self, SEL, NSSize newSize)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view frame size changed: width=" << String (static_cast<double> (newSize.width)) << ", height=" << String (static_cast<double> (newSize.height)));

            auto* state = getState (self);

            if (state != nullptr)
                newSize = state->getConstrainedFrameSize (newSize);

            sendSuperclassMessage<void> (self, @selector (setFrameSize:), newSize);

            if (state != nullptr)
                state->resizeEditorToBounds();
        });

        addMethod (sel_registerName ("dealloc"), [] (__unsafe_unretained id self, SEL)
        {
            delete getState (self);
            setIvar<AudioPluginEditorViewAU*> (self, "state", nullptr);

            sendSuperclassMessage<void> (self, sel_registerName ("dealloc"));
        });

        registerClass();
    }

    static NSView* create (AudioPluginProcessorAU* processorWrapper, NSSize size)
    {
        static AudioPluginEditorViewAUClass viewClass;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "creating editor view: requestedWidth=" << String (static_cast<double> (size.width)) << ", requestedHeight=" << String (static_cast<double> (size.height)) << ", wrapper=" << describePointer (processorWrapper));

        NSView* view = [viewClass.createInstance() initWithFrame:NSMakeRect (0, 0, size.width, size.height)];

        if (view == nil)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view initialisation failed");
            return nil;
        }

        auto* state = new AudioPluginEditorViewAU (view, processorWrapper);
        setIvar (view, "state", state);
        state->initialise();

        return view;
    }

    static AudioPluginEditorViewAU* getState (id self) { return getIvar<AudioPluginEditorViewAU*> (self, "state"); }
};

void AudioPluginProcessorAU::closeEditorViews()
{
    std::vector<AudioPluginEditorViewAU*> viewsToClose;

    {
        std::lock_guard<std::mutex> lock (editorViewsMutex);
        viewsToClose.swap (editorViews);
    }

    for (auto* view : viewsToClose)
        if (view != nullptr)
            view->closeEditorForProcessorDestruction();
}

} // namespace yup

//==============================================================================
// Cocoa view factory

/* Hosts resolve the factory with [NSBundle classNamed:], which only finds classes compiled into
   the bundle image, so this can't be a runtime class. The build system gives it a name unique to
   the plugin and its version, so different plugins never clash in the same process. */
#ifndef YupPlugin_AUViewFactoryClass
#error "YupPlugin_AUViewFactoryClass must be defined by the build system"
#endif

@interface YupPlugin_AUViewFactoryClass : NSObject <AUCocoaUIBase>
@end

@implementation YupPlugin_AUViewFactoryClass

- (unsigned) interfaceVersion
{
    return 0;
}

- (NSString*) description
{
    return @YupPlugin_Name;
}

- (NSView*) uiViewForAudioUnit:(AudioUnit) inAudioUnit withSize:(NSSize) inPreferredSize
{
    using namespace yup;

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "view factory requested editor view: audioUnit=" << describePointer (inAudioUnit) << ", preferredWidth=" << String (static_cast<double> (inPreferredSize.width)) << ", preferredHeight=" << String (static_cast<double> (inPreferredSize.height)));

    auto* proc = AudioPluginProcessorAU::findInstance (inAudioUnit);
    if (proc == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "view factory failed: AU instance not found");
        return nil;
    }

    if (proc->getProcessor() == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "view factory failed: processor is null");
        return nil;
    }

    return AudioPluginEditorViewAUClass::create (proc, inPreferredSize);
}

@end

//==============================================================================
// GetProperty implementation (needs ObjC)

namespace yup
{

OSStatus AudioPluginProcessorAU::GetProperty (AudioUnitPropertyID inID,
                                              AudioUnitScope inScope,
                                              AudioUnitElement inElement,
                                              void* outData)
{
    if (inID == kAudioUnitProperty_OfflineRender)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty OfflineRender requested: " << describeScopeAndElement (inScope, inElement));

        if (inScope != kAudioUnitScope_Global)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty OfflineRender failed: invalid scope");
            return kAudioUnitErr_InvalidScope;
        }

        if (outData == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty OfflineRender failed: outData is null");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        *static_cast<UInt32*> (outData) = renderingOffline ? 1u : 0u;
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty OfflineRender returned " << String (renderingOffline ? "true" : "false"));
        return noErr;
    }

    if (inID == kAudioUnitProperty_BypassEffect)
    {
        if (inScope != kAudioUnitScope_Global)
            return kAudioUnitErr_InvalidScope;

        if (outData == nullptr)
            return kAudioUnitErr_InvalidPropertyValue;

        *static_cast<UInt32*> (outData) = bypass.isBypassed() ? 1u : 0u;
        return noErr;
    }

    if (inID == kAudioUnitProperty_CocoaUI)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty CocoaUI requested: " << describeScopeAndElement (inScope, inElement));

        if (inScope != kAudioUnitScope_Global)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty CocoaUI failed: invalid scope");
            return kAudioUnitErr_InvalidScope;
        }

        if (processor == nullptr || ! processor->hasEditor())
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty CocoaUI failed: editor not available");
            return kAudioUnitErr_PropertyNotInUse;
        }

        if (outData == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty CocoaUI failed: outData is null");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        auto* info = static_cast<AudioUnitCocoaViewInfo*> (outData);

        // The bundle location is this plugin's own bundle
        NSBundle* bundle = [NSBundle bundleForClass:[YupPlugin_AUViewFactoryClass class]];
        if (bundle == nil)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty CocoaUI failed: bundle is nil");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        auto* bundleLocation = (__bridge_retained CFURLRef)[bundle bundleURL];
        auto* viewClass = (__bridge_retained CFStringRef) NSStringFromClass ([YupPlugin_AUViewFactoryClass class]);

        if (bundleLocation == nullptr || viewClass == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty CocoaUI failed: bundleURL=" << describePointer (bundleLocation) << ", viewClass=" << describePointer (viewClass));

            if (bundleLocation != nullptr)
                CFRelease (bundleLocation);

            if (viewClass != nullptr)
                CFRelease (viewClass);

            return kAudioUnitErr_InvalidPropertyValue;
        }

        info->mCocoaAUViewBundleLocation = bundleLocation;
        info->mCocoaAUViewClass[0] = viewClass;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty CocoaUI returned view factory: bundle=" << String::fromCFString ((__bridge CFStringRef)[[bundle bundleURL] absoluteString]));

        return noErr;
    }

    const auto result = AudioPluginAUBase::GetProperty (inID, inScope, inElement, outData);
    if (result != noErr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty delegated failed: property=" << String (static_cast<int> (inID)) << ", " << describeScopeAndElement (inScope, inElement) << ", status=" << describeStatus (result));
    }

    return result;
}

} // namespace yup

//==============================================================================
// Factory entry point

#if YupPlugin_IsSynth
using AudioPluginProcessorAU = yup::AudioPluginProcessorAU;
AUSDK_COMPONENT_ENTRY (ausdk::AUMusicDeviceFactory, AudioPluginProcessorAU)
#else
using AudioPluginProcessorAU = yup::AudioPluginProcessorAU;
AUSDK_COMPONENT_ENTRY (ausdk::AUBaseProcessFactory, AudioPluginProcessorAU)
#endif

#endif // YUP_MAC
