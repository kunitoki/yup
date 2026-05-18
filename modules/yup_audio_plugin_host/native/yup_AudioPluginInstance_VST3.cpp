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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3

namespace yup
{

using namespace Steinberg;

namespace
{

//==============================================================================
// Converts a Steinberg TChar (UTF-16) string to a yup::String.
String toString (const Vst::TChar* src)
{
    return String (CharPointer_UTF16 (reinterpret_cast<const CharPointer_UTF16::CharType*> (src)));
}

String classIDToString (const TUID& cid)
{
    return String::toHexString (cid, static_cast<int> (sizeof (TUID)));
}

//==============================================================================
// RAII wrapper around a dynamically loaded VST3 module.
struct VST3Module
{
    ModuleHandle handle = nullptr;
    using FactoryFn = IPluginFactory* (*) ();
    FactoryFn getFactory = nullptr;

    static std::unique_ptr<VST3Module> load (const File& file)
    {
        auto m = std::make_unique<VST3Module>();

#if YUP_MAC
        const auto bundlePath = file.getFullPathName().toStdString();
        m->handle = loadModule (bundlePath.c_str());
#else
        m->handle = loadModule (file.getFullPathName().toRawUTF8());
#endif

        if (m->handle == nullptr)
            return nullptr;

        m->getFactory = reinterpret_cast<FactoryFn> (
            getFunctionAddress (m->handle, "GetPluginFactory"));

        if (m->getFactory == nullptr)
        {
            unloadModule (m->handle);
            return nullptr;
        }

        return m;
    }

    ~VST3Module()
    {
        if (handle != nullptr)
            unloadModule (handle);
    }
};

//==============================================================================
// Minimal IComponentHandler stub — required by IAudioProcessor::initialize().
class HostComponentHandler : public Vst::IComponentHandler
{
public:
    tresult PLUGIN_API beginEdit (Vst::ParamID) override { return kResultOk; }

    tresult PLUGIN_API performEdit (Vst::ParamID, Vst::ParamValue) override { return kResultOk; }

    tresult PLUGIN_API endEdit (Vst::ParamID) override { return kResultOk; }

    tresult PLUGIN_API restartComponent (int32) override { return kResultOk; }

    DECLARE_FUNKNOWN_METHODS
};

} // namespace

//==============================================================================

class VST3Instance : public AudioPluginInstance
{
public:
    VST3Instance (const AudioPluginDescription& desc,
                  const AudioPluginHostContext& context,
                  std::unique_ptr<VST3Module> module,
                  IPtr<Vst::IComponent> component,
                  IPtr<Vst::IAudioProcessor> processor,
                  IPtr<Vst::IEditController> controller)
        : AudioPluginInstance (desc,
                               buildBusLayout (component.get()))
        , hostContext (context)
        , vst3Module (std::move (module))
        , vst3Component (std::move (component))
        , vst3Processor (std::move (processor))
        , vst3Controller (std::move (controller))
    {
        buildParameterList();
    }

    ~VST3Instance() override
    {
        releaseResources();
        vst3Processor = nullptr;
        vst3Controller = nullptr;
        vst3Component->terminate();
        vst3Component = nullptr;
    }

    //==============================================================================

    void prepareToPlay (float sampleRate, int maxBlockSize) override
    {
        Vst::ProcessSetup setup;
        setup.processMode = Vst::kRealtime;
        setProcessingPrecision (hostContext.preferDoublePrecision && supportsDoublePrecisionProcessing()
                                    ? ProcessingPrecision::doublePrecision
                                    : ProcessingPrecision::singlePrecision);

        setup.symbolicSampleSize = isUsingDoublePrecision() ? Vst::kSample64 : Vst::kSample32;
        setup.maxSamplesPerBlock = maxBlockSize;
        setup.sampleRate = sampleRate;

        if (isUsingDoublePrecision())
        {
            const int numChannels = jmax (1, pluginDescription.numInputChannels, pluginDescription.numOutputChannels);
            doublePrecisionBuffer.setSize (numChannels, maxBlockSize, false, true, false);
        }

        vst3Processor->setupProcessing (setup);

        const int numInputs = vst3Component->getBusCount (Vst::kAudio, Vst::kInput);
        for (int i = 0; i < numInputs; ++i)
            vst3Component->activateBus (Vst::kAudio, Vst::kInput, i, true);

        const int numOutputs = vst3Component->getBusCount (Vst::kAudio, Vst::kOutput);
        for (int i = 0; i < numOutputs; ++i)
            vst3Component->activateBus (Vst::kAudio, Vst::kOutput, i, true);

        vst3Component->setActive (true);
        vst3Processor->setProcessing (true);
    }

    void releaseResources() override
    {
        if (vst3Processor != nullptr)
            vst3Processor->setProcessing (false);

        if (vst3Component != nullptr)
            vst3Component->setActive (false);
    }

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer) override
    {
        ScopedNoDenormals noDenormals;

        if (isUsingDoublePrecision())
        {
            doublePrecisionBuffer.makeCopyOf (audioBuffer, true);
            processBlock (doublePrecisionBuffer, midiBuffer);

            const int numChannels = jmin (audioBuffer.getNumChannels(), doublePrecisionBuffer.getNumChannels());
            const int numSamples = jmin (audioBuffer.getNumSamples(), doublePrecisionBuffer.getNumSamples());

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* destination = audioBuffer.getWritePointer (channel);
                const auto* source = doublePrecisionBuffer.getReadPointer (channel);

                for (int sample = 0; sample < numSamples; ++sample)
                    destination[sample] = static_cast<float> (source[sample]);
            }

            return;
        }

        Vst::ProcessData data {};
        prepareProcessData (data, audioBuffer.getNumSamples(), Vst::kSample32);

        // Input busses
        Vst::AudioBusBuffers inputBus {};
        inputBus.numChannels = audioBuffer.getNumChannels();
        inputBus.channelBuffers32 = const_cast<float**> (audioBuffer.getArrayOfReadPointers());
        data.inputs = &inputBus;
        data.numInputs = 1;

        // Output busses
        Vst::AudioBusBuffers outputBus {};
        outputBus.numChannels = audioBuffer.getNumChannels();
        outputBus.channelBuffers32 = const_cast<float**> (audioBuffer.getArrayOfWritePointers());
        data.outputs = &outputBus;
        data.numOutputs = 1;

        ignoreUnused (midiBuffer);

        // TODO: map MidiBuffer to Vst::EventList when MIDI support is added in a later task

        vst3Processor->process (data);
    }

    void processBlock (AudioBuffer<double>& audioBuffer, MidiBuffer& midiBuffer) override
    {
        ScopedNoDenormals noDenormals;

        if (! isUsingDoublePrecision())
        {
            jassertfalse;
            audioBuffer.clear();
            midiBuffer.clear();
            return;
        }

        Vst::ProcessData data {};
        prepareProcessData (data, audioBuffer.getNumSamples(), Vst::kSample64);

        Vst::AudioBusBuffers inputBus {};
        inputBus.numChannels = audioBuffer.getNumChannels();
        inputBus.channelBuffers64 = const_cast<double**> (audioBuffer.getArrayOfReadPointers());
        data.inputs = &inputBus;
        data.numInputs = 1;

        Vst::AudioBusBuffers outputBus {};
        outputBus.numChannels = audioBuffer.getNumChannels();
        outputBus.channelBuffers64 = const_cast<double**> (audioBuffer.getArrayOfWritePointers());
        data.outputs = &outputBus;
        data.numOutputs = 1;

        ignoreUnused (midiBuffer);

        // TODO: map MidiBuffer to Vst::EventList when MIDI support is added in a later task

        vst3Processor->process (data);
    }

    bool supportsDoublePrecisionProcessing() const override
    {
        return vst3Processor != nullptr && vst3Processor->canProcessSampleSize (Vst::kSample64) == kResultTrue;
    }

    //==============================================================================

    int getCurrentPreset() const noexcept override { return currentPreset; }

    void setCurrentPreset (int index) noexcept override
    {
        if (vst3Controller != nullptr)
        {
            // VST3 presets are loaded externally; track index only
            currentPreset = index;
        }
    }

    int getNumPresets() const override { return numPresets; }

    String getPresetName (int index) const override
    {
        if (index < 0 || index >= numPresets)
            return {};

        return "Preset " + String (index);
    }

    void setPresetName (int, StringRef) override {}

    //==============================================================================

    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
    {
        if (vst3Component == nullptr)
            return Result::fail ("Plugin not loaded");

        MemoryBlock mutable_copy = memoryBlock;
        IBStream* stream = new MemoryStream (mutable_copy.getData(),
                                             static_cast<TSize> (mutable_copy.getSize()));

        const auto res = vst3Component->setState (stream);
        stream->release();

        if (res != kResultOk)
            return Result::fail ("VST3 setState() failed");

        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
    {
        if (vst3Component == nullptr)
            return Result::fail ("Plugin not loaded");

        MemoryStream* stream = new MemoryStream();
        const auto res = vst3Component->getState (stream);

        if (res != kResultOk)
        {
            stream->release();
            return Result::fail ("VST3 getState() failed");
        }

        memoryBlock.replaceAll (stream->getData(), static_cast<std::size_t> (stream->getSize()));
        stream->release();
        return Result::ok();
    }

    //==============================================================================

    bool hasEditor() const override { return false; }

    //==============================================================================

    static std::unique_ptr<VST3Instance> create (const AudioPluginDescription& desc,
                                                 const AudioPluginHostContext& context)
    {
        auto mod = VST3Module::load (File (desc.fileOrBundlePath));
        if (mod == nullptr)
            return nullptr;

        IPluginFactory* rawFactory = mod->getFactory();
        if (rawFactory == nullptr)
            return nullptr;

        IPtr<IPluginFactory> factory (rawFactory);

        // Find the component class matching identifier
        const int classCount = factory->countClasses();
        for (int i = 0; i < classCount; ++i)
        {
            PClassInfo classInfo;
            if (factory->getClassInfo (i, &classInfo) != kResultOk)
                continue;

            if (String (classInfo.category) != "Audio Module Class")
                continue;

            if (desc.identifier.isNotEmpty() && ! classIDToString (classInfo.cid).equalsIgnoreCase (desc.identifier))
                continue;

            if (desc.identifier.isEmpty() && String (classInfo.name) != desc.name)
                continue;

            Vst::IComponent* rawComponent = nullptr;
            if (factory->createInstance (classInfo.cid, Vst::IComponent::iid, reinterpret_cast<void**> (&rawComponent)) != kResultOk)
                continue;

            IPtr<Vst::IComponent> component = IPtr<Vst::IComponent>::adopt (rawComponent);

            IPtr<Vst::IHostApplication> host;
            if (component->initialize (host) != kResultOk)
                continue;

            Vst::IAudioProcessor* rawProcessor = nullptr;
            if (component->queryInterface (Vst::IAudioProcessor::iid,
                                           reinterpret_cast<void**> (&rawProcessor))
                != kResultOk)
                continue;
            IPtr<Vst::IAudioProcessor> processor = IPtr<Vst::IAudioProcessor>::adopt (rawProcessor);

            Vst::IEditController* rawController = nullptr;
            component->queryInterface (Vst::IEditController::iid,
                                       reinterpret_cast<void**> (&rawController));
            IPtr<Vst::IEditController> controller = IPtr<Vst::IEditController>::adopt (rawController);

            return std::make_unique<VST3Instance> (desc, context, std::move (mod), std::move (component), std::move (processor), std::move (controller));
        }

        return nullptr;
    }

private:
    static AudioBusLayout buildBusLayout (Vst::IComponent* component)
    {
        std::vector<AudioBus> inputs, outputs;

        const int numInputs = component->getBusCount (Vst::kAudio, Vst::kInput);
        for (int i = 0; i < numInputs; ++i)
        {
            Vst::BusInfo info;
            component->getBusInfo (Vst::kAudio, Vst::kInput, i, info);
            inputs.emplace_back (toString (info.name), AudioBus::Type::Audio, AudioBus::Direction::Input, static_cast<int> (info.channelCount));
        }

        const int numOutputs = component->getBusCount (Vst::kAudio, Vst::kOutput);
        for (int i = 0; i < numOutputs; ++i)
        {
            Vst::BusInfo info;
            component->getBusInfo (Vst::kAudio, Vst::kOutput, i, info);
            outputs.emplace_back (toString (info.name), AudioBus::Type::Audio, AudioBus::Direction::Output, static_cast<int> (info.channelCount));
        }

        return AudioBusLayout (std::move (inputs), std::move (outputs));
    }

    void buildParameterList()
    {
        if (vst3Controller == nullptr)
            return;

        const int count = vst3Controller->getParameterCount();
        numPresets = 0;

        for (int i = 0; i < count; ++i)
        {
            Vst::ParameterInfo info;
            vst3Controller->getParameterInfo (i, info);

            if (info.flags & Vst::ParameterInfo::kIsProgramChange)
            {
                numPresets = static_cast<int> (info.stepCount) + 1;
                continue;
            }

            auto param = AudioParameterBuilder()
                             .withID (String (static_cast<int64> (info.id)))
                             .withName (toString (info.title))
                             .withRange (0.0f, 1.0f)
                             .withDefault (static_cast<float> (info.defaultNormalizedValue))
                             .build();

            vst3ParameterIds.push_back (info.id);
            addParameter (std::move (param));
        }

        inputParameterChanges.setMaxParameters (static_cast<int32> (vst3ParameterIds.size()));
    }

    AudioPluginHostContext hostContext;
    std::unique_ptr<VST3Module> vst3Module;
    IPtr<Vst::IComponent> vst3Component;
    IPtr<Vst::IAudioProcessor> vst3Processor;
    IPtr<Vst::IEditController> vst3Controller;
    Vst::ProcessContext vst3ProcessContext {};
    Vst::ParameterChanges inputParameterChanges;
    AudioBuffer<double> doublePrecisionBuffer;
    std::vector<Vst::ParamID> vst3ParameterIds;
    int currentPreset = 0;
    int numPresets = 0;

    void prepareProcessData (Vst::ProcessData& data, int numSamples, int32 symbolicSampleSize)
    {
        data.processMode = Vst::kRealtime;
        data.symbolicSampleSize = symbolicSampleSize;
        data.numSamples = numSamples;

        inputParameterChanges.clearQueue();
        const auto params = getParameters();
        const auto numParams = yup::jmin (params.size(), vst3ParameterIds.size());

        for (std::size_t i = 0; i < numParams; ++i)
        {
            int32 queueIndex = 0;
            if (auto* queue = inputParameterChanges.addParameterData (vst3ParameterIds[i], queueIndex))
            {
                int32 pointIndex = 0;
                queue->addPoint (0,
                                 static_cast<Vst::ParamValue> (params[i]->getValue()),
                                 pointIndex);
            }
        }

        data.inputParameterChanges = &inputParameterChanges;

        if (hostContext.playHead == nullptr)
            return;

        const auto optPos = hostContext.playHead->getPosition();
        if (! optPos.has_value())
            return;

        const auto& posInfo = optPos.value();
        vst3ProcessContext = {};
        vst3ProcessContext.state = Vst::ProcessContext::kPlaying;
        vst3ProcessContext.sampleRate = getSampleRate();

        if (auto timeSamples = posInfo.getTimeInSamples())
            vst3ProcessContext.projectTimeSamples = *timeSamples;

        if (auto tempo = posInfo.getBpm())
            vst3ProcessContext.tempo = *tempo;

        data.processContext = &vst3ProcessContext;
    }
};

//==============================================================================

VST3Format::VST3Format() = default;
VST3Format::~VST3Format() = default;

AudioPluginFormatType VST3Format::getFormatType() const
{
    return AudioPluginFormatType::vst3;
}

String VST3Format::getFormatName() const
{
    return "VST3";
}

FileSearchPath VST3Format::getDefaultSearchPaths() const
{
    FileSearchPath paths;

#if YUP_MAC
    paths.add (File ("/Library/Audio/Plug-Ins/VST3"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory)
                   .getChildFile ("Library/Audio/Plug-Ins/VST3"));
#elif YUP_WINDOWS
    // %CommonProgramFiles%\VST3
    if (const char* pf = getenv ("CommonProgramFiles"))
        paths.add (File (String (pf) + "\\VST3"));
    // %APPDATA%\VST3
    if (const char* appdata = getenv ("APPDATA"))
        paths.add (File (String (appdata) + "\\VST3"));
#elif YUP_LINUX
    paths.add (File ("/usr/lib/vst3"));
    paths.add (File ("/usr/local/lib/vst3"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile (".vst3"));
#endif

    return paths;
}

ResultValue<std::vector<AudioPluginDescription>> VST3Format::scanFile (const File& file)
{
    if (file.getFileExtension().toLowerCase() != ".vst3"
        && ! file.isDirectory())
        return makeResultValueFail ("Not a VST3 file");

    auto mod = VST3Module::load (file);
    if (mod == nullptr)
        return makeResultValueFail ("Failed to load VST3 module: " + file.getFullPathName());

    IPluginFactory* rawFactory = mod->getFactory();
    if (rawFactory == nullptr)
        return makeResultValueFail ("No factory in " + file.getFullPathName());

    IPtr<IPluginFactory> factory (rawFactory);

    std::vector<AudioPluginDescription> results;
    const int classCount = factory->countClasses();

    for (int i = 0; i < classCount; ++i)
    {
        PClassInfo2 info2 {};
        if (auto* factory2 = FUnknownPtr<IPluginFactory2> (factory).getInterface())
        {
            if (factory2->getClassInfo2 (i, &info2) != kResultOk)
                continue;
        }
        else
        {
            PClassInfo info;
            if (factory->getClassInfo (i, &info) != kResultOk)
                continue;

            std::memcpy (info2.cid, info.cid, sizeof (TUID));
            std::memcpy (info2.name, info.name, PClassInfo::kNameSize);
            std::memcpy (info2.category, info.category, PClassInfo::kCategorySize);
            info2.vendor[0] = '\0';
            info2.version[0] = '\0';
            info2.subCategories[0] = '\0';
        }

        if (String (info2.category) != "Audio Module Class")
            continue;

        AudioPluginDescription desc;
        desc.formatType = AudioPluginFormatType::vst3;
        desc.fileOrBundlePath = file.getFullPathName();
        desc.name = String (info2.name);
        desc.vendor = String (info2.vendor);
        desc.version = String (info2.version);
        desc.category = String (info2.subCategories);
        desc.isInstrument = String (info2.subCategories).containsIgnoreCase ("Instrument");
        desc.isEffect = ! desc.isInstrument;

        desc.identifier = classIDToString (info2.cid);

        // Briefly instantiate the component to collect channel counts
        Vst::IComponent* rawComponent = nullptr;
        if (factory->createInstance (info2.cid, Vst::IComponent::iid, reinterpret_cast<void**> (&rawComponent)) == kResultOk)
        {
            IPtr<Vst::IComponent> component = IPtr<Vst::IComponent>::adopt (rawComponent);
            IPtr<Vst::IHostApplication> host;

            if (component->initialize (host) == kResultOk)
            {
                const int numInputs = component->getBusCount (Vst::kAudio, Vst::kInput);
                for (int b = 0; b < numInputs; ++b)
                {
                    Vst::BusInfo busInfo;
                    if (component->getBusInfo (Vst::kAudio, Vst::kInput, b, busInfo) == kResultOk)
                        desc.numInputChannels += static_cast<int> (busInfo.channelCount);
                }

                const int numOutputs = component->getBusCount (Vst::kAudio, Vst::kOutput);
                for (int b = 0; b < numOutputs; ++b)
                {
                    Vst::BusInfo busInfo;
                    if (component->getBusInfo (Vst::kAudio, Vst::kOutput, b, busInfo) == kResultOk)
                        desc.numOutputChannels += static_cast<int> (busInfo.channelCount);
                }

                component->terminate();
            }
        }

        results.push_back (std::move (desc));
    }

    if (results.empty())
        return makeResultValueFail ("No Audio Module Class entries in " + file.getFullPathName());

    return makeResultValueOk (std::move (results));
}

ResultValue<std::unique_ptr<AudioPluginInstance>> VST3Format::loadPlugin (
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    auto instance = VST3Instance::create (description, context);

    if (instance == nullptr)
        return makeResultValueFail ("Failed to load VST3 plugin: " + description.name);

    return makeResultValueOk (std::move (instance));
}

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
