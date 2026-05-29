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

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

//==============================================================================

extern "C" yup::AudioProcessor* createPluginProcessor();

@class AudioPluginEditorViewAU;

namespace yup
{

static String describeScopeAndElement (AudioUnitScope scope, AudioUnitElement element)
{
    return "scope=" + String (static_cast<int> (scope)) + ", element=" + String (static_cast<int> (element));
}

static String describePointer (const void* value)
{
    return "0x" + String::toHexString (static_cast<int64> (reinterpret_cast<uintptr_t> (value)));
}

static String describeStatus (OSStatus status)
{
    return String (static_cast<int> (status));
}

//==============================================================================

namespace
{

//==============================================================================

static CFStringRef getProcessorStateKey()
{
    return CFSTR ("YUPProcessorState");
}

//==============================================================================

struct AUScopedYupInitialiser
{
    AUScopedYupInitialiser()
    {
        if (numAUScopedInitInstances.fetch_add (1) == 0)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "initialising YUP GUI");
            initialiseYup_GUI();
        }
    }

    ~AUScopedYupInitialiser()
    {
        if (numAUScopedInitInstances.fetch_sub (1) == 1)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "shutting down YUP GUI");
            shutdownYup_GUI();
        }
    }

private:
    static std::atomic_int numAUScopedInitInstances;
};

std::atomic_int AUScopedYupInitialiser::numAUScopedInitInstances = 0;

struct AUScopedYupWindowingInitialiser
{
    AUScopedYupWindowingInitialiser()
    {
        if (numAUScopedInitInstances.fetch_add (1) == 0)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "initialising YUP windowing for editor");
            initialiseYup_Windowing();
        }
    }

    ~AUScopedYupWindowingInitialiser()
    {
        if (numAUScopedInitInstances.fetch_sub (1) == 1)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "shutting down YUP windowing for editor");
            shutdownYup_Windowing();
        }
    }

private:
    static std::atomic_int numAUScopedInitInstances;
};

std::atomic_int AUScopedYupWindowingInitialiser::numAUScopedInitInstances = 0;

//==============================================================================

static OSType osTypeFromString (const char* s)
{
    if (s == nullptr || std::strlen (s) < 4)
        return 0;

    return static_cast<OSType> (
        (static_cast<uint32_t> (static_cast<uint8_t> (s[0])) << 24) | (static_cast<uint32_t> (static_cast<uint8_t> (s[1])) << 16) | (static_cast<uint32_t> (static_cast<uint8_t> (s[2])) << 8) | static_cast<uint32_t> (static_cast<uint8_t> (s[3])));
}

} // namespace

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
        processor.reset (::createPluginProcessor());

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "created processor instance: wrapper=" << yup::describePointer (this) << ", component=" << yup::describePointer (componentInstance) << ", processor=" << yup::describePointer (processor.get()));

        if (processor == nullptr)
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "createPluginProcessor returned null");

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

        processor->setOfflineProcessing (renderingOffline);
        processor->setPlaybackConfiguration (static_cast<float> (getCurrentSampleRate()),
                                             static_cast<int> (GetMaxFramesPerSlice()));

        midiBuffer.ensureSize (4096);
        midiBuffer.clear();
        emptyMidiBuffer.ensureSize (4096);
        emptyMidiBuffer.clear();
        paramChangeBuffer.reserve (getDefaultParameterChangeCapacity (*processor));
        emptyParamChangeBuffer.reserve (getDefaultParameterChangeCapacity (*processor));
        audioChannels.reserve (static_cast<size_t> (getTotalAudioOutputChannels (*processor)));

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

        outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_HasCFNameString;

        if (! param->isReadOnly())
            outParameterInfo.flags |= kAudioUnitParameterFlag_IsWritable;

        outParameterInfo.cfNameString = param->getName().toCFString();
        param->getName().copyToUTF8 (outParameterInfo.name, sizeof (outParameterInfo.name));

        outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
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
            std::unique_lock<std::mutex> lock (midiMutex, std::try_to_lock);
            auto& processMidiBuffer = lock.owns_lock() ? midiBuffer : emptyMidiBuffer;
            std::unique_lock<std::mutex> parameterLock (parameterChangeMutex, std::try_to_lock);
            auto& processParamChangeBuffer = parameterLock.owns_lock() ? paramChangeBuffer : emptyParamChangeBuffer;

            AudioProcessContext<float> context { audioBuffer,
                                                 processMidiBuffer,
                                                 processParamChangeBuffer,
                                                 &playHead };
            processAudioBlock (*processor, context, isBypassed);

            processMidiBuffer.clear();
            processParamChangeBuffer.clear();
        }

        return noErr;
    }

    //==============================================================================

    OSStatus HandleMIDIEvent (UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 offsetSampleFrame) override
    {
        std::unique_lock<std::mutex> lock (midiMutex, std::try_to_lock);
        if (! lock.owns_lock())
            return noErr;

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
        std::unique_lock<std::mutex> lock (midiMutex, std::try_to_lock);
        if (! lock.owns_lock())
            return noErr;

        if (inData != nullptr && inLength > 0)
            midiBuffer.addEvent (inData, static_cast<int> (inLength), 0);

        return noErr;
    }

#else
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

        AudioPluginPlayHeadAU playHead (*this, nullptr);
        std::unique_lock<std::mutex> parameterLock (parameterChangeMutex, std::try_to_lock);
        auto& processParamChangeBuffer = parameterLock.owns_lock() ? paramChangeBuffer : emptyParamChangeBuffer;

        AudioProcessContext<float> context { audioBuffer,
                                             midiBuffer,
                                             processParamChangeBuffer,
                                             &playHead };
        processAudioBlock (*processor, context, isBypassed);
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
                                  getProcessorStateKey(),
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
                                                                           getProcessorStateKey()));

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

            isBypassed = *static_cast<const UInt32*> (inData) != 0;
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

    void registerEditorView (AudioPluginEditorViewAU* view)
    {
        if (view == nil)
            return;

        std::lock_guard<std::mutex> lock (editorViewsMutex);
        editorViews.push_back ((__bridge void*) view);
    }

    void unregisterEditorView (AudioPluginEditorViewAU* view)
    {
        if (view == nil)
            return;

        std::lock_guard<std::mutex> lock (editorViewsMutex);
        editorViews.erase (std::remove (editorViews.begin(), editorViews.end(), (__bridge void*) view), editorViews.end());
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
    ScopedYupInitialiser_Windowing scopeWindowingInitialiser;
    std::unique_ptr<AudioProcessor> processor;

    MidiBuffer midiBuffer;
    MidiBuffer emptyMidiBuffer;
    ParameterChangeBuffer paramChangeBuffer;
    ParameterChangeBuffer emptyParamChangeBuffer;
    std::mutex midiMutex;
    std::mutex parameterChangeMutex;
    std::mutex editorViewsMutex;
    std::vector<AUChannelInfo> channelInfoCache;
    std::vector<AudioParameter::Ptr> listenedParameters;
    std::vector<float*> audioChannels;
    std::vector<void*> editorViews;
    AudioUnit componentInstance = nullptr;
    bool renderingOffline = false;
    bool isBypassed = false;
};

} // namespace yup

//==============================================================================
// Objective-C editor view

namespace yup
{

class AudioPluginEditorViewAUListener final : public ComponentListener
{
public:
    explicit AudioPluginEditorViewAUListener (AudioPluginEditorViewAU* owner)
        : owner (owner)
    {
    }

    void componentResized (Component& component) override;

private:
    AudioPluginEditorViewAU* owner = nil;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginEditorViewAUListener)
};

} // namespace yup

@interface AudioPluginEditorViewAU : NSView
{
    yup::AudioPluginProcessorAU* _processorWrapper;
    yup::AudioProcessor* _processor;
    std::unique_ptr<yup::AudioProcessorEditor> _processorEditor;
    std::unique_ptr<yup::AudioPluginEditorViewAUListener> _processorEditorListener;
    bool _resizingEditorToBounds;
}
- (instancetype)initWithAudioUnitWrapper:(yup::AudioPluginProcessorAU*)processorWrapper
                           preferredSize:(NSSize)size;
- (void)attachEditorIfNeeded;
- (void)detachEditorIfNeeded;
- (void)closeEditorIfNeeded;
- (void)closeEditorForProcessorDestruction;
- (void)resizeEditorToBounds;
- (void)resizeViewToEditorSize;
- (void)processorEditorResized;
@end

@implementation AudioPluginEditorViewAU

- (instancetype)initWithAudioUnitWrapper:(yup::AudioPluginProcessorAU*)processorWrapper
                           preferredSize:(NSSize)size
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "creating editor view: requestedWidth=" << yup::String (static_cast<double> (size.width)) << ", requestedHeight=" << yup::String (static_cast<double> (size.height)) << ", wrapper=" << yup::describePointer (processorWrapper) << ", view=" << yup::describePointer ((__bridge void*) self));

    if ((self = [super initWithFrame:NSMakeRect (0, 0, size.width, size.height)]))
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view initialised: view=" << yup::describePointer ((__bridge void*) self));
        _processorWrapper = processorWrapper;
        _processor = processorWrapper != nullptr ? processorWrapper->getProcessor() : nullptr;
        _resizingEditorToBounds = false;
        [self setPostsFrameChangedNotifications:YES];

        if (_processorWrapper != nullptr)
            _processorWrapper->registerEditorView (self);

        if (_processor != nullptr && _processor->hasEditor())
        {
            _processorEditor.reset (_processor->createEditor());

            if (_processorEditor != nullptr)
            {
                const auto preferredSize = _processorEditor->getPreferredSize();
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "created editor: preferredWidth=" << yup::String (preferredSize.getWidth()) << ", preferredHeight=" << yup::String (preferredSize.getHeight()) << ", editor=" << yup::describePointer (_processorEditor.get()));

                if (_processorEditor->isResizable())
                    [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
                else
                    [self setAutoresizingMask:NSViewNotSizable];

                _processorEditorListener = std::make_unique<yup::AudioPluginEditorViewAUListener> (self);
                _processorEditor->addComponentListener (_processorEditorListener.get());

                [self setFrameSize:NSMakeSize (preferredSize.getWidth(), preferredSize.getHeight())];
                [self resizeEditorToBounds];
            }
            else
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "processor returned null editor");
            }
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "processor has no editor");
        }
    }
    else
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view initialisation failed");
    }

    return self;
}

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];

    if ([self window] != nil)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view moved to window: view=" << yup::describePointer ((__bridge void*) self) << ", window=" << yup::describePointer ((__bridge void*) [self window]) << ", contentView=" << yup::describePointer ((__bridge void*) [[self window] contentView]));

        [self attachEditorIfNeeded];
    }
    else
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view removed from window: view=" << yup::describePointer ((__bridge void*) self));

        [self detachEditorIfNeeded];
    }
}

- (void)setFrameSize:(NSSize)newSize
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view frame size changed: width=" << yup::String (static_cast<double> (newSize.width)) << ", height=" << yup::String (static_cast<double> (newSize.height)));

    [super setFrameSize:newSize];
    [self resizeEditorToBounds];
}

- (void)attachEditorIfNeeded
{
    if (_processorEditor == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "attachEditorIfNeeded skipped: editor is null");
        return;
    }

    if (_processorEditor->isOnDesktop())
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "attachEditorIfNeeded skipped: editor is already on desktop");
        return;
    }

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "attaching editor to native view: editor=" << yup::describePointer (_processorEditor.get()) << ", view=" << yup::describePointer ((__bridge void*) self) << ", window=" << yup::describePointer ((__bridge void*) [self window]));

    [self resizeEditorToBounds];

    yup::ComponentNative::Flags flags = yup::ComponentNative::defaultFlags & ~yup::ComponentNative::decoratedWindow;

    if (_processorEditor->shouldRenderContinuous())
        flags.set (yup::ComponentNative::renderContinuous);

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor native options: renderContinuous=" << yup::String (_processorEditor->shouldRenderContinuous() ? "true" : "false") << ", resizable=" << yup::String (_processorEditor->isResizable() ? "true" : "false"));

    auto options = yup::ComponentNative::Options()
                       .withFlags (flags)
                       .withResizableWindow (_processorEditor->isResizable());

    _processorEditor->addToDesktop (options, (__bridge void*) self);
    _processorEditor->setVisible (true);
    _processorEditor->attachedToNative();

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor attached to native view: isOnDesktop=" << yup::String (_processorEditor->isOnDesktop() ? "true" : "false"));
}

- (void)detachEditorIfNeeded
{
    if (_processorEditor == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "detachEditorIfNeeded skipped: editor is null");
        return;
    }

    if (! _processorEditor->isOnDesktop())
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "detachEditorIfNeeded skipped: editor is not on desktop");
        return;
    }

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "detaching editor from native view: editor=" << yup::describePointer (_processorEditor.get()) << ", view=" << yup::describePointer ((__bridge void*) self) << ", window=" << yup::describePointer ((__bridge void*) [self window]));

    yup::endActiveParameterGestures (_processor);
    _processorEditor->setVisible (false);
    _processorEditor->removeFromDesktop();

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor detached from native view: isOnDesktop=" << yup::String (_processorEditor->isOnDesktop() ? "true" : "false"));
}

- (void)closeEditorIfNeeded
{
    [self detachEditorIfNeeded];

    yup::endActiveParameterGestures (_processor);

    if (_processorEditor != nullptr && _processorEditorListener != nullptr)
        _processorEditor->removeComponentListener (_processorEditorListener.get());

    _processorEditorListener.reset();
    _processorEditor.reset();
}

- (void)closeEditorForProcessorDestruction
{
    [self closeEditorIfNeeded];

    _processorWrapper = nullptr;
    _processor = nullptr;
}

- (void)resizeEditorToBounds
{
    if (_processorEditor == nullptr)
        return;

    const auto bounds = [self bounds];

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "resizing editor to bounds: width=" << yup::String (static_cast<double> (NSWidth (bounds))) << ", height=" << yup::String (static_cast<double> (NSHeight (bounds))) << ", editor=" << yup::describePointer (_processorEditor.get()));

    const auto scoped = yup::ScopedValueSetter<bool> (_resizingEditorToBounds, true);

    _processorEditor->setBounds ({ 0.0f,
                                   0.0f,
                                   yup::jmax (1.0f, static_cast<float> (NSWidth (bounds))),
                                   yup::jmax (1.0f, static_cast<float> (NSHeight (bounds))) });
}

- (void)resizeViewToEditorSize
{
    if (_processorEditor == nullptr || ! _processorEditor->isResizable())
        return;

    const auto newSize = NSMakeSize (yup::jmax (1.0f, _processorEditor->getWidth()),
                                     yup::jmax (1.0f, _processorEditor->getHeight()));

    if (NSEqualSizes ([self frame].size, newSize))
        return;

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "resizing editor view to editor: width=" << yup::String (static_cast<double> (newSize.width)) << ", height=" << yup::String (static_cast<double> (newSize.height)) << ", editor=" << yup::describePointer (_processorEditor.get()));

    [super setFrameSize:newSize];
}

- (void)processorEditorResized
{
    if (_resizingEditorToBounds)
        return;

    [self resizeViewToEditorSize];
}

- (void)dealloc
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "destroying editor view: view=" << yup::describePointer ((__bridge void*) self) << ", editor=" << yup::describePointer (_processorEditor.get()) << ", processor=" << yup::describePointer (_processor));

    [self closeEditorIfNeeded];

    if (_processorWrapper != nullptr)
        _processorWrapper->unregisterEditorView (self);

    _processorWrapper = nullptr;
    _processor = nullptr;
}

@end

namespace yup
{

void AudioPluginEditorViewAUListener::componentResized (Component& component)
{
    ignoreUnused (component);

    if (owner != nil)
        [owner processorEditorResized];
}

void AudioPluginProcessorAU::closeEditorViews()
{
    std::vector<void*> viewsToClose;

    {
        std::lock_guard<std::mutex> lock (editorViewsMutex);
        viewsToClose.swap (editorViews);
    }

    for (auto* view : viewsToClose)
        if (view != nullptr)
            [(__bridge AudioPluginEditorViewAU*) view closeEditorForProcessorDestruction];
}

} // namespace yup

//==============================================================================
// Cocoa view factory

@interface AudioPluginProcessorAUViewFactory : NSObject <AUCocoaUIBase>
@end

@implementation AudioPluginProcessorAUViewFactory

- (unsigned)interfaceVersion
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "view factory interfaceVersion requested");
    return 0;
}

- (NSString*)description
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "view factory description requested");
    return @YupPlugin_Name;
}

- (NSView*)uiViewForAudioUnit:(AudioUnit)inAudioUnit withSize:(NSSize)inPreferredSize
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "view factory requested editor view: audioUnit=" << yup::describePointer (inAudioUnit) << ", preferredWidth=" << yup::String (static_cast<double> (inPreferredSize.width)) << ", preferredHeight=" << yup::String (static_cast<double> (inPreferredSize.height)));

    auto* proc = yup::AudioPluginProcessorAU::findInstance (inAudioUnit);
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

    return [[AudioPluginEditorViewAU alloc] initWithAudioUnitWrapper:proc
                                                       preferredSize:inPreferredSize];
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

        *static_cast<UInt32*> (outData) = isBypassed ? 1u : 0u;
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
        NSBundle* bundle = [NSBundle bundleForClass:[AudioPluginProcessorAUViewFactory class]];
        if (bundle == nil)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "GetProperty CocoaUI failed: bundle is nil");
            return kAudioUnitErr_InvalidPropertyValue;
        }

        auto* bundleLocation = (__bridge_retained CFURLRef)[bundle bundleURL];
        auto* viewClass = CFStringCreateWithCString (kCFAllocatorDefault,
                                                     "AudioPluginProcessorAUViewFactory",
                                                     kCFStringEncodingUTF8);

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
