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

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_AUv3)
#error "YUP_AUDIO_PLUGIN_ENABLE_AUv3 must be defined"
#endif

#if YUP_MAC

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudioKit/CoreAudioKit.h>

#import <AppKit/AppKit.h>

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

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

//==============================================================================

static String describePointer (const void* value)
{
    return "0x" + String::toHexString (static_cast<int64> (reinterpret_cast<uintptr_t> (value)));
}

//==============================================================================

struct AUScopedYupInitialiser
{
    AUScopedYupInitialiser()
    {
        if (numAUScopedInitInstances.fetch_add (1) == 0)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "initialising YUP GUI");
            initialiseYup_GUI();
        }
    }

    ~AUScopedYupInitialiser()
    {
        if (numAUScopedInitInstances.fetch_sub (1) == 1)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "shutting down YUP GUI");
            shutdownYup_GUI();
        }
    }

private:
    static std::atomic_int numAUScopedInitInstances;
};

std::atomic_int AUScopedYupInitialiser::numAUScopedInitInstances = 0;

//==============================================================================

static float getMaximumParameterValue (const AudioParameter& p)
{
    return p.getMaximumValue();
}

//==============================================================================

} // namespace yup

//==============================================================================
// Forward declarations

@class YUPAUv3ViewController;

//==============================================================================
// The main C++ AUv3 wrapper class

namespace yup
{

class AudioPluginProcessorAUv3 final
    : public AudioProcessorBase::Listener
    , public AudioPlayHead
    , private AudioParameter::Listener
{
public:
    //==============================================================================

    AudioPluginProcessorAUv3 (AUAudioUnit* audioUnit,
                              AudioComponentDescription,
                              AudioComponentInstantiationOptions,
                              NSError**)
        : au (audioUnit)
    {
        processor.reset (::createPluginProcessor());
        init();
    }

    ~AudioPluginProcessorAUv3() override
    {
        if (editorObserverToken != nullptr)
        {
            if (paramTree.get() != nil)
                [paramTree.get() removeParameterObserver:*editorObserverToken];

            delete editorObserverToken;
            editorObserverToken = nullptr;
        }

        if (processor != nullptr)
        {
            processor->removeListener (this);
            yup::endActiveParameterGestures (processor.get());
            processor->releaseResources();
        }

        unregisterAllParameterListeners();
    }

    //==============================================================================

    void init()
    {
        if (processor == nullptr)
            return;

        inParameterChangedCallback = false;

        const AUAudioFrameCount maxFrames = [au maximumFramesToRender];

        const auto& busLayout = processor->getBusLayout();

        totalInChannels = 0;
        for (const auto& bus : busLayout.getInputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                totalInChannels += bus.getNumChannels();

        totalOutChannels = 0;
        for (const auto& bus : busLayout.getOutputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                totalOutChannels += bus.getNumChannels();

        // Build channel capabilities
        {
            channelCapabilities.reset ([[NSMutableArray<NSNumber*> alloc] init]);

            int maxInputCh = 0;
            int maxOutputCh = 0;

            for (const auto& bus : busLayout.getInputBuses())
                if (bus.getType() == AudioBus::Type::Audio)
                    maxInputCh = std::max (maxInputCh, bus.getNumChannels());

            for (const auto& bus : busLayout.getOutputBuses())
                if (bus.getType() == AudioBus::Type::Audio)
                    maxOutputCh = std::max (maxOutputCh, bus.getNumChannels());

            [channelCapabilities.get() addObject:[NSNumber numberWithInteger:maxInputCh]];
            [channelCapabilities.get() addObject:[NSNumber numberWithInteger:maxOutputCh]];
        }

        internalRenderBlock = CreateObjCBlock (this, &AudioPluginProcessorAUv3::renderCallback);

        renderContextObserver = ^(const AudioUnitRenderContext*) {};

        processor->setPlaybackConfiguration (static_cast<float> (44100.0),
                                             static_cast<int> (maxFrames));
        processor->addListener (this);

        addParameters();
        addPresets();
        addAudioUnitBusses (true);
        addAudioUnitBusses (false);

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "initialised: processor=" << describePointer (processor.get()) << ", inCh=" << String (totalInChannels) << ", outCh=" << String (totalOutChannels));
    }

    AudioProcessor* getProcessor() const noexcept { return processor.get(); }

    //==============================================================================
    // Parameter management

    void addParameters()
    {
        if (processor == nullptr)
            return;

        const auto parameters = processor->getParameters();
        addressForIndex.resize (parameters.size());

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            auto* param = parameters[i].get();
            param->addListener (this);

            const auto address = param->getHostParameterID();
            addressForIndex[i] = address;
        }

        installParameterTree (createTopLevelNodes());
    }

    void installParameterTree (NSMutableArray<AUParameterNode*>* topLevelNodes)
    {
        if (editorObserverToken != nullptr)
        {
            [paramTree.get() removeParameterObserver:*editorObserverToken];
            delete editorObserverToken;
            editorObserverToken = nullptr;
        }

        @try
        {
            paramTree.reset ([AUParameterTree createTreeWithChildren:topLevelNodes]);
        }
        @catch (NSException* exception)
        {
            ignoreUnused (exception);
            return;
        }

        auto* self = this;

        [paramTree.get() setImplementorValueObserver:^(AUParameter* p, AUValue value) {
            self->valueChangedFromHost (p, value);
        }];

        [paramTree.get() setImplementorValueProvider:^(AUParameter* p) {
            return self->getValueForHost (p);
        }];

        [paramTree.get() setImplementorStringFromValueCallback:^(AUParameter* p, const AUValue* v) {
            return self->stringFromValue (p, v);
        }];

        [paramTree.get() setImplementorValueFromStringCallback:^(AUParameter* p, NSString* str) {
            return self->valueFromString (p, str);
        }];

        if (processor->hasEditor())
        {
            editorObserverToken = new AUParameterObserverToken ([paramTree.get() tokenByAddingParameterObserver:^(AUParameterAddress, AUValue) {
            }]);
        }
    }

    NSMutableArray<AUParameterNode*>* createTopLevelNodes()
    {
        auto* nodes = [[NSMutableArray<AUParameterNode*> alloc] init];

        const auto parameters = processor->getParameters();

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            auto* param = parameters[i].get();
            const auto address = param->getHostParameterID();
            const auto name = yupStringToNS (param->getName());
            const auto identifier = yupStringToNS (param->getID());

            AUValue minVal = static_cast<AUValue> (param->getMinimumValue());
            AUValue maxVal = static_cast<AUValue> (param->getMaximumValue());
            AUValue defaultVal = static_cast<AUValue> (param->getDefaultValue());

            auto* auParam = [AUParameterTree createParameterWithIdentifier:identifier
                                                                       name:name
                                                                    address:address
                                                                        min:minVal
                                                                        max:maxVal
                                                                       unit:kAudioUnitParameterUnit_Generic
                                                                    unitName:nil
                                                                       flags:0
                                                                valueStrings:nil
                                                         dependentParameters:nil];

            if (auParam != nullptr)
            {
                auParam.value = defaultVal;
                [nodes addObject:auParam];
            }
        }

        return nodes;
    }

    void valueChangedFromHost (AUParameter* param, AUValue value)
    {
        if (param == nullptr)
            return;

        AudioParameter* yupParam = getParamForAUAddress ([param address]);
        if (yupParam == nullptr)
            return;

        const auto normalisedValue = static_cast<float> (value) / getMaximumParameterValue (*yupParam);

        if (! approximatelyEqual (normalisedValue, yupParam->getNormalizedValue()))
        {
            yupParam->setNormalizedValue (normalisedValue);

            inParameterChangedCallback = true;
            yupParam->beginChangeGesture();
            yupParam->endChangeGesture();
        }
    }

    AUValue getValueForHost (AUParameter* param) const
    {
        if (param == nullptr)
            return 0;

        AudioParameter* yupParam = getParamForAUAddress ([param address]);
        if (yupParam == nullptr)
            return 0;

        return static_cast<AUValue> (yupParam->getNormalizedValue() * getMaximumParameterValue (*yupParam));
    }

    NSString* stringFromValue (AUParameter* param, const AUValue* value) const
    {
        if (param == nullptr || value == nullptr)
            return @"";

        AudioParameter* yupParam = getParamForAUAddress ([param address]);
        if (yupParam == nullptr)
            return @"";

        const auto normalised = static_cast<float> (*value) / getMaximumParameterValue (*yupParam);
        return yupStringToNS (yupParam->convertToString (normalised));
    }

    AUValue valueFromString (AUParameter* param, NSString* str) const
    {
        if (param == nullptr || str == nullptr)
            return 0;

        AudioParameter* yupParam = getParamForAUAddress ([param address]);
        if (yupParam == nullptr)
            return 0;

        const auto normalised = yupParam->convertFromString (String::fromCFString ((__bridge CFStringRef) str));
        return static_cast<AUValue> (normalised * getMaximumParameterValue (*yupParam));
    }

    AudioParameter* getParamForAUAddress (AUParameterAddress address) const
    {
        for (size_t i = 0; i < addressForIndex.size(); ++i)
        {
            if (addressForIndex[i] == address)
            {
                const auto parameters = processor->getParameters();
                if (i < parameters.size())
                    return parameters[i].get();
            }
        }

        return nullptr;
    }

    AudioParameter* getParamForIndex (int index) const
    {
        const auto parameters = processor->getParameters();
        if (isPositiveAndBelow (index, static_cast<int> (parameters.size())))
            return parameters[index].get();

        return nullptr;
    }

    //==============================================================================
    // Presets

    void addPresets()
    {
        if (processor == nullptr)
            return;

        auto* newPresets = [[NSMutableArray<AUAudioUnitPreset*> alloc] init];

        const int n = static_cast<int> (processor->getNumPresets());

        for (int i = 0; i < n; ++i)
        {
            auto* preset = [[AUAudioUnitPreset alloc] init];
            [preset setName:yupStringToNS (processor->getPresetName (i))];
            [preset setNumber:static_cast<NSInteger> (i)];
            [newPresets addObject:preset];
        }

        std::lock_guard<std::mutex> lock (factoryPresetsMutex);
        factoryPresets.reset (newPresets);
    }

    NSArray<AUAudioUnitPreset*>* getFactoryPresets() const
    {
        std::lock_guard<std::mutex> lock (factoryPresetsMutex);
        return factoryPresets.get();
    }

    AUAudioUnitPreset* getCurrentPreset() const
    {
        if (processor == nullptr)
            return nil;

        std::lock_guard<std::mutex> lock (factoryPresetsMutex);
        const auto current = processor->getCurrentPreset();

        if (isPositiveAndBelow (current, static_cast<int> ([factoryPresets.get() count])))
            return [factoryPresets.get() objectAtIndex:static_cast<NSUInteger> (current)];

        return nil;
    }

    void setCurrentPreset (AUAudioUnitPreset* preset)
    {
        if (processor != nullptr && preset != nullptr)
            processor->setCurrentPreset (static_cast<int> ([preset number]));
    }

    //==============================================================================
    // State

    NSDictionary<NSString*, id>* getFullState() const
    {
        auto* retval = [[NSMutableDictionary<NSString*, id> alloc] init];

        {
            auto superRetval = ObjCMsgSendSuper<AUAudioUnit, NSDictionary<NSString*, id>*> (au, @selector (fullState));
            if (superRetval != nil)
                [retval addEntriesFromDictionary:superRetval];
        }

        MemoryBlock state;
        if (processor != nullptr)
            processor->saveStateIntoMemory (state);

        if (state.getSize() > 0)
        {
            [retval setObject:[[NSData alloc] initWithBytes:state.getData() length:state.getSize()]
                       forKey:@"YUPProcessorState"];
        }

        return retval;
    }

    void setFullState (NSDictionary<NSString*, id>* state)
    {
        if (state == nil || processor == nullptr)
            return;

        id obj = [state objectForKey:@"YUPProcessorState"];
        if (obj == nil || ! [obj isKindOfClass:[NSData class]])
            return;

        auto* data = static_cast<NSData*> (obj);
        const auto numBytes = static_cast<int> ([data length]);
        if (numBytes <= 0)
            return;

        {
            ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (willChangeValueForKey:), @"allParameterValues");
        }

        MemoryBlock stateBlock ([data bytes], static_cast<size_t> (numBytes));
        processor->loadStateFromMemory (stateBlock);

        {
            ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (didChangeValueForKey:), @"allParameterValues");
        }
    }

    //==============================================================================
    // Bus management

    void addAudioUnitBusses (bool isInput)
    {
        auto* array = [[NSMutableArray<AUAudioUnitBus*> alloc] init];

        const auto& busLayout = processor->getBusLayout();
        const auto& buses = isInput ? busLayout.getInputBuses() : busLayout.getOutputBuses();

        const int numBuses = static_cast<int> (buses.size());

        for (int i = 0; i < numBuses; ++i)
        {
            const auto& bus = buses[i];
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            const int numChannels = bus.getNumChannels();

            AVAudioChannelLayout* layout = nil;
            if (numChannels <= 2)
                layout = [[AVAudioChannelLayout alloc] initWithLayoutTag:(numChannels == 1 ? kAudioChannelLayoutTag_Mono : kAudioChannelLayoutTag_Stereo)];

            AVAudioFormat* format = nil;
            if (layout != nil)
            {
                format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0 channelLayout:layout];
            }
            else
            {
                format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0 channels:static_cast<AVAudioChannelCount> (numChannels)];
            }

            NSError* error = nil;
            auto* auBus = [[AUAudioUnitBus alloc] initWithFormat:format error:&error];

            if (auBus != nil)
                [array addObject:auBus];
        }

        if (isInput)
            inputBusses.reset ([[AUAudioUnitBusArray alloc] initWithAudioUnit:au
                                                                      busType:AUAudioUnitBusTypeInput
                                                                       busses:array]);
        else
            outputBusses.reset ([[AUAudioUnitBusArray alloc] initWithAudioUnit:au
                                                                       busType:AUAudioUnitBusTypeOutput
                                                                        busses:array]);
    }

    AUAudioUnitBusArray* getInputBusses() const  { return inputBusses.get(); }
    AUAudioUnitBusArray* getOutputBusses() const { return outputBusses.get(); }

    NSArray<NSNumber*>* getChannelCapabilities() const { return channelCapabilities.get(); }

    bool shouldChangeToFormat (AVAudioFormat* format, AUAudioUnitBus* auBus)
    {
        if (allocated)
            return false;

        const auto isInput = ([auBus busType] == AUAudioUnitBusTypeInput);
        const auto busIdx = static_cast<int> ([auBus index]);
        const auto newNumChannels = static_cast<int> ([format channelCount]);

        const auto& busLayout = processor->getBusLayout();
        const auto& buses = isInput ? busLayout.getInputBuses() : busLayout.getOutputBuses();

        if (! isPositiveAndBelow (busIdx, static_cast<int> (buses.size())))
            return false;

        const auto& bus = buses[busIdx];
        if (bus.getType() != AudioBus::Type::Audio)
            return false;

        return newNumChannels > 0 && newNumChannels <= bus.getNumChannels();
    }

    //==============================================================================
    // Render resources

    bool allocateRenderResourcesAndReturnError (NSError** outError)
    {
        allocated = false;

        if (processor == nullptr)
            return false;

        if (outError != nullptr)
            *outError = nil;

        const AUAudioFrameCount maxFrames = [au maximumFramesToRender];

        auto sampleRate = 44100.0;
        for (auto* busses : { inputBusses.get(), outputBusses.get() })
        {
            if ([busses count] > 0)
            {
                sampleRate = [[[busses objectAtIndexedSubscript:0] format] sampleRate];
                break;
            }
        }

        processor->setPlaybackConfiguration (sampleRate, static_cast<int> (maxFrames));

        midiMessages.ensureSize (2048);
        midiMessages.clear();

        hostMusicalContextCallback = [au musicalContextBlock];
        hostTransportStateCallback = [au transportStateBlock];

        if (@available (macOS 10.13, *))
            midiOutputEventBlock = [au MIDIOutputEventBlock];

        lastTimeStamp.mSampleTime = std::numeric_limits<Float64>::max();
        lastTimeStamp.mFlags = 0;

        allocated = true;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "render resources allocated: sr=" << String (sampleRate) << ", maxFrames=" << String (static_cast<int> (maxFrames)));

        return true;
    }

    void deallocateRenderResources()
    {
        allocated = false;
        midiOutputEventBlock = nullptr;
        hostMusicalContextCallback = nullptr;
        hostTransportStateCallback = nullptr;
        midiMessages.clear();

        ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (deallocateRenderResources));

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "render resources deallocated");
    }

    void reset()
    {
        midiMessages.clear();
        lastTimeStamp.mSampleTime = std::numeric_limits<Float64>::max();
        lastTimeStamp.mFlags = 0;
    }

    //==============================================================================
    // Render callback

    AUAudioUnitStatus renderCallback (AudioUnitRenderActionFlags* actionFlags,
                                      const AudioTimeStamp* timestamp,
                                      AUAudioFrameCount frameCount,
                                      NSInteger outputBusNumber,
                                      AudioBufferList* outputData,
                                      const AURenderEvent* realtimeEventListHead,
                                      AURenderPullInputBlock pullInputBlock)
    {
        if (processor == nullptr)
            return kAudioUnitErr_NoConnection;

        const int numFrames = static_cast<int> (frameCount);

        if (! approximatelyEqual (lastTimeStamp.mSampleTime, timestamp->mSampleTime))
        {
            midiMessages.clear();

            // Process events (MIDI and parameters)
            processEvents (realtimeEventListHead, static_cast<AUEventSampleTime> (timestamp->mSampleTime));

            lastTimeStamp = *timestamp;

            // Prepare audio buffer
            scratchBuffer.setSize (std::max (totalInChannels, totalOutChannels), numFrames);
            scratchBuffer.clear();

            const auto& busLayout = processor->getBusLayout();

            // Pull inputs
            {
                int chIdx = 0;
                const auto& inputBuses = busLayout.getInputBuses();

                for (int busIdx = 0; busIdx < static_cast<int> (inputBuses.size()); ++busIdx)
                {
                    const auto& bus = inputBuses[busIdx];
                    if (bus.getType() != AudioBus::Type::Audio)
                        continue;

                    const int numCh = bus.getNumChannels();
                    AudioBufferList* pullData = nullptr;

                    if (pullInputBlock != nullptr)
                    {
                        AudioBufferList localBuffer = {};
                        localBuffer.mNumberBuffers = static_cast<UInt32> (numCh);

                        float* channelPtrs[16] = {};
                        for (int ch = 0; ch < numCh && ch < 16; ++ch)
                        {
                            localBuffer.mBuffers[ch].mNumberChannels = 1;
                            localBuffer.mBuffers[ch].mData = scratchBuffer.getWritePointer (chIdx + ch);
                            localBuffer.mBuffers[ch].mDataByteSize = static_cast<UInt32> (numFrames * sizeof (float));
                        }

                        if (pullInputBlock (actionFlags, timestamp, frameCount, busIdx, &localBuffer) != noErr)
                        {
                            for (int ch = 0; ch < numCh; ++ch)
                                scratchBuffer.clear (chIdx + ch, 0, numFrames);
                        }
                    }
                    else
                    {
                        for (int ch = 0; ch < numCh; ++ch)
                            scratchBuffer.clear (chIdx + ch, 0, numFrames);
                    }

                    chIdx += numCh;
                }
            }

            // Process audio
            {
                AudioPluginPlayHeadAU playHead (*this, timestamp);

                AudioProcessContext<float> context { scratchBuffer,
                                                     midiMessages,
                                                     emptyParamChangeBuffer,
                                                     &playHead };

                if (bypassHostParam != nullptr)
                    processAudioBlock (*processor, context, bypassHostParam->getNormalizedValue() > 0.5f);
                else
                    processAudioBlock (*processor, context, false);
            }

            sendMidi (static_cast<int64_t> (timestamp->mSampleTime + 0.5), frameCount);
        }

        // Copy output data
        if (outputData != nullptr && outputBusNumber >= 0)
        {
            const auto& outputBuses = processor->getBusLayout().getOutputBuses();
            int chIdx = 0;

            for (int busIdx = 0; busIdx < static_cast<int> (outputBuses.size()); ++busIdx)
            {
                const auto& bus = outputBuses[busIdx];
                if (bus.getType() != AudioBus::Type::Audio)
                    continue;

                const int numCh = bus.getNumChannels();

                if (busIdx == static_cast<int> (outputBusNumber))
                {
                    for (int ch = 0; ch < numCh && ch < static_cast<int> (outputData->mNumberBuffers); ++ch)
                    {
                        const auto* src = scratchBuffer.getReadPointer (chIdx + ch);
                        auto* dst = static_cast<float*> (outputData->mBuffers[ch].mData);

                        std::copy (src, src + numFrames, dst);
                    }
                }

                chIdx += numCh;
            }
        }

        return noErr;
    }

    void processEvents (const AURenderEvent* realtimeEventListHead, AUEventSampleTime startTime)
    {
        for (const AURenderEvent* event = realtimeEventListHead; event != nullptr; event = event->head.next)
        {
            switch (event->head.eventType)
            {
                case AURenderEventMIDI:
                case AURenderEventMIDISysEx:
                {
                    const AUMIDIEvent& midiEvent = event->MIDI;
                    midiMessages.addEvent (midiEvent.data,
                                           static_cast<int> (midiEvent.length),
                                           static_cast<int> (midiEvent.eventSampleTime - startTime));
                }
                break;

                case AURenderEventParameter:
                case AURenderEventParameterRamp:
                {
                    const AUParameterEvent& paramEvent = event->parameter;

                    if (auto* p = getParamForAUAddress (paramEvent.parameterAddress))
                    {
                        auto normalisedValue = static_cast<float> (paramEvent.value) / getMaximumParameterValue (*p);
                        p->setNormalizedValue (normalisedValue);

                        inParameterChangedCallback = true;
                    }
                }
                break;

                default:
                    break;
            }
        }
    }

    void sendMidi (int64_t baseTimeStamp, AUAudioFrameCount frameCount)
    {
        ignoreUnused (baseTimeStamp);
        ignoreUnused (frameCount);

        if (! processor->producesMidi())
            return;

        if (@available (macOS 10.13, *))
        {
            if (auto midiOut = midiOutputEventBlock)
            {
                for (const auto& metadata : midiMessages)
                {
                    if (! isPositiveAndBelow (metadata.samplePosition, static_cast<int> (frameCount)))
                        continue;

                    midiOut (static_cast<int64_t> (metadata.samplePosition) + baseTimeStamp,
                             0,
                             metadata.numBytes,
                             metadata.data);
                }
            }
        }
    }

    //==============================================================================
    // Bypass

    bool getShouldBypassEffect() const
    {
        if (bypassHostParam != nullptr)
            return bypassHostParam->getNormalizedValue() > 0.5f;

        return false;
    }

    void setShouldBypassEffect (bool shouldBypass)
    {
        if (bypassHostParam != nullptr)
            bypassHostParam->setNormalizedValue (shouldBypass ? 1.0f : 0.0f);
    }

    //==============================================================================
    // Properties

    NSTimeInterval getLatency() const
    {
        if (processor == nullptr)
            return 0.0;

        return static_cast<NSTimeInterval> (processor->getLatencySamples()) / processor->getSampleRate();
    }

    NSTimeInterval getTailTime() const
    {
        if (processor == nullptr)
            return 0.0;

        return static_cast<NSTimeInterval> (processor->getTailSamples()) / processor->getSampleRate();
    }

    bool getRenderingOffline() const
    {
        return processor != nullptr && processor->isOfflineProcessing();
    }

    void setRenderingOffline (bool offline)
    {
        if (processor != nullptr)
            processor->setOfflineProcessing (offline);
    }

    //==============================================================================
    // MIDI

    int getVirtualMIDICableCount() const
    {
        return (processor != nullptr && processor->acceptsMidi()) ? 1 : 0;
    }

    bool getSupportsMPE() const
    {
        return false;
    }

    NSArray<NSString*>* getMIDIOutputNames() const
    {
        if (processor != nullptr && processor->producesMidi())
            return @[@"MIDI Out"];

        return @[];
    }

    //==============================================================================
    // Transport / PlayHead

    std::optional<AudioPlayHead::PositionInfo> getPosition() const override
    {
        PositionInfo info;

        info.setTimeInSamples (static_cast<int64_t> (lastTimeStamp.mSampleTime + 0.5));

        if (processor != nullptr && processor->getSampleRate() > 0.0)
            info.setTimeInSeconds (static_cast<double> (*info.getTimeInSamples()) / processor->getSampleRate());

        double num = 0, ppqPosition = 0;
        NSInteger den = 0;
        NSInteger deltaSampleOffsetToNextBeat = 0;
        double currentMeasureDownBeat = 0, bpm = 0;

        if (hostMusicalContextCallback != nullptr)
        {
            AUHostMusicalContextBlock musicalContextCallback = hostMusicalContextCallback;

            if (musicalContextCallback (&bpm, &num, &den, &ppqPosition, &deltaSampleOffsetToNextBeat, &currentMeasureDownBeat))
            {
                info.setTimeSignature (TimeSignature { static_cast<int> (num), static_cast<int> (den) });
                info.setPpqPositionOfLastBarStart (currentMeasureDownBeat);
                info.setBpm (bpm);
                info.setPpqPosition (ppqPosition);
            }
        }

        double outCurrentSampleInTimeLine = 0, outCycleStartBeat = 0, outCycleEndBeat = 0;
        AUHostTransportStateFlags flags = 0;

        if (hostTransportStateCallback != nullptr)
        {
            AUHostTransportStateBlock transportStateCallback = hostTransportStateCallback;

            if (transportStateCallback (&flags, &outCurrentSampleInTimeLine, &outCycleStartBeat, &outCycleEndBeat))
            {
                info.setTimeInSamples (static_cast<int64_t> (outCurrentSampleInTimeLine + 0.5));

                if (processor != nullptr && processor->getSampleRate() > 0.0)
                    info.setTimeInSeconds (static_cast<double> (outCurrentSampleInTimeLine) / processor->getSampleRate());

                info.setIsPlaying ((flags & AUHostTransportStateMoving) != 0);
                info.setIsLooping ((flags & AUHostTransportStateCycling) != 0);
                info.setIsRecording ((flags & AUHostTransportStateRecording) != 0);
                info.setLoopPoints (LoopPoints { outCycleStartBeat, outCycleEndBeat });
            }
        }

        if ((lastTimeStamp.mFlags & kAudioTimeStampHostTimeValid) != 0)
            info.setHostTimeNs (static_cast<int64_t> (AudioConvertHostTimeToNanos (lastTimeStamp.mHostTime)));

        return info;
    }

    //==============================================================================
    // View configurations

    NSIndexSet* supportedViewConfigurations (NSArray<AUAudioUnitViewConfiguration*>* configs) const
    {
        auto* indices = [[NSMutableIndexSet alloc] init];

        if (! processor->hasEditor())
            return indices;

        auto* editor = processor->createEditor();
        if (editor == nullptr)
            return indices;

        for (NSUInteger i = 0; i < [configs count]; ++i)
        {
            [[maybe_unused]] auto* config = [configs objectAtIndex:i];
            [indices addIndex:i];
        }

        delete editor;
        return indices;
    }

    void selectViewConfiguration (AUAudioUnitViewConfiguration* config)
    {
        viewConfigWidth = [config width];
        viewConfigHeight = [config height];
    }

    //==============================================================================
    // Listener callbacks

    void audioProcessorChanged (AudioProcessorBase*, const AudioProcessorBase::ChangeDetails& details) override
    {
        if (details.programChanged)
        {
            {
                ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (willChangeValueForKey:), @"allParameterValues");
            }
            addPresets();
            {
                ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (didChangeValueForKey:), @"allParameterValues");
            }

            {
                ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (willChangeValueForKey:), @"currentPreset");
            }
            {
                ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (didChangeValueForKey:), @"currentPreset");
            }
        }

        if (details.latencyChanged)
        {
            {
                ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (willChangeValueForKey:), @"latency");
            }
            {
                ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (didChangeValueForKey:), @"latency");
            }
        }

        if (details.parameterInfoChanged)
        {
            updateParameterTree();
        }
    }

    void updateParameterTree()
    {
        unregisterAllParameterListeners();

        {
            ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (willChangeValueForKey:), @"parameterTree");
        }

        installParameterTree (createTopLevelNodes());

        {
            ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (didChangeValueForKey:), @"parameterTree");
        }

        registerAllParameterListeners();
    }

    void sendParameterEvent (int idx, float newValue, AUParameterAutomationEventType type)
    {
        if (inParameterChangedCallback.get())
        {
            inParameterChangedCallback = false;
            return;
        }

        if (! isPositiveAndBelow (static_cast<size_t> (idx), addressForIndex.size()))
            return;

        if (auto* p = [paramTree.get() parameterWithAddress:addressForIndex[idx]])
        {
            auto* yupParam = getParamForIndex (idx);
            if (yupParam == nullptr)
                return;

            const auto value = newValue * getMaximumParameterValue (*yupParam);

            if (@available (macOS 10.12, *))
            {
                [p setValue:value
                 originator:editorObserverToken
                 atHostTime:lastTimeStamp.mHostTime
                  eventType:type];
            }
            else if (type == AUParameterAutomationEventTypeValue)
            {
                [p setValue:value originator:editorObserverToken];
            }
        }
    }

    void parameterValueChanged (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        ignoreUnused (parameter);
        sendParameterEvent (indexInContainer, parameter->getNormalizedValue(), AUParameterAutomationEventTypeValue);
    }

    void parameterGestureBegin (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        ignoreUnused (parameter);
        sendParameterEvent (indexInContainer, parameter->getNormalizedValue(), AUParameterAutomationEventTypeTouch);
    }

    void parameterGestureEnd (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        ignoreUnused (parameter);
        sendParameterEvent (indexInContainer, parameter->getNormalizedValue(), AUParameterAutomationEventTypeRelease);
    }

    //==============================================================================
    // Instance management

    void registerAllParameterListeners()
    {
        const auto parameters = processor->getParameters();
        for (auto& p : parameters)
            p->addListener (this);
    }

    void unregisterAllParameterListeners()
    {
        const auto parameters = processor->getParameters();
        for (auto& p : parameters)
            p->removeListener (this);
    }

    //==============================================================================
    // Accessors

    AUAudioUnit* getAudioUnit() const { return au; }
    AUParameterTree* getParameterTree() const { return paramTree.get(); }
    AUInternalRenderBlock getInternalRenderBlock() const { return internalRenderBlock; }
    AURenderContextObserver getRenderContextObserver() const { return renderContextObserver; }
    bool isRenderResourcesAllocated() const { return allocated; }

private:
    //==============================================================================
    // PlayHead helper

    class AudioPluginPlayHeadAU final : public AudioPlayHead
    {
    public:
        AudioPluginPlayHeadAU (AudioPluginProcessorAUv3& o, const AudioTimeStamp* ts)
            : owner (o), timeStamp (ts)
        {
        }

        std::optional<PositionInfo> getPosition() const override
        {
            return owner.getPosition();
        }

    private:
        AudioPluginProcessorAUv3& owner;
        const AudioTimeStamp* timeStamp;
    };

    //==============================================================================

    AUAudioUnit* au = nil;
    std::unique_ptr<AudioProcessor> processor;

    int totalInChannels = 0;
    int totalOutChannels = 0;

    std::vector<AUParameterAddress> addressForIndex;

    NSUniquePtr<AUAudioUnitBusArray> inputBusses;
    NSUniquePtr<AUAudioUnitBusArray> outputBusses;
    NSUniquePtr<NSMutableArray<NSNumber*>> channelCapabilities;

    NSUniquePtr<AUParameterTree> paramTree;
    AUParameterObserverToken* editorObserverToken = nullptr;

    mutable std::mutex factoryPresetsMutex;
    NSUniquePtr<NSMutableArray<AUAudioUnitPreset*>> factoryPresets;

    ObjCBlock<AUInternalRenderBlock> internalRenderBlock;
    ObjCBlock<AURenderContextObserver> renderContextObserver;

    MidiBuffer midiMessages;
    ParameterChangeBuffer emptyParamChangeBuffer;
    AudioBuffer<float> scratchBuffer;

    AUMIDIOutputEventBlock midiOutputEventBlock = nullptr;

    ObjCBlock<AUHostMusicalContextBlock> hostMusicalContextCallback;
    ObjCBlock<AUHostTransportStateBlock> hostTransportStateCallback;

    AudioTimeStamp lastTimeStamp;

    AudioParameter* bypassHostParam = nullptr;

    double viewConfigWidth = 0;
    double viewConfigHeight = 0;

    ThreadLocalValue<bool> inParameterChangedCallback;
    bool allocated = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginProcessorAUv3)
};

//==============================================================================
// Dynamic AUAudioUnit subclass using YUP's ObjCClass

struct AUAudioUnitSubclass final : public ObjCClass<AUAudioUnit>
{
    AUAudioUnitSubclass()
        : ObjCClass<AUAudioUnit> ("YUPAUAudioUnit_")
    {
        addIvar<AudioPluginProcessorAUv3*> ("cppObject");

        YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
        addMethod (@selector (initWithComponentDescription:options:error:yupClass:), [] (id self,
                                                                                         SEL,
                                                                                         AudioComponentDescription descr,
                                                                                         AudioComponentInstantiationOptions options,
                                                                                         NSError** error,
                                                                                         AudioPluginProcessorAUv3* cpp) {
            self = ObjCMsgSendSuper<AUAudioUnit, AUAudioUnit*, AudioComponentDescription,
                                    AudioComponentInstantiationOptions, NSError * __autoreleasing *> (self, @selector (initWithComponentDescription:options:error:), descr, options, error);
            setThis (self, cpp);
            return self;
        });
        YUP_END_IGNORE_WARNINGS_GCC_LIKE

        addMethod (@selector (initWithComponentDescription:options:error:), [] (id self, SEL, AudioComponentDescription descr, AudioComponentInstantiationOptions options, NSError** error) {
            self = ObjCMsgSendSuper<AUAudioUnit, AUAudioUnit*, AudioComponentDescription,
                                    AudioComponentInstantiationOptions, NSError * __autoreleasing *> (self, @selector (initWithComponentDescription:options:error:), descr, options, error);

            auto* cpp = new AudioPluginProcessorAUv3 (self, descr, options, error);
            setThis (self, cpp);
            return self;
        });

        YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
        addMethod (sel_registerName ("dealloc"), [] (id self, SEL) {
            auto* cpp = _this (self);
            delete cpp;
            setThis (self, nullptr);
        });
        YUP_END_IGNORE_WARNINGS_GCC_LIKE

        // Internal render block
        addMethod (@selector (internalRenderBlock), [] (id self, SEL) {
            return _this (self)->getInternalRenderBlock();
        });

        // Parameter tree
        addMethod (@selector (parameterTree), [] (id self, SEL) {
            return _this (self)->getParameterTree();
        });

        // Busses
        addMethod (@selector (inputBusses), [] (id self, SEL) {
            return _this (self)->getInputBusses();
        });

        addMethod (@selector (outputBusses), [] (id self, SEL) {
            return _this (self)->getOutputBusses();
        });

        addMethod (@selector (channelCapabilities), [] (id self, SEL) {
            return _this (self)->getChannelCapabilities();
        });

        addMethod (@selector (shouldChangeToFormat:forBus:), [] (id self, SEL, AVAudioFormat* format, AUAudioUnitBus* bus) {
            return _this (self)->shouldChangeToFormat (format, bus) ? YES : NO;
        });

        // Render resources
        addMethod (@selector (allocateRenderResourcesAndReturnError:), [] (id self, SEL, NSError** error) {
            return _this (self)->allocateRenderResourcesAndReturnError (error) ? YES : NO;
        });

        addMethod (@selector (deallocateRenderResources), [] (id self, SEL) {
            _this (self)->deallocateRenderResources();
        });

        addMethod (@selector (renderResourcesAllocated), [] (id self, SEL) {
            return _this (self)->isRenderResourcesAllocated() ? YES : NO;
        });

        addMethod (@selector (reset), [] (id self, SEL) {
            _this (self)->reset();
        });

        // State
        addMethod (@selector (fullState), [] (id self, SEL) {
            return _this (self)->getFullState();
        });

        addMethod (@selector (setFullState:), [] (id self, SEL, NSDictionary<NSString*, id>* state) {
            _this (self)->setFullState (state);
        });

        // Presets
        addMethod (@selector (factoryPresets), [] (id self, SEL) {
            return _this (self)->getFactoryPresets();
        });

        addMethod (@selector (currentPreset), [] (id self, SEL) {
            return _this (self)->getCurrentPreset();
        });

        addMethod (@selector (setCurrentPreset:), [] (id self, SEL, AUAudioUnitPreset* preset) {
            _this (self)->setCurrentPreset (preset);
        });

        // Latency / tail
        addMethod (@selector (latency), [] (id self, SEL) {
            return _this (self)->getLatency();
        });

        addMethod (@selector (tailTime), [] (id self, SEL) {
            return _this (self)->getTailTime();
        });

        // Rendering offline
        addMethod (@selector (isRenderingOffline), [] (id self, SEL) {
            return _this (self)->getRenderingOffline() ? YES : NO;
        });

        addMethod (@selector (setRenderingOffline:), [] (id self, SEL, BOOL offline) {
            _this (self)->setRenderingOffline (offline);
        });

        // Bypass
        addMethod (@selector (shouldBypassEffect), [] (id self, SEL) {
            return _this (self)->getShouldBypassEffect() ? YES : NO;
        });

        addMethod (@selector (setShouldBypassEffect:), [] (id self, SEL, BOOL bypass) {
            _this (self)->setShouldBypassEffect (bypass);
        });

        // Can process in place
        addMethod (@selector (canProcessInPlace), [] (id, SEL) {
            return NO;
        });

        // MIDI
        addMethod (@selector (virtualMIDICableCount), [] (id self, SEL) {
            return _this (self)->getVirtualMIDICableCount();
        });

        YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
        addMethod (@selector (supportsMPE), [] (id self, SEL) {
            return _this (self)->getSupportsMPE() ? YES : NO;
        });
        YUP_END_IGNORE_WARNINGS_GCC_LIKE

        if (@available (macOS 10.13, *))
        {
            addMethod (@selector (MIDIOutputNames), [] (id self, SEL) {
                return _this (self)->getMIDIOutputNames();
            });
        }

        // View configurations
        if (@available (macOS 10.13, *))
        {
            addMethod (@selector (supportedViewConfigurations:), [] (id self, SEL, NSArray<AUAudioUnitViewConfiguration*>* configs) {
                return _this (self)->supportedViewConfigurations (configs);
            });

            addMethod (@selector (selectViewConfiguration:), [] (id self, SEL, AUAudioUnitViewConfiguration* config) {
                _this (self)->selectViewConfiguration (config);
            });
        }

        // Render context observer
        addMethod (@selector (renderContextObserver), [] (id self, SEL) {
            return _this (self)->getRenderContextObserver();
        });

        registerClass();
    }

    static AudioPluginProcessorAUv3* _this (id self)
    {
        return getIvar<AudioPluginProcessorAUv3*> (self, "cppObject");
    }

    static void setThis (id self, AudioPluginProcessorAUv3* cpp)
    {
        setIvar (self, "cppObject", cpp);
    }
};

//==============================================================================
// View controller C++ companion

class AudioPluginViewControllerv3
    : public AudioProcessorBase::Listener
{
public:
    explicit AudioPluginViewControllerv3 (AUViewController<AUAudioUnitFactory>* vc)
        : myself (vc)
    {
        initialiseYup_GUI();
        processor.reset (::createPluginProcessor());
    }

    ~AudioPluginViewControllerv3() override
    {
        if (processor != nullptr)
        {
            yup::endActiveParameterGestures (processor.get());
            processor->removeListener (this);

            if (editor != nullptr)
            {
                delete editor;
                editor = nullptr;
            }
        }
    }

    void loadView()
    {
        if (processor == nullptr)
            return;

        if (processor->hasEditor())
        {
            editor = processor->createEditor();

            if (editor != nullptr)
            {
                preferredSize = { editor->getWidth(), editor->getHeight() };

                NSView* view = [[NSView alloc] initWithFrame:NSMakeRect (0, 0, preferredSize.getWidth(), preferredSize.getHeight())];
                [myself setView:view];

                editor->setVisible (true);

                auto options = ComponentNative::Options()
                                   .withFlags (ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow);

                editor->addToDesktop (options, (__bridge void*) view);
            }
        }
    }

    void viewDidLayoutSubviews()
    {
        if (processor == nullptr)
            return;

        if ([myself view] != nil)
        {
            if (editor != nullptr)
            {
                const auto bounds = [[myself view] bounds];
                editor->setBounds ({ 0.0f,
                                     0.0f,
                                     static_cast<float> (NSWidth (bounds)),
                                     static_cast<float> (NSHeight (bounds)) });
            }
        }
    }

    CGSize getPreferredContentSize() const
    {
        return CGSizeMake (static_cast<CGFloat> (preferredSize.getWidth()),
                           static_cast<CGFloat> (preferredSize.getHeight()));
    }

    //==============================================================================
    // Listener

    void audioProcessorChanged (AudioProcessorBase*, const AudioProcessorBase::ChangeDetails&) override {}

    //==============================================================================

    AUAudioUnit* createAudioUnit (const AudioComponentDescription& desc, NSError** error)
    {
        if (processor == nullptr)
            return nil;

        auto* cpp = new AudioPluginProcessorAUv3 (nil, desc, 0, error);

        static AUAudioUnitSubclass auClass;
        auto* au = auClass.createInstance();
        au = ObjCMsgSendSuper<AUAudioUnit, AUAudioUnit*, AudioComponentDescription,
                              AudioComponentInstantiationOptions, NSError * __autoreleasing *> (au, @selector (initWithComponentDescription:options:error:), desc, 0, error);

        if (au == nil)
        {
            delete cpp;
            return nil;
        }

        AUAudioUnitSubclass::setThis (au, cpp);
        cpp->init();

        return au;
    }

    AudioProcessor* getProcessor() const { return processor.get(); }

private:
    AUViewController<AUAudioUnitFactory>* myself = nil;
    std::unique_ptr<AudioProcessor> processor;
    AudioProcessorEditor* editor = nullptr;
    Rectangle<float> preferredSize { 1.0f, 1.0f };
};

} // namespace yup

//==============================================================================
// Static AUViewController implementation

@interface YUPAUv3ViewController : AUViewController <AUAudioUnitFactory>
{
    std::unique_ptr<yup::AudioPluginViewControllerv3> cpp;
}
@end

@implementation YUPAUv3ViewController

- (instancetype) initWithNibName:(nullable NSString*)nib bundle:(nullable NSBundle*)bndl
{
    self = [super initWithNibName:nib bundle:bndl];
    cpp.reset (new yup::AudioPluginViewControllerv3 (self));
    return self;
}

- (void) loadView
{
    cpp->loadView();
}

- (AUAudioUnit*) createAudioUnitWithComponentDescription:(AudioComponentDescription)desc
                                                   error:(NSError**)error
{
    return cpp->createAudioUnit (desc, error);
}

- (CGSize) preferredContentSize
{
    return cpp->getPreferredContentSize();
}

- (void) viewDidLayoutSubviews
{
    cpp->viewDidLayoutSubviews();
}

- (void) viewDidLayout
{
    cpp->viewDidLayoutSubviews();
}

@end

#endif // YUP_MAC
