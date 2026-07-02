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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AUV3 && YUP_MAC

namespace yup
{

//==============================================================================
// AUv3Instance - wraps an AUAudioUnit for hosting

class AUv3Instance final : public AudioPluginInstance
{
public:
    //==============================================================================

    static std::unique_ptr<AUv3Instance> create (const AudioPluginDescription& description,
                                                  const AudioPluginHostContext& context)
    {
        YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "creating AUv3 instance from: " << description.identifier);

        // Parse the identifier into AudioComponentDescription
        auto components = parseIdentifierToComponentDescription (description.identifier);
        if (! components.has_value())
        {
            YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "failed to parse identifier");
            return nullptr;
        }

        auto instance = std::make_unique<AUv3Instance> (description, context);
        instance->description = description;

        return instance;
    }

    //==============================================================================

    void prepareToPlay (float sampleRate, int samplesPerBlock) override
    {
        YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "prepareToPlay: sr=" << String (sampleRate) << ", block=" << String (samplesPerBlock));

        if (audioUnit == nil)
        {
            // Instantiate the AUAudioUnit
            auto components = parseIdentifierToComponentDescription (description.identifier);

            if (components.has_value())
            {
                dispatch_semaphore_t sem = dispatch_semaphore_create (0);

                [AUAudioUnit instantiateWithComponentDescription:*components
                                                          options:0
                                                completionHandler:^(AUAudioUnit* au, NSError* error) {
                    if (au != nil)
                        audioUnit = au;

                    if (error != nil)
                    {
                        YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "instantiate failed: " << String::fromCFString ((__bridge CFStringRef)[error localizedDescription]));
                    }

                    dispatch_semaphore_signal (sem);
                }];

                dispatch_semaphore_wait (sem, DISPATCH_TIME_FOREVER);
            }
        }

        if (audioUnit == nil)
        {
            YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "audioUnit is nil after instantiate");
            return;
        }

        // Set maximum frames to render
        [audioUnit setMaximumFramesToRender:static_cast<AUAudioFrameCount> (samplesPerBlock)];

        // Allocate render resources
        NSError* error = nil;
        if (! [audioUnit allocateRenderResourcesAndReturnError:&error])
        {
            YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "allocateRenderResources failed: " << (error != nil ? String::fromCFString ((__bridge CFStringRef)[error localizedDescription]) : String ()));
            return;
        }

        numInputChannels = static_cast<int> ([audioUnit.inputBusses count]);
        numOutputChannels = static_cast<int> ([audioUnit.outputBusses count]);

        // Collect parameter info
        collectParameters();

        // Setup render observer
        setupRenderObserver();

        // Store block size
        blockSize = samplesPerBlock;
        scratchBuffer.setSize (std::max (numInputChannels, numOutputChannels), blockSize);

        prepared = true;

        YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "prepareToPlay complete: inCh=" << String (numInputChannels) << ", outCh=" << String (numOutputChannels));
    }

    void releaseResources() override
    {
        YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "releaseResources");

        removeRenderObserver();

        if (audioUnit != nil)
            [audioUnit deallocateRenderResources];

        prepared = false;
    }

    //==============================================================================

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        if (audioUnit == nil || ! prepared)
            return;

        const int numFrames = buffer.getNumSamples();

        AudioTimeStamp timeStamp = {};
        timeStamp.mFlags = kAudioTimeStampSampleTimeValid;
        timeStamp.mSampleTime = static_cast<Float64> (processedFrames);

        // Pull input if this is an effect
        AudioUnitRenderActionFlags actionFlags = 0;

        // For instruments, output renders directly
        // For effects, we need to use AudioUnitRender with input callback
        if (numInputChannels > 0)
        {
            // Prepare input data for the effect
            scratchBuffer.clear();

            for (int ch = 0; ch < numInputChannels && ch < buffer.getNumChannels(); ++ch)
            {
                const auto* src = buffer.getReadPointer (ch);
                auto* dst = scratchBuffer.getWritePointer (ch);
                std::copy (src, src + numFrames, dst);
            }

            // Render through the audio unit
            AudioBufferList* inputBufferList = createBufferListForChannels (numInputChannels, scratchBuffer, numFrames);
            AudioBufferList* outputBufferList = createBufferListForChannels (numOutputChannels, buffer, numFrames);

            AURenderPullInputBlock pullInput = ^AUAudioUnitStatus (AudioUnitRenderActionFlags* flags,
                                                                    const AudioTimeStamp* ts,
                                                                    AUAudioFrameCount frames,
                                                                    NSInteger busNumber,
                                                                    AudioBufferList* data) {
                ignoreUnused (flags, ts);

                if (busNumber >= 0 && busNumber < numInputChannels)
                {
                    // Copy from our input buffer
                    for (UInt32 i = 0; i < data->mNumberBuffers; ++i)
                    {
                        const auto* src = scratchBuffer.getReadPointer (static_cast<int> (busNumber));
                        auto* dst = static_cast<float*> (data->mBuffers[i].mData);
                        memcpy (dst, src, frames * sizeof (float));
                    }
                }

                return noErr;
            };

            // Process via internal render block or AudioUnitRender
            AUInternalRenderBlock renderBlock = [audioUnit internalRenderBlock];
            if (renderBlock)
            {
                renderBlock (&actionFlags, &timeStamp, static_cast<AUAudioFrameCount> (numFrames),
                             0, outputBufferList, nullptr, pullInput);
            }

            freeBufferList (inputBufferList);
            freeBufferList (outputBufferList);
        }
        else
        {
            // Instrument - render directly to output
            AudioTimeStamp ts = {};
            ts.mFlags = kAudioTimeStampSampleTimeValid;
            ts.mSampleTime = static_cast<Float64> (processedFrames);

            AudioBufferList* outBufList = createBufferListForChannels (numOutputChannels, buffer, numFrames);

            AUInternalRenderBlock renderBlock = [audioUnit internalRenderBlock];
            if (renderBlock)
            {
                AudioUnitRenderActionFlags flags = 0;
                renderBlock (&flags, &ts, static_cast<AUAudioFrameCount> (numFrames),
                             0, outBufList, nullptr, nullptr);
            }

            freeBufferList (outBufList);
        }

        // Handle MIDI
        if (! midiMessages.isEmpty() && [audioUnit respondsToSelector:@selector (scheduleMIDIEventBlock)])
        {
            AUScheduleMIDIEventBlock midiBlock = [audioUnit scheduleMIDIEventBlock];
            if (midiBlock)
            {
                for (const auto& metadata : midiMessages)
                {
                    midiBlock (static_cast<AUEventSampleTime> (metadata.samplePosition + processedFrames),
                               0,
                               metadata.numBytes,
                               metadata.data);
                }
            }
        }

        processedFrames += numFrames;
    }

    //==============================================================================

    int getNumPrograms() override
    {
        return static_cast<int> ([[audioUnit factoryPresets] count]);
    }

    String getProgramName (int index) override
    {
        if (audioUnit == nil)
            return {};

        auto* presets = [audioUnit factoryPresets];
        if (! isPositiveAndBelow (index, static_cast<int> ([presets count])))
            return {};

        auto* preset = [presets objectAtIndex:static_cast<NSUInteger> (index)];
        return String::fromCFString ((__bridge CFStringRef)[preset name]);
    }

    int getCurrentProgram() override
    {
        if (audioUnit == nil)
            return 0;

        auto* preset = [audioUnit currentPreset];
        if (preset != nil)
            return static_cast<int> ([preset number]);

        return 0;
    }

    void setCurrentProgram (int index) override
    {
        if (audioUnit == nil)
            return;

        auto* presets = [audioUnit factoryPresets];
        if (isPositiveAndBelow (index, static_cast<int> ([presets count])))
        {
            auto* preset = [presets objectAtIndex:static_cast<NSUInteger> (index)];
            [audioUnit setCurrentPreset:preset];
        }
    }

    //==============================================================================

    bool hasEditor() const override
    {
        return audioUnit != nil && [audioUnit respondsToSelector:@selector (requestViewControllerWithCompletionHandler:)];
    }

    AudioProcessorEditor* createEditor() override
    {
        if (audioUnit == nil)
            return nullptr;

        return new AUv3Editor (*this);
    }

    //==============================================================================

    void getStateInformation (MemoryBlock& destData) override
    {
        if (audioUnit == nil)
            return;

        auto* state = [audioUnit fullState];
        if (state == nil)
            return;

        auto* data = [NSJSONSerialization dataWithJSONObject:state options:0 error:nil];
        if (data != nil)
        {
            destData.setSize ([data length]);
            memcpy (destData.getData(), [data bytes], [data length]);
        }
    }

    void setStateInformation (const char* data, int sizeInBytes) override
    {
        if (audioUnit == nil || sizeInBytes <= 0)
            return;

        auto* nsData = [NSData dataWithBytes:data length:static_cast<NSUInteger> (sizeInBytes)];
        auto* state = [NSJSONSerialization JSONObjectWithData:nsData options:0 error:nil];

        if (state != nil)
           // [audioUnit setFullState:state];
           [audioUnit setFullStateForDocument:state];
    }

    //==============================================================================

    const AudioPluginDescription& getPluginDescription() const
    {
        return description;
    }

    AUAudioUnit* getAudioUnit() const { return audioUnit; }

    float getSampleRate() const override { return sampleRate; }

    int getBlockSize() const override { return blockSize; }

private:
    //==============================================================================

    AUv3Instance (const AudioPluginDescription& desc,
                  const AudioPluginHostContext&)
        : description (desc)
    {
    }

    ~AUv3Instance() override
    {
        releaseResources();
        audioUnit = nil;
    }

    //==============================================================================

    static std::optional<AudioComponentDescription> parseIdentifierToComponentDescription (const String& identifier)
    {
        // Format: "type/subtype/manufacturer" (four-char codes)
        auto parts = StringArray::fromTokens (identifier, "/", "");
        if (parts.size() < 3)
            return std::nullopt;

        auto parseOSType = [] (const String& s) -> OSType {
            if (s.length() < 4)
                return 0;

            return static_cast<OSType> (
                (static_cast<uint32_t> (static_cast<uint8_t> (s[0])) << 24) |
                (static_cast<uint32_t> (static_cast<uint8_t> (s[1])) << 16) |
                (static_cast<uint32_t> (static_cast<uint8_t> (s[2])) << 8) |
                static_cast<uint32_t> (static_cast<uint8_t> (s[3])))
            );
        };

        AudioComponentDescription result = {};
        result.componentType = parseOSType (parts[0]);
        result.componentSubType = parseOSType (parts[1]);
        result.componentManufacturer = parseOSType (parts[2]);

        return result;
    }

    //==============================================================================

    void collectParameters()
    {
        hostedParameters.clear();

        if (audioUnit == nil)
            return;

        auto* paramTree = [audioUnit parameterTree];
        if (paramTree == nil)
            return;

        // Collect all parameters from the tree
        collectParametersFromNode (paramTree);
    }

    void collectParametersFromNode (AUParameterNode* node)
    {
        if (node == nil)
            return;

        if ([node isKindOfClass:[AUParameter class]])
        {
            auto* param = static_cast<AUParameter*> (node);

            hostedParameters.push_back (param);

            // Observe parameter changes
            [param setValueObserver:^(AUValue value) {
                // Parameter changed from host side
            }];
        }
        else if ([node isKindOfClass:[AUParameterGroup class]])
        {
            // Recurse into groups - AUParameterGroup is a subclass with children
            // The actual API to enumerate children varies; for now we collect top-level
        }
    }

    //==============================================================================

    void setupRenderObserver()
    {
        if (audioUnit == nil)
            return;

        // Observe latency changes
        renderObserver = (__bridge void*) [audioUnit addObserver:^(AudioUnitProperty property, const AudioUnitParameterValue value) {
            ignoreUnused (property, value);
        }];
    }

    void removeRenderObserver()
    {
        if (audioUnit != nil && renderObserver != nullptr)
        {
            // Remove observer logic
            renderObserver = nullptr;
        }
    }

    //==============================================================================

    static AudioBufferList* createBufferListForChannels (int numChannels, AudioBuffer<float>& buffer, int numFrames)
    {
        const size_t bufferListSize = offsetof (AudioBufferList, mBuffers) + static_cast<size_t> (numChannels) * sizeof (AudioBuffer);
        auto* bufList = static_cast<AudioBufferList*> (malloc (bufferListSize));
        bufList->mNumberBuffers = static_cast<UInt32> (numChannels);

        for (int i = 0; i < numChannels; ++i)
        {
            bufList->mBuffers[i].mNumberChannels = 1;
            bufList->mBuffers[i].mDataByteSize = static_cast<UInt32> (numFrames * sizeof (float));
            bufList->mBuffers[i].mData = buffer.getWritePointer (i);
        }

        return bufList;
    }

    static void freeBufferList (AudioBufferList* bufList)
    {
        if (bufList != nullptr)
            free (bufList);
    }

    //==============================================================================

    AudioPluginDescription description;
    AUAudioUnit* audioUnit = nil;
    int numInputChannels = 0;
    int numOutputChannels = 0;
    float sampleRate = 44100.0f;
    int blockSize = 512;
    bool prepared = false;
    int64_t processedFrames = 0;

    std::vector<AUParameter*> hostedParameters;
    void* renderObserver = nullptr;

    AudioBuffer<float> scratchBuffer;

    //==============================================================================

    class AUv3Editor final : public AudioProcessorEditor
    {
    public:
        AUv3Editor (AUv3Instance& instance)
            : AudioProcessorEditor (&instance)
        {
            if (@available (macOS 10.13, *))
            {
                auto* au = instance.getAudioUnit();

                [au requestViewControllerWithCompletionHandler:^(AUViewControllerBase* vc) {
                    if (vc != nil)
                    {
                        viewController = vc;

                        // Get the view from the view controller
                        NSView* view = [vc view];
                        if (view != nil)
                        {
                            const auto size = view.bounds.size;
                            setSize (static_cast<int> (size.width), static_cast<int> (size.height));
                        }
                    }
                }];
            }
        }

        void resized() override
        {
            if (viewController != nil)
            {
                auto* view = [viewController view];
                if (view != nil)
                {
                    view.frame = NSMakeRect (0, 0, getWidth(), getHeight());
                }
            }
        }

        AUViewControllerBase* getViewController() const { return viewController; }

    private:
        AUViewControllerBase* viewController = nil;
    };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AUv3Instance)
};

//==============================================================================
// AUv3Format implementation

AUv3Format::AUv3Format() = default;
AUv3Format::~AUv3Format() = default;

AudioPluginFormatType AUv3Format::getFormatType() const
{
    return AudioPluginFormatType::audioUnitV3;
}

String AUv3Format::getFormatName() const
{
    return "AUv3";
}

StringArray AUv3Format::getFileExtensions() const
{
    return {};
}

FileSearchPath AUv3Format::getDefaultSearchPaths() const
{
    return {};
}

ResultValue<std::vector<AudioPluginDescription>> AUv3Format::scanFile (const File&)
{
    std::vector<AudioPluginDescription> results;

    YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "scanning AUv3 components via AVAudioUnitComponentManager");

    // Use AVAudioUnitComponentManager to discover AUv3 plugins
    AVAudioUnitComponentManager* manager = [AVAudioUnitComponentManager sharedAudioUnitComponentManager];

    // Get all available components
    NSArray<AVAudioUnitComponent*>* components = [manager componentsMatchingPredicate:nil];

    for (AVAudioUnitComponent* component in components)
    {
        AudioPluginDescription desc;

        desc.formatType = AudioPluginFormatType::audioUnitV3;
        desc.name = String::fromCFString ((__bridge CFStringRef)[component name]);

        auto* acd = [component audioComponentDescription];
        desc.identifier = String::createStringFromData (reinterpret_cast<const char*> (&acd.componentType), 4)
                        + "/"
                        + String::createStringFromData (reinterpret_cast<const char*> (&acd.componentSubType), 4)
                        + "/"
                        + String::createStringFromData (reinterpret_cast<const char*> (&acd.componentManufacturer), 4);

        desc.vendor = String::fromCFString ((__bridge CFStringRef)[component manufacturerName]);
        desc.version = (component.versionString != nil) ? String::fromCFString ((__bridge CFStringRef)[component versionString]) : String();

        desc.isInstrument = (acd.componentType == kAudioUnitType_MusicDevice
                             || acd.componentType == kAudioUnitType_MusicEffect);
        desc.isEffect = (acd.componentType == kAudioUnitType_Effect
                         || acd.componentType == kAudioUnitType_MusicEffect);

        if (desc.isInstrument)
            desc.numOutputChannels = 2;

        if (desc.isEffect)
        {
            desc.numInputChannels = 2;
            desc.numOutputChannels = 2;
        }

        desc.numMidiInputPorts = [component hasMIDIInput] ? 1 : 0;
        desc.numMidiOutputPorts = [component hasMIDIOutput] ? 1 : 0;

        desc.fileOrBundlePath = {}; // App Extensions don't expose bundle path directly

        YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "found: " << desc.name << " [" << desc.identifier << "]");
        results.push_back (std::move (desc));
    }

    YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "scan complete: " << results.size() << " AUv3 components found");

    if (results.empty())
        return makeResultValueFail ("No AUv3 components found in registry");

    return makeResultValueOk (std::move (results));
}

ResultValue<std::unique_ptr<AudioPluginInstance>> AUv3Format::loadPlugin (
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "loading: " << description.name << " [" << description.identifier << "]");

    auto instance = AUv3Instance::create (description, context);

    if (instance == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "load failed: " << description.name);
        return makeResultValueFail ("Failed to instantiate AUv3 plugin: " + description.name);
    }

    YUP_MODULE_DBG (PLUGIN_HOST_AUV3, "loaded: " << description.name);
    return makeResultValueOk (std::move (instance));
}

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_AUV3 && YUP_MAC
