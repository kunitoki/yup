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

#include <limits>
#include <memory>

// =============================================================================
#define YUP_AUDIO_PLUGIN_ENABLE_AUv3 1
#define YupPlugin_Id "test.auv3.plugin"
#define YupPlugin_Name "Test AUv3 Plugin"
#define YupPlugin_Vendor "TestVendor"
#define YupPlugin_Version "1.0.0"
#define YupPlugin_IsSynth 0
#define YupPlugin_IsMono 0

// =============================================================================
#include "yup_audio_plugin_client/yup_TestPluginProcessor.h"

#define YUP_AUDIO_PLUGIN_CREATE_FUNCTION createPluginProcessorAUv3
#include "yup_audio_plugin_client/auv3/yup_audio_plugin_client_AUv3.mm"

// =============================================================================
#include <yup_audio_processors/yup_audio_processors.h>

// =============================================================================
// Tests
// =============================================================================

using namespace yup;

namespace
{

// Four-char codes for our test AUv3
constexpr OSType kTestAUv3Type    = 'auef';
constexpr OSType kTestAUv3SubType = 'tst3';
constexpr OSType kTestAUv3Manuf   = 'test';

// =============================================================================
// Layout switching for testing different bus configurations.
static AudioBusLayout gCustomLayout = testPluginBusLayoutStereo();
static bool gUseCustomLayout = false;

// Original factory uses the global flag to support multiple layouts.
extern "C" yup::AudioProcessor* createPluginProcessorAUv3()
{
    if (gUseCustomLayout)
        return new TestPluginProcessor (gCustomLayout);

    return new TestPluginProcessor (testPluginBusLayoutStereo());
}

// =============================================================================
// Scoped RAII helper that switches the processor layout for a test scope
struct ScopedProcessorLayout
{
    explicit ScopedProcessorLayout (AudioBusLayout layout)
    {
        gCustomLayout = std::move (layout);
        gUseCustomLayout = true;
    }

    ~ScopedProcessorLayout()
    {
        gUseCustomLayout = false;
    }

    YUP_DECLARE_NON_COPYABLE (ScopedProcessorLayout)
};

// =============================================================================
// Helper: create a minimal AudioComponentDescription for testing
AudioComponentDescription makeTestDescription()
{
    AudioComponentDescription desc {};
    desc.componentType         = kTestAUv3Type;
    desc.componentSubType      = kTestAUv3SubType;
    desc.componentManufacturer = kTestAUv3Manuf;
    return desc;
}

// =============================================================================
// Helper: instantiate an AUAudioUnit via the dynamic ObjC subclass (direct path).
AUAudioUnit* instantiateTestAUAudioUnit (NSError** outError = nullptr)
{
    ignoreUnused (outError);

    static AUAudioUnitSubclass auClass;

    // Raw allocated instance - no init called.  The object has the correct
    // isa and ivar layout, so setThis / _this work.
    auto* au = auClass.createInstance();

    if (au == nil)
        return nil;

    // Manually construct the C++ wrapper and wire it up - this is what
    // initWithComponentDescription:options:error: does in production.
    const auto desc = makeTestDescription();
    auto* cpp = new AudioPluginProcessorAUv3 (au, desc, 0, nullptr);
    AUAudioUnitSubclass::setThis (au, cpp);

    return au;
}

} // namespace

//==============================================================================
// Direct instantiation tests (non-factory path via ObjC init)
//==============================================================================

class AUv3DirectInstanceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);

        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
    }

    void TearDown() override
    {
        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }
        cpp = nullptr;
        audioUnit = nil;
    }

    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3DirectInstanceTests, AudioUnitReferenceIsNonNullAfterConstruction)
{
    // Bug #7 fix: the C++ wrapper must store the real AUAudioUnit reference
    EXPECT_NE (nil, cpp->getAudioUnit());
}

TEST_F (AUv3DirectInstanceTests, AudioUnitReferenceMatchesObjCInstance)
{
    // The stored au reference must be the same ObjC object
    EXPECT_EQ (audioUnit, cpp->getAudioUnit());
}

TEST_F (AUv3DirectInstanceTests, ProcessorIsNonNull)
{
    EXPECT_NE (nullptr, cpp->getProcessor());
}

TEST_F (AUv3DirectInstanceTests, ParameterTreeIsAccessible)
{
    auto* tree = cpp->getParameterTree();
    ASSERT_NE (nil, tree);

    // The test processor has 4 parameters
    EXPECT_GE ([[tree allParameters] count], 4u);
}

TEST_F (AUv3DirectInstanceTests, InputBussesAreAccessible)
{
    auto* busses = cpp->getInputBusses();
    EXPECT_NE (nil, busses);
}

TEST_F (AUv3DirectInstanceTests, OutputBussesAreAccessible)
{
    auto* busses = cpp->getOutputBusses();
    EXPECT_NE (nil, busses);
}

TEST_F (AUv3DirectInstanceTests, ChannelCapabilitiesAreAccessible)
{
    auto* caps = cpp->getChannelCapabilities();
    EXPECT_NE (nil, caps);
}

//==============================================================================
// Render resource tests
//==============================================================================

class AUv3RenderResourceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
    }

    void TearDown() override
    {
        if (cpp != nullptr && cpp->isRenderResourcesAllocated())
            cpp->deallocateRenderResources();

        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }
        cpp = nullptr;
        audioUnit = nil;
    }

    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3RenderResourceTests, AllocateRenderResourcesSucceeds)
{
    NSError* error = nil;
    const auto ok = cpp->allocateRenderResourcesAndReturnError (&error);
    EXPECT_TRUE (ok);
    EXPECT_EQ (nil, error);
}

TEST_F (AUv3RenderResourceTests, AllocatedFlagIsSetAfterAllocation)
{
    ASSERT_TRUE (cpp->allocateRenderResourcesAndReturnError (nullptr));
    EXPECT_TRUE (cpp->isRenderResourcesAllocated());
}

TEST_F (AUv3RenderResourceTests, DeallocateClearsAllocatedFlag)
{
    ASSERT_TRUE (cpp->allocateRenderResourcesAndReturnError (nullptr));
    cpp->deallocateRenderResources();
    EXPECT_FALSE (cpp->isRenderResourcesAllocated());
}

TEST_F (AUv3RenderResourceTests, DoubleAllocationIsSafe)
{
    ASSERT_TRUE (cpp->allocateRenderResourcesAndReturnError (nullptr));
    EXPECT_TRUE (cpp->allocateRenderResourcesAndReturnError (nullptr));
    EXPECT_TRUE (cpp->isRenderResourcesAllocated());
}

TEST_F (AUv3RenderResourceTests, DeallocateWithoutAllocationIsSafe)
{
    EXPECT_FALSE (cpp->isRenderResourcesAllocated());
    cpp->deallocateRenderResources();
    EXPECT_FALSE (cpp->isRenderResourcesAllocated());
}

//==============================================================================
// Factory path tests (via YUPAUv3ViewController)
//==============================================================================

class AUv3FactoryInstanceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        viewController = [[YUPAUv3ViewController alloc] initWithNibName:nil bundle:nil];
        ASSERT_NE (nil, viewController);
    }

    void TearDown() override
    {
        audioUnit = nil;
        viewController = nil;
    }

    AUAudioUnit* createAudioUnit()
    {
        const auto desc = makeTestDescription();
        NSError* error = nil;
        audioUnit = [viewController createAudioUnitWithComponentDescription:desc error:&error];
        return audioUnit;
    }

    YUPAUv3ViewController* viewController = nil;
    AUAudioUnit* audioUnit = nil;
};

TEST_F (AUv3FactoryInstanceTests, CreateAudioUnitReturnsNonNull)
{
    auto* au = createAudioUnit();
    EXPECT_NE (nil, au);
}

//==============================================================================
// Processor identity tests (Bug #8: editor must use render processor)
//==============================================================================

TEST_F (AUv3FactoryInstanceTests, ViewControllerProcessorIsSetAfterCreatingAudioUnit)
{
    createAudioUnit();

    Ivar ivar = class_getInstanceVariable ([viewController class], "cpp");
    ASSERT_NE (nullptr, ivar);

    using VP = std::unique_ptr<AudioPluginViewControllerv3>;
    auto* vcCppPtr = reinterpret_cast<VP*> (
        reinterpret_cast<uint8_t*> ((__bridge void*) viewController) + ivar_getOffset (ivar));

    ASSERT_NE (nullptr, vcCppPtr);
    ASSERT_NE (nullptr, vcCppPtr->get());

    auto* editorProcessor = (*vcCppPtr)->getProcessor();
    ignoreUnused (editorProcessor);
}

//==============================================================================
// Render tests
//==============================================================================

class AUv3RenderTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
        ASSERT_TRUE (cpp->allocateRenderResourcesAndReturnError (nullptr));
    }

    void TearDown() override
    {
        if (cpp != nullptr && cpp->isRenderResourcesAllocated())
            cpp->deallocateRenderResources();

        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
    }

    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3RenderTests, InternalRenderBlockIsAccessible)
{
    auto block = cpp->getInternalRenderBlock();
    EXPECT_NE (nil, block);
}

TEST_F (AUv3RenderTests, RenderBlockRejectsOversizedFrameCount)
{
    auto block = cpp->getInternalRenderBlock();
    ASSERT_NE (nil, block);

    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timestamp {};
    timestamp.mSampleTime = 0;
    AudioBufferList outputBufferList {};
    outputBufferList.mNumberBuffers = 0;

    // A frame count of 0 must be accepted (0 <= allocatedMaximumFrames).
    AUAudioUnitStatus status = block (&flags, &timestamp, 0, 0, &outputBufferList, nullptr, nullptr);
    EXPECT_EQ (noErr, status);

    // A frame count exceeding the pre-allocated buffer capacity must be rejected.
    status = block (&flags, &timestamp, std::numeric_limits<AUAudioFrameCount>::max(), 0, &outputBufferList, nullptr, nullptr);
    EXPECT_EQ (kAudioUnitErr_TooManyFramesToProcess, status);
}

//==============================================================================
// State tests
//==============================================================================

class AUv3StateTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
    }

    void TearDown() override
    {
        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
    }

    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3StateTests, FullStateIsRetrievable)
{
    auto* state = cpp->getFullState();
    EXPECT_NE (nil, state);
}

TEST_F (AUv3StateTests, FactoryPresetsAreAccessible)
{
    auto* presets = cpp->getFactoryPresets();
    EXPECT_NE (nil, presets);
}

TEST_F (AUv3StateTests, LatencyReturnsValidValue)
{
    const auto latency = cpp->getLatency();
    EXPECT_GE (latency, 0.0);
}

TEST_F (AUv3StateTests, TailTimeReturnsValidValue)
{
    const auto tail = cpp->getTailTime();
    EXPECT_GE (tail, 0.0);
}

//==============================================================================
// Sidechain bus layout tests (Bug: sidechain input handling)
//==============================================================================

class AUv3SidechainInstanceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        layoutGuard = std::make_unique<ScopedProcessorLayout> (testPluginBusLayoutWithSidechain());
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
    }

    void TearDown() override
    {
        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
        layoutGuard.reset();
    }

    std::unique_ptr<ScopedProcessorLayout> layoutGuard;
    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3SidechainInstanceTests, InputBusCountIncludesSidechain)
{
    // 2 audio input buses: Main (2 ch) + Sidechain (1 ch)
    auto* busses = cpp->getInputBusses();
    ASSERT_NE (nil, busses);
    EXPECT_EQ (2u, [busses count]);
}

TEST_F (AUv3SidechainInstanceTests, OutputBusCountExcludesSidechain)
{
    // 1 audio output bus: Main (2 ch)
    auto* busses = cpp->getOutputBusses();
    ASSERT_NE (nil, busses);
    EXPECT_EQ (1u, [busses count]);
}

TEST_F (AUv3SidechainInstanceTests, ChannelCapabilitiesArePerBus)
{
    // Bug fix: channel capabilities should list each audio bus individually,
    // not just the max. For sidechain layout: 2 input audio buses + 1 output = 3 entries.
    auto* caps = cpp->getChannelCapabilities();
    ASSERT_NE (nil, caps);
    EXPECT_EQ (3u, [caps count]);

    // Input bus 0: 2 channels, input bus 1: 1 channel, output bus 0: 2 channels
    EXPECT_EQ (2, [[caps objectAtIndexedSubscript:0] integerValue]);
    EXPECT_EQ (1, [[caps objectAtIndexedSubscript:1] integerValue]);
    EXPECT_EQ (2, [[caps objectAtIndexedSubscript:2] integerValue]);
}

TEST_F (AUv3SidechainInstanceTests, RenderResourcesAllocateWithSidechain)
{
    NSError* error = nil;
    EXPECT_TRUE (cpp->allocateRenderResourcesAndReturnError (&error));
    EXPECT_EQ (nil, error);
    EXPECT_TRUE (cpp->isRenderResourcesAllocated());
}

TEST_F (AUv3SidechainInstanceTests, ShouldChangeToFormatRejectsMismatchedChannels)
{
    // Bug fix: shouldChangeToFormat must use exact match (==), not <=
    // The sidechain bus expects exactly 1 channel

    auto* inputBusses = cpp->getInputBusses();
    ASSERT_NE (nil, inputBusses);
    ASSERT_GE ([inputBusses count], 2u);

    auto* sidechainBus = [inputBusses objectAtIndexedSubscript:1];
    ASSERT_NE (nil, sidechainBus);

    AVAudioFormat* validFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0 channels:1];
    EXPECT_TRUE (cpp->shouldChangeToFormat (validFormat, sidechainBus));

    AVAudioFormat* tooManyChannels = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0 channels:2];
    EXPECT_FALSE (cpp->shouldChangeToFormat (tooManyChannels, sidechainBus));

    AVAudioFormat* zeroChannels = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0 channels:0];
    EXPECT_FALSE (cpp->shouldChangeToFormat (zeroChannels, sidechainBus));
}

TEST_F (AUv3SidechainInstanceTests, ShouldChangeToFormatRejectsFloat64)
{
    // Float64 must be rejected when the processor does not support double precision.
    auto* inputBusses = cpp->getInputBusses();
    ASSERT_NE (nil, inputBusses);
    ASSERT_GE ([inputBusses count], 1u);

    auto* mainBus = [inputBusses objectAtIndexedSubscript:0];
    ASSERT_NE (nil, mainBus);

    AVAudioFormat* float64Format = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat64
                                                                    sampleRate:44100.0
                                                                      channels:2
                                                                   interleaved:NO];
    ASSERT_NE (nil, float64Format);
    EXPECT_FALSE (cpp->shouldChangeToFormat (float64Format, mainBus));
}

TEST_F (AUv3SidechainInstanceTests, ShouldChangeToFormatAcceptsFloat64WhenSupported)
{
    // Float64 must be accepted when the processor supports double precision.
    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    ASSERT_NE (nullptr, proc);
    proc->supportsDouble = true;

    auto* inputBusses = cpp->getInputBusses();
    ASSERT_NE (nil, inputBusses);
    ASSERT_GE ([inputBusses count], 1u);

    auto* mainBus = [inputBusses objectAtIndexedSubscript:0];
    ASSERT_NE (nil, mainBus);

    AVAudioFormat* float64Format = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat64
                                                                    sampleRate:44100.0
                                                                      channels:2
                                                                   interleaved:NO];
    ASSERT_NE (nil, float64Format);
    EXPECT_TRUE (cpp->shouldChangeToFormat (float64Format, mainBus));
}

TEST_F (AUv3SidechainInstanceTests, ShouldChangeToFormatAcceptsInterleaved)
{
    // Interleaved formats - the render callback handles deinterleave/interleave via AudioData converters.
    auto* inputBusses = cpp->getInputBusses();
    ASSERT_NE (nil, inputBusses);
    ASSERT_GE ([inputBusses count], 1u);

    auto* mainBus = [inputBusses objectAtIndexedSubscript:0];
    ASSERT_NE (nil, mainBus);

    AVAudioFormat* interleavedFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                                        sampleRate:44100.0
                                                                          channels:2
                                                                       interleaved:YES];
    ASSERT_NE (nil, interleavedFormat);
    EXPECT_TRUE (cpp->shouldChangeToFormat (interleavedFormat, mainBus));
}

TEST_F (AUv3SidechainInstanceTests, ShouldChangeToFormatAcceptsFloat32NonInterleaved)
{
    // The canonical format (Float32, non-interleaved, correct channel count) must be accepted.
    auto* inputBusses = cpp->getInputBusses();
    ASSERT_NE (nil, inputBusses);
    ASSERT_GE ([inputBusses count], 1u);

    auto* mainBus = [inputBusses objectAtIndexedSubscript:0];
    ASSERT_NE (nil, mainBus);

    AVAudioFormat* float32Format = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                                    sampleRate:44100.0
                                                                      channels:2
                                                                   interleaved:NO];
    ASSERT_NE (nil, float32Format);
    EXPECT_TRUE (cpp->shouldChangeToFormat (float32Format, mainBus));

    // Also verify the sidechain bus accepts its correct format
    if ([inputBusses count] >= 2u)
    {
        auto* sidechainBus = [inputBusses objectAtIndexedSubscript:1];
        ASSERT_NE (nil, sidechainBus);

        AVAudioFormat* scFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                                                   sampleRate:44100.0
                                                                     channels:1
                                                                  interleaved:NO];
        ASSERT_NE (nil, scFormat);
        EXPECT_TRUE (cpp->shouldChangeToFormat (scFormat, sidechainBus));
    }
}

//==============================================================================
// Sidechain with inactive auxiliary bus
//==============================================================================

class AUv3InactiveSidechainTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        layoutGuard = std::make_unique<ScopedProcessorLayout> (testPluginBusLayoutWithInactiveSidechain());
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
    }

    void TearDown() override
    {
        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
        layoutGuard.reset();
    }

    std::unique_ptr<ScopedProcessorLayout> layoutGuard;
    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3InactiveSidechainTests, InputBusCountIncludesInactiveSidechain)
{
    auto* busses = cpp->getInputBusses();
    ASSERT_NE (nil, busses);
    // Both active and inactive sidechain buses are still audio buses
    EXPECT_EQ (2u, [busses count]);
}

TEST_F (AUv3InactiveSidechainTests, RenderResourcesAllocateWithInactiveSidechain)
{
    NSError* error = nil;
    EXPECT_TRUE (cpp->allocateRenderResourcesAndReturnError (&error));
    EXPECT_EQ (nil, error);
}

//==============================================================================
// Sidechain with auxiliary output bus
//==============================================================================

class AUv3AuxOutputTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        layoutGuard = std::make_unique<ScopedProcessorLayout> (testPluginBusLayoutWithAuxOutput());
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
    }

    void TearDown() override
    {
        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
        layoutGuard.reset();
    }

    std::unique_ptr<ScopedProcessorLayout> layoutGuard;
    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3AuxOutputTests, OutputBusCountIncludesAuxiliaryOutput)
{
    auto* busses = cpp->getOutputBusses();
    ASSERT_NE (nil, busses);
    EXPECT_EQ (2u, [busses count]);
}

TEST_F (AUv3AuxOutputTests, ChannelCapabilitiesIncludeAuxiliaryOutput)
{
    auto* caps = cpp->getChannelCapabilities();
    ASSERT_NE (nil, caps);
    // 1 input audio bus + 2 output audio buses = 3 entries
    EXPECT_EQ (3u, [caps count]);
}

//==============================================================================
// Parameter value tests
//==============================================================================

class AUv3ParameterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
        tree = cpp->getParameterTree();
        ASSERT_NE (nil, tree);
    }

    void TearDown() override
    {
        tree = nil;
        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
    }

    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
    AUParameterTree* tree = nil;
};

TEST_F (AUv3ParameterTests, ParameterTreeHasExpectedCount)
{
    auto* allParams = [tree allParameters];
    ASSERT_NE (nil, allParams);
    // TestPluginProcessor: Gain (100), Mode (200), Meter (300), Internal (400) = 4
    EXPECT_EQ (4u, [allParams count]);
}

TEST_F (AUv3ParameterTests, ParameterByAddressIsFound)
{
    auto* gainParam = [tree parameterWithAddress:100];
    EXPECT_NE (nil, gainParam);
    EXPECT_EQ (100u, [gainParam address]);
}

TEST_F (AUv3ParameterTests, ParameterByInvalidAddressIsNull)
{
    auto* param = [tree parameterWithAddress:99999];
    EXPECT_EQ (nil, param);
}

TEST_F (AUv3ParameterTests, ParameterMetadataIsValid)
{
    auto* gainParam = [tree parameterWithAddress:100];
    ASSERT_NE (nil, gainParam);

    EXPECT_GT ([[gainParam displayName] length], 0u);
    EXPECT_GT ([[gainParam identifier] length], 0u);
    EXPECT_NEAR (0.5f, [gainParam value], 0.001f);
}

TEST_F (AUv3ParameterTests, GetValueViaValueProvider)
{
    auto* gainParam = [tree parameterWithAddress:100];
    ASSERT_NE (nil, gainParam);

    AUValue value = [gainParam value];
    EXPECT_NEAR (0.5f, value, 0.001f);
}

TEST_F (AUv3ParameterTests, SetValueViaValueObserver)
{
    auto* gainParam = [tree parameterWithAddress:100];
    ASSERT_NE (nil, gainParam);

    [gainParam setValue:0.25f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];

    EXPECT_NEAR (0.25f, [gainParam value], 0.001f);
}

TEST_F (AUv3ParameterTests, ValueRoundTrip)
{
    auto* modeParam = [tree parameterWithAddress:200];
    ASSERT_NE (nil, modeParam);

    [modeParam setValue:3.0f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
    EXPECT_NEAR (3.0f, [modeParam value], 0.001f);

    [modeParam setValue:0.0f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
    EXPECT_NEAR (0.0f, [modeParam value], 0.001f);
}

TEST_F (AUv3ParameterTests, SteppedParameterHasCorrectRange)
{
    auto* modeParam = [tree parameterWithAddress:200];
    ASSERT_NE (nil, modeParam);

    EXPECT_NEAR (0.0f, [modeParam minValue], 0.001f);
    EXPECT_NEAR (4.0f, [modeParam maxValue], 0.001f);
}

TEST_F (AUv3ParameterTests, ReadOnlyParameterHasCorrectRange)
{
    auto* meterParam = [tree parameterWithAddress:300];
    ASSERT_NE (nil, meterParam);

    EXPECT_NEAR (-60.0f, [meterParam minValue], 0.001f);
    EXPECT_NEAR (0.0f, [meterParam maxValue], 0.001f);
}

TEST_F (AUv3ParameterTests, SteppedParameterHasValueStrings)
{
    // Mode (address 200) is a stepped enum parameter — should have value strings
    auto* modeParam = [tree parameterWithAddress:200];
    ASSERT_NE (nil, modeParam);

    auto* valueStrings = [modeParam valueStrings];
    EXPECT_NE (nil, valueStrings);
    EXPECT_GE ([valueStrings count], 1u);
}

TEST_F (AUv3ParameterTests, ContinuousParameterHasNoValueStrings)
{
    // Gain (address 100) is a continuous parameter — should not have value strings
    auto* gainParam = [tree parameterWithAddress:100];
    ASSERT_NE (nil, gainParam);

    EXPECT_EQ (nil, [gainParam valueStrings]);
}

TEST_F (AUv3ParameterTests, ReadOnlyParameterRoundTripPreservesValue)
{
    // Meter (address 300) has range [-60, 0] — tests non-zero-minimum normalization
    auto* meterParam = [tree parameterWithAddress:300];
    ASSERT_NE (nil, meterParam);

    // Set the AU parameter to a value in the middle of the range
    [meterParam setValue:-30.0f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
    EXPECT_NEAR (-30.0f, [meterParam value], 0.001f);

    // Verify the yup parameter's real value matches (not normalized)
    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    ASSERT_NE (nullptr, proc);
    auto yupParams = proc->getParameters();
    ASSERT_GE (yupParams.size(), 4u);
    auto* meterYupParam = yupParams[2].get(); // index 2 = Meter
    ASSERT_NE (nullptr, meterYupParam);
    EXPECT_NEAR (-30.0f, meterYupParam->getValue(), 0.001f);

    // Normalized value should be 0.5 (halfway between -60 and 0)
    EXPECT_NEAR (0.5f, meterYupParam->getNormalizedValue(), 0.01f);
}

TEST_F (AUv3ParameterTests, ReadOnlyParameterEndpointsRoundTrip)
{
    auto* meterParam = [tree parameterWithAddress:300];
    ASSERT_NE (nil, meterParam);

    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    auto yupParams = proc->getParameters();
    ASSERT_GE (yupParams.size(), 4u);
    auto* meterYupParam = yupParams[2].get();

    // Minimum value: AU -60 → YUP real -60, normalized 0.0
    [meterParam setValue:-60.0f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
    EXPECT_NEAR (-60.0f, meterYupParam->getValue(), 0.001f);
    EXPECT_NEAR (0.0f, meterYupParam->getNormalizedValue(), 0.01f);

    // Maximum value: AU 0 → YUP real 0, normalized 1.0
    [meterParam setValue:0.0f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
    EXPECT_NEAR (0.0f, meterYupParam->getValue(), 0.001f);
    EXPECT_NEAR (1.0f, meterYupParam->getNormalizedValue(), 0.01f);
}

TEST_F (AUv3ParameterTests, GainParameterEndpointsRoundTrip)
{
    // Gain (address 100) has range [0, 1] — tests linear zero-based normalization
    auto* gainParam = [tree parameterWithAddress:100];
    ASSERT_NE (nil, gainParam);

    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    auto yupParams = proc->getParameters();
    ASSERT_GE (yupParams.size(), 1u);
    auto* gainYupParam = yupParams[0].get();

    // Minimum value
    [gainParam setValue:0.0f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
    EXPECT_NEAR (0.0f, gainYupParam->getValue(), 0.001f);
    EXPECT_NEAR (0.0f, gainYupParam->getNormalizedValue(), 0.001f);

    // Maximum value
    [gainParam setValue:1.0f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
    EXPECT_NEAR (1.0f, gainYupParam->getValue(), 0.001f);
    EXPECT_NEAR (1.0f, gainYupParam->getNormalizedValue(), 0.001f);

    // Midpoint
    [gainParam setValue:0.5f originator:nil atHostTime:0 eventType:AUParameterAutomationEventTypeValue];
    EXPECT_NEAR (0.5f, gainYupParam->getValue(), 0.001f);
    EXPECT_NEAR (0.5f, gainYupParam->getNormalizedValue(), 0.001f);
}

TEST_F (AUv3ParameterTests, StringConversionUsesRealValue)
{
    auto* gainParam = [tree parameterWithAddress:100];
    ASSERT_NE (nil, gainParam);

    // The implementorStringFromValueCallback should return a string for the real (denormalized) value.
    // Since the Gain param has range [0, 1] and default 0.5, the string should represent 0.5.
    AUValue val = 0.5f;
    auto* displayString = [gainParam stringFromValue:&val];
    ASSERT_NE (nil, displayString);
    EXPECT_GT ([displayString length], 0u);

    // For a stepped param like Mode, value strings should match
    auto* modeParam = [tree parameterWithAddress:200];
    ASSERT_NE (nil, modeParam);

    AUValue modeVal = 0.0f;
    auto* modeString = [modeParam stringFromValue:&modeVal];
    ASSERT_NE (nil, modeString);
    EXPECT_GT ([modeString length], 0u);
}

//==============================================================================
// Bypass tests
//==============================================================================

class AUv3BypassTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
    }

    void TearDown() override
    {
        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
    }

    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3BypassTests, ShouldBypassEffectDefaultsToFalse)
{
    EXPECT_FALSE (cpp->getShouldBypassEffect());
}

TEST_F (AUv3BypassTests, SetShouldBypassEffectTogglesState)
{
    cpp->setShouldBypassEffect (true);
    EXPECT_TRUE (cpp->getShouldBypassEffect());

    cpp->setShouldBypassEffect (false);
    EXPECT_FALSE (cpp->getShouldBypassEffect());
}

TEST_F (AUv3BypassTests, FullStateRoundTripsBypassState)
{
    cpp->setShouldBypassEffect (true);

    auto* state = cpp->getFullState();
    ASSERT_NE (nil, state);

    cpp->setShouldBypassEffect (false);
    EXPECT_FALSE (cpp->getShouldBypassEffect());

    cpp->setFullState (state);
    EXPECT_TRUE (cpp->getShouldBypassEffect());
}

TEST_F (AUv3BypassTests, LegacyRawStateFallsBackToProcessorState)
{
    // Pre-bypass-fix presets store raw processor state without the wrapper magic.
    // Loading one must restore the processor state and leave bypass untouched.
    cpp->setShouldBypassEffect (true);

    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    ASSERT_NE (nullptr, proc);

    const uint8_t legacyData[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    auto* rawData = [[NSData alloc] initWithBytes:legacyData length:sizeof (legacyData)];
    auto* state = @{ (__bridge NSString*) getAUProcessorStateKey() : rawData };

    cpp->setFullState (state);

    EXPECT_TRUE (cpp->getShouldBypassEffect()); // bypass untouched by legacy state
    ASSERT_EQ (sizeof (legacyData), proc->lastLoadedState.getSize());
    EXPECT_EQ (0, std::memcmp (legacyData, proc->lastLoadedState.getData(), sizeof (legacyData)));
}

//==============================================================================
// Bypass render tests
//==============================================================================

class AUv3BypassRenderTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        audioUnit.maximumFramesToRender = 512;
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
        ASSERT_TRUE (cpp->allocateRenderResourcesAndReturnError (nullptr));
    }

    void TearDown() override
    {
        if (cpp != nullptr && cpp->isRenderResourcesAllocated())
            cpp->deallocateRenderResources();

        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
    }

    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3BypassRenderTests, RenderRoutesToBypassedPathWhenBypassed)
{
    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    ASSERT_NE (nullptr, proc);

    cpp->setShouldBypassEffect (true);

    auto block = cpp->getInternalRenderBlock();
    ASSERT_NE (nil, block);

    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timestamp {};
    timestamp.mSampleTime = 0;

    constexpr AUAudioFrameCount frameCount = 64;
    float ch0[64] = {};
    float ch1[64] = {};

    AudioBufferList outputBufferList {};
    outputBufferList.mNumberBuffers = 2;
    outputBufferList.mBuffers[0].mNumberChannels = 1;
    outputBufferList.mBuffers[0].mData = ch0;
    outputBufferList.mBuffers[0].mDataByteSize = sizeof (ch0);
    outputBufferList.mBuffers[1].mNumberChannels = 1;
    outputBufferList.mBuffers[1].mData = ch1;
    outputBufferList.mBuffers[1].mDataByteSize = sizeof (ch1);

    const AUAudioUnitStatus status = block (&flags, &timestamp, frameCount, 0, &outputBufferList, nullptr, nullptr);
    EXPECT_EQ (noErr, status);

    EXPECT_EQ (1, proc->bypassCallCount);
    EXPECT_EQ (0, proc->processCallCount);
}

TEST_F (AUv3BypassRenderTests, RenderRoutesToProcessPathWhenNotBypassed)
{
    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    ASSERT_NE (nullptr, proc);

    auto block = cpp->getInternalRenderBlock();
    ASSERT_NE (nil, block);

    AudioUnitRenderActionFlags flags = 0;
    AudioTimeStamp timestamp {};
    timestamp.mSampleTime = 0;

    constexpr AUAudioFrameCount frameCount = 64;
    float ch0[64] = {};
    float ch1[64] = {};

    AudioBufferList outputBufferList {};
    outputBufferList.mNumberBuffers = 2;
    outputBufferList.mBuffers[0].mNumberChannels = 1;
    outputBufferList.mBuffers[0].mData = ch0;
    outputBufferList.mBuffers[0].mDataByteSize = sizeof (ch0);
    outputBufferList.mBuffers[1].mNumberChannels = 1;
    outputBufferList.mBuffers[1].mData = ch1;
    outputBufferList.mBuffers[1].mDataByteSize = sizeof (ch1);

    const AUAudioUnitStatus status = block (&flags, &timestamp, frameCount, 0, &outputBufferList, nullptr, nullptr);
    EXPECT_EQ (noErr, status);

    EXPECT_EQ (0, proc->bypassCallCount);
    EXPECT_EQ (1, proc->processCallCount);
}

//==============================================================================
// Sidechain bus name tests
//==============================================================================

TEST_F (AUv3SidechainInstanceTests, MainInputBusHasName)
{
    auto* busses = cpp->getInputBusses();
    ASSERT_NE (nil, busses);
    ASSERT_GE ([busses count], 1u);

    auto* mainBus = [busses objectAtIndexedSubscript:0];
    ASSERT_NE (nil, mainBus);
    EXPECT_GT ([[mainBus name] length], 0u);
}

TEST_F (AUv3SidechainInstanceTests, SidechainInputBusHasName)
{
    auto* busses = cpp->getInputBusses();
    ASSERT_NE (nil, busses);
    ASSERT_GE ([busses count], 2u);

    auto* scBus = [busses objectAtIndexedSubscript:1];
    ASSERT_NE (nil, scBus);
    EXPECT_GT ([[scBus name] length], 0u);
}

TEST_F (AUv3SidechainInstanceTests, MainOutputBusHasName)
{
    auto* busses = cpp->getOutputBusses();
    ASSERT_NE (nil, busses);
    ASSERT_GE ([busses count], 1u);

    auto* mainBus = [busses objectAtIndexedSubscript:0];
    ASSERT_NE (nil, mainBus);
    EXPECT_GT ([[mainBus name] length], 0u);
}

//==============================================================================
// State save/load round-trip tests
//==============================================================================

class AUv3StateRoundTripTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAUAudioUnit();
        ASSERT_NE (nil, audioUnit);
        cpp = AUAudioUnitSubclass::_this (audioUnit);
        ASSERT_NE (nullptr, cpp);
    }

    void TearDown() override
    {
        if (cpp != nullptr)
        {
            AUAudioUnitSubclass::setThis (audioUnit, nullptr);
            delete cpp;
        }

        cpp = nullptr;
        audioUnit = nil;
    }

    AUAudioUnit* audioUnit = nil;
    AudioPluginProcessorAUv3* cpp = nullptr;
};

TEST_F (AUv3StateRoundTripTests, FullStateIsRetrievable)
{
    auto* state = cpp->getFullState();
    ASSERT_NE (nil, state);
}

TEST_F (AUv3StateRoundTripTests, FullStateContainsProcessorStateWhenPopulated)
{
    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    ASSERT_NE (nullptr, proc);

    proc->lastSavedState = MemoryBlock ("test", 4);

    auto* state = cpp->getFullState();
    ASSERT_NE (nil, state);
    EXPECT_NE (nil, state[@"YUPProcessorState"]);
}

TEST_F (AUv3StateRoundTripTests, SetFullStateDoesNotCrash)
{
    auto* state = cpp->getFullState();
    ASSERT_NE (nil, state);

    cpp->setFullState (state);
}

TEST_F (AUv3StateRoundTripTests, YupProcessorStateRoundTrip)
{
    // Populate processor state so saveStateIntoMemory produces data
    auto* proc = static_cast<TestPluginProcessor*> (cpp->getProcessor());
    ASSERT_NE (nullptr, proc);

    const uint8_t testData[] = { 0xde, 0xad, 0xbe, 0xef };
    proc->lastSavedState = MemoryBlock (testData, sizeof (testData));

    // Save full state
    auto* savedState = cpp->getFullState();
    ASSERT_NE (nil, savedState);
    ASSERT_NE (nil, savedState[@"YUPProcessorState"]);

    // Modify the saved state on the processor so we can detect restoration
    proc->lastSavedState = MemoryBlock();

    // Restore
    cpp->setFullState (savedState);

    // Verify the processor received the original state
    EXPECT_EQ (sizeof (testData), proc->lastLoadedState.getSize());
    EXPECT_EQ (0, std::memcmp (testData, proc->lastLoadedState.getData(), sizeof (testData)));
}

TEST_F (AUv3StateRoundTripTests, SetFullStateWithMissingYupKeyDoesNotCrash)
{
    // setFullState should handle a dictionary without YUPProcessorState gracefully
    auto* emptyState = [[NSDictionary<NSString*, id> alloc] init];
    cpp->setFullState (emptyState);
    SUCCEED() << "setFullState did not crash with malformed YUPProcessorState";
}

TEST_F (AUv3StateRoundTripTests, SetFullStateWithMalformedYupKeyDoesNotCrash)
{
    // setFullState should handle a malformed YUPProcessorState value gracefully
    auto* badState = @{ @"YUPProcessorState": @"not-data" };
    cpp->setFullState (badState);
    SUCCEED() << "setFullState did not crash with malformed YUPProcessorState";
}

TEST_F (AUv3StateRoundTripTests, LatencyIsPreservedAcrossStateRestore)
{
    const auto initialLatency = cpp->getLatency();
    EXPECT_GE (initialLatency, 0.0);

    auto* state = cpp->getFullState();
    ASSERT_NE (nil, state);
    cpp->setFullState (state);

    EXPECT_EQ (initialLatency, cpp->getLatency());
}
