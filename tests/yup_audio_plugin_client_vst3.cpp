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

#include <gtest/gtest.h>

// =============================================================================
#define YUP_AUDIO_PLUGIN_ENABLE_VST3 1
#define YupPlugin_Id "test.vst3.plugin"
#define YupPlugin_Name "Test VST3 Plugin"
#define YupPlugin_Vendor "TestVendor"
#define YupPlugin_Version "1.0.0"
#define YupPlugin_URL "https://test.example"
#define YupPlugin_Email "test@example.com"
#define YupPlugin_IsSynth 0

// =============================================================================
#include <yup_audio_processors/yup_audio_processors.h>
#include "yup_audio_plugin_client/yup_TestPluginProcessor.h"

#define YUP_AUDIO_PLUGIN_CREATE_FUNCTION createPluginProcessorVST3
#include "yup_audio_plugin_client/vst3/yup_audio_plugin_client_VST3.cpp"

extern "C" yup::AudioProcessor* createPluginProcessorVST3()
{
    return new TestPluginProcessor (testPluginBusLayoutWithInactiveSidechain());
}

// =============================================================================
namespace
{

class MemoryStream : public Steinberg::IBStream
{
public:
    MemoryStream() = default;

    // --- FUnknown ---
    tresult PLUGIN_API queryInterface (const Steinberg::TUID, void**) override { return Steinberg::kNoInterface; }

    uint32 PLUGIN_API addRef() override { return 1; }

    uint32 PLUGIN_API release() override { return 1; }

    // --- IBStream ---
    tresult PLUGIN_API read (void* buffer, int32 numBytes, int32* numBytesRead) override
    {
        if (buffer == nullptr || numBytes < 0)
            return Steinberg::kInvalidArgument;

        const auto bytesToRead = std::min (static_cast<size_t> (numBytes), data.getSize() - readPos);
        if (bytesToRead > 0)
            std::memcpy (buffer, static_cast<const char*> (data.getData()) + readPos, bytesToRead);

        readPos += bytesToRead;

        if (numBytesRead != nullptr)
            *numBytesRead = static_cast<int32> (bytesToRead);

        return Steinberg::kResultOk;
    }

    tresult PLUGIN_API write (void* buffer, int32 numBytes, int32* numBytesWritten) override
    {
        if (buffer == nullptr || numBytes < 0)
            return Steinberg::kInvalidArgument;

        data.append (buffer, static_cast<size_t> (numBytes));

        if (numBytesWritten != nullptr)
            *numBytesWritten = numBytes;

        return Steinberg::kResultOk;
    }

    tresult PLUGIN_API seek (int64 pos, int32 mode, int64* result) override
    {
        int64 newPos = static_cast<int64> (readPos);

        switch (mode)
        {
            case kIBSeekSet:
                newPos = pos;
                break;
            case kIBSeekCur:
                newPos = static_cast<int64> (readPos) + pos;
                break;
            case kIBSeekEnd:
                newPos = static_cast<int64> (data.getSize()) + pos;
                break;
            default:
                return Steinberg::kInvalidArgument;
        }

        if (newPos < 0 || static_cast<size_t> (newPos) > data.getSize())
            return Steinberg::kInvalidArgument;

        readPos = static_cast<size_t> (newPos);

        if (result != nullptr)
            *result = newPos;

        return Steinberg::kResultOk;
    }

    tresult PLUGIN_API tell (int64* pos) override
    {
        if (pos == nullptr)
            return Steinberg::kInvalidArgument;

        *pos = static_cast<int64> (readPos);
        return Steinberg::kResultOk;
    }

    yup::MemoryBlock getData() const { return data; }

private:
    yup::MemoryBlock data;
    size_t readPos = 0;
};

// Minimal IParamValueQueue delivering a single value for one parameter
class SingleParamValueQueue final : public Steinberg::Vst::IParamValueQueue
{
public:
    SingleParamValueQueue (Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value)
        : paramId (id)
        , paramValue (value)
    {
    }

    tresult PLUGIN_API queryInterface (const Steinberg::TUID, void**) override { return Steinberg::kNoInterface; }

    uint32 PLUGIN_API addRef() override { return 1; }

    uint32 PLUGIN_API release() override { return 1; }

    Steinberg::Vst::ParamID PLUGIN_API getParameterId() override { return paramId; }

    int32 PLUGIN_API getPointCount() override { return 1; }

    tresult PLUGIN_API getPoint (int32 index, int32& sampleOffset, Steinberg::Vst::ParamValue& value) override
    {
        if (index != 0)
            return Steinberg::kResultFalse;

        sampleOffset = 0;
        value = paramValue;
        return Steinberg::kResultOk;
    }

    tresult PLUGIN_API addPoint (int32, Steinberg::Vst::ParamValue, int32&) override { return Steinberg::kResultFalse; }

private:
    Steinberg::Vst::ParamID paramId = 0;
    Steinberg::Vst::ParamValue paramValue = 0.0;
};

// Minimal IParameterChanges holding a single parameter value queue
class SingleParameterChanges final : public Steinberg::Vst::IParameterChanges
{
public:
    explicit SingleParameterChanges (SingleParamValueQueue& queue)
        : queueRef (queue)
    {
    }

    tresult PLUGIN_API queryInterface (const Steinberg::TUID, void**) override { return Steinberg::kNoInterface; }

    uint32 PLUGIN_API addRef() override { return 1; }

    uint32 PLUGIN_API release() override { return 1; }

    int32 PLUGIN_API getParameterCount() override { return 1; }

    Steinberg::Vst::IParamValueQueue* PLUGIN_API getParameterData (int32 index) override
    {
        return index == 0 ? &queueRef : nullptr;
    }

    Steinberg::Vst::IParamValueQueue* PLUGIN_API addParameterData (const Steinberg::Vst::ParamID&, int32&) override
    {
        return nullptr;
    }

private:
    SingleParamValueQueue& queueRef;
};

} // namespace

// =============================================================================
// Tests
// =============================================================================

using namespace yup;

//------------------------------------------------------------------------------
// Factory tests
//------------------------------------------------------------------------------

TEST (VST3WrapperTest, GetPluginFactoryReturnsNonNull)
{
    auto* factory = GetPluginFactory();
    ASSERT_NE (nullptr, factory);
}

TEST (VST3WrapperTest, FactoryHasClasses)
{
    auto* factory = GetPluginFactory();
    ASSERT_NE (nullptr, factory);

    const auto count = factory->countClasses();
    EXPECT_GE (count, 2);

    for (int32 i = 0; i < count; ++i)
    {
        Steinberg::PClassInfo info {};
        EXPECT_EQ (Steinberg::kResultOk, factory->getClassInfo (i, &info));
    }
}

TEST (VST3WrapperTest, CreateProcessorInstance)
{
    auto* factory = GetPluginFactory();
    ASSERT_NE (nullptr, factory);

    Steinberg::FUnknown* component = nullptr;
    const auto count = factory->countClasses();

    for (int32 i = 0; i < count; ++i)
    {
        Steinberg::PClassInfo info {};
        ASSERT_EQ (Steinberg::kResultOk, factory->getClassInfo (i, &info));

        if (std::strcmp (info.category, kVstAudioEffectClass) == 0)
        {
            EXPECT_EQ (Steinberg::kResultOk, factory->createInstance (info.cid, Steinberg::Vst::IComponent::iid, (void**) &component));
            break;
        }
    }

    ASSERT_NE (nullptr, component);

    Steinberg::Vst::IAudioProcessor* audioProc = nullptr;
    EXPECT_EQ (Steinberg::kResultOk, component->queryInterface (Steinberg::Vst::IAudioProcessor::iid, (void**) &audioProc));
    ASSERT_NE (nullptr, audioProc);

    audioProc->release();
    component->release();
}

TEST (VST3WrapperTest, CreateControllerInstance)
{
    auto* factory = GetPluginFactory();
    ASSERT_NE (nullptr, factory);

    Steinberg::FUnknown* controller = nullptr;
    const auto count = factory->countClasses();

    for (int32 i = 0; i < count; ++i)
    {
        Steinberg::PClassInfo info {};
        ASSERT_EQ (Steinberg::kResultOk, factory->getClassInfo (i, &info));

        if (std::strcmp (info.category, kVstComponentControllerClass) == 0)
        {
            EXPECT_EQ (Steinberg::kResultOk, factory->createInstance (info.cid, Steinberg::Vst::IEditController::iid, (void**) &controller));
            break;
        }
    }

    ASSERT_NE (nullptr, controller);
    controller->release();
}

//------------------------------------------------------------------------------
// Processor tests
//------------------------------------------------------------------------------

class VST3ProcessorTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        factory = GetPluginFactory();
        ASSERT_NE (nullptr, factory);

        const auto count = factory->countClasses();
        for (int32 i = 0; i < count; ++i)
        {
            Steinberg::PClassInfo info {};
            ASSERT_EQ (Steinberg::kResultOk, factory->getClassInfo (i, &info));

            if (std::strcmp (info.category, kVstAudioEffectClass) == 0)
            {
                std::memcpy (processorUID, info.cid, sizeof (Steinberg::TUID));
                ASSERT_EQ (Steinberg::kResultOk,
                           factory->createInstance (info.cid,
                                                    Steinberg::Vst::IComponent::iid,
                                                    (void**) &component));

                ASSERT_EQ (Steinberg::kResultOk,
                           component->queryInterface (Steinberg::Vst::IAudioProcessor::iid,
                                                      (void**) &audioProcessor));
            }
            else if (std::strcmp (info.category, kVstComponentControllerClass) == 0)
            {
                std::memcpy (controllerUID, info.cid, sizeof (Steinberg::TUID));
            }
        }

        ASSERT_NE (nullptr, component);
        ASSERT_NE (nullptr, audioProcessor);
    }

    void TearDown() override
    {
        if (audioProcessor != nullptr)
            audioProcessor->release();

        if (component != nullptr)
            component->release();
    }

    Steinberg::IPluginFactory* factory = nullptr;
    Steinberg::TUID processorUID {};
    Steinberg::TUID controllerUID {};
    Steinberg::Vst::IComponent* component = nullptr;
    Steinberg::Vst::IAudioProcessor* audioProcessor = nullptr;
};

TEST_F (VST3ProcessorTests, InitializeSucceeds)
{
    EXPECT_EQ (Steinberg::kResultOk, component->initialize (nullptr));
}

TEST_F (VST3ProcessorTests, CanProcessSinglePrecision)
{
    EXPECT_EQ (Steinberg::kResultTrue, audioProcessor->canProcessSampleSize (Steinberg::Vst::kSample32));
}

TEST_F (VST3ProcessorTests, CannotProcessDoublePrecisionByDefault)
{
    EXPECT_NE (Steinberg::kResultTrue, audioProcessor->canProcessSampleSize (Steinberg::Vst::kSample64));
}

TEST_F (VST3ProcessorTests, BusActivationWorks)
{
    // Buses are created in initialize() per VST3 convention
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

    const auto numInputs = component->getBusCount (Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    EXPECT_GE (numInputs, 1);

    const auto numOutputs = component->getBusCount (Steinberg::Vst::kAudio, Steinberg::Vst::kOutput);
    EXPECT_GE (numOutputs, 1);

    for (int32 i = 0; i < numInputs; ++i)
    {
        Steinberg::Vst::BusInfo info {};
        EXPECT_EQ (Steinberg::kResultOk,
                   component->getBusInfo (Steinberg::Vst::kAudio, Steinberg::Vst::kInput, i, info));
    }

    for (int32 i = 0; i < numOutputs; ++i)
    {
        Steinberg::Vst::BusInfo info {};
        EXPECT_EQ (Steinberg::kResultOk,
                   component->getBusInfo (Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, i, info));
    }

    // Verify activation cycle: setupProcessing → setActive(true) → setActive(false)
    Steinberg::Vst::ProcessSetup setup {};
    setup.sampleRate = 44100.0;
    setup.maxSamplesPerBlock = 512;
    setup.processMode = Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize = Steinberg::Vst::kSample32;
    ASSERT_EQ (Steinberg::kResultOk, audioProcessor->setupProcessing (setup));

    EXPECT_EQ (Steinberg::kResultOk, component->setActive (true));
    EXPECT_EQ (Steinberg::kResultOk, component->setActive (false));
}

TEST_F (VST3ProcessorTests, GetControllerClassID)
{
    Steinberg::TUID cid {};
    const auto result = component->getControllerClassId (cid);
    if (result == Steinberg::kResultOk)
    {
        EXPECT_EQ (0, std::memcmp (cid, controllerUID, sizeof (Steinberg::TUID)));
    }
}

//------------------------------------------------------------------------------
// State save/load tests
//------------------------------------------------------------------------------

TEST_F (VST3ProcessorTests, StateSaveAndLoadRoundTrip)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

    MemoryStream saveStream;
    EXPECT_EQ (Steinberg::kResultOk, component->getState (&saveStream));

    auto savedData = saveStream.getData();
    EXPECT_GT (savedData.getSize(), 0u);

    MemoryStream loadStream;
    int32 written = 0;
    loadStream.write (const_cast<void*> (savedData.getData()),
                      static_cast<int32> (savedData.getSize()),
                      &written);
    ASSERT_EQ (static_cast<int32> (savedData.getSize()), written);

    loadStream.seek (0, MemoryStream::kIBSeekSet, nullptr);

    EXPECT_EQ (Steinberg::kResultOk, component->setState (&loadStream));
}

//------------------------------------------------------------------------------
// EditController tests
//------------------------------------------------------------------------------

class VST3ControllerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        factory = GetPluginFactory();
        ASSERT_NE (nullptr, factory);

        const auto count = factory->countClasses();
        for (int32 i = 0; i < count; ++i)
        {
            Steinberg::PClassInfo info {};
            ASSERT_EQ (Steinberg::kResultOk, factory->getClassInfo (i, &info));

            if (std::strcmp (info.category, kVstAudioEffectClass) == 0)
            {
                ASSERT_EQ (Steinberg::kResultOk,
                           factory->createInstance (info.cid,
                                                    Steinberg::Vst::IComponent::iid,
                                                    (void**) &component));
            }
            else if (std::strcmp (info.category, kVstComponentControllerClass) == 0)
            {
                ASSERT_EQ (Steinberg::kResultOk,
                           factory->createInstance (info.cid,
                                                    Steinberg::Vst::IEditController::iid,
                                                    (void**) &controller));
            }
        }

        ASSERT_NE (nullptr, component);
        ASSERT_NE (nullptr, controller);
    }

    void TearDown() override
    {
        if (component != nullptr)
            component->release();

        if (controller != nullptr)
            controller->release();
    }

    Steinberg::IPluginFactory* factory = nullptr;
    Steinberg::Vst::IComponent* component = nullptr;
    Steinberg::Vst::IEditController* controller = nullptr;
};

TEST_F (VST3ControllerTests, InitializeSucceeds)
{
    EXPECT_EQ (Steinberg::kResultOk, controller->initialize (nullptr));
}

TEST_F (VST3ControllerTests, ParameterCountIsExpected)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));
    ASSERT_EQ (Steinberg::kResultOk, controller->initialize (nullptr));

    // Before the processor connects via IMessage, the controller has 0 parameters
    EXPECT_EQ (0, controller->getParameterCount());
}

TEST_F (VST3ControllerTests, TerminateCleansUp)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));
    ASSERT_EQ (Steinberg::kResultOk, controller->initialize (nullptr));

    EXPECT_EQ (Steinberg::kResultOk, controller->terminate());
    EXPECT_EQ (Steinberg::kResultOk, component->terminate());
}

//------------------------------------------------------------------------------
// Sidechain bus tests (AudioBus::Role → VST3 BusInfo)
//------------------------------------------------------------------------------

class VST3SidechainBusTests : public VST3ProcessorTests
{
};

TEST_F (VST3SidechainBusTests, MainBusReportsMainType)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

    const auto numInputs = component->getBusCount (Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    ASSERT_GE (numInputs, 1);

    // The first audio bus should be the main input (kMain)
    Steinberg::Vst::BusInfo info {};
    ASSERT_EQ (Steinberg::kResultOk,
               component->getBusInfo (Steinberg::Vst::kAudio, Steinberg::Vst::kInput, 0, info));
    EXPECT_EQ (Steinberg::Vst::kMain, info.busType);
}

TEST_F (VST3SidechainBusTests, SidechainBusReportsAuxiliaryType)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

    const auto numInputs = component->getBusCount (Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    ASSERT_GE (numInputs, 2);

    // The second audio bus should be the auxiliary sidechain input (kAux)
    Steinberg::Vst::BusInfo info {};
    ASSERT_EQ (Steinberg::kResultOk,
               component->getBusInfo (Steinberg::Vst::kAudio, Steinberg::Vst::kInput, 1, info));
    EXPECT_EQ (Steinberg::Vst::kAux, info.busType);
}

TEST_F (VST3SidechainBusTests, MainBusHasDefaultActiveFlag)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

    const auto numInputs = component->getBusCount (Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    ASSERT_GE (numInputs, 1);

    Steinberg::Vst::BusInfo info {};
    ASSERT_EQ (Steinberg::kResultOk,
               component->getBusInfo (Steinberg::Vst::kAudio, Steinberg::Vst::kInput, 0, info));
    EXPECT_NE (0u, info.flags & Steinberg::Vst::BusInfo::kDefaultActive);
}

TEST_F (VST3SidechainBusTests, InactiveSidechainBusLacksDefaultActiveFlag)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

    const auto numInputs = component->getBusCount (Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    ASSERT_GE (numInputs, 2);

    Steinberg::Vst::BusInfo info {};
    ASSERT_EQ (Steinberg::kResultOk,
               component->getBusInfo (Steinberg::Vst::kAudio, Steinberg::Vst::kInput, 1, info));
    EXPECT_EQ (0u, info.flags & Steinberg::Vst::BusInfo::kDefaultActive);
}

TEST_F (VST3SidechainBusTests, MainOutputBusReportsMainType)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

    const auto numOutputs = component->getBusCount (Steinberg::Vst::kAudio, Steinberg::Vst::kOutput);
    ASSERT_GE (numOutputs, 1);

    Steinberg::Vst::BusInfo info {};
    ASSERT_EQ (Steinberg::kResultOk,
               component->getBusInfo (Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, 0, info));
    EXPECT_EQ (Steinberg::Vst::kMain, info.busType);
}

TEST_F (VST3SidechainBusTests, SidechainInputBusNameIsExposed)
{
    ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

    const auto numInputs = component->getBusCount (Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    ASSERT_GE (numInputs, 2);

    Steinberg::Vst::BusInfo info {};
    ASSERT_EQ (Steinberg::kResultOk,
               component->getBusInfo (Steinberg::Vst::kAudio, Steinberg::Vst::kInput, 1, info));

    // The bus name should be non-empty
    EXPECT_NE (0, info.name[0]);
}

//------------------------------------------------------------------------------
// Bypass tests
//------------------------------------------------------------------------------

class VST3BypassTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        factory = GetPluginFactory();
        ASSERT_NE (nullptr, factory);

        const auto count = factory->countClasses();
        for (int32 i = 0; i < count; ++i)
        {
            Steinberg::PClassInfo info {};
            ASSERT_EQ (Steinberg::kResultOk, factory->getClassInfo (i, &info));

            if (std::strcmp (info.category, kVstAudioEffectClass) == 0)
            {
                ASSERT_EQ (Steinberg::kResultOk,
                           factory->createInstance (info.cid,
                                                    Steinberg::Vst::IComponent::iid,
                                                    (void**) &component));
                break;
            }
        }

        ASSERT_NE (nullptr, component);
        processor = static_cast<TestPluginProcessor*> (static_cast<AudioPluginProcessorVST3*> (component)->getProcessor());
        ASSERT_NE (nullptr, processor);

        ASSERT_EQ (Steinberg::kResultOk,
                   component->queryInterface (Steinberg::Vst::IAudioProcessor::iid,
                                              (void**) &audioProcessor));
        ASSERT_NE (nullptr, audioProcessor);

        ASSERT_EQ (Steinberg::kResultOk, component->initialize (nullptr));

        Steinberg::Vst::ProcessSetup setup {};
        setup.sampleRate = 44100.0;
        setup.maxSamplesPerBlock = 512;
        setup.processMode = Steinberg::Vst::kRealtime;
        setup.symbolicSampleSize = Steinberg::Vst::kSample32;
        ASSERT_EQ (Steinberg::kResultOk, audioProcessor->setupProcessing (setup));

        ASSERT_EQ (Steinberg::kResultOk, component->setActive (true));
    }

    void TearDown() override
    {
        if (component != nullptr)
            component->setActive (false);

        if (audioProcessor != nullptr)
            audioProcessor->release();

        if (component != nullptr)
            component->release();
    }

    Steinberg::Vst::ParamID getBypassParameterID() const
    {
        return static_cast<Steinberg::Vst::ParamID> (getVST3BypassParameterID (*processor));
    }

    // Runs one process block with a single parameter change on the bypass tag
    void processWithBypassValue (Steinberg::Vst::ParamValue value)
    {
        SingleParamValueQueue queue (getBypassParameterID(), value);
        SingleParameterChanges changes (queue);

        float inputMainData[2][64] = {};
        float* inputMainChannels[2] = { inputMainData[0], inputMainData[1] };

        Steinberg::Vst::AudioBusBuffers inputMain {};
        inputMain.numChannels = 2;
        inputMain.channelBuffers32 = inputMainChannels;

        float inputSidechainData[1][64] = {};
        float* inputSidechainChannels[1] = { inputSidechainData[0] };

        Steinberg::Vst::AudioBusBuffers inputSidechain {};
        inputSidechain.numChannels = 1;
        inputSidechain.channelBuffers32 = inputSidechainChannels;

        float outputData[2][64] = {};
        float* outputChannels[2] = { outputData[0], outputData[1] };

        Steinberg::Vst::AudioBusBuffers outputBus {};
        outputBus.numChannels = 2;
        outputBus.channelBuffers32 = outputChannels;

        Steinberg::Vst::AudioBusBuffers inputs[] = { inputMain, inputSidechain };
        Steinberg::Vst::AudioBusBuffers outputs[] = { outputBus };

        Steinberg::Vst::ProcessData data {};
        data.processMode = Steinberg::Vst::kRealtime;
        data.symbolicSampleSize = Steinberg::Vst::kSample32;
        data.numSamples = 64;
        data.numInputs = 2;
        data.numOutputs = 1;
        data.inputs = inputs;
        data.outputs = outputs;
        data.inputParameterChanges = &changes;

        EXPECT_EQ (Steinberg::kResultOk, audioProcessor->process (data));
    }

    // Runs one process block without any parameter changes
    void processBlock()
    {
        float outputData[2][64] = {};
        float* outputChannels[2] = { outputData[0], outputData[1] };

        Steinberg::Vst::AudioBusBuffers outputBus {};
        outputBus.numChannels = 2;
        outputBus.channelBuffers32 = outputChannels;

        Steinberg::Vst::AudioBusBuffers outputs[] = { outputBus };

        Steinberg::Vst::ProcessData data {};
        data.processMode = Steinberg::Vst::kRealtime;
        data.symbolicSampleSize = Steinberg::Vst::kSample32;
        data.numSamples = 64;
        data.numOutputs = 1;
        data.outputs = outputs;

        EXPECT_EQ (Steinberg::kResultOk, audioProcessor->process (data));
    }

    Steinberg::IPluginFactory* factory = nullptr;
    Steinberg::Vst::IComponent* component = nullptr;
    Steinberg::Vst::IAudioProcessor* audioProcessor = nullptr;
    TestPluginProcessor* processor = nullptr;
};

TEST_F (VST3BypassTests, BypassChangeRoutesToBypassedPath)
{
    processWithBypassValue (1.0);

    EXPECT_EQ (1, processor->bypassCallCount);
    EXPECT_EQ (0, processor->processCallCount);
}

TEST_F (VST3BypassTests, BypassChangeRoutesToProcessPath)
{
    processWithBypassValue (0.0);

    EXPECT_EQ (0, processor->bypassCallCount);
    EXPECT_EQ (1, processor->processCallCount);
}

TEST_F (VST3BypassTests, StateRoundTripsBypassState)
{
    processWithBypassValue (1.0);
    EXPECT_EQ (1, processor->bypassCallCount);

    MemoryStream saveStream;
    EXPECT_EQ (Steinberg::kResultOk, component->getState (&saveStream));

    const auto savedData = saveStream.getData();
    EXPECT_GT (savedData.getSize(), 0u);

    // Un-bypass so restoring the state actually proves persistence
    processWithBypassValue (0.0);
    EXPECT_EQ (1, processor->bypassCallCount);
    EXPECT_EQ (1, processor->processCallCount);

    MemoryStream loadStream;
    int32 written = 0;
    loadStream.write (const_cast<void*> (savedData.getData()),
                      static_cast<int32> (savedData.getSize()),
                      &written);
    ASSERT_EQ (static_cast<int32> (savedData.getSize()), written);

    loadStream.seek (0, MemoryStream::kIBSeekSet, nullptr);

    EXPECT_EQ (Steinberg::kResultOk, component->setState (&loadStream));

    // A block without parameter changes must now route to the bypass path
    processor->bypassCallCount = 0;
    processor->processCallCount = 0;

    processBlock();
    EXPECT_EQ (1, processor->bypassCallCount);
    EXPECT_EQ (0, processor->processCallCount);
}
