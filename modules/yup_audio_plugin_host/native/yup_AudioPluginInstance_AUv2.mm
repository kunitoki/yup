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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC

namespace yup
{

namespace
{

//==============================================================================
// Converts a four-char code to a 4-char String (e.g. 0x61756666 -> "auff").
String fourCCToString(OSType code)
{
    char buf[5] = {};
    buf[0] = static_cast<char>((code >> 24) & 0xFF);
    buf[1] = static_cast<char>((code >> 16) & 0xFF);
    buf[2] = static_cast<char>((code >> 8) & 0xFF);
    buf[3] = static_cast<char>(code & 0xFF);
    return String(buf);
}

String copyCFString(CFStringRef string)
{
    return String::fromCFString(string).trim();
}

// Builds a stable identifier string "type/subt/mfgr" from AudioComponentDescription.
String makeIdentifier(const AudioComponentDescription& acd)
{
    return fourCCToString(acd.componentType) + "/" + fourCCToString(acd.componentSubType) + "/" + fourCCToString(acd.componentManufacturer);
}

AudioPluginDescription descriptionFromComponent(AudioComponent comp,
                                                const AudioComponentDescription& acd)
{
    AudioPluginDescription desc;
    desc.formatType = AudioPluginFormatType::audioUnit;
    desc.identifier = makeIdentifier(acd);

    CFStringRef nameRef = nullptr;
    AudioComponentCopyName(comp, &nameRef);
    if (nameRef != nullptr)
    {
        desc.name = copyCFString(nameRef);
        CFRelease(nameRef);
    }

    // Collect vendor from the manufacturer four-char code
    desc.vendor = fourCCToString(acd.componentManufacturer);

    desc.isInstrument = (acd.componentType == kAudioUnitType_MusicDevice);
    desc.isEffect = (acd.componentType == kAudioUnitType_Effect || acd.componentType == kAudioUnitType_MusicEffect);

    if (acd.componentType == kAudioUnitType_MusicDevice || acd.componentType == kAudioUnitType_MusicEffect || acd.componentType == kAudioUnitType_MIDIProcessor)
    {
        desc.numMidiInputPorts = 1;
    }

    if (acd.componentType == kAudioUnitType_MIDIProcessor)
        desc.numMidiOutputPorts = 1;

    if (acd.componentType == kAudioUnitType_Effect || acd.componentType == kAudioUnitType_MusicEffect)
    {
        desc.numInputChannels = 2;
        desc.numOutputChannels = 2;
    }
    else if (acd.componentType == kAudioUnitType_MusicDevice || acd.componentType == kAudioUnitType_Generator)
    {
        desc.numOutputChannels = 2;
    }

    return desc;
}

String makeParameterName(const AudioUnitParameterInfo& info,
                         AudioUnitParameterID paramId)
{
    String name;

    if ((info.flags & kAudioUnitParameterFlag_HasCFNameString) != 0)
        name = copyCFString(info.cfNameString);

    if (name.isEmpty())
        name = String(info.name).trim();

    return name.isNotEmpty() ? name : "Parameter " + String(paramId);
}

void releaseParameterInfoStrings(AudioUnitParameterInfo& info)
{
    if ((info.flags & kAudioUnitParameterFlag_CFNameRelease) == 0)
        return;

    if (info.cfNameString != nullptr)
    {
        CFRelease(info.cfNameString);
        info.cfNameString = nullptr;
    }

    if (info.unitName != nullptr)
    {
        CFRelease(info.unitName);
        info.unitName = nullptr;
    }
}

NormalisableRange<float> makeParameterRange(const AudioUnitParameterInfo& info)
{
    const auto minValue = static_cast<float>(info.minValue);
    const auto maxValue = static_cast<float>(info.maxValue);

    if (maxValue > minValue)
        return {minValue, maxValue};

    return {0.0f, 1.0f};
}

AudioUnitParameter makeAUParameter(AudioUnit audioUnit, AudioUnitParameterID paramId)
{
    AudioUnitParameter parameter;
    parameter.mAudioUnit = audioUnit;
    parameter.mParameterID = paramId;
    parameter.mScope = kAudioUnitScope_Global;
    parameter.mElement = 0;
    return parameter;
}

AudioUnitEvent makeAUParameterEvent(AudioUnit audioUnit,
                                    AudioUnitParameterID paramId,
                                    AudioUnitEventType eventType)
{
    AudioUnitEvent event;
    event.mEventType = eventType;
    event.mArgument.mParameter = makeAUParameter(audioUnit, paramId);
    return event;
}

} // namespace

//==============================================================================

class AUv2Editor : public AudioProcessorEditor
{
   public:
    static std::unique_ptr<AUv2Editor> create(AudioUnit audioUnit)
    {
        UInt32 size = 0;
        Boolean writable = false;

        if (AudioUnitGetPropertyInfo(audioUnit,
                                     kAudioUnitProperty_CocoaUI,
                                     kAudioUnitScope_Global, 0,
                                     &size, &writable) != noErr)
        {
            return nullptr;
        }

        if (size < offsetof(AudioUnitCocoaViewInfo, mCocoaAUViewClass) + sizeof(CFStringRef))
            return nullptr;

        std::vector<uint8_t> storage(size);
        auto* viewInfo = reinterpret_cast<AudioUnitCocoaViewInfo*>(storage.data());

        if (AudioUnitGetProperty(audioUnit,
                                 kAudioUnitProperty_CocoaUI,
                                 kAudioUnitScope_Global, 0,
                                 viewInfo, &size) != noErr)
        {
            return nullptr;
        }

        NSView* view = nil;
        NSBundle* viewBundle = nil;
        id<AUCocoaUIBase> viewFactory = nil;

        if (viewInfo->mCocoaAUViewBundleLocation != nullptr && viewInfo->mCocoaAUViewClass[0] != nullptr)
        {
            viewBundle = [NSBundle bundleWithURL:(__bridge NSURL*)viewInfo->mCocoaAUViewBundleLocation];

            if (viewBundle != nil && [viewBundle load])
            {
                Class viewClass = [viewBundle classNamed:(__bridge NSString*)viewInfo->mCocoaAUViewClass[0]];

                if (viewClass != Nil && [viewClass conformsToProtocol:@protocol(AUCocoaUIBase)])
                {
                    viewFactory = [[viewClass alloc] init];
                    view = [viewFactory uiViewForAudioUnit:audioUnit withSize:NSZeroSize];
                }
            }
        }

        if (viewInfo->mCocoaAUViewBundleLocation != nullptr)
            CFRelease(viewInfo->mCocoaAUViewBundleLocation);

        const auto numViewClasses = static_cast<int>((size - offsetof(AudioUnitCocoaViewInfo, mCocoaAUViewClass)) / sizeof(CFStringRef));
        for (int i = 0; i < numViewClasses; ++i)
            if (viewInfo->mCocoaAUViewClass[i] != nullptr)
                CFRelease(viewInfo->mCocoaAUViewClass[i]);

        if (view == nil)
            return nullptr;

        return std::unique_ptr<AUv2Editor>(new AUv2Editor(view, viewBundle, viewFactory));
    }

    ~AUv2Editor() override
    {
        detachCocoaView();
    }

    bool isResizable() const override { return true; }

    Size<int> getPreferredSize() const override { return preferredSize; }

    void paint(Graphics& g) override
    {
        g.setFillColor(Color(0xff101417));
        g.fillAll();
    }

    void resized() override
    {
        updateCocoaViewFrame();
    }

    void attachedToNative() override
    {
        attachCocoaView();
    }

    void detachedFromNative() override
    {
        detachCocoaView();
    }

   private:
    AUv2Editor(NSView* view, NSBundle* viewBundle, id<AUCocoaUIBase> viewFactory)
        : cocoaView(view), cocoaViewBundle(viewBundle), cocoaViewFactory(viewFactory)
    {
        auto size = [cocoaView frame].size;

        if (size.width <= 1.0 || size.height <= 1.0)
            size = [cocoaView fittingSize];

        preferredSize = {
            jmax(320, static_cast<int>(std::ceil(size.width))),
            jmax(240, static_cast<int>(std::ceil(size.height)))};

        [cocoaView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [cocoaView setFrameSize:NSMakeSize(preferredSize.getWidth(), preferredSize.getHeight())];
    }

    void attachCocoaView()
    {
        if (cocoaView == nil)
            return;

        auto* nativeComponent = getNativeComponent();
        if (nativeComponent == nullptr)
            return;

        NSWindow* window = (__bridge NSWindow*)nativeComponent->getNativeHandle();
        NSView* contentView = [window contentView];

        if (contentView == nil)
            return;

        if ([cocoaView superview] != contentView)
            [contentView addSubview:cocoaView positioned:NSWindowAbove relativeTo:nil];

        updateCocoaViewFrame();
    }

    void detachCocoaView()
    {
        if (cocoaView != nil)
            [cocoaView removeFromSuperview];
    }

    void updateCocoaViewFrame()
    {
        if (cocoaView == nil)
            return;

        auto* nativeComponent = getNativeComponent();
        if (nativeComponent == nullptr)
            return;

        NSWindow* window = (__bridge NSWindow*)nativeComponent->getNativeHandle();
        NSView* contentView = [window contentView];

        if (contentView == nil)
            return;

        const auto bounds = getBoundsRelativeToTopLevelComponent();
        const auto contentBounds = [contentView bounds];
        const auto width = jmax(1.0f, bounds.getWidth());
        const auto height = jmax(1.0f, bounds.getHeight());

        [cocoaView setFrame:NSMakeRect(static_cast<CGFloat>(bounds.getX()),
                                       static_cast<CGFloat>(NSHeight(contentBounds) - bounds.getY() - height),
                                       static_cast<CGFloat>(width),
                                       static_cast<CGFloat>(height))];
    }

    Size<int> preferredSize{640, 480};
    NSView* __strong cocoaView = nil;
    NSBundle* __strong cocoaViewBundle = nil;
    id<AUCocoaUIBase> __strong cocoaViewFactory = nil;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AUv2Editor)
};

//==============================================================================

class AUv2Instance : public AudioPluginInstance, private AudioParameter::Listener
{
   public:
    AUv2Instance(const AudioPluginDescription& desc,
                 AudioUnit unit,
                 AudioBusLayout busLayout)
        : AudioPluginInstance(desc, std::move(busLayout)), audioUnit(unit)
    {
        buildParameterList();
        installLatencyListener();
    }

    ~AUv2Instance() override
    {
        removeLatencyListener();
        removeParameterListeners();
        releaseResources();

        if (audioUnit != nullptr)
        {
            // AudioComponentInstanceDispose must run on the main thread so that
            // CoreAudio's internal timers (MAudioPluginInterface::OnTimer etc.)
            // registered on CFRunLoopGetMain() are properly invalidated before
            // the AudioUnit object is freed. Calling from a background thread
            // leaves a window where those timers fire on freed memory.
            AudioUnit capturedUnit = audioUnit;
            audioUnit = nullptr;

            if ([NSThread isMainThread])
            {
                AudioComponentInstanceDispose (capturedUnit);
            }
            else
            {
                dispatch_sync (dispatch_get_main_queue(), ^{
                    AudioComponentInstanceDispose (capturedUnit);
                });
            }
        }
    }

    //==============================================================================

    void prepareToPlay(float sampleRate, int maxBlockSize) override
    {
        releaseResources();

        renderSampleTime = 0.0;

        const auto numHostedChannels = jmax(2,
                                            pluginDescription.numInputChannels,
                                            pluginDescription.numOutputChannels);
        prepareRenderStorage(numHostedChannels, maxBlockSize);

        const Float64 sampleRateValue = static_cast<Float64>(sampleRate);
        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_SampleRate,
                             kAudioUnitScope_Global, 0,
                             &sampleRateValue, sizeof(sampleRateValue));

        UInt32 blockSize = static_cast<UInt32>(maxBlockSize);
        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_MaximumFramesPerSlice,
                             kAudioUnitScope_Global, 0,
                             &blockSize, sizeof(blockSize));

        configureStreamFormat(sampleRateValue, numHostedChannels);
        installInputCallback();
        installMIDIOutputCallback();

        if (![NSThread isMainThread])
        {
            AudioUnitInitialize(audioUnit);
            return;
        }

        // AudioUnitInitialize for AUv3 plugins (exposed via AUv2 API) deadlocks
        // on the main thread: allocateRenderResources triggers WorkgroupManager
        // init which tries to acquire HALB_Mutex while the HAL I/O thread holds
        // it waiting on the main run loop. Dispatching to a background thread
        // and pumping the run loop breaks the cycle.
        AudioUnit capturedUnit = audioUnit;
        __block bool initialized = false;

        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
          AudioUnitInitialize(capturedUnit);
          dispatch_async(dispatch_get_main_queue(), ^{
            initialized = true;
          });
        });

        while (!initialized)
            [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode
                                  beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.005]];
    }

    void releaseResources() override
    {
        if (audioUnit != nullptr)
            AudioUnitUninitialize(audioUnit);
    }

    void processBlock (AudioProcessContext<float>& context) override
    {
        auto& audioBuffer = context.audio;
        auto& midiBuffer = context.midi;

        ScopedNoDenormals noDenormals;

        if (isBypassed())
        {
            processBlockBypassed (context);
            return;
        }

        const int numSamples = audioBuffer.getNumSamples();
        const int numChannels = audioBuffer.getNumChannels();

        if (numSamples <= 0)
            return;

        if (numChannels > preparedNumChannels || numSamples > preparedMaxBlockSize)
        {
            jassertfalse;
            return;
        }

        outputMidiBuffer.clear();
        currentMidiOutputBuffer = &outputMidiBuffer;
        sendMidiInputEvents(midiBuffer);

        if (numChannels <= 0 || pluginDescription.numOutputChannels <= 0)
        {
            currentMidiOutputBuffer = nullptr;
            midiBuffer.clear();
            midiBuffer.addEvents(outputMidiBuffer, 0, -1, 0);
            return;
        }

        for (int c = 0; c < numChannels; ++c)
        {
            std::memcpy(inputBuffer.getWritePointer(c),
                        audioBuffer.getReadPointer(c),
                        static_cast<std::size_t>(numSamples) * sizeof(float));
        }

        currentInputBuffer = &inputBuffer;
        currentInputNumChannels = numChannels;
        currentInputNumSamples = numSamples;

        AudioUnitRenderActionFlags flags = 0;
        AudioTimeStamp timeStamp{};
        timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
        timeStamp.mSampleTime = renderSampleTime;
        renderSampleTime += numSamples;

        AudioBufferList* abl = makeOutputABL(audioBuffer);

        const auto status = AudioUnitRender(audioUnit,
                                            &flags,
                                            &timeStamp,
                                            0, // output bus
                                            static_cast<UInt32>(numSamples),
                                            abl);

        currentInputBuffer = nullptr;
        currentInputNumChannels = 0;
        currentInputNumSamples = 0;
        currentMidiOutputBuffer = nullptr;

        if (status != noErr)
        {
            copyInputBackTo(audioBuffer, numChannels, numSamples);
            return;
        }

        // Copy rendered output back to the YUP buffer
        for (int c = 0; c < numChannels; ++c)
        {
            std::memcpy(audioBuffer.getWritePointer(c),
                        abl->mBuffers[c].mData,
                        static_cast<std::size_t>(numSamples) * sizeof(float));
        }

        midiBuffer.clear();
        midiBuffer.addEvents(outputMidiBuffer, 0, -1, 0);
    }

    //==============================================================================

    int getCurrentPreset() const noexcept override { return currentPreset; }

    void setCurrentPreset(int index) noexcept override
    {
        CFArrayRef presets = nullptr;
        UInt32 size = sizeof(presets);

        if (AudioUnitGetProperty(audioUnit,
                                 kAudioUnitProperty_FactoryPresets,
                                 kAudioUnitScope_Global, 0,
                                 &presets, &size) != noErr)
        {
            return;
        }

        if (presets == nullptr || !isPositiveAndBelow(index, static_cast<int>(CFArrayGetCount(presets))))
        {
            if (presets != nullptr)
                CFRelease(presets);

            return;
        }

        auto* auPreset = static_cast<const AUPreset*>(CFArrayGetValueAtIndex(presets, index));
        if (auPreset != nullptr)
        {
            AUPreset preset = *auPreset;

            if (AudioUnitSetProperty(audioUnit,
                                     kAudioUnitProperty_PresentPreset,
                                     kAudioUnitScope_Global, 0,
                                     &preset, sizeof(preset)) == noErr)
            {
                currentPreset = index;
                syncParameterValuesFromAudioUnit();
                notifyAudioUnitParametersChanged();
            }
        }

        CFRelease(presets);
    }

    int getNumPresets() const override
    {
        CFArrayRef presets = nullptr;
        UInt32 size = sizeof(presets);

        AudioUnitGetProperty(audioUnit,
                             kAudioUnitProperty_FactoryPresets,
                             kAudioUnitScope_Global, 0,
                             &presets, &size);

        if (presets == nullptr)
            return 0;

        const int count = static_cast<int>(CFArrayGetCount(presets));
        CFRelease(presets);
        return count;
    }

    String getPresetName(int index) const override
    {
        CFArrayRef presets = nullptr;
        UInt32 size = sizeof(presets);

        AudioUnitGetProperty(audioUnit,
                             kAudioUnitProperty_FactoryPresets,
                             kAudioUnitScope_Global, 0,
                             &presets, &size);

        if (presets == nullptr || !isPositiveAndBelow(index, static_cast<int>(CFArrayGetCount(presets))))
        {
            if (presets != nullptr)
                CFRelease(presets);
            return {};
        }

        auto* auPreset = static_cast<const AUPreset*>(CFArrayGetValueAtIndex(presets, index));
        const String name = auPreset->presetName != nullptr
                                ? String::fromCFString(auPreset->presetName)
                                : String();
        CFRelease(presets);
        return name;
    }

    void setPresetName(int, StringRef) override {}

    //==============================================================================

    Result loadStateFromMemory(const MemoryBlock& memoryBlock) override
    {
        CFDataRef data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                     static_cast<const UInt8*>(memoryBlock.getData()),
                                                     static_cast<CFIndex>(memoryBlock.getSize()),
                                                     kCFAllocatorNull);
        if (data == nullptr)
            return Result::fail("Failed to create CFData");

        CFPropertyListRef plist = CFPropertyListCreateWithData(kCFAllocatorDefault,
                                                               data,
                                                               kCFPropertyListImmutable,
                                                               nullptr, nullptr);
        CFRelease(data);

        if (plist == nullptr)
            return Result::fail("Failed to deserialize AU state");

        const OSStatus status = AudioUnitSetProperty(audioUnit,
                                                     kAudioUnitProperty_ClassInfo,
                                                     kAudioUnitScope_Global, 0,
                                                     &plist, sizeof(plist));
        CFRelease(plist);

        if (status != noErr)
            return Result::fail("AudioUnitSetProperty ClassInfo failed: " + String(status));

        syncParameterValuesFromAudioUnit();
        notifyAudioUnitParametersChanged();
        return Result::ok();
    }

    Result saveStateIntoMemory(MemoryBlock& memoryBlock) override
    {
        CFPropertyListRef plist = nullptr;
        UInt32 size = sizeof(plist);

        const OSStatus status = AudioUnitGetProperty(audioUnit,
                                                     kAudioUnitProperty_ClassInfo,
                                                     kAudioUnitScope_Global, 0,
                                                     &plist, &size);
        if (status != noErr || plist == nullptr)
            return Result::fail("AudioUnitGetProperty ClassInfo failed: " + String(status));

        CFDataRef data = CFPropertyListCreateData(kCFAllocatorDefault,
                                                  plist,
                                                  kCFPropertyListBinaryFormat_v1_0,
                                                  0, nullptr);
        CFRelease(plist);

        if (data == nullptr)
            return Result::fail("Failed to serialize AU state");

        memoryBlock.replaceAll(CFDataGetBytePtr(data),
                               static_cast<std::size_t>(CFDataGetLength(data)));
        CFRelease(data);
        return Result::ok();
    }

    //==============================================================================

    bool hasEditor() const override
    {
        UInt32 size = 0;
        Boolean writable = false;

        return AudioUnitGetPropertyInfo(audioUnit,
                                        kAudioUnitProperty_CocoaUI,
                                        kAudioUnitScope_Global, 0,
                                        &size, &writable) == noErr &&
               size >= offsetof(AudioUnitCocoaViewInfo, mCocoaAUViewClass) + sizeof(CFStringRef);
    }

    AudioProcessorEditor* createEditor() override
    {
        if (auto editor = AUv2Editor::create(audioUnit))
            return editor.release();

        return nullptr;
    }

    int getLatencySamples() override
    {
        if (audioUnit == nullptr)
            return 0;

        Float64 latencySeconds = 0.0;
        UInt32 propertySize = sizeof(latencySeconds);

        if (AudioUnitGetProperty(audioUnit,
                                 kAudioUnitProperty_Latency,
                                 kAudioUnitScope_Global,
                                 0,
                                 &latencySeconds,
                                 &propertySize) != noErr)
        {
            return 0;
        }

        return jmax(0, roundToInt(latencySeconds * static_cast<double>(getSampleRate())));
    }

    //==============================================================================

    static std::unique_ptr<AUv2Instance> create(const AudioPluginDescription& desc,
                                                const AudioPluginHostContext& context)
    {
        // Parse "type/subt/mfgr" identifier
        const auto tokens = StringArray::fromTokens(desc.identifier, "/", "");
        if (tokens.size() != 3)
            return nullptr;

        auto fourCC = [](const String& s) -> OSType
        {
            if (s.length() != 4)
                return 0;
            return static_cast<OSType>(
                (static_cast<uint32_t>(s[0]) << 24) | (static_cast<uint32_t>(s[1]) << 16) | (static_cast<uint32_t>(s[2]) << 8) | static_cast<uint32_t>(s[3]));
        };

        AudioComponentDescription acd{};
        acd.componentType = fourCC(tokens[0]);
        acd.componentSubType = fourCC(tokens[1]);
        acd.componentManufacturer = fourCC(tokens[2]);

        AudioComponent comp = AudioComponentFindNext(nullptr, &acd);
        if (comp == nullptr)
            return nullptr;

        AudioUnit unit = nullptr;
        if (AudioComponentInstanceNew(comp, &unit) != noErr || unit == nullptr)
            return nullptr;

        AudioBusLayout busLayout = makeBusLayout(desc, acd.componentType);

        auto instance = std::make_unique<AUv2Instance>(desc, unit, std::move(busLayout));
        instance->setNonRealtime(context.isNonRealtime);
        return instance;
    }

   private:
    static AudioBusLayout makeBusLayout(const AudioPluginDescription& desc, OSType componentType)
    {
        std::vector<AudioBus> inputs;
        std::vector<AudioBus> outputs;

        int inputChannels = desc.numInputChannels;
        int outputChannels = desc.numOutputChannels;

        if (componentType == kAudioUnitType_Effect || componentType == kAudioUnitType_MusicEffect)
        {
            outputChannels = jmax(1, outputChannels, 2);
            inputChannels = jmax(1, inputChannels, outputChannels);
        }
        else if (componentType == kAudioUnitType_MusicDevice || componentType == kAudioUnitType_Generator)
        {
            inputChannels = jmax(0, inputChannels);
            outputChannels = jmax(1, outputChannels, 2);
        }

        if (inputChannels > 0)
            inputs.emplace_back("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, inputChannels);

        if (outputChannels > 0)
            outputs.emplace_back("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, outputChannels);

        if (desc.numMidiInputPorts > 0)
            inputs.emplace_back("MIDI Input", AudioBus::Type::MIDI, AudioBus::Direction::Input, 1);

        if (desc.numMidiOutputPorts > 0)
            outputs.emplace_back("MIDI Output", AudioBus::Type::MIDI, AudioBus::Direction::Output, 1);

        return AudioBusLayout(std::move(inputs), std::move(outputs));
    }

    void parameterValueChanged(const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        if (audioUnit == nullptr || !isPositiveAndBelow(indexInContainer, static_cast<int>(auParameterIds.size())))
            return;

        const auto auParameter = makeAUParameter(audioUnit, auParameterIds[static_cast<std::size_t>(indexInContainer)]);

        AUParameterSet(eventListener,
                       this,
                       &auParameter,
                       static_cast<AudioUnitParameterValue>(parameter->getValue()),
                       0);
    }

    void parameterGestureBegin(const AudioParameter::Ptr&, int indexInContainer) override
    {
        notifyParameterGesture(indexInContainer, kAudioUnitEvent_BeginParameterChangeGesture);
    }

    void parameterGestureEnd(const AudioParameter::Ptr&, int indexInContainer) override
    {
        notifyParameterGesture(indexInContainer, kAudioUnitEvent_EndParameterChangeGesture);
    }

    void notifyParameterGesture(int indexInContainer, AudioUnitEventType eventType)
    {
        if (audioUnit == nullptr || !isPositiveAndBelow(indexInContainer, static_cast<int>(auParameterIds.size())))
            return;

        const auto event = makeAUParameterEvent(audioUnit,
                                                auParameterIds[static_cast<std::size_t>(indexInContainer)],
                                                eventType);

        AUEventListenerNotify(eventListener, this, &event);
    }

    static void parameterEventCallback(void* userData,
                                       void*,
                                       const AudioUnitEvent* event,
                                       UInt64,
                                       AudioUnitParameterValue parameterValue)
    {
        auto* instance = static_cast<AUv2Instance*>(userData);
        if (instance == nullptr || event == nullptr || event->mEventType != kAudioUnitEvent_ParameterValueChange)
            return;

        instance->handleAudioUnitParameterChanged(event->mArgument.mParameter, parameterValue);
    }

    void handleAudioUnitParameterChanged(const AudioUnitParameter& parameter,
                                         AudioUnitParameterValue parameterValue)
    {
        if (parameter.mAudioUnit != audioUnit || parameter.mScope != kAudioUnitScope_Global || parameter.mElement != 0)
        {
            return;
        }

        const auto params = getParameters();
        const auto numParams = jmin(params.size(), auParameterIds.size());

        for (std::size_t i = 0; i < numParams; ++i)
        {
            if (auParameterIds[i] == parameter.mParameterID)
            {
                params[i]->setValue(static_cast<float>(parameterValue));
                return;
            }
        }
    }

    static OSStatus inputRenderCallback(void* refCon,
                                        AudioUnitRenderActionFlags*,
                                        const AudioTimeStamp*,
                                        UInt32,
                                        UInt32 numFrames,
                                        AudioBufferList* ioData)
    {
        auto* instance = static_cast<AUv2Instance*>(refCon);
        if (ioData == nullptr)
            return noErr;

        if (instance == nullptr || instance->currentInputBuffer == nullptr)
        {
            for (UInt32 bufferIndex = 0; bufferIndex < ioData->mNumberBuffers; ++bufferIndex)
            {
                auto& buffer = ioData->mBuffers[bufferIndex];
                auto* dest = static_cast<float*>(buffer.mData);

                if (dest != nullptr)
                    FloatVectorOperations::clear(dest, static_cast<int>(numFrames));

                buffer.mDataByteSize = static_cast<UInt32>(numFrames * sizeof(float));
            }

            return noErr;
        }

        const int framesToCopy = jmin(static_cast<int>(numFrames),
                                      instance->currentInputNumSamples);

        for (UInt32 bufferIndex = 0; bufferIndex < ioData->mNumberBuffers; ++bufferIndex)
        {
            auto& buffer = ioData->mBuffers[bufferIndex];
            auto* dest = static_cast<float*>(buffer.mData);

            if (dest == nullptr)
                continue;

            if (static_cast<int>(bufferIndex) < instance->currentInputNumChannels)
            {
                FloatVectorOperations::copy(dest,
                                            instance->currentInputBuffer->getReadPointer(static_cast<int>(bufferIndex)),
                                            framesToCopy);
            }
            else
            {
                FloatVectorOperations::clear(dest, framesToCopy);
            }

            if (framesToCopy < static_cast<int>(numFrames))
                FloatVectorOperations::clear(dest + framesToCopy,
                                             static_cast<int>(numFrames) - framesToCopy);

            buffer.mDataByteSize = static_cast<UInt32>(numFrames * sizeof(float));
        }

        return noErr;
    }

    void prepareRenderStorage(int numChannels, int maxBlockSize)
    {
        preparedNumChannels = jmax(1, numChannels);
        preparedMaxBlockSize = jmax(1, maxBlockSize);

        inputBuffer.setSize(preparedNumChannels,
                            preparedMaxBlockSize,
                            false,
                            false,
                            true);

        outputABLStorage.resize(getAudioBufferListSize(preparedNumChannels));
    }

    void configureStreamFormat(Float64 sampleRate, int numChannels)
    {
        AudioStreamBasicDescription streamFormat{};
        streamFormat.mSampleRate = sampleRate;
        streamFormat.mFormatID = kAudioFormatLinearPCM;
        streamFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved | kAudioFormatFlagsNativeEndian;
        streamFormat.mBytesPerPacket = sizeof(float);
        streamFormat.mFramesPerPacket = 1;
        streamFormat.mBytesPerFrame = sizeof(float);
        streamFormat.mChannelsPerFrame = static_cast<UInt32>(numChannels);
        streamFormat.mBitsPerChannel = static_cast<UInt32>(sizeof(float) * 8);

        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Input, 0,
                             &streamFormat, sizeof(streamFormat));

        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Output, 0,
                             &streamFormat, sizeof(streamFormat));
    }

    void installInputCallback()
    {
        AURenderCallbackStruct callback{};
        callback.inputProc = inputRenderCallback;
        callback.inputProcRefCon = this;

        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_SetRenderCallback,
                             kAudioUnitScope_Input, 0,
                             &callback, sizeof(callback));
    }

    void installMIDIOutputCallback()
    {
        AUMIDIOutputCallbackStruct callback{};
        callback.midiOutputCallback = midiOutputCallback;
        callback.userData = this;

        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_MIDIOutputCallback,
                             kAudioUnitScope_Global, 0,
                             &callback, sizeof(callback));
    }

    void sendMidiInputEvents(const MidiBuffer& midiBuffer)
    {
        for (const auto& metadata : midiBuffer)
        {
            const auto message = metadata.getMessage();

            if (message.isSysEx())
            {
                MusicDeviceSysEx(audioUnit,
                                 message.getSysExData(),
                                 static_cast<UInt32>(message.getSysExDataSize()));
                continue;
            }

            const auto* data = message.getRawData();
            const int size = message.getRawDataSize();

            if (size <= 0)
                continue;

            MusicDeviceMIDIEvent(audioUnit,
                                 data[0],
                                 size > 1 ? data[1] : 0,
                                 size > 2 ? data[2] : 0,
                                 static_cast<UInt32>(jmax(0, metadata.samplePosition)));
        }
    }

    static OSStatus midiOutputCallback(void* userData,
                                       const AudioTimeStamp*,
                                       UInt32,
                                       const MIDIPacketList* packetList)
    {
        auto* instance = static_cast<AUv2Instance*>(userData);
        if (instance == nullptr || instance->currentMidiOutputBuffer == nullptr || packetList == nullptr)
            return noErr;

        const MIDIPacket* packet = &packetList->packet[0];

        for (UInt32 i = 0; i < packetList->numPackets; ++i)
        {
            instance->currentMidiOutputBuffer->addEvent(packet->data,
                                                        packet->length,
                                                        static_cast<int>(packet->timeStamp));
            packet = MIDIPacketNext(packet);
        }

        return noErr;
    }

    static std::size_t getAudioBufferListSize(int numChannels)
    {
        return sizeof(AudioBufferList) + static_cast<std::size_t>(jmax(0, numChannels - 1)) * sizeof(::AudioBuffer);
    }

    AudioBufferList* makeOutputABL(AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        auto* abl = reinterpret_cast<AudioBufferList*>(outputABLStorage.data());
        abl->mNumberBuffers = static_cast<UInt32>(numChannels);

        for (int c = 0; c < numChannels; ++c)
        {
            abl->mBuffers[c].mNumberChannels = 1;
            abl->mBuffers[c].mDataByteSize = static_cast<UInt32>(buffer.getNumSamples()) * sizeof(float);
            abl->mBuffers[c].mData = buffer.getWritePointer(c);
        }

        return abl;
    }

    void copyInputBackTo(AudioBuffer<float>& audioBuffer, int numChannels, int numSamples)
    {
        for (int c = 0; c < numChannels; ++c)
        {
            FloatVectorOperations::copy(audioBuffer.getWritePointer(c),
                                        inputBuffer.getReadPointer(c),
                                        numSamples);
        }
    }

    void buildParameterList()
    {
        UInt32 size = 0;
        if (AudioUnitGetPropertyInfo(audioUnit,
                                     kAudioUnitProperty_ParameterList,
                                     kAudioUnitScope_Global, 0,
                                     &size, nullptr) != noErr)
        {
            return;
        }

        const int count = static_cast<int>(size / sizeof(AudioUnitParameterID));
        if (count == 0)
            return;

        std::vector<AudioUnitParameterID> paramIds(static_cast<std::size_t>(count));
        if (AudioUnitGetProperty(audioUnit,
                                 kAudioUnitProperty_ParameterList,
                                 kAudioUnitScope_Global, 0,
                                 paramIds.data(), &size) != noErr)
        {
            return;
        }

        installParameterEventListener();

        for (auto paramId : paramIds)
        {
            AudioUnitParameterInfo info{};
            UInt32 infoSize = sizeof(info);
            if (AudioUnitGetProperty(audioUnit,
                                     kAudioUnitProperty_ParameterInfo,
                                     kAudioUnitScope_Global, paramId,
                                     &info, &infoSize) != noErr)
            {
                continue;
            }

            const auto name = makeParameterName(info, paramId);
            const auto range = makeParameterRange(info);
            releaseParameterInfoStrings(info);

            auto param = AudioParameterBuilder()
                             .withID(name + "_" + String(paramId))
                             .withName(name)
                             .withRange(range)
                             .withDefault(info.defaultValue)
                             .withSmoothing(0.0f)
                             .build();

            param->addListener(this);

            auParameterIds.push_back(paramId);
            addParameter(std::move(param));

            addAudioUnitParameterListener(paramId);
        }

        syncParameterValuesFromAudioUnit();
    }

    void installParameterEventListener()
    {
        if (eventListener != nullptr)
            return;

        AUEventListenerCreate(parameterEventCallback,
                              this,
                              CFRunLoopGetMain(),
                              kCFRunLoopCommonModes,
                              0.02f,
                              0.01f,
                              &eventListener);
    }

    void addAudioUnitParameterListener(AudioUnitParameterID paramId)
    {
        if (eventListener == nullptr)
            return;

        const auto event = makeAUParameterEvent(audioUnit, paramId, kAudioUnitEvent_ParameterValueChange);
        AUEventListenerAddEventType(eventListener, this, &event);
    }

    void notifyAudioUnitParametersChanged()
    {
        if (audioUnit == nullptr)
            return;

        for (auto paramId : auParameterIds)
        {
            const auto parameter = makeAUParameter(audioUnit, paramId);
            AUParameterListenerNotify(eventListener, this, &parameter);
        }
    }

    void syncParameterValuesFromAudioUnit() noexcept
    {
        const auto params = getParameters();
        const auto numParams = jmin(params.size(), auParameterIds.size());

        for (std::size_t i = 0; i < numParams; ++i)
        {
            AudioUnitParameterValue value = 0.0f;
            if (AudioUnitGetParameter(audioUnit,
                                      auParameterIds[i],
                                      kAudioUnitScope_Global,
                                      0,
                                      &value) == noErr)
            {
                params[i]->setValue(static_cast<float>(value));
            }
        }
    }

    void removeParameterListeners()
    {
        for (auto& param : getParameters())
            param->removeListener(this);

        if (eventListener != nullptr)
        {
            AUListenerDispose(eventListener);
            eventListener = nullptr;
        }
    }

    void installLatencyListener()
    {
        if (audioUnit != nullptr)
            AudioUnitAddPropertyListener(audioUnit, kAudioUnitProperty_Latency, latencyPropertyChanged, this);
    }

    void removeLatencyListener()
    {
        if (audioUnit != nullptr)
            AudioUnitRemovePropertyListenerWithUserData(audioUnit, kAudioUnitProperty_Latency, latencyPropertyChanged, this);
    }

    static void latencyPropertyChanged(void* userData,
                                       AudioUnit,
                                       AudioUnitPropertyID propertyID,
                                       AudioUnitScope,
                                       AudioUnitElement)
    {
        if (propertyID != kAudioUnitProperty_Latency)
            return;

        auto* instance = static_cast<AUv2Instance*> (userData);
        if (instance != nullptr)
            instance->setLatencySamples(instance->getLatencySamples());
    }

    AudioUnit audioUnit = nullptr;
    AUEventListenerRef eventListener = nullptr;
    int currentPreset = 0;
    double renderSampleTime = 0.0;
    std::vector<AudioUnitParameterID> auParameterIds;
    AudioBuffer<float> inputBuffer;
    MidiBuffer outputMidiBuffer;
    AudioBuffer<float>* currentInputBuffer = nullptr;
    MidiBuffer* currentMidiOutputBuffer = nullptr;
    int currentInputNumChannels = 0;
    int currentInputNumSamples = 0;
    int preparedNumChannels = 0;
    int preparedMaxBlockSize = 0;
    std::vector<uint8_t> outputABLStorage;
};

//==============================================================================

AUv2Format::AUv2Format() = default;
AUv2Format::~AUv2Format() = default;

AudioPluginFormatType AUv2Format::getFormatType() const
{
    return AudioPluginFormatType::audioUnit;
}

String AUv2Format::getFormatName() const
{
    return "AUv2";
}

StringArray AUv2Format::getFileExtensions() const
{
    return {};
}

FileSearchPath AUv2Format::getDefaultSearchPaths() const
{
    return {};
}

ResultValue<std::vector<AudioPluginDescription>> AUv2Format::scanFile(const File&)
{
    // Scan by enumerating the AudioComponent registry (ignores file argument)
    std::vector<AudioPluginDescription> results;

    const OSType types[] = {
        kAudioUnitType_MusicDevice,
        kAudioUnitType_Effect,
        kAudioUnitType_MusicEffect,
        kAudioUnitType_Generator,
        kAudioUnitType_MIDIProcessor};

    for (OSType type : types)
    {
        AudioComponentDescription search{};
        search.componentType = type;

        AudioComponent comp = nullptr;
        while ((comp = AudioComponentFindNext(comp, &search)) != nullptr)
        {
            AudioComponentDescription acd{};
            AudioComponentGetDescription(comp, &acd);
            auto desc = descriptionFromComponent(comp, acd);
            results.push_back(std::move(desc));
        }
    }

    if (results.empty())
        return makeResultValueFail("No AudioComponents found in registry");

    return makeResultValueOk(std::move(results));
}

ResultValue<std::unique_ptr<AudioPluginInstance>> AUv2Format::loadPlugin(
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    auto instance = AUv2Instance::create(description, context);

    if (instance == nullptr)
        return makeResultValueFail("Failed to instantiate AUv2 plugin: " + description.name);

    return makeResultValueOk(std::move(instance));
}

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC
