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

#if !defined(YUP_AUDIO_PLUGIN_ENABLE_AU)
#error "YUP_AUDIO_PLUGIN_ENABLE_AU must be defined"
#endif

#if YUP_MAC

#include <AudioUnitSDK/AUBase.h>
#include <AudioUnitSDK/AUEffectBase.h>
#include <AudioUnitSDK/AUMIDIBase.h>
#include <AudioUnitSDK/ComponentBase.h>
#include <AudioUnitSDK/MusicDeviceBase.h>

#import <AppKit/AppKit.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreMIDI/CoreMIDI.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

//==============================================================================

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

namespace
{

//==============================================================================

struct AUScopedYupInitialiser
{
    AUScopedYupInitialiser()
    {
        if (numAUScopedInitInstances.fetch_add(1) == 0)
        {
            initialiseYup_GUI();
        }
    }

    ~AUScopedYupInitialiser()
    {
        if (numAUScopedInitInstances.fetch_sub(1) == 1)
        {
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
        if (numAUScopedInitInstances.fetch_add(1) == 0)
        {
            initialiseYup_Windowing();
        }
    }

    ~AUScopedYupWindowingInitialiser()
    {
        if (numAUScopedInitInstances.fetch_sub(1) == 1)
        {
            shutdownYup_Windowing();
        }
    }

private:
    static std::atomic_int numAUScopedInitInstances;
};

std::atomic_int AUScopedYupWindowingInitialiser::numAUScopedInitInstances = 0;

//==============================================================================

static OSType osTypeFromString(const char* s)
{
    if (s == nullptr || std::strlen(s) < 4)
        return 0;

    return static_cast<OSType>(
        (static_cast<uint32_t>(static_cast<uint8_t>(s[0])) << 24) | (static_cast<uint32_t>(static_cast<uint8_t>(s[1])) << 16) | (static_cast<uint32_t>(static_cast<uint8_t>(s[2])) << 8) | static_cast<uint32_t>(static_cast<uint8_t>(s[3])));
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

    AudioPluginProcessorAU(AudioComponentInstance component)
#if YupPlugin_IsSynth
        : AudioPluginAUBase(component, 0, 1),
#else
        : AudioPluginAUBase(component),
#endif
          componentInstance(component)
    {
        processor.reset(::createPluginProcessor());
        registerInstance(componentInstance, this);
    }

    ~AudioPluginProcessorAU() override
    {
        unregisterInstance(componentInstance);
        processor.reset();
    }

    //==============================================================================

    OSStatus Initialize() override
    {
        const auto result = AudioPluginAUBase::Initialize();
        if (result != noErr)
            return result;

        if (processor == nullptr)
            return kAudioUnitErr_FailedInitialization;

        processor->setOfflineProcessing(renderingOffline);
        processor->setPlaybackConfiguration(static_cast<float>(getCurrentSampleRate()),
                                            static_cast<int>(GetMaxFramesPerSlice()));

        midiBuffer.ensureSize(4096);
        midiBuffer.clear();

        return noErr;
    }

    void Cleanup() override
    {
        if (processor != nullptr)
            processor->releaseResources();

        AudioPluginAUBase::Cleanup();
    }

    //==============================================================================

    OSStatus GetParameterList(AudioUnitScope inScope,
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
                outParameterList[i] = static_cast<AudioUnitParameterID>(parameters[i]->getHostParameterID());
        }

        outNumParameters = static_cast<UInt32>(parameters.size());
        return noErr;
    }

    OSStatus GetParameterInfo(AudioUnitScope inScope,
                              AudioUnitParameterID inParameterID,
                              AudioUnitParameterInfo& outParameterInfo) override
    {
        if (inScope != kAudioUnitScope_Global || processor == nullptr)
            return kAudioUnitErr_InvalidParameter;

        const auto parameters = processor->getParameters();
        const auto parameterIndex = processor->getParameterIndexByHostID(static_cast<uint32>(inParameterID));
        if (! isPositiveAndBelow(parameterIndex, static_cast<int>(parameters.size())))
            return kAudioUnitErr_InvalidParameter;

        const auto& param = parameters[parameterIndex];

        outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_HasCFNameString;

        outParameterInfo.cfNameString = param->getName().toCFString();
        param->getName().copyToUTF8(outParameterInfo.name, sizeof(outParameterInfo.name));

        outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
        outParameterInfo.minValue = param->getMinimumValue();
        outParameterInfo.maxValue = param->getMaximumValue();
        outParameterInfo.defaultValue = param->getDefaultValue();
        outParameterInfo.clumpID = 0;

        return noErr;
    }

    OSStatus GetParameter(AudioUnitParameterID inID,
                          AudioUnitScope inScope,
                          AudioUnitElement inElement,
                          AudioUnitParameterValue& outValue) override
    {
        if (inScope != kAudioUnitScope_Global || processor == nullptr)
            return kAudioUnitErr_InvalidParameter;

        const auto parameters = processor->getParameters();
        const auto parameterIndex = processor->getParameterIndexByHostID(static_cast<uint32>(inID));
        if (! isPositiveAndBelow(parameterIndex, static_cast<int>(parameters.size())))
            return kAudioUnitErr_InvalidParameter;

        outValue = static_cast<AudioUnitParameterValue>(parameters[parameterIndex]->getValue());
        return noErr;
    }

    OSStatus SetParameter(AudioUnitParameterID inID,
                          AudioUnitScope inScope,
                          AudioUnitElement inElement,
                          AudioUnitParameterValue inValue,
                          UInt32 inBufferOffsetInFrames) override
    {
        if (inScope != kAudioUnitScope_Global || processor == nullptr)
            return kAudioUnitErr_InvalidParameter;

        const auto parameters = processor->getParameters();
        const auto parameterIndex = processor->getParameterIndexByHostID(static_cast<uint32>(inID));
        if (! isPositiveAndBelow(parameterIndex, static_cast<int>(parameters.size())))
            return kAudioUnitErr_InvalidParameter;

        parameters[parameterIndex]->setValue(static_cast<float>(inValue));
        return noErr;
    }

    //==============================================================================

    UInt32 SupportedNumChannels(const AUChannelInfo** outInfo) override
    {
        if (processor == nullptr)
            return 0;

        channelInfoCache.clear();

        const auto& busLayout = processor->getBusLayout();

        int inputChannels = 0;
        for (const auto& bus : busLayout.getInputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                inputChannels = std::max(inputChannels, bus.getNumChannels());

        int outputChannels = 0;
        for (const auto& bus : busLayout.getOutputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                outputChannels = std::max(outputChannels, bus.getNumChannels());

        if (inputChannels > 0 || outputChannels > 0)
        {
            AUChannelInfo info;
            info.inChannels = static_cast<SInt16>(inputChannels);
            info.outChannels = static_cast<SInt16>(outputChannels);
            channelInfoCache.push_back(info);
        }

        if (outInfo != nullptr)
            *outInfo = channelInfoCache.data();

        return static_cast<UInt32>(channelInfoCache.size());
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

        return static_cast<Float64>(processor->getTailSamples()) / sampleRate;
    }

    Float64 GetLatency() override
    {
        const auto sampleRate = getCurrentSampleRate();
        if (processor == nullptr || sampleRate <= 0.0)
            return 0.0;

        return static_cast<Float64>(processor->getLatencySamples()) / sampleRate;
    }

    //==============================================================================

#if YupPlugin_IsSynth
    // Instrument: render audio and drain the MIDI buffer
    OSStatus RenderBus(AudioUnitRenderActionFlags& ioActionFlags,
                       const AudioTimeStamp& inTimeStamp,
                       UInt32 inBusNumber,
                       UInt32 inNumberFrames) override
    {
        if (processor == nullptr)
            return kAudioUnitErr_NoConnection;

        auto& outputBus = Output(inBusNumber);

        outputBus.PrepareBuffer(inNumberFrames);
        AudioBufferList& outBufList = outputBus.GetBufferList();

        std::vector<float*> channels;
        for (UInt32 ch = 0; ch < outBufList.mNumberBuffers; ++ch)
            channels.push_back(static_cast<float*>(outBufList.mBuffers[ch].mData));

        AudioSampleBuffer audioBuffer(channels.data(),
                                      static_cast<int>(channels.size()),
                                      0,
                                      static_cast<int>(inNumberFrames));

        {
            std::lock_guard<std::mutex> lock(midiMutex);
            AudioProcessContext<float> context { audioBuffer, midiBuffer, emptyParamChanges };
            processor->processBlock(context);
            midiBuffer.clear();
        }

        return noErr;
    }

    //==============================================================================

    OSStatus HandleMIDIEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2, UInt32 offsetSampleFrame) override
    {
        std::lock_guard<std::mutex> lock(midiMutex);

        const uint8_t rawData[3] = {
            static_cast<uint8_t>(status | channel),
            data1,
            data2};

        const int numBytes = MidiMessage::getMessageLengthFromFirstByte(rawData[0]);
        midiBuffer.addEvent(rawData, numBytes, static_cast<int>(offsetSampleFrame));

        return noErr;
    }

    [[nodiscard]] bool CanScheduleParameters() const override
    {
        return false;
    }

    bool StreamFormatWritable(AudioUnitScope inScope, AudioUnitElement inElement) override
    {
        return inScope == kAudioUnitScope_Output && inElement == 0;
    }

    OSStatus HandleSysEx(const UInt8* inData, UInt32 inLength) override
    {
        std::lock_guard<std::mutex> lock(midiMutex);

        if (inData != nullptr && inLength > 0)
            midiBuffer.addEvent(inData, static_cast<int>(inLength), 0);

        return noErr;
    }

#else
    // Effect: copy input to output and call processBlock
    OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags,
                                const AudioBufferList& inBuffer,
                                AudioBufferList& outBuffer,
                                UInt32 inFramesToProcess) override
    {
        if (processor == nullptr)
            return kAudioUnitErr_NoConnection;

        const UInt32 numBuffers = std::min(inBuffer.mNumberBuffers, outBuffer.mNumberBuffers);

        std::vector<float*> channels;
        for (UInt32 ch = 0; ch < numBuffers; ++ch)
        {
            const auto* in = static_cast<const float*>(inBuffer.mBuffers[ch].mData);
            auto* out = static_cast<float*>(outBuffer.mBuffers[ch].mData);

            if (in != out)
                std::memcpy(out, in, inFramesToProcess * sizeof(float));

            channels.push_back(out);
        }

        AudioSampleBuffer audioBuffer(channels.data(),
                                      static_cast<int>(channels.size()),
                                      0,
                                      static_cast<int>(inFramesToProcess));

        AudioProcessContext<float> context { audioBuffer, midiBuffer, emptyParamChanges };
        processor->processBlock(context);
        midiBuffer.clear();

        return noErr;
    }
#endif

    //==============================================================================

    OSStatus SaveState(CFPropertyListRef* outData) override
    {
        if (processor == nullptr || outData == nullptr)
            return kAudioUnitErr_InvalidPropertyValue;

        MemoryBlock data;
        if (processor->saveStateIntoMemory(data).failed())
            return kAudioUnitErr_InvalidPropertyValue;

        NSData* nsData = [NSData dataWithBytes:data.getData()
                                        length:data.getSize()];
        *outData = (__bridge_retained CFPropertyListRef)nsData;

        return noErr;
    }

    OSStatus RestoreState(CFPropertyListRef inData) override
    {
        if (processor == nullptr || inData == nullptr)
            return kAudioUnitErr_InvalidPropertyValue;

        NSData* nsData = (__bridge NSData*)inData;

        MemoryBlock data([nsData bytes], [nsData length]);

        processor->suspendProcessing(true);
        const bool ok = processor->loadStateFromMemory(data).wasOk();
        processor->suspendProcessing(false);

        return ok ? static_cast<OSStatus>(noErr)
                  : static_cast<OSStatus>(kAudioUnitErr_InvalidPropertyValue);
    }

    //==============================================================================

    OSStatus GetPresets(CFArrayRef* outData) const override
    {
        if (processor == nullptr || outData == nullptr)
            return kAudioUnitErr_InvalidPropertyValue;

        const int numPresets = processor->getNumPresets();
        NSMutableArray* presetsArray = [[NSMutableArray alloc] initWithCapacity:numPresets];

        for (int i = 0; i < numPresets; ++i)
        {
            AUPreset preset;
            preset.presetNumber = i;
            preset.presetName = processor->getPresetName(i).toCFString();

            [presetsArray addObject:[NSValue valueWithBytes:&preset objCType:@encode(AUPreset)]];
            CFRelease(preset.presetName);
        }

        *outData = (__bridge_retained CFArrayRef)presetsArray;
        return noErr;
    }

    OSStatus NewFactoryPresetSet(const AUPreset& inNewFactoryPreset) override
    {
        if (processor == nullptr)
            return kAudioUnitErr_InvalidPropertyValue;

        if (!isPositiveAndBelow(static_cast<int>(inNewFactoryPreset.presetNumber),
                                processor->getNumPresets()))
            return kAudioUnitErr_InvalidPropertyValue;

        processor->setCurrentPreset(static_cast<int>(inNewFactoryPreset.presetNumber));
        return noErr;
    }

    //==============================================================================

    OSStatus GetPropertyInfo(AudioUnitPropertyID inID,
                             AudioUnitScope inScope,
                             AudioUnitElement inElement,
                             UInt32& outDataSize,
                             bool& outWritable) override
    {
        if (inID == kAudioUnitProperty_OfflineRender)
        {
            if (inScope != kAudioUnitScope_Global)
                return kAudioUnitErr_InvalidScope;

            outDataSize = sizeof(UInt32);
            outWritable = true;
            return noErr;
        }

        if (inID == kAudioUnitProperty_CocoaUI)
        {
            if (processor != nullptr && processor->hasEditor())
            {
                outDataSize = sizeof(AudioUnitCocoaViewInfo);
                outWritable = false;
                return noErr;
            }

            return kAudioUnitErr_PropertyNotInUse;
        }

        return AudioPluginAUBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
    }

    OSStatus GetProperty(AudioUnitPropertyID inID,
                         AudioUnitScope inScope,
                         AudioUnitElement inElement,
                         void* outData) override; // Implemented below (needs ObjC)

    OSStatus SetProperty(AudioUnitPropertyID inID,
                         AudioUnitScope inScope,
                         AudioUnitElement inElement,
                         const void* inData,
                         UInt32 inDataSize) override
    {
        if (inID == kAudioUnitProperty_OfflineRender)
        {
            if (inScope != kAudioUnitScope_Global)
                return kAudioUnitErr_InvalidScope;

            if (inData == nullptr || inDataSize < sizeof(UInt32))
                return kAudioUnitErr_InvalidPropertyValue;

            renderingOffline = *static_cast<const UInt32*>(inData) != 0;

            if (processor != nullptr)
                processor->setOfflineProcessing(renderingOffline);

            return noErr;
        }

        return AudioPluginAUBase::SetProperty(inID, inScope, inElement, inData, inDataSize);
    }

    //==============================================================================

    AudioProcessor* getProcessor() const { return processor.get(); }

    static AudioPluginProcessorAU* findInstance(AudioUnit component)
    {
        std::lock_guard<std::mutex> lock(getInstanceRegistryMutex());

        const auto iter = getInstanceRegistry().find(component);
        return iter != getInstanceRegistry().end() ? iter->second : nullptr;
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

    static void registerInstance(AudioUnit component, AudioPluginProcessorAU* instance)
    {
        std::lock_guard<std::mutex> lock(getInstanceRegistryMutex());
        getInstanceRegistry()[component] = instance;
    }

    static void unregisterInstance(AudioUnit component)
    {
        std::lock_guard<std::mutex> lock(getInstanceRegistryMutex());
        getInstanceRegistry().erase(component);
    }

    Float64 getCurrentSampleRate()
    {
        return Output(0).GetStreamFormat().mSampleRate;
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
    std::unique_ptr<yup::AudioProcessorEditor> _processorEditor;
}
- (instancetype)initWithProcessor:(yup::AudioProcessor*)processor
                    preferredSize:(NSSize)size;
@end

@implementation AudioPluginEditorViewAU

- (instancetype)initWithProcessor:(yup::AudioProcessor*)processor
                    preferredSize:(NSSize)size
{

    if ((self = [super initWithFrame:NSMakeRect(0, 0, size.width, size.height)]))
    {
        if (processor != nullptr && processor->hasEditor())
        {
            _processorEditor.reset(processor->createEditor());

            if (_processorEditor != nullptr)
            {
                const auto preferredSize = _processorEditor->getPreferredSize();

                [self setFrameSize:NSMakeSize(preferredSize.getWidth(), preferredSize.getHeight())];

                yup::ComponentNative::Flags flags = yup::ComponentNative::defaultFlags & ~yup::ComponentNative::decoratedWindow;

                if (_processorEditor->shouldRenderContinuous())
                    flags.set(yup::ComponentNative::renderContinuous);

                auto options = yup::ComponentNative::Options()
                                   .withFlags(flags)
                                   .withResizableWindow(_processorEditor->isResizable());

                _processorEditor->addToDesktop(options, (__bridge void*)self);
                _processorEditor->setVisible(true);
                _processorEditor->attachedToNative();
            }
        }
    }
    return self;
}

- (void)dealloc
{
    if (_processorEditor != nullptr)
    {
        _processorEditor->setVisible(false);
        _processorEditor->removeFromDesktop();
        _processorEditor.reset();
    }
}

@end

//==============================================================================
// Cocoa view factory

@interface AudioPluginProcessorAUViewFactory : NSObject
@end

@implementation AudioPluginProcessorAUViewFactory

- (unsigned)interfaceVersion
{
    return 0;
}

- (NSString*)description
{
    return @YupPlugin_Name;
}

- (NSView*)uiViewForAudioUnit:(AudioUnit)inAudioUnit withSize:(NSSize)inPreferredSize
{
    auto* proc = yup::AudioPluginProcessorAU::findInstance(inAudioUnit);
    if (proc == nullptr)
        return nil;

    return [[AudioPluginEditorViewAU alloc] initWithProcessor:proc->getProcessor()
                                                preferredSize:inPreferredSize];
}

@end

//==============================================================================
// GetProperty implementation (needs ObjC)

namespace yup
{

OSStatus AudioPluginProcessorAU::GetProperty(AudioUnitPropertyID inID,
                                             AudioUnitScope inScope,
                                             AudioUnitElement inElement,
                                             void* outData)
{
    if (inID == kAudioUnitProperty_OfflineRender)
    {
        if (inScope != kAudioUnitScope_Global)
            return kAudioUnitErr_InvalidScope;

        if (outData == nullptr)
            return kAudioUnitErr_InvalidPropertyValue;

        *static_cast<UInt32*>(outData) = renderingOffline ? 1u : 0u;
        return noErr;
    }

    if (inID == kAudioUnitProperty_CocoaUI)
    {
        if (processor == nullptr || !processor->hasEditor())
            return kAudioUnitErr_PropertyNotInUse;

        auto* info = static_cast<AudioUnitCocoaViewInfo*>(outData);

        // The bundle location is this plugin's own bundle
        NSBundle* bundle = [NSBundle bundleForClass:[AudioPluginProcessorAUViewFactory class]];
        info->mCocoaAUViewBundleLocation = (__bridge_retained CFURLRef)[bundle bundleURL];
        info->mCocoaAUViewClass[0] = CFSTR("AudioPluginProcessorAUViewFactory");

        return noErr;
    }

    return AudioPluginAUBase::GetProperty(inID, inScope, inElement, outData);
}

} // namespace yup

//==============================================================================
// Factory entry point

#if YupPlugin_IsSynth
using AudioPluginProcessorAU = yup::AudioPluginProcessorAU;
AUSDK_COMPONENT_ENTRY(ausdk::AUMusicDeviceFactory, AudioPluginProcessorAU)
#else
using AudioPluginProcessorAU = yup::AudioPluginProcessorAU;
AUSDK_COMPONENT_ENTRY(ausdk::AUBaseProcessFactory, AudioPluginProcessorAU)
#endif

#endif // YUP_MAC
