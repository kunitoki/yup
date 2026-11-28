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
    desc.isV3 = (acd.componentFlags & kAudioComponentFlag_IsV3AudioUnit) != 0;

    CFStringRef nameRef = nullptr;
    AudioComponentCopyName(comp, &nameRef);
    if (nameRef != nullptr)
    {
        desc.name = copyCFString(nameRef);
        CFRelease(nameRef);
    }

    // Component names follow the "Manufacturer: Plugin Name" convention
    if (const auto separator = desc.name.indexOfChar(':'); separator > 0)
    {
        desc.vendor = desc.name.substring(0, separator).trim();
        desc.name = desc.name.substring(separator + 1).trim();
    }

    if (desc.vendor.isEmpty())
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

#if YUP_MODULE_AVAILABLE_yup_gui
class AUEditor : public AudioProcessorEditor
{
   public:
    static std::unique_ptr<AUEditor> create(AudioUnit audioUnit)
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

        return std::unique_ptr<AUEditor>(new AUEditor(view, viewBundle, viewFactory));
    }

    ~AUEditor() override
    {
        if (frameObserver != nil)
            [[NSNotificationCenter defaultCenter] removeObserver:frameObserver];

        detachCocoaView();
    }

    bool isResizable() const override { return resizable; }

    Size<int> getPreferredSize() const override { return preferredSize; }

    bool shouldPreserveAspectRatio() const override { return aspectRatioSize.has_value(); }

    Size<int> getPreferredAspectRatioSize() const override { return aspectRatioSize.value_or(preferredSize); }

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
    AUEditor(NSView* view, NSBundle* viewBundle, id<AUCocoaUIBase> viewFactory)
        : cocoaView(view), cocoaViewBundle(viewBundle), cocoaViewFactory(viewFactory)
    {
        auto size = [cocoaView frame].size;

        if (size.width <= 1.0 || size.height <= 1.0)
            size = [cocoaView fittingSize];

        preferredSize = {
            jmax(320, static_cast<int>(std::ceil(size.width))),
            jmax(240, static_cast<int>(std::ceil(size.height)))};

        resizable = ([cocoaView autoresizingMask] & (NSViewWidthSizable | NSViewHeightSizable)) != 0;

        if (resizable)
        {
            const auto originalSize = [cocoaView frame].size;

            aspectRatioSize = probeHostedEditorAspectRatio({static_cast<int>(std::lround(size.width)), static_cast<int>(std::lround(size.height))}, [this](Size<int> sizeToConstrain)
            {
                [cocoaView setFrameSize:NSMakeSize(sizeToConstrain.getWidth(), sizeToConstrain.getHeight())];

                const auto constrainedSize = [cocoaView frame].size;
                return Size<int>(static_cast<int>(std::lround(constrainedSize.width)), static_cast<int>(std::lround(constrainedSize.height)));
            });

            [cocoaView setFrameSize:originalSize];
        }

        [cocoaView setAutoresizingMask:NSViewNotSizable];
        [cocoaView setFrameSize:NSMakeSize(preferredSize.getWidth(), preferredSize.getHeight())];

        // Plugins resize their view on their own (corner resizers, or constraining the size we set), follow them
        [cocoaView setPostsFrameChangedNotifications:YES];
        frameObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSViewFrameDidChangeNotification
                                                                          object:cocoaView
                                                                           queue:nil
                                                                      usingBlock:^(NSNotification*)
        {
            if ([cocoaView superview] == nil)
                return;

            const auto size = [cocoaView frame].size;
            const Size<int> pluginSize{static_cast<int>(std::lround(size.width)), static_cast<int>(std::lround(size.height))};

            if (updatingCocoaViewFrame)
                followHostedEditorSize(*this, pluginSize);
            else if (pluginSize != getSize().to<int>())
                setHostedEditorSize(*this, pluginSize);
        }];
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

        const ScopedValueSetter<bool> scopedUpdating(updatingCocoaViewFrame, true);

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
    std::optional<Size<int>> aspectRatioSize;
    bool resizable = false;
    bool updatingCocoaViewFrame = false;
    NSView* __strong cocoaView = nil;
    NSBundle* __strong cocoaViewBundle = nil;
    id<AUCocoaUIBase> __strong cocoaViewFactory = nil;
    id __strong frameObserver = nil;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AUEditor)
};
#endif

//==============================================================================

class AUInstance : public AudioPluginInstance, private AudioParameter::Listener
{
   public:
    AUInstance(const AudioPluginDescription& desc,
                 AudioUnit unit,
                 AudioBusLayout busLayout)
        : AudioPluginInstance(desc, std::move(busLayout)), audioUnit(unit)
    {
        buildParameterList();
        installPropertyListeners();
    }

    ~AUInstance() override
    {
        removePropertyListeners();
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

    void prepareToPlay(const AudioSpec& spec) override
    {
        releaseResources();

        renderSampleTime = 0.0;
        updateOfflineRenderMode();

        const auto numHostedChannels = jmax(2,
                                            pluginDescription.numInputChannels,
                                            pluginDescription.numOutputChannels);
        prepareRenderStorage(numHostedChannels, spec.maxBlockSize);

        const Float64 sampleRateValue = static_cast<Float64>(spec.sampleRate);
        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_SampleRate,
                             kAudioUnitScope_Global, 0,
                             &sampleRateValue, sizeof(sampleRateValue));

        UInt32 blockSize = static_cast<UInt32>(spec.maxBlockSize);
        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_MaximumFramesPerSlice,
                             kAudioUnitScope_Global, 0,
                             &blockSize, sizeof(blockSize));

        configureStreamFormat(sampleRateValue, numHostedChannels);
        installInputCallback();
        installHostCallbacks();
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

    void nonRealtimeStateChanged() override
    {
        updateOfflineRenderMode();
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
        currentPlayHead = context.playHead;
        currentPlayHeadPositionQueried = false;
        currentPlayHeadPosition = std::nullopt;

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
        currentPlayHead = nullptr;
        currentPlayHeadPositionQueried = false;
        currentPlayHeadPosition = std::nullopt;

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
#if YUP_MODULE_AVAILABLE_yup_gui
        UInt32 size = 0;
        Boolean writable = false;

        return AudioUnitGetPropertyInfo(audioUnit,
                                        kAudioUnitProperty_CocoaUI,
                                        kAudioUnitScope_Global, 0,
                                        &size, &writable) == noErr &&
               size >= offsetof(AudioUnitCocoaViewInfo, mCocoaAUViewClass) + sizeof(CFStringRef);
#else
        return false;
#endif
    }

    AudioProcessorEditor* createEditor() override
    {
#if YUP_MODULE_AVAILABLE_yup_gui
        if (auto editor = AUEditor::create(audioUnit))
            return editor.release();
#endif

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

    int getTailSamples() override
    {
        if (audioUnit == nullptr)
            return 0;

        Float64 tailSeconds = 0.0;
        UInt32 propertySize = sizeof(tailSeconds);

        if (AudioUnitGetProperty(audioUnit,
                                 kAudioUnitProperty_TailTime,
                                 kAudioUnitScope_Global,
                                 0,
                                 &tailSeconds,
                                 &propertySize) != noErr)
        {
            return 0;
        }

        const auto tailSamples = tailSeconds * static_cast<double>(getSampleRate());

        if (! (tailSamples > 0.0))
            return 0;

        return tailSamples >= static_cast<double>(infiniteTailSamples) ? infiniteTailSamples : roundToInt(tailSamples);
    }

    //==============================================================================

    static std::unique_ptr<AUInstance> create(const AudioPluginDescription& desc,
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

        if (desc.isV3)
        {
            // AUv3 components must be instantiated asynchronously; use a semaphore to wait
            dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
            __block AudioUnit blockUnit = nullptr;

            AudioComponentInstantiate(comp,
                                      kAudioComponentInstantiation_LoadOutOfProcess,
                                      ^(AudioComponentInstance au, OSStatus err) {
                                          if (err == noErr)
                                              blockUnit = au;
                                          dispatch_semaphore_signal(semaphore);
                                      });

            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
            unit = blockUnit;
        }
        else
        {
            if (AudioComponentInstanceNew(comp, &unit) != noErr)
                return nullptr;
        }

        if (unit == nullptr)
            return nullptr;

        auto loadedDesc = desc;

        if (acd.componentType == kAudioUnitType_MIDIProcessor && loadedDesc.numOutputChannels <= 0)
            loadedDesc.numOutputChannels = getBusChannelCount(unit, kAudioUnitScope_Output);

        if ((acd.componentType == kAudioUnitType_MusicDevice || acd.componentType == kAudioUnitType_MIDIProcessor)
            && loadedDesc.numInputChannels <= 0)
            loadedDesc.numInputChannels = getBusChannelCount(unit, kAudioUnitScope_Input);

        if (loadedDesc.numMidiOutputPorts <= 0 && hasMidiOutput(unit))
            loadedDesc.numMidiOutputPorts = 1;

        AudioBusLayout busLayout = makeBusLayout(loadedDesc, acd.componentType);

        auto instance = std::make_unique<AUInstance>(loadedDesc, unit, std::move(busLayout));
        instance->setNonRealtime(context.isNonRealtime);
        return instance;
    }

   private:
    /** Whether the AU names MIDI outputs (kAudioUnitProperty_MIDIOutputCallbackInfo), which an instrument may have */
    static bool hasMidiOutput(AudioUnit unit)
    {
        CFArrayRef names = nullptr;
        UInt32 size = sizeof(names);

        if (AudioUnitGetProperty(unit, kAudioUnitProperty_MIDIOutputCallbackInfo, kAudioUnitScope_Global, 0, &names, &size) != noErr
            || names == nullptr)
            return false;

        const auto count = CFArrayGetCount(names);
        CFRelease(names);
        return count > 0;
    }

    /** The channels of the first bus of a scope, or 0 when the AU has none there. */
    static int getBusChannelCount(AudioUnit unit, AudioUnitScope scope)
    {
        UInt32 elementCount = 0;
        UInt32 size = sizeof(elementCount);

        if (AudioUnitGetProperty(unit, kAudioUnitProperty_ElementCount, scope, 0, &elementCount, &size) != noErr || elementCount == 0)
            return 0;

        AudioStreamBasicDescription format{};
        size = sizeof(format);

        if (AudioUnitGetProperty(unit, kAudioUnitProperty_StreamFormat, scope, 0, &format, &size) != noErr)
            return 0;

        return static_cast<int>(format.mChannelsPerFrame);
    }

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
            inputs.emplace_back("MIDI Input", AudioBus::Type::Midi, AudioBus::Direction::Input, 1);

        if (desc.numMidiOutputPorts > 0)
            outputs.emplace_back("MIDI Output", AudioBus::Type::Midi, AudioBus::Direction::Output, 1);

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
        auto* instance = static_cast<AUInstance*>(userData);
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
        auto* instance = static_cast<AUInstance*>(refCon);
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

    void installHostCallbacks()
    {
        HostCallbackInfo callbacks {};
        callbacks.hostUserData = this;
        callbacks.beatAndTempoProc = hostBeatAndTempoCallback;
        callbacks.musicalTimeLocationProc = hostMusicalTimeLocationCallback;
        callbacks.transportStateProc = hostTransportStateCallback;
        callbacks.transportStateProc2 = hostTransportState2Callback;

        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_HostCallbacks,
                             kAudioUnitScope_Global, 0,
                             &callbacks, sizeof(callbacks));
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

    static OSStatus hostBeatAndTempoCallback(void* userData,
                                             Float64* outCurrentBeat,
                                             Float64* outCurrentTempo)
    {
        auto* instance = static_cast<AUInstance*>(userData);
        if (instance == nullptr)
            return kAudioUnitErr_CannotDoInCurrentContext;

        const auto* position = instance->getCurrentPlayHeadPosition();
        if (position == nullptr)
            return kAudioUnitErr_CannotDoInCurrentContext;

        if (outCurrentBeat != nullptr)
        {
            const auto ppq = position->getPpqPosition();
            if (! ppq.has_value())
                return kAudioUnitErr_CannotDoInCurrentContext;

            *outCurrentBeat = *ppq;
        }

        if (outCurrentTempo != nullptr)
        {
            const auto bpm = position->getBpm();
            if (! bpm.has_value())
                return kAudioUnitErr_CannotDoInCurrentContext;

            *outCurrentTempo = *bpm;
        }

        return noErr;
    }

    static OSStatus hostMusicalTimeLocationCallback(void* userData,
                                                    UInt32* outDeltaSampleOffsetToNextBeat,
                                                    Float32* outTimeSigNumerator,
                                                    UInt32* outTimeSigDenominator,
                                                    Float64* outCurrentMeasureDownBeat)
    {
        auto* instance = static_cast<AUInstance*>(userData);
        if (instance == nullptr)
            return kAudioUnitErr_CannotDoInCurrentContext;

        const auto* position = instance->getCurrentPlayHeadPosition();
        if (position == nullptr)
            return kAudioUnitErr_CannotDoInCurrentContext;

        if (outDeltaSampleOffsetToNextBeat != nullptr)
        {
            const auto ppq = position->getPpqPosition();
            const auto bpm = position->getBpm();
            const auto sampleRate = instance->getSampleRate();

            if (! ppq.has_value() || ! bpm.has_value() || sampleRate <= 0.0)
                return kAudioUnitErr_CannotDoInCurrentContext;

            const auto nextBeat = std::ceil (*ppq);
            const auto beatsToNext = jmax (0.0, nextBeat - *ppq);
            *outDeltaSampleOffsetToNextBeat = static_cast<UInt32> (beatsToNext * (60.0 / *bpm) * sampleRate);
        }

        if (outTimeSigNumerator != nullptr || outTimeSigDenominator != nullptr)
        {
            const auto timeSignature = position->getTimeSignature();
            if (! timeSignature.has_value())
                return kAudioUnitErr_CannotDoInCurrentContext;

            if (outTimeSigNumerator != nullptr)
                *outTimeSigNumerator = static_cast<Float32> (timeSignature->numerator);

            if (outTimeSigDenominator != nullptr)
                *outTimeSigDenominator = static_cast<UInt32> (timeSignature->denominator);
        }

        if (outCurrentMeasureDownBeat != nullptr)
        {
            const auto barStart = position->getPpqPositionOfLastBarStart();
            if (! barStart.has_value())
                return kAudioUnitErr_CannotDoInCurrentContext;

            *outCurrentMeasureDownBeat = *barStart;
        }

        return noErr;
    }

    static OSStatus hostTransportStateCallback(void* userData,
                                               Boolean* outIsPlaying,
                                               Boolean* outTransportStateChanged,
                                               Float64* outCurrentSampleInTimeLine,
                                               Boolean* outIsCycling,
                                               Float64* outCycleStartBeat,
                                               Float64* outCycleEndBeat)
    {
        return fillHostTransportState(userData,
                                      outIsPlaying,
                                      nullptr,
                                      outTransportStateChanged,
                                      outCurrentSampleInTimeLine,
                                      outIsCycling,
                                      outCycleStartBeat,
                                      outCycleEndBeat);
    }

    static OSStatus hostTransportState2Callback(void* userData,
                                                Boolean* outIsPlaying,
                                                Boolean* outIsRecording,
                                                Boolean* outTransportStateChanged,
                                                Float64* outCurrentSampleInTimeLine,
                                                Boolean* outIsCycling,
                                                Float64* outCycleStartBeat,
                                                Float64* outCycleEndBeat)
    {
        return fillHostTransportState(userData,
                                      outIsPlaying,
                                      outIsRecording,
                                      outTransportStateChanged,
                                      outCurrentSampleInTimeLine,
                                      outIsCycling,
                                      outCycleStartBeat,
                                      outCycleEndBeat);
    }

    static OSStatus fillHostTransportState(void* userData,
                                           Boolean* outIsPlaying,
                                           Boolean* outIsRecording,
                                           Boolean* outTransportStateChanged,
                                           Float64* outCurrentSampleInTimeLine,
                                           Boolean* outIsCycling,
                                           Float64* outCycleStartBeat,
                                           Float64* outCycleEndBeat)
    {
        auto* instance = static_cast<AUInstance*>(userData);
        if (instance == nullptr)
            return kAudioUnitErr_CannotDoInCurrentContext;

        const auto* position = instance->getCurrentPlayHeadPosition();
        if (position == nullptr)
            return kAudioUnitErr_CannotDoInCurrentContext;

        if (outIsPlaying != nullptr)
            *outIsPlaying = position->getIsPlaying();

        if (outIsRecording != nullptr)
            *outIsRecording = position->getIsRecording();

        if (outTransportStateChanged != nullptr)
        {
            const bool isPlaying = position->getIsPlaying();
            *outTransportStateChanged = instance->lastReportedPlaying.has_value() && *instance->lastReportedPlaying != isPlaying;
            instance->lastReportedPlaying = isPlaying;
        }

        if (outCurrentSampleInTimeLine != nullptr)
        {
            const auto timeInSamples = position->getTimeInSamples();
            if (! timeInSamples.has_value())
                return kAudioUnitErr_CannotDoInCurrentContext;

            *outCurrentSampleInTimeLine = static_cast<Float64> (*timeInSamples);
        }

        const auto loopPoints = position->getLoopPoints();

        if (outIsCycling != nullptr)
            *outIsCycling = loopPoints.has_value() && position->getIsLooping();

        if (outCycleStartBeat != nullptr)
            *outCycleStartBeat = loopPoints.has_value() ? loopPoints->ppqStart : 0.0;

        if (outCycleEndBeat != nullptr)
            *outCycleEndBeat = loopPoints.has_value() ? loopPoints->ppqEnd : 0.0;

        return noErr;
    }

    void sendMidiInputEvents(const MidiBuffer& midiBuffer)
    {
        for (const auto& metadata : midiBuffer)
        {
            const auto message = metadata.getMessage();

            if (message.isSysEx())
            {
                MusicDeviceSysEx(audioUnit,
                                 metadata.data,
                                 static_cast<UInt32>(metadata.numBytes));
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
        auto* instance = static_cast<AUInstance*>(userData);
        if (instance == nullptr || instance->currentMidiOutputBuffer == nullptr || packetList == nullptr)
            return noErr;

        const MIDIPacket* packet = &packetList->packet[0];

        for (UInt32 i = 0; i < packetList->numPackets; ++i)
        {
            const auto* data = packet->data;
            const int size = static_cast<int>(packet->length);
            const auto time = static_cast<int>(packet->timeStamp);

            for (int offset = 0; offset < size;)
            {
                int length = 1;

                if (data[offset] == 0xf0)
                {
                    while (offset + length < size && data[offset + length - 1] != 0xf7)
                        ++length;
                }
                else if (data[offset] >= 0x80)
                {
                    length = jmin(size - offset, MidiMessage::getMessageLengthFromFirstByte(data[offset]));
                }
                else
                {
                    ++offset;   // a stray data byte (running status isn't used in packets)
                    continue;
                }

                instance->currentMidiOutputBuffer->addEvent(data + offset, length, time);
                offset += jmax(1, length);
            }

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
            const bool isIndexed = info.unit == kAudioUnitParameterUnit_Indexed;
            const bool isBoolean = info.unit == kAudioUnitParameterUnit_Boolean;
            const bool hasValueStrings = (info.flags & kAudioUnitParameterFlag_ValuesHaveStrings) != 0;
            auto range = makeParameterRange(info);

            if ((isIndexed || isBoolean) && range.end > range.start)
                range = NormalisableRange<float>(range.start, range.end, 1.0f);

            auto valueStrings = std::make_shared<StringArray>(isIndexed ? getParameterValueStrings(paramId) : StringArray());
            releaseParameterInfoStrings(info);

            AudioParameterBuilder builder;
            builder.withID(String(paramId))   // the AU's parameter id, as hosts know it
                .withName(name)
                .withRange(range)
                .withDefault(info.defaultValue)
                .withSmoothing(0.0f)
                .withStepped(isIndexed || isBoolean)
                .withEnum(isIndexed)
                .withAutomatable((info.flags & kAudioUnitParameterFlag_NonRealTime) == 0);

            if (! valueStrings->isEmpty() || hasValueStrings)
            {
                const auto minimum = range.start;

                builder.withValueToString([this, paramId, valueStrings, minimum, hasValueStrings](float value)
                {
                    if (const auto index = roundToInt(value - minimum); isPositiveAndBelow(index, valueStrings->size()))
                        return (*valueStrings)[index];

                    if (hasValueStrings)
                        if (const auto text = getParameterStringFromValue(paramId, value); text.isNotEmpty())
                            return text;

                    return String(value, 2);
                });

                builder.withStringToValue([this, paramId, valueStrings, minimum, hasValueStrings](const String& text)
                {
                    if (const auto index = valueStrings->indexOf(text); index >= 0)
                        return minimum + static_cast<float>(index);

                    if (hasValueStrings)
                        if (const auto value = getParameterValueFromString(paramId, text))
                            return *value;

                    return text.getFloatValue();
                });
            }

            auto param = builder.build();

            param->addListener(this);

            auParameterIds.push_back(paramId);
            addParameter(std::move(param));

            addAudioUnitParameterListener(paramId);
        }

        syncParameterValuesFromAudioUnit();
    }

    /** The names of an indexed parameter's values (kAudioUnitProperty_ParameterValueStrings), if the AU has them */
    StringArray getParameterValueStrings(AudioUnitParameterID paramId) const
    {
        StringArray strings;
        CFArrayRef array = nullptr;
        UInt32 size = sizeof(array);

        if (AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterValueStrings, kAudioUnitScope_Global, paramId, &array, &size) != noErr
            || array == nullptr)
            return strings;

        for (CFIndex i = 0; i < CFArrayGetCount(array); ++i)
            strings.add(String::fromCFString(static_cast<CFStringRef>(CFArrayGetValueAtIndex(array, i))));

        CFRelease(array);
        return strings;
    }

    /** A value's text from the AU (kAudioUnitProperty_ParameterStringFromValue), or an empty string */
    String getParameterStringFromValue(AudioUnitParameterID paramId, float value) const
    {
        AudioUnitParameterValue parameterValue = value;
        AudioUnitParameterStringFromValue request{};
        request.inParamID = paramId;
        request.inValue = &parameterValue;
        UInt32 size = sizeof(request);

        if (AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterStringFromValue, kAudioUnitScope_Global, 0, &request, &size) != noErr
            || request.outString == nullptr)
            return {};

        const auto text = String::fromCFString(request.outString);
        CFRelease(request.outString);
        return text;
    }

    /** A text's value from the AU (kAudioUnitProperty_ParameterValueFromString), if it can read it */
    std::optional<float> getParameterValueFromString(AudioUnitParameterID paramId, const String& text) const
    {
        AudioUnitParameterValueFromString request{};
        request.inParamID = paramId;
        request.inString = text.toCFString();
        UInt32 size = sizeof(request);

        const auto result = AudioUnitGetProperty(audioUnit, kAudioUnitProperty_ParameterValueFromString, kAudioUnitScope_Global, 0, &request, &size);
        CFRelease(request.inString);

        if (result != noErr)
            return std::nullopt;

        return static_cast<float>(request.outValue);
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

    static constexpr AudioUnitPropertyID observedProperties[] = { kAudioUnitProperty_Latency, kAudioUnitProperty_TailTime };

    void installPropertyListeners()
    {
        if (audioUnit == nullptr)
            return;

        for (const auto propertyID : observedProperties)
            AudioUnitAddPropertyListener(audioUnit, propertyID, latencyOrTailChanged, this);
    }

    void removePropertyListeners()
    {
        if (audioUnit == nullptr)
            return;

        for (const auto propertyID : observedProperties)
            AudioUnitRemovePropertyListenerWithUserData(audioUnit, propertyID, latencyOrTailChanged, this);
    }

    void updateOfflineRenderMode()
    {
        if (audioUnit == nullptr)
            return;

        UInt32 offline = isNonRealtime() ? 1u : 0u;
        AudioUnitSetProperty(audioUnit,
                             kAudioUnitProperty_OfflineRender,
                             kAudioUnitScope_Global,
                             0,
                             &offline,
                             sizeof(offline));
    }

    static void latencyOrTailChanged(void* userData,
                                     AudioUnit,
                                     AudioUnitPropertyID propertyID,
                                     AudioUnitScope,
                                     AudioUnitElement)
    {
        auto* instance = static_cast<AUInstance*> (userData);
        if (instance == nullptr)
            return;

        if (propertyID == kAudioUnitProperty_Latency)
            instance->setLatencySamples(instance->getLatencySamples());
        else if (propertyID == kAudioUnitProperty_TailTime)
            instance->updateHostDisplay(ChangeDetails().withTailChanged(true));
    }

    const AudioPlayHead::PositionInfo* getCurrentPlayHeadPosition()
    {
        if (currentPlayHead == nullptr)
            return nullptr;

        if (! currentPlayHeadPositionQueried)
        {
            currentPlayHeadPosition = currentPlayHead->getPosition();
            currentPlayHeadPositionQueried = true;
        }

        return currentPlayHeadPosition.has_value() ? &*currentPlayHeadPosition : nullptr;
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
    AudioPlayHead* currentPlayHead = nullptr;
    bool currentPlayHeadPositionQueried = false;
    std::optional<bool> lastReportedPlaying;    // The playing state the AU was last given (transport state changed)
    std::optional<AudioPlayHead::PositionInfo> currentPlayHeadPosition;
    int currentInputNumChannels = 0;
    int currentInputNumSamples = 0;
    int preparedNumChannels = 0;
    int preparedMaxBlockSize = 0;
    std::vector<uint8_t> outputABLStorage;
};

//==============================================================================

AUFormat::AUFormat() = default;
AUFormat::~AUFormat() = default;

AudioPluginFormatType AUFormat::getFormatType() const
{
    return AudioPluginFormatType::audioUnit;
}

String AUFormat::getFormatName() const
{
    return "AU";
}

StringArray AUFormat::getFileExtensions() const
{
    return {};
}

FileSearchPath AUFormat::getDefaultSearchPaths() const
{
    return {};
}

ResultValue<std::vector<AudioPluginDescription>> AUFormat::scanFile(const File&)
{
    // Scan all AudioComponents in one pass using a zeroed descriptor, then filter to hostable types
    std::vector<AudioPluginDescription> results;

    auto isHostableType = [](OSType type) -> bool
    {
        return type == kAudioUnitType_MusicDevice
            || type == kAudioUnitType_Effect
            || type == kAudioUnitType_MusicEffect
            || type == kAudioUnitType_Generator
            || type == kAudioUnitType_MIDIProcessor
            || type == kAudioUnitType_Panner
            || type == kAudioUnitType_Mixer;
    };

    AudioComponentDescription search{};
    AudioComponent comp = nullptr;

    while ((comp = AudioComponentFindNext(comp, &search)) != nullptr)
    {
        AudioComponentDescription acd{};
        AudioComponentGetDescription(comp, &acd);

        if (! isHostableType(acd.componentType))
            continue;

        auto desc = descriptionFromComponent(comp, acd);
        YUP_MODULE_DBG (PLUGIN_HOST_AU, "scan found: " << desc.name << " [" << desc.identifier << "] v3=" << (int) desc.isV3);
        results.push_back(std::move(desc));
    }

    YUP_MODULE_DBG (PLUGIN_HOST_AU, "scan complete: " << results.size() << " AudioComponents found");

    if (results.empty())
        return makeResultValueFail("No AudioComponents found in registry");

    return makeResultValueOk(std::move(results));
}

ResultValue<std::unique_ptr<AudioPluginInstance>> AUFormat::loadPlugin(
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    YUP_MODULE_DBG (PLUGIN_HOST_AU, "loading: " << description.name << " [" << description.identifier << "]");

    auto instance = AUInstance::create(description, context);

    if (instance == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_HOST_AU, "load failed: " << description.name);
        return makeResultValueFail("Failed to instantiate AU plugin: " + description.name);
    }

    YUP_MODULE_DBG (PLUGIN_HOST_AU, "loaded: " << description.name);
    return makeResultValueOk(std::move(instance));
}

} // namespace yup
