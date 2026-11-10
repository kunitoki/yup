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

NS_ASSUME_NONNULL_BEGIN

extern "C" yup::AudioProcessor* YUP_AUDIO_PLUGIN_CREATE_FUNCTION();

namespace yup
{

//==============================================================================

/** Returns value-strings for enumerated/stepped parameters so the host
    can display discrete choices instead of a raw numeric value.
*/
static NSArray<NSString*>* _Nullable makeAUValueStrings (const AudioParameter& param)
{
    if (! param.isEnum() && ! param.isStepped())
        return nil;

    const int numSteps = param.getNumSteps();
    if (numSteps <= 0)
        return nil;

    const int numValues = numSteps + 1; // a range with N steps has N+1 discrete values
    auto* strings = [[NSMutableArray<NSString*> alloc] initWithCapacity:static_cast<NSUInteger> (numValues)];

    const float stepSize = (param.getMaximumValue() - param.getMinimumValue()) / static_cast<float> (numSteps);

    for (int i = 0; i < numValues; ++i)
    {
        const auto stepValue = param.getMinimumValue() + static_cast<float> (i) * stepSize;
        [strings addObject:yupStringToNS (param.convertToString (stepValue))];
    }

    return strings;
}

//==============================================================================

/** Magic/version used to wrap the YUP processor state blob with wrapper-owned
    bypass state so host bypass survives preset/session restore, matching the
    approach used by the VST3, CLAP, LV2, and AAX wrappers. Legacy raw processor
    state (without this magic) is still loaded via readWrapperBypassState's fallback.
*/
constexpr int auv3WrapperStateMagic = 0x33564159; // "YAV3"
constexpr int auv3WrapperStateVersion = 1;

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
                              AudioComponentDescription description,
                              AudioComponentInstantiationOptions,
                              NSError**)
        : au (audioUnit)
    {
        ignoreUnused (description);

        processor.reset (yup::createPluginProcessorForWrapper (yup::AudioPluginWrapperType::auv3, ::YUP_AUDIO_PLUGIN_CREATE_FUNCTION));

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "created processor instance: wrapper=" << describePointer (this) << ", audioUnit=" << describePointer ((__bridge void*) audioUnit) << ", processor=" << describePointer (processor.get()) << ", type=" << String::toHexString (static_cast<int> (description.componentType)) << ", subType=" << String::toHexString (static_cast<int> (description.componentSubType)) << ", manufacturer=" << String::toHexString (static_cast<int> (description.componentManufacturer)));

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "createPluginProcessor returned null");
        }

        init();
    }

    ~AudioPluginProcessorAUv3() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "destroying processor instance: wrapper=" << describePointer (this) << ", audioUnit=" << describePointer ((__bridge void*) au) << ", processor=" << describePointer (processor.get()));

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
        jassert (au != nil); // The AUAudioUnit must be set before initialization

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "init skipped: processor is null");
            return;
        }

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

        // Build channel capabilities (one entry per audio bus)
        {
            channelCapabilities.reset ([[NSMutableArray<NSNumber*> alloc] init]);

            for (const auto& bus : busLayout.getInputBuses())
                if (bus.getType() == AudioBus::Type::Audio)
                    [channelCapabilities.get() addObject:[NSNumber numberWithInteger:bus.getNumChannels()]];

            for (const auto& bus : busLayout.getOutputBuses())
                if (bus.getType() == AudioBus::Type::Audio)
                    [channelCapabilities.get() addObject:[NSNumber numberWithInteger:bus.getNumChannels()]];
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

        bypass.attach (*processor);

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
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "installParameterTree failed: " << String::fromCFString ((__bridge CFStringRef)[exception reason]));
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
                                                                       unit:makeAUUnit (*param)
                                                                    unitName:(param->getUnit() == AudioParameter::ParameterUnit::Custom
                                                                                  ? yupStringToNS (param->getUnitName())
                                                                                  : nil)
                                                                       flags:makeAUv3ParameterFlags (*param)
                                                                valueStrings:makeAUValueStrings (*param)
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

        if (! approximatelyEqual (static_cast<float> (value), yupParam->getValue()))
        {
            yupParam->setValue (static_cast<float> (value));

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

        return static_cast<AUValue> (yupParam->getValue());
    }

    NSString* stringFromValue (AUParameter* param, const AUValue* value) const
    {
        if (param == nullptr || value == nullptr)
            return @"";

        AudioParameter* yupParam = getParamForAUAddress ([param address]);
        if (yupParam == nullptr)
            return @"";

        return yupStringToNS (yupParam->convertToString (static_cast<float> (*value)));
    }

    AUValue valueFromString (AUParameter* param, NSString* str) const
    {
        if (param == nullptr || str == nullptr)
            return 0;

        AudioParameter* yupParam = getParamForAUAddress ([param address]);
        if (yupParam == nullptr)
            return 0;

        return static_cast<AUValue> (yupParam->convertFromString (String::fromCFString ((__bridge CFStringRef) str)));
    }

    AudioParameter* _Nullable getParamForAUAddress (AUParameterAddress address) const
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

    AudioParameter* _Nullable getParamForIndex (int index) const
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

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "factory presets built: count=" << String (n));
    }

    NSArray<AUAudioUnitPreset*>* getFactoryPresets() const
    {
        std::lock_guard<std::mutex> lock (factoryPresetsMutex);
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "factoryPresets requested: count=" << String (static_cast<int> ([factoryPresets.get() count])));
        return factoryPresets.get();
    }

    AUAudioUnitPreset* _Nullable getCurrentPreset() const
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
        if (processor == nullptr || preset == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setCurrentPreset failed: processor=" << describePointer (processor.get()) << ", preset=" << describePointer ((__bridge void*) preset));
            return;
        }

        const auto index = static_cast<int> ([preset number]);
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setCurrentPreset requested: preset=" << String (index));

        if (isPositiveAndBelow (index, processor->getNumPresets()))
        {
            processor->setCurrentPreset (index);
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setCurrentPreset completed");
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setCurrentPreset failed: preset out of range");
        }
    }

    //==============================================================================
    // State

    NSDictionary<NSString*, id>* getFullState() const
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "fullState requested");

        auto* retval = [[NSMutableDictionary<NSString*, id> alloc] init];

        {
            auto superRetval = ObjCMsgSendSuper<AUAudioUnit, NSDictionary<NSString*, id>*> (au, @selector (fullState));
            if (superRetval != nil)
                [retval addEntriesFromDictionary:superRetval];
        }

        MemoryBlock state;
        if (processor != nullptr)
        {
            const auto result = processor->saveStateIntoMemory (state);

            if (result.failed())
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "fullState processor save failed: " << result.getErrorMessage());
            }
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "fullState without processor state: processor is null");
        }

        // Wrap processor state together with the wrapper-owned bypass state so
        // host bypass survives preset/session restore (see auv3WrapperStateMagic).
        const auto wrapperState = writeWrapperBypassState (auv3WrapperStateMagic,
                                                          auv3WrapperStateVersion,
                                                          getShouldBypassEffect(),
                                                          state,
                                                          state.getSize() > 0);

        if (wrapperState.getSize() > 0)
        {
            [retval setObject:[[NSData alloc] initWithBytes:wrapperState.getData() length:wrapperState.getSize()]
                       forKey:(__bridge NSString*) getAUProcessorStateKey()];
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "fullState completed: processorBytes=" << String (static_cast<int64> (state.getSize())) << ", wrapperBytes=" << String (static_cast<int64> (wrapperState.getSize())));

        return retval;
    }

    void setFullState (NSDictionary<NSString*, id>* state)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setFullState requested");

        if (state == nil || processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setFullState failed: state=" << describePointer ((__bridge void*) state) << ", processor=" << describePointer (processor.get()));
            return;
        }

        id obj = [state objectForKey:(__bridge NSString*) getAUProcessorStateKey()];
        if (obj == nil || ! [obj isKindOfClass:[NSData class]])
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setFullState completed without processor state: key missing or not data");
            return;
        }

        auto* data = static_cast<NSData*> (obj);
        const auto numBytes = static_cast<int> ([data length]);
        if (numBytes <= 0)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setFullState completed without processor state: data is empty");
            return;
        }

        {
            ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (willChangeValueForKey:), @"allParameterValues");
        }

        MemoryBlock stateBlock ([data bytes], static_cast<size_t> (numBytes));
        const auto wrapperState = readWrapperBypassState (stateBlock, auv3WrapperStateMagic, auv3WrapperStateVersion);

        // Restore wrapper-owned bypass state when present, otherwise fall back to
        // treating the whole blob as legacy raw processor state.
        if (wrapperState.hasWrapperState)
            setShouldBypassEffect (wrapperState.isBypassed);

        const bool shouldLoadProcessorState = ! wrapperState.hasWrapperState || wrapperState.hasProcessorState;
        if (shouldLoadProcessorState && wrapperState.processorState.getSize() > 0)
        {
            const auto result = processor->loadStateFromMemory (wrapperState.processorState);
            ignoreUnused (result);

            if (wrapperState.hasWrapperState)
                setShouldBypassEffect (wrapperState.isBypassed);

            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setFullState " << String (result.wasOk() ? "completed" : "failed") << ": bytes=" << String (numBytes) << ", processorBytes=" << String (static_cast<int64> (wrapperState.processorState.getSize())) << ", hasWrapperState=" << String (wrapperState.hasWrapperState ? "true" : "false") << (result.wasOk() ? String() : ", error=" + result.getErrorMessage()));
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "setFullState completed without processor state: bytes=" << String (numBytes) << ", hasWrapperState=" << String (wrapperState.hasWrapperState ? "true" : "false"));
        }

        {
            ObjCMsgSendSuper<AUAudioUnit, void> (au, @selector (didChangeValueForKey:), @"allParameterValues");
        }
    }

    //==============================================================================
    // Bus management

    void addAudioUnitBusses (bool isInput)
    {
        jassert (au != nil); // The AUAudioUnit must be set before creating bus arrays

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
            {
                auBus.name = yupStringToNS (bus.getName());
                [array addObject:auBus];
            }
            else
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "bus creation failed: isInput=" << String (isInput ? "true" : "false") << ", bus=" << String (i) << ", numChannels=" << String (numChannels) << ", error=" << (error != nil ? String::fromCFString ((__bridge CFStringRef)[error localizedDescription]) : String ("unknown")));
            }
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "busses created: isInput=" << String (isInput ? "true" : "false") << ", count=" << String (static_cast<int> ([array count])));

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

    NSArray<NSNumber*>* getChannelCapabilities() const
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "channelCapabilities returned " << String (static_cast<int> ([channelCapabilities.get() count])) << " entries");
        return channelCapabilities.get();
    }

    bool shouldChangeToFormat (AVAudioFormat* format, AUAudioUnitBus* auBus)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "shouldChangeToFormat requested: isInput=" << String ([auBus busType] == AUAudioUnitBusTypeInput ? "true" : "false") << ", bus=" << String (static_cast<int> ([auBus index])) << ", sampleRate=" << String ([format sampleRate]) << ", channels=" << String (static_cast<int> ([format channelCount])) << ", interleaved=" << String ([format isInterleaved] ? "true" : "false"));

        if (allocated)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "shouldChangeToFormat rejected: render resources allocated");
            return false;
        }

        // Accept Float32 always, Float64 if the processor supports it.
        // Other sample formats (Int16, Int32, etc.) are rejected — AUv3 hosts
        // predominantly use floating-point.
        const auto commonFormat = [format commonFormat];
        if (commonFormat != AVAudioPCMFormatFloat32)
        {
            if (commonFormat != AVAudioPCMFormatFloat64
                || processor == nullptr
                || ! processor->supportsDoublePrecisionProcessing())
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "shouldChangeToFormat rejected: unsupported sample format " << String (static_cast<int> (commonFormat)));
                return false;
            }
        }

        // Accept both interleaved and non-interleaved.  The render callback
        // handles deinterleave / interleave internally via AudioData converters,
        // so there is no need to reject interleaved formats here.

        const auto isInput = ([auBus busType] == AUAudioUnitBusTypeInput);
        const auto busIdx = static_cast<int> ([auBus index]);
        const auto newNumChannels = static_cast<int> ([format channelCount]);

        const auto& busLayout = processor->getBusLayout();
        const auto& buses = isInput ? busLayout.getInputBuses() : busLayout.getOutputBuses();

        if (! isPositiveAndBelow (busIdx, static_cast<int> (buses.size())))
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "shouldChangeToFormat rejected: bus index out of range");
            return false;
        }

        const auto& bus = buses[busIdx];
        if (bus.getType() != AudioBus::Type::Audio)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "shouldChangeToFormat rejected: bus is not audio");
            return false;
        }

        const auto accepted = newNumChannels > 0 && newNumChannels == bus.getNumChannels();
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "shouldChangeToFormat " << String (accepted ? "accepted" : "rejected") << ": requestedChannels=" << String (newNumChannels) << ", busChannels=" << String (bus.getNumChannels()));
        return accepted;
    }

    //==============================================================================
    // Render resources

    bool allocateRenderResourcesAndReturnError (NSError** outError)
    {
        jassert (au != nil); // The AUAudioUnit must be set before allocating render resources

        allocated = false;

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "allocateRenderResources failed: processor is null");
            return false;
        }

        if (outError != nullptr)
            *outError = nil;

        const AUAudioFrameCount maxFrames = [au maximumFramesToRender];

        auto sampleRate = 44100.0;
        bool sampleRateSet = false;
        size_t maxInterleavedBytes = 0;

        // Validate bus formats and compute required scratch space.
        // Float32 and Float64 are accepted (Float64 gated by shouldChangeToFormat).
        // Both interleaved and non-interleaved are accepted — conversion happens in the
        // render callback via AudioData converters.
        for (auto* busses : { inputBusses.get(), outputBusses.get() })
        {
            for (NSUInteger i = 0; i < [busses count]; ++i)
            {
                auto* auBus = [busses objectAtIndexedSubscript:i];
                auto* fmt = [auBus format];

                // Only float formats are supported (shouldChangeToFormat already gates this)
                const auto commonFormat = [fmt commonFormat];
                if (commonFormat != AVAudioPCMFormatFloat32 && commonFormat != AVAudioPCMFormatFloat64)
                {
                    if (outError != nullptr)
                        *outError = [NSError errorWithDomain:NSOSStatusErrorDomain
                                                       code:kAudioUnitErr_FormatNotSupported
                                                   userInfo:@{
                                                       NSLocalizedDescriptionKey:
                                                           [NSString stringWithFormat:@"Unsupported sample format for bus %@",
                                                                                     [auBus name]]
                                                   }];

                    YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "allocateRenderResources failed: unsupported sample format " << String (static_cast<int> (commonFormat)) << " for bus " << String (static_cast<int> (i)));
                    return false;
                }

                // Track the largest interleaved buffer needed for format conversion
                const auto numCh = [fmt channelCount];
                const auto bytesPerSample = (commonFormat == AVAudioPCMFormatFloat64) ? sizeof (double) : sizeof (float);
                const auto interleavedBytes = static_cast<size_t> (maxFrames) * static_cast<size_t> (numCh) * bytesPerSample;
                maxInterleavedBytes = jmax (maxInterleavedBytes, interleavedBytes);

                // Validate consistent sample rate across all buses
                const auto busSampleRate = [fmt sampleRate];
                if (! sampleRateSet)
                {
                    sampleRate = busSampleRate;
                    sampleRateSet = true;
                }
                else if (! approximatelyEqual (sampleRate, busSampleRate))
                {
                    if (outError != nullptr)
                        *outError = [NSError errorWithDomain:NSOSStatusErrorDomain
                                                       code:kAudioUnitErr_FormatNotSupported
                                                   userInfo:@{
                                                       NSLocalizedDescriptionKey:
                                                           [NSString stringWithFormat:@"Inconsistent sample rate for bus %@",
                                                                                     [auBus name]]
                                                   }];

                    YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "allocateRenderResources failed: inconsistent sample rate " << String (busSampleRate) << " (expected " << String (sampleRate) << ") for bus " << String (static_cast<int> (i)));
                    return false;
                }
            }
        }

        // Pre-allocate interleaved scratch buffer for format conversion (real-time safe)
        if (maxInterleavedBytes > interleavedScratchSize)
        {
            interleavedScratchData.allocate (maxInterleavedBytes, false);
            interleavedScratchSize = maxInterleavedBytes;
        }

        allocatedMaximumFrames = maxFrames;

        processor->setPlaybackConfiguration (sampleRate, static_cast<int> (maxFrames));

        midiMessages.ensureSize (2048);
        midiMessages.clear();

        // Reserve parameter change buffer capacity for sample-accurate automation.
        // Capacity = number of parameters * maxFrames per block so that per-sample
        // ramps never exceed the pre-allocated storage.
        {
            const auto numParams = static_cast<int> (processor->getParameters().size());
            paramChangeBuffer.reserve (numParams * static_cast<int> (maxFrames));
        }

        // Pre-allocate scratch buffers (real-time safe — no allocation in render callback)
        scratchBuffer.setSize (totalInChannels, static_cast<int> (maxFrames));
        scratchBuffer.clear();
        scratchOutputBuffer.setSize (totalOutChannels, static_cast<int> (maxFrames));
        scratchOutputBuffer.clear();

        // Pre-allocate per-bus view storage
        renderViews.prepare (*processor, totalInChannels, totalOutChannels);

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
        allocatedMaximumFrames = 0;
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

        // Reject frame counts exceeding the pre-allocated buffer capacity.
        // The host should never request more than maximumFramesToRender, but if it
        // does, proceeding would cause a buffer overrun in the scratch buffers.
        if (frameCount > allocatedMaximumFrames)
            return kAudioUnitErr_TooManyFramesToProcess;

        const int numFrames = static_cast<int> (frameCount);

        if (! approximatelyEqual (lastTimeStamp.mSampleTime, timestamp->mSampleTime))
        {
            midiMessages.clear();

            // Process events (MIDI and parameters)
            processEvents (realtimeEventListHead, static_cast<AUEventSampleTime> (timestamp->mSampleTime), frameCount);

            lastTimeStamp = *timestamp;

            // Clear scratch buffers (already sized in allocateRenderResources)
            scratchBuffer.clear();
            scratchOutputBuffer.clear();

            const auto& busLayout = processor->getBusLayout();

            // Build per-bus input views and pull inputs
            renderViews.inputBusViews.clear();
            {
                int chIdx = 0;
                const auto& inputBuses = busLayout.getInputBuses();

                for (int busIdx = 0; busIdx < static_cast<int> (inputBuses.size()); ++busIdx)
                {
                    const auto& bus = inputBuses[busIdx];
                    if (bus.getType() != AudioBus::Type::Audio)
                        continue;

                    const int numCh = bus.getNumChannels();

                    if (pullInputBlock != nullptr)
                    {
                        // Look up the AU bus format to determine pull-buffer layout
                        const auto auBusIdx = static_cast<int> (renderViews.inputBusViews.size());
                        auto* auBus = [inputBusses.get() objectAtIndexedSubscript:static_cast<NSUInteger> (auBusIdx)];
                        auto* busFmt = [auBus format];
                        const bool busIsInterleaved = [busFmt isInterleaved];
                        const bool busIsFloat64 = ([busFmt commonFormat] == AVAudioPCMFormatFloat64);

                        if (! busIsInterleaved && ! busIsFloat64)
                        {
                            // Planar Float32 — direct pull into scratch (fast path)
                            const auto bufferListSize = offsetof (AudioBufferList, mBuffers) + static_cast<size_t> (numCh) * sizeof (::AudioBuffer);
                            auto* pullBuffer = static_cast<AudioBufferList*> (alloca (bufferListSize));
                            pullBuffer->mNumberBuffers = static_cast<UInt32> (numCh);

                            for (int ch = 0; ch < numCh; ++ch)
                            {
                                pullBuffer->mBuffers[ch].mNumberChannels = 1;
                                pullBuffer->mBuffers[ch].mData = scratchBuffer.getWritePointer (chIdx + ch);
                                pullBuffer->mBuffers[ch].mDataByteSize = static_cast<UInt32> (numFrames * sizeof (float));
                            }

                            if (pullInputBlock (actionFlags, timestamp, frameCount, auBusIdx, pullBuffer) != noErr)
                            {
                                for (int ch = 0; ch < numCh; ++ch)
                                    scratchBuffer.clear (chIdx + ch, 0, numFrames);
                            }
                        }
                        else if (busIsInterleaved)
                        {
                            // Interleaved pull — pull into a single interleaved buffer, then deinterleave
                            const auto bytesPerSample = busIsFloat64 ? sizeof (double) : sizeof (float);
                            const auto interleavedBytes = static_cast<size_t> (numFrames) * static_cast<size_t> (numCh) * bytesPerSample;

                            const auto bufferListSize = offsetof (AudioBufferList, mBuffers) + sizeof (::AudioBuffer);
                            auto* pullBuffer = static_cast<AudioBufferList*> (alloca (bufferListSize));
                            pullBuffer->mNumberBuffers = 1;
                            pullBuffer->mBuffers[0].mNumberChannels = static_cast<UInt32> (numCh);
                            pullBuffer->mBuffers[0].mData = interleavedScratchData.getData();
                            pullBuffer->mBuffers[0].mDataByteSize = static_cast<UInt32> (interleavedBytes);

                            const auto pullStatus = pullInputBlock (actionFlags, timestamp, frameCount, auBusIdx, pullBuffer);

                            // Build destination channel pointers for deinterleave.
                            // Stack allocation is real-time safe and avoids const-correctness
                            // issues with renderViews.inputChannelPtrStorage (which stores const float*
                            // for the later AudioBusBufferView build).
                            auto* chanPtrs = static_cast<float**> (
                                alloca (static_cast<size_t> (numCh) * sizeof (float*)));
                            for (int ch = 0; ch < numCh; ++ch)
                                chanPtrs[ch] = scratchBuffer.getWritePointer (chIdx + ch);

                            if (pullStatus == noErr)
                            {
                                if (busIsFloat64)
                                {
                                    using SrcFmt = AudioData::Format<AudioData::Float64, AudioData::NativeEndian>;
                                    using DstFmt = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
                                    AudioData::deinterleaveSamples (
                                        AudioData::InterleavedSource<SrcFmt> { reinterpret_cast<const double*> (interleavedScratchData.getData()), numCh },
                                        AudioData::NonInterleavedDest<DstFmt> { chanPtrs, numCh },
                                        numFrames);
                                }
                                else
                                {
                                    using SrcFmt = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
                                    using DstFmt = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
                                    AudioData::deinterleaveSamples (
                                        AudioData::InterleavedSource<SrcFmt> { reinterpret_cast<const float*> (interleavedScratchData.getData()), numCh },
                                        AudioData::NonInterleavedDest<DstFmt> { chanPtrs, numCh },
                                        numFrames);
                                }
                            }
                            else
                            {
                                for (int ch = 0; ch < numCh; ++ch)
                                    scratchBuffer.clear (chIdx + ch, 0, numFrames);
                            }
                        }
                        else
                        {
                            // Planar Float64 — pull into planar double, then convert double → float
                            const auto bufferListSize = offsetof (AudioBufferList, mBuffers) + static_cast<size_t> (numCh) * sizeof (::AudioBuffer);
                            auto* pullBuffer = static_cast<AudioBufferList*> (alloca (bufferListSize));
                            pullBuffer->mNumberBuffers = static_cast<UInt32> (numCh);

                            auto* doubleScratch = reinterpret_cast<double*> (interleavedScratchData.getData());
                            for (int ch = 0; ch < numCh; ++ch)
                            {
                                pullBuffer->mBuffers[ch].mNumberChannels = 1;
                                pullBuffer->mBuffers[ch].mData = doubleScratch + static_cast<size_t> (ch) * static_cast<size_t> (numFrames);
                                pullBuffer->mBuffers[ch].mDataByteSize = static_cast<UInt32> (numFrames * sizeof (double));
                            }

                            const auto pullStatus = pullInputBlock (actionFlags, timestamp, frameCount, auBusIdx, pullBuffer);

                            if (pullStatus == noErr)
                            {
                                for (int ch = 0; ch < numCh; ++ch)
                                {
                                    const auto* src = doubleScratch + static_cast<size_t> (ch) * static_cast<size_t> (numFrames);
                                    auto* dst = scratchBuffer.getWritePointer (chIdx + ch);
                                    for (int s = 0; s < numFrames; ++s)
                                        dst[s] = static_cast<float> (src[s]);
                                }
                            }
                            else
                            {
                                for (int ch = 0; ch < numCh; ++ch)
                                    scratchBuffer.clear (chIdx + ch, 0, numFrames);
                            }
                        }
                    }
                    else
                    {
                        for (int ch = 0; ch < numCh; ++ch)
                            scratchBuffer.clear (chIdx + ch, 0, numFrames);
                    }

                    // Build AudioBusBufferView for this input bus (real-time safe — uses pre-allocated storage)
                    auto* chPtrs = renderViews.inputChannelPtrStorage.data() + chIdx;
                    for (int ch = 0; ch < numCh; ++ch)
                        chPtrs[ch] = scratchBuffer.getReadPointer (chIdx + ch);

                    renderViews.inputBusViews.emplace_back (chPtrs, numCh, bus.getRole());

                    // Copy main inputs to the corresponding output area
                    if (bus.getRole() == AudioBus::Role::Main)
                    {
                        // Find matching output bus for this main input
                        int outputChIdx = 0;
                        for (const auto& outBus : busLayout.getOutputBuses())
                        {
                            if (outBus.getType() != AudioBus::Type::Audio)
                                continue;
                            if (outBus.getRole() == AudioBus::Role::Main
                                && outputChIdx == chIdx) // match by channel offset
                            {
                                for (int ch = 0; ch < std::min (numCh, outBus.getNumChannels()); ++ch)
                                    scratchOutputBuffer.copyFrom (outputChIdx + ch, 0, scratchBuffer, chIdx + ch, 0, numFrames);
                                break;
                            }
                            outputChIdx += outBus.getNumChannels();
                        }
                    }

                    chIdx += numCh;
                }
            }

            // Build per-bus output views
            renderViews.outputBusViews.clear();
            {
                int chIdx = 0;
                const auto& outputBuses = busLayout.getOutputBuses();

                for (int busIdx = 0; busIdx < static_cast<int> (outputBuses.size()); ++busIdx)
                {
                    const auto& bus = outputBuses[busIdx];
                    if (bus.getType() != AudioBus::Type::Audio)
                        continue;

                    const int numCh = bus.getNumChannels();

                    // Build AudioBusBufferView for this output bus (real-time safe — uses pre-allocated storage)
                    auto* chPtrs = renderViews.outputChannelPtrStorage.data() + chIdx;
                    for (int ch = 0; ch < numCh; ++ch)
                        chPtrs[ch] = scratchOutputBuffer.getWritePointer (chIdx + ch);

                    renderViews.outputBusViews.emplace_back (chPtrs, numCh, bus.getRole());

                    chIdx += numCh;
                }
            }

            // Process audio
            {
                AudioPluginPlayHeadAU playHead (*this, timestamp);

                AudioProcessContext<float> context {
                    scratchOutputBuffer,
                    midiMessages,
                    paramChangeBuffer,
                    &playHead,
                    { renderViews.inputBusViews.data(), renderViews.inputBusViews.size() },
                    { renderViews.outputBusViews.data(), renderViews.outputBusViews.size() }
                };

                processAudioBlock (*processor, context, bypass.isBypassed());
            }

            sendMidi (static_cast<int64_t> (timestamp->mSampleTime + 0.5), frameCount);
        }

        // Copy output data
        if (outputData != nullptr && outputBusNumber >= 0)
        {
            const auto& outputBuses = processor->getBusLayout().getOutputBuses();
            int chIdx = 0;
            int auOutBusIdx = 0;

            for (int busIdx = 0; busIdx < static_cast<int> (outputBuses.size()); ++busIdx)
            {
                const auto& bus = outputBuses[busIdx];
                if (bus.getType() != AudioBus::Type::Audio)
                    continue;

                const int numCh = bus.getNumChannels();

                if (auOutBusIdx == static_cast<int> (outputBusNumber))
                {
                    // Look up the AU bus format to determine output layout
                    auto* auBus = [outputBusses.get() objectAtIndexedSubscript:static_cast<NSUInteger> (auOutBusIdx)];
                    auto* busFmt = [auBus format];
                    const bool busIsInterleaved = [busFmt isInterleaved];
                    const bool busIsFloat64 = ([busFmt commonFormat] == AVAudioPCMFormatFloat64);

                    if (! busIsInterleaved && ! busIsFloat64)
                    {
                        // Planar Float32 — direct copy (fast path)
                        for (int ch = 0; ch < numCh && ch < static_cast<int> (outputData->mNumberBuffers); ++ch)
                        {
                            const auto* src = scratchOutputBuffer.getReadPointer (chIdx + ch);
                            auto* buf = &outputData->mBuffers[ch];
                            auto* dst = static_cast<float*> (buf->mData);

                            if (dst == nullptr || src == nullptr)
                                continue;

                            if (buf->mDataByteSize < static_cast<UInt32> (numFrames * static_cast<int> (sizeof (float))))
                                continue;

                            std::copy (src, src + numFrames, dst);
                        }
                    }
                    else if (busIsInterleaved)
                    {
                        // Interleaved output — interleave from planar scratch.
                        // Validate the host buffer can hold the interleaved data.
                        const auto bytesPerSample = busIsFloat64 ? sizeof (double) : sizeof (float);
                        const auto requiredBytes = static_cast<UInt32> (numFrames * numCh * static_cast<int> (bytesPerSample));

                        if (outputData->mNumberBuffers >= 1
                            && outputData->mBuffers[0].mData != nullptr
                            && outputData->mBuffers[0].mDataByteSize >= requiredBytes)
                        {
                            // Build source channel pointers from scratch buffer (stack allocation, real-time safe).
                            // NonInterleavedSource expects const float* const* — we allocate as
                            // const float** and reinterpret_cast to add the inner const qualifier.
                            auto* storage = static_cast<const float**> (
                                alloca (static_cast<size_t> (numCh) * sizeof (const float*)));
                            for (int ch = 0; ch < numCh; ++ch)
                                storage[ch] = scratchOutputBuffer.getReadPointer (chIdx + ch);
                            auto* chanPtrs = reinterpret_cast<const float* const*> (storage);

                            if (busIsFloat64)
                            {
                                using SrcFmt = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
                                using DstFmt = AudioData::Format<AudioData::Float64, AudioData::NativeEndian>;
                                AudioData::interleaveSamples (
                                    AudioData::NonInterleavedSource<SrcFmt> { chanPtrs, numCh },
                                    AudioData::InterleavedDest<DstFmt> { reinterpret_cast<double*> (outputData->mBuffers[0].mData), numCh },
                                    numFrames);
                            }
                            else
                            {
                                using SrcFmt = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
                                using DstFmt = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
                                AudioData::interleaveSamples (
                                    AudioData::NonInterleavedSource<SrcFmt> { chanPtrs, numCh },
                                    AudioData::InterleavedDest<DstFmt> { static_cast<float*> (outputData->mBuffers[0].mData), numCh },
                                    numFrames);
                            }
                        }
                    }
                    else
                    {
                        // Planar Float64 — convert float → double per channel
                        for (int ch = 0; ch < numCh && ch < static_cast<int> (outputData->mNumberBuffers); ++ch)
                        {
                            const auto* src = scratchOutputBuffer.getReadPointer (chIdx + ch);
                            auto* buf = &outputData->mBuffers[ch];
                            auto* dst = static_cast<double*> (buf->mData);

                            if (dst == nullptr || src == nullptr)
                                continue;

                            if (buf->mDataByteSize < static_cast<UInt32> (numFrames * static_cast<int> (sizeof (double))))
                                continue;

                            for (int s = 0; s < numFrames; ++s)
                                dst[s] = static_cast<double> (src[s]);
                        }
                    }
                }

                ++auOutBusIdx;
                chIdx += numCh;
            }
        }

        return noErr;
    }

    void processEvents (const AURenderEvent* realtimeEventListHead, AUEventSampleTime startTime, AUAudioFrameCount frameCount)
    {
        paramChangeBuffer.clear();

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
                                           static_cast<int> (jlimit<int64> (0, jmax (0, static_cast<int> (frameCount) - 1), midiEvent.eventSampleTime - startTime)));
                }
                break;

                case AURenderEventParameter:
                {
                    const AUParameterEvent& paramEvent = event->parameter;

                    if (auto* p = getParamForAUAddress (paramEvent.parameterAddress))
                    {
                        const int offset = static_cast<int> (paramEvent.eventSampleTime - startTime);
                        const auto normalised = p->convertToNormalizedValue (static_cast<float> (paramEvent.value));
                        p->setValue (static_cast<float> (paramEvent.value));

                        if (isPositiveAndBelow (offset, static_cast<int> (frameCount)))
                            paramChangeBuffer.addChange (p->getIndexInContainer(), normalised, offset);

                        inParameterChangedCallback = true;
                    }
                }
                break;

                case AURenderEventParameterRamp:
                {
                    const AUParameterEvent& paramEvent = event->parameter;

                    if (auto* p = getParamForAUAddress (paramEvent.parameterAddress))
                    {
                        const int startOffset = static_cast<int> (paramEvent.eventSampleTime - startTime);
                        const int rampFrames = static_cast<int> (paramEvent.rampDurationSampleFrames);
                        const int rampEndOffset = startOffset + rampFrames;
                        const auto startValue = p->getValue();
                        const auto endValue = static_cast<float> (paramEvent.value);
                        const auto startNormalised = p->convertToNormalizedValue (startValue);
                        const auto endNormalised = p->convertToNormalizedValue (endValue);

                        // Add ramp start event at the ramp's start offset
                        if (isPositiveAndBelow (startOffset, static_cast<int> (frameCount)))
                            paramChangeBuffer.addChange (p->getIndexInContainer(), startNormalised, startOffset);

                        // Add ramp end event. If the ramp extends past this block, clamp it to the last frame.
                        const int endOffsetClamped = jmin (rampEndOffset, static_cast<int> (frameCount) - 1);
                        if (endOffsetClamped > startOffset)
                            paramChangeBuffer.addChange (p->getIndexInContainer(), endNormalised, endOffsetClamped);

                        // Always set the final value so the next block picks up the correct value
                        p->setValue (endValue);

                        inParameterChangedCallback = true;
                    }
                }
                break;

                default:
                    break;
            }
        }

        // Sort changes by sample offset for binary-search lookups during processing
        paramChangeBuffer.sort();
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
        return bypass.isBypassed();
    }

    void setShouldBypassEffect (bool shouldBypass)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "bypass set to " << String (shouldBypass ? "true" : "false"));

        bypass.setBypassed (shouldBypass);
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
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "offline rendering set to " << String (offline ? "true" : "false") << ", processor=" << describePointer (processor.get()));

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
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "supportedViewConfigurations: processor has no editor");
            return indices;
        }

        auto* editor = processor->createEditor();
        if (editor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "supportedViewConfigurations: processor returned null editor");
            return indices;
        }

        for (NSUInteger i = 0; i < [configs count]; ++i)
        {
            [[maybe_unused]] auto* config = [configs objectAtIndex:i];
            [indices addIndex:i];
        }

        delete editor;

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "supportedViewConfigurations returned " << String (static_cast<int> ([indices count])) << " of " << String (static_cast<int> ([configs count])) << " configurations");
        return indices;
    }

    void selectViewConfiguration (AUAudioUnitViewConfiguration* config)
    {
        viewConfigWidth = [config width];
        viewConfigHeight = [config height];

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "view configuration selected: width=" << String (viewConfigWidth) << ", height=" << String (viewConfigHeight));
    }

    //==============================================================================
    // Listener callbacks

    void audioProcessorChanged (AudioProcessorBase*, const AudioProcessorBase::ChangeDetails& details) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "processor changed: program=" << String (details.programChanged ? "true" : "false") << ", latency=" << String (details.latencyChanged ? "true" : "false") << ", parameterInfo=" << String (details.parameterInfoChanged ? "true" : "false"));

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
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "updating parameter tree");

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

            const auto value = yupParam->convertToDenormalizedValue (newValue);

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
    AUParameterObserverToken _Nullable * _Nullable editorObserverToken = nullptr;

    mutable std::mutex factoryPresetsMutex;
    NSUniquePtr<NSMutableArray<AUAudioUnitPreset*>> factoryPresets;

    ObjCBlock<AUInternalRenderBlock> internalRenderBlock;
    ObjCBlock<AURenderContextObserver> renderContextObserver;

    MidiBuffer midiMessages;
    ParameterChangeBuffer paramChangeBuffer;
    AudioBuffer<float> scratchBuffer;
    AudioBuffer<float> scratchOutputBuffer;

    // Per-bus view state, pre-allocated in allocateRenderResources so the
    // render callback only clears and refills the vectors without allocating
    AudioPluginAURenderViews renderViews;

    // Interleaved scratch buffer for format conversion (deinterleave/interleave)
    HeapBlock<uint8> interleavedScratchData;
    size_t interleavedScratchSize = 0;

    AUMIDIOutputEventBlock midiOutputEventBlock = nullptr;

    ObjCBlock<AUHostMusicalContextBlock> hostMusicalContextCallback;
    ObjCBlock<AUHostTransportStateBlock> hostTransportStateCallback;

    AudioTimeStamp lastTimeStamp;

    WrapperBypass bypass;

    double viewConfigWidth = 0;
    double viewConfigHeight = 0;

    ThreadLocalValue<bool> inParameterChangedCallback;
    bool allocated = false;
    AUAudioFrameCount allocatedMaximumFrames = 0;

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
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "AUAudioUnit init with existing wrapper: audioUnit=" << describePointer ((__bridge void*) self) << ", wrapper=" << describePointer (cpp));
            setThis (self, cpp);
            return self;
        });
        YUP_END_IGNORE_WARNINGS_GCC_LIKE

        addMethod (@selector (initWithComponentDescription:options:error:), [] (id self, SEL, AudioComponentDescription descr, AudioComponentInstantiationOptions options, NSError** error) {
            self = ObjCMsgSendSuper<AUAudioUnit, AUAudioUnit*, AudioComponentDescription,
                                    AudioComponentInstantiationOptions, NSError * __autoreleasing *> (self, @selector (initWithComponentDescription:options:error:), descr, options, error);

            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "AUAudioUnit init: audioUnit=" << describePointer ((__bridge void*) self));

            if (self == nil)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "AUAudioUnit super init returned nil: error=" << (error != nullptr && *error != nil ? String::fromCFString ((__bridge CFStringRef)[*error localizedDescription]) : String ("none")));
            }

            auto* cpp = new AudioPluginProcessorAUv3 (self, descr, options, error);
            setThis (self, cpp);
            return self;
        });

        YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
        addMethod (sel_registerName ("dealloc"), [] (__unsafe_unretained id self, SEL) {
            auto* cpp = _this (self);
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "AUAudioUnit dealloc: audioUnit=" << describePointer ((__bridge void*) self) << ", wrapper=" << describePointer (cpp));
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

    static void setThis (id self, AudioPluginProcessorAUv3* _Nullable cpp)
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

        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "created view controller: controller=" << describePointer (this) << ", viewController=" << describePointer ((__bridge void*) vc));
    }

    ~AudioPluginViewControllerv3() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "destroying view controller: controller=" << describePointer (this) << ", editor=" << describePointer (editor) << ", processor=" << describePointer (processor));

        if (processor != nullptr)
        {
            yup::endActiveParameterGestures (processor);
            processor->removeListener (this);
        }

        if (editor != nullptr)
        {
            delete editor;
            editor = nullptr;
        }
    }

    void loadView()
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "loadView requested: processor=" << describePointer (processor) << ", editor=" << describePointer (editor));
        createEditorIfNeeded();
    }

    void viewDidLayoutSubviews()
    {
        if ([myself view] != nil)
        {
            if (editor != nullptr)
            {
                const auto bounds = [[myself view] bounds];

                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "resizing editor to view bounds: width=" << String (static_cast<double> (NSWidth (bounds))) << ", height=" << String (static_cast<double> (NSHeight (bounds))) << ", editor=" << describePointer (editor));

                editor->setBounds ({ 0.0f,
                                     0.0f,
                                     static_cast<float> (NSWidth (bounds)),
                                     static_cast<float> (NSHeight (bounds)) });
            }
            else
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "viewDidLayoutSubviews skipped: editor is null");
            }
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "viewDidLayoutSubviews skipped: view is nil");
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
        YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "createAudioUnit requested: type=" << String::toHexString (static_cast<int> (desc.componentType)) << ", subType=" << String::toHexString (static_cast<int> (desc.componentSubType)) << ", manufacturer=" << String::toHexString (static_cast<int> (desc.componentManufacturer)));

        // Let the ObjC init create the C++ wrapper — it correctly passes self (the
        // real AUAudioUnit) to the AudioPluginProcessorAUv3 constructor, so au is
        // valid when init() runs.
        static AUAudioUnitSubclass auClass;
        auto* au = auClass.createInstance();
        au = ObjCMsgSendSuper<AUAudioUnit, AUAudioUnit*, AudioComponentDescription,
                              AudioComponentInstantiationOptions, NSError * __autoreleasing *> (au, @selector (initWithComponentDescription:options:error:), desc, 0, error);

        if (au != nil)
        {
            auto* cpp = AUAudioUnitSubclass::_this (au);
            processor = cpp != nullptr ? cpp->getProcessor() : nullptr;

            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "createAudioUnit completed: audioUnit=" << describePointer ((__bridge void*) au) << ", wrapper=" << describePointer (cpp) << ", processor=" << describePointer (processor));

            createEditorIfNeeded();
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "createAudioUnit failed: error=" << (error != nullptr && *error != nil ? String::fromCFString ((__bridge CFStringRef)[*error localizedDescription]) : String ("none")));
        }

        return au;
    }

    AudioProcessor* getProcessor() const { return processor; }

private:
    void createEditorIfNeeded()
    {
        // Already created, or no processor available yet
        if (editor != nullptr || processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "createEditorIfNeeded skipped: editor=" << describePointer (editor) << ", processor=" << describePointer (processor));
            return;
        }

        if (processor->hasEditor())
        {
            editor = processor->createEditor();

            if (editor != nullptr)
            {
                preferredSize = { editor->getWidth(), editor->getHeight() };

                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "created editor: preferredWidth=" << String (preferredSize.getWidth()) << ", preferredHeight=" << String (preferredSize.getHeight()) << ", editor=" << describePointer (editor));

                NSView* view = [[NSView alloc] initWithFrame:NSMakeRect (0, 0, preferredSize.getWidth(), preferredSize.getHeight())];
                [myself setView:view];

                editor->setVisible (true);

                auto options = ComponentNative::Options()
                                   .withFlags (ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow);

                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "attaching editor to native view: editor=" << describePointer (editor) << ", view=" << describePointer ((__bridge void*) view));

                editor->addToDesktop (options, (__bridge void*) view);

                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "editor attached to native view: isOnDesktop=" << String (editor->isOnDesktop() ? "true" : "false"));
            }
            else
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "processor returned null editor");
            }
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AUV3, "processor has no editor");
        }
    }

    AUViewController<AUAudioUnitFactory>* myself = nil;
    AudioProcessor* processor = nullptr;
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

- (AUAudioUnit* _Nullable) createAudioUnitWithComponentDescription:(AudioComponentDescription)desc
                                                             error:(NSError* _Nullable* _Nullable)error
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

NS_ASSUME_NONNULL_END

#endif // YUP_MAC
