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
class AudioPluginProcessorAU final : public AudioPluginAUBase
{
public:
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

        registerInstance (componentInstance, this);
    }

    ~AudioPluginProcessorAU() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "destroying processor instance: wrapper=" << yup::describePointer (this) << ", component=" << yup::describePointer (componentInstance) << ", processor=" << yup::describePointer (processor.get()));

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

        outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_HasCFNameString;

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

        if (parameters[parameterIndex]->isPerformingChangeGesture())
            return noErr;

        parameters[parameterIndex]->setValue (static_cast<float> (inValue));
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

        std::vector<float*> channels;
        for (UInt32 ch = 0; ch < outBufList.mNumberBuffers; ++ch)
            channels.push_back (static_cast<float*> (outBufList.mBuffers[ch].mData));

        AudioSampleBuffer audioBuffer (channels.data(),
                                       static_cast<int> (channels.size()),
                                       0,
                                       static_cast<int> (inNumberFrames));

        {
            std::lock_guard<std::mutex> lock (midiMutex);
            AudioProcessContext<float> context { audioBuffer, midiBuffer, emptyParamChanges };
            processor->processBlock (context);
            midiBuffer.clear();
        }

        return noErr;
    }

    //==============================================================================

    OSStatus HandleMIDIEvent (UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 offsetSampleFrame) override
    {
        std::lock_guard<std::mutex> lock (midiMutex);

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
        std::lock_guard<std::mutex> lock (midiMutex);

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

        std::vector<float*> channels;
        for (UInt32 ch = 0; ch < numBuffers; ++ch)
        {
            const auto* in = static_cast<const float*> (inBuffer.mBuffers[ch].mData);
            auto* out = static_cast<float*> (outBuffer.mBuffers[ch].mData);

            if (in != out)
                std::memcpy (out, in, inFramesToProcess * sizeof (float));

            channels.push_back (out);
        }

        AudioSampleBuffer audioBuffer (channels.data(),
                                       static_cast<int> (channels.size()),
                                       0,
                                       static_cast<int> (inFramesToProcess));

        AudioProcessContext<float> context { audioBuffer, midiBuffer, emptyParamChanges };
        processor->processBlock (context);
        midiBuffer.clear();

        return noErr;
    }
#endif

    //==============================================================================

    OSStatus SaveState (CFPropertyListRef* outData) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState requested");

        if (processor == nullptr || outData == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState failed: processor=" << describePointer (processor.get()) << ", outData=" << describePointer (outData));
            return kAudioUnitErr_InvalidPropertyValue;
        }

        MemoryBlock data;
        const auto result = processor->saveStateIntoMemory (data);
        if (result.failed())
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState failed: " << result.getErrorMessage());
            return kAudioUnitErr_InvalidPropertyValue;
        }

        NSData* nsData = [NSData dataWithBytes:data.getData()
                                        length:data.getSize()];
        *outData = (__bridge_retained CFPropertyListRef) nsData;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SaveState completed: bytes=" << String (static_cast<int64> (data.getSize())));

        return noErr;
    }

    OSStatus RestoreState (CFPropertyListRef inData) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "RestoreState requested");

        if (processor == nullptr || inData == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "RestoreState failed: processor=" << describePointer (processor.get()) << ", inData=" << describePointer (inData));
            return kAudioUnitErr_InvalidPropertyValue;
        }

        NSData* nsData = (__bridge NSData*) inData;

        MemoryBlock data ([nsData bytes], [nsData length]);

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

        const auto result = AudioPluginAUBase::SetProperty (inID, inScope, inElement, inData, inDataSize);
        if (result != noErr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "SetProperty delegated failed: property=" << String (static_cast<int> (inID)) << ", " << describeScopeAndElement (inScope, inElement) << ", status=" << describeStatus (result));
        }

        return result;
    }

    //==============================================================================

    AudioProcessor* getProcessor() const { return processor.get(); }

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

    Float64 getCurrentSampleRate()
    {
        return Output (0).GetStreamFormat().mSampleRate;
    }

    AUScopedYupInitialiser scopeInitialiser;
    std::unique_ptr<AudioProcessor> processor;

    MidiBuffer midiBuffer;
    ParameterChangeBuffer emptyParamChanges; // AU delivers param changes via SetParameter, not in the audio stream
    std::mutex midiMutex;
    std::vector<AUChannelInfo> channelInfoCache;
    AudioUnit componentInstance = nullptr;
    bool renderingOffline = false;
};

} // namespace yup

//==============================================================================
// Objective-C editor view

@interface AudioPluginEditorViewAU : NSView
{
    yup::AUScopedYupWindowingInitialiser _scopeInitialiser;
    yup::AudioProcessor* _processor;
    std::unique_ptr<yup::AudioProcessorEditor> _processorEditor;
}
- (instancetype)initWithProcessor:(yup::AudioProcessor*)processor
                    preferredSize:(NSSize)size;
- (void)attachEditorIfNeeded;
- (void)detachEditorIfNeeded;
- (void)resizeEditorToBounds;
@end

@implementation AudioPluginEditorViewAU

- (instancetype)initWithProcessor:(yup::AudioProcessor*)processor
                    preferredSize:(NSSize)size
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "creating editor view: requestedWidth=" << yup::String (static_cast<double> (size.width)) << ", requestedHeight=" << yup::String (static_cast<double> (size.height)) << ", processor=" << yup::describePointer (processor) << ", view=" << yup::describePointer ((__bridge void*) self));

    if ((self = [super initWithFrame:NSMakeRect (0, 0, size.width, size.height)]))
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "editor view initialised: view=" << yup::describePointer ((__bridge void*) self));
        _processor = processor;

        if (processor != nullptr && processor->hasEditor())
        {
            _processorEditor.reset (processor->createEditor());

            if (_processorEditor != nullptr)
            {
                const auto preferredSize = _processorEditor->getPreferredSize();
                YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "created editor: preferredWidth=" << yup::String (preferredSize.getWidth()) << ", preferredHeight=" << yup::String (preferredSize.getHeight()) << ", editor=" << yup::describePointer (_processorEditor.get()));

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

- (void)resizeEditorToBounds
{
    if (_processorEditor == nullptr)
        return;

    const auto bounds = [self bounds];

    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "resizing editor to bounds: width=" << yup::String (static_cast<double> (NSWidth (bounds))) << ", height=" << yup::String (static_cast<double> (NSHeight (bounds))) << ", editor=" << yup::describePointer (_processorEditor.get()));

    _processorEditor->setBounds ({ 0.0f,
                                   0.0f,
                                   yup::jmax (1.0f, static_cast<float> (NSWidth (bounds))),
                                   yup::jmax (1.0f, static_cast<float> (NSHeight (bounds))) });
}

- (void)dealloc
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AU, "destroying editor view: view=" << yup::describePointer ((__bridge void*) self) << ", editor=" << yup::describePointer (_processorEditor.get()) << ", processor=" << yup::describePointer (_processor));

    [self detachEditorIfNeeded];

    yup::endActiveParameterGestures (_processor);

    _processorEditor.reset();
    _processor = nullptr;
}

@end

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

    return [[AudioPluginEditorViewAU alloc] initWithProcessor:proc->getProcessor()
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
