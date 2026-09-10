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
#define YUP_AUDIO_PLUGIN_ENABLE_AU 1
#define YupPlugin_Id "test.au.plugin"
#define YupPlugin_Name "Test AU Plugin"
#define YupPlugin_Vendor "TestVendor"
#define YupPlugin_Version "1.0.0"
#define YupPlugin_IsSynth 0
#define YupPlugin_IsMono 0

// =============================================================================
#include "yup_audio_plugin_client/yup_TestPluginProcessor.h"
#include "yup_audio_plugin_client/yup_TestAudioBufferList.h"

#include <utility>

#define YUP_AUDIO_PLUGIN_CREATE_FUNCTION createPluginProcessorAU
#include "yup_audio_plugin_client/au/yup_audio_plugin_client_AU.mm"

// =============================================================================
// Layout switching for testing different bus configurations, mirroring the AUv3 suite.
static yup::AudioBusLayout gCustomLayout = testPluginBusLayoutStereo();
static bool gUseCustomLayout = false;

extern "C" yup::AudioProcessor* createPluginProcessorAU()
{
    if (gUseCustomLayout)
        return new TestPluginProcessor (gCustomLayout);

    return new TestPluginProcessor (testPluginBusLayoutStereo());
}

// =============================================================================
/** Scoped helper that switches the processor layout for a test scope. */
struct ScopedProcessorLayout
{
    explicit ScopedProcessorLayout (yup::AudioBusLayout layout)
    {
        gCustomLayout = std::move (layout);
        gUseCustomLayout = true;
    }

    ~ScopedProcessorLayout()
    {
        gUseCustomLayout = false;
        gCustomLayout = testPluginBusLayoutStereo();
    }

    ScopedProcessorLayout (const ScopedProcessorLayout&) = delete;
    ScopedProcessorLayout& operator= (const ScopedProcessorLayout&) = delete;
};

// =============================================================================
#include <yup_audio_processors/yup_audio_processors.h>

// =============================================================================
// Tests
// =============================================================================

namespace
{

// Four-char codes for our test AU
constexpr OSType kTestAUType = 'auef';
constexpr OSType kTestAUSubType = 'tst1';
constexpr OSType kTestAUManuf = 'test';

// Register the test component once — subsequent tests reuse via AudioComponentFindNext
static const AudioComponent kRegisteredComponent = []
{
    return ausdk::AUBaseProcessFactory<AudioPluginProcessorAU>::Register (
        kTestAUType,
        kTestAUSubType,
        kTestAUManuf,
        CFSTR ("Test AU"),
        0);
}();

} // namespace

//------------------------------------------------------------------------------
// Registration test
//------------------------------------------------------------------------------

TEST (AUWrapperTest, RegisterComponentSucceeds)
{
    EXPECT_NE (nullptr, kRegisteredComponent);
}

//------------------------------------------------------------------------------
// Helper: instantiate an AudioUnit for a test fixture
static AudioUnit instantiateTestAU()
{
    AudioComponentDescription desc {};
    desc.componentType = kTestAUType;
    desc.componentSubType = kTestAUSubType;
    desc.componentManufacturer = kTestAUManuf;

    const auto found = AudioComponentFindNext (nullptr, &desc);
    if (found == nullptr)
        return nullptr;

    AudioUnit au = nullptr;
    AudioComponentInstanceNew (found, &au);
    return au;
}

//------------------------------------------------------------------------------
// Instantiation and lifecycle tests
//------------------------------------------------------------------------------

class AUInstanceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAU();
        ASSERT_NE (nullptr, audioUnit);
    }

    void TearDown() override
    {
        if (audioUnit != nullptr)
        {
            AudioUnitUninitialize (audioUnit);
            AudioComponentInstanceDispose (audioUnit);
        }
    }

    AudioUnit audioUnit = nullptr;
};

TEST_F (AUInstanceTests, InitializeSucceeds)
{
    const auto status = AudioUnitInitialize (audioUnit);
    EXPECT_EQ (noErr, status);
}

TEST_F (AUInstanceTests, UninitializeAfterInitialize)
{
    ASSERT_EQ (noErr, AudioUnitInitialize (audioUnit));
    const auto status = AudioUnitUninitialize (audioUnit);
    EXPECT_EQ (noErr, status);
}

//------------------------------------------------------------------------------
// Parameter tests
//------------------------------------------------------------------------------

class AUParameterTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAU();
        ASSERT_NE (nullptr, audioUnit);
        ASSERT_EQ (noErr, AudioUnitInitialize (audioUnit));
    }

    void TearDown() override
    {
        if (audioUnit != nullptr)
        {
            AudioUnitUninitialize (audioUnit);
            AudioComponentInstanceDispose (audioUnit);
        }
    }

    AudioUnit audioUnit = nullptr;
};

TEST_F (AUParameterTests, ParameterListIsRetrievable)
{
    UInt32 dataSize = 0;
    const auto status = AudioUnitGetPropertyInfo (
        audioUnit,
        kAudioUnitProperty_ParameterList,
        kAudioUnitScope_Global,
        0,
        &dataSize,
        nullptr);

    EXPECT_EQ (noErr, status);
    EXPECT_GT (dataSize, 0u);

    const auto numParams = dataSize / sizeof (AudioUnitParameterID);
    EXPECT_GE (numParams, 2u);

    std::vector<AudioUnitParameterID> paramIDs (numParams);
    const auto getStatus = AudioUnitGetProperty (
        audioUnit,
        kAudioUnitProperty_ParameterList,
        kAudioUnitScope_Global,
        0,
        paramIDs.data(),
        &dataSize);

    EXPECT_EQ (noErr, getStatus);
}

TEST_F (AUParameterTests, ParameterInfoIsValid)
{
    UInt32 dataSize = 0;
    ASSERT_EQ (noErr, AudioUnitGetPropertyInfo (
        audioUnit,
        kAudioUnitProperty_ParameterList,
        kAudioUnitScope_Global,
        0,
        &dataSize,
        nullptr));

    const auto numParams = dataSize / sizeof (AudioUnitParameterID);
    ASSERT_GE (numParams, 1u);

    std::vector<AudioUnitParameterID> paramIDs (numParams);
    ASSERT_EQ (noErr, AudioUnitGetProperty (
        audioUnit,
        kAudioUnitProperty_ParameterList,
        kAudioUnitScope_Global,
        0,
        paramIDs.data(),
        &dataSize));

    AudioUnitParameterInfo info {};
    dataSize = sizeof (info);

    const auto status = AudioUnitGetProperty (
        audioUnit,
        kAudioUnitProperty_ParameterInfo,
        kAudioUnitScope_Global,
        paramIDs[0],
        &info,
        &dataSize);

    EXPECT_EQ (noErr, status);
    EXPECT_GT (info.name[0], 0);
}

TEST_F (AUParameterTests, GetAndSetParameter)
{
    UInt32 dataSize = 0;
    ASSERT_EQ (noErr, AudioUnitGetPropertyInfo (
        audioUnit,
        kAudioUnitProperty_ParameterList,
        kAudioUnitScope_Global,
        0,
        &dataSize,
        nullptr));

    const auto numParams = dataSize / sizeof (AudioUnitParameterID);
    ASSERT_GE (numParams, 1u);

    std::vector<AudioUnitParameterID> paramIDs (numParams);
    ASSERT_EQ (noErr, AudioUnitGetProperty (
        audioUnit,
        kAudioUnitProperty_ParameterList,
        kAudioUnitScope_Global,
        0,
        paramIDs.data(),
        &dataSize));

    AudioUnitParameterValue value = 0.0f;
    auto status = AudioUnitGetParameter (audioUnit, paramIDs[0], kAudioUnitScope_Global, 0, &value);
    EXPECT_EQ (noErr, status);

    status = AudioUnitSetParameter (audioUnit, paramIDs[0], kAudioUnitScope_Global, 0, 0.5f, 0);
    EXPECT_EQ (noErr, status);

    AudioUnitParameterValue newValue = 0.0f;
    status = AudioUnitGetParameter (audioUnit, paramIDs[0], kAudioUnitScope_Global, 0, &newValue);
    EXPECT_EQ (noErr, status);
    EXPECT_NEAR (0.5f, newValue, 0.001f);
}

//------------------------------------------------------------------------------
// State save/load tests
//------------------------------------------------------------------------------

class AUStateTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAU();
        ASSERT_NE (nullptr, audioUnit);
        ASSERT_EQ (noErr, AudioUnitInitialize (audioUnit));
    }

    void TearDown() override
    {
        if (audioUnit != nullptr)
        {
            AudioUnitUninitialize (audioUnit);
            AudioComponentInstanceDispose (audioUnit);
        }
    }

    AudioUnit audioUnit = nullptr;
};

TEST_F (AUStateTests, GetClassInfoProducesPropertyList)
{
    UInt32 dataSize = 0;
    Boolean writable = false;

    auto status = AudioUnitGetPropertyInfo (
        audioUnit,
        kAudioUnitProperty_ClassInfo,
        kAudioUnitScope_Global,
        0,
        &dataSize,
        &writable);

    EXPECT_EQ (noErr, status);
    EXPECT_TRUE (writable);
    EXPECT_GT (dataSize, 0u);
}

TEST_F (AUStateTests, RenderProducesOutput)
{
    constexpr UInt32 numFrames = 64;
    constexpr UInt32 numChannels = 2;

    TestAudioBufferList bufferList (numChannels);

    std::vector<float> bufferData (numFrames * numChannels, 0.0f);
    for (UInt32 i = 0; i < numChannels; ++i)
    {
        bufferList->mBuffers[i].mNumberChannels = 1;
        bufferList->mBuffers[i].mDataByteSize = numFrames * sizeof (float);
        bufferList->mBuffers[i].mData = bufferData.data() + (i * numFrames);
    }

    AudioTimeStamp timeStamp {};
    timeStamp.mSampleTime = 0;
    timeStamp.mFlags = kAudioTimeStampSampleTimeValid;

    AudioUnitRenderActionFlags actionFlags = 0;

    // Effect AUs require an input connection — set an input callback providing
    // silence so AUEffectBase::Render doesn't return kAudioUnitErr_NoConnection
    AURenderCallbackStruct inputCallback {};
    inputCallback.inputProc = [] (void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32 inNumberFrames, AudioBufferList* ioData) -> OSStatus
    {
        for (UInt32 i = 0; i < ioData->mNumberBuffers; ++i)
            if (ioData->mBuffers[i].mData != nullptr)
                std::memset (ioData->mBuffers[i].mData, 0, inNumberFrames * sizeof (float));
        return noErr;
    };

    ASSERT_EQ (noErr, AudioUnitSetProperty (
        audioUnit,
        kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Input,
        0,
        &inputCallback,
        sizeof (inputCallback)));

    const auto status = AudioUnitRender (
        audioUnit,
        &actionFlags,
        &timeStamp,
        0,
        numFrames,
        bufferList.get());

    EXPECT_EQ (noErr, status);
}

//------------------------------------------------------------------------------
// Sidechain input view tests
//------------------------------------------------------------------------------

class AUSidechainViewTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAU();
        ASSERT_NE (nullptr, audioUnit);
        ASSERT_EQ (noErr, AudioUnitInitialize (audioUnit));

        wrapper = yup::AudioPluginProcessorAU::findInstance (audioUnit);
        ASSERT_NE (nullptr, wrapper);
        processor = static_cast<TestPluginProcessor*> (wrapper->getProcessor());
        ASSERT_NE (nullptr, processor);

        // Element 0 has to be connected, otherwise AUEffectBase::Render bails out
        // with kAudioUnitErr_NoConnection before rendering anything
        ASSERT_EQ (noErr, setInputCallback (0, silenceCallback, nullptr));
    }

    void TearDown() override
    {
        if (audioUnit != nullptr)
        {
            AudioUnitUninitialize (audioUnit);
            AudioComponentInstanceDispose (audioUnit);
        }
    }

    OSStatus setInputCallback (UInt32 element, AURenderCallback proc, void* refCon)
    {
        AURenderCallbackStruct callback {};
        callback.inputProc = proc;
        callback.inputProcRefCon = refCon;

        return AudioUnitSetProperty (audioUnit,
                                     kAudioUnitProperty_SetRenderCallback,
                                     kAudioUnitScope_Input,
                                     element,
                                     &callback,
                                     sizeof (callback));
    }

    OSStatus render()
    {
        TestAudioBufferList output (numChannels);

        std::vector<float> data (numFrames * numChannels, 0.0f);
        for (UInt32 ch = 0; ch < numChannels; ++ch)
        {
            output->mBuffers[ch].mNumberChannels = 1;
            output->mBuffers[ch].mDataByteSize = numFrames * sizeof (float);
            output->mBuffers[ch].mData = data.data() + (ch * numFrames);
        }

        AudioTimeStamp timeStamp {};
        timeStamp.mSampleTime = currentSampleTime;
        timeStamp.mFlags = kAudioTimeStampSampleTimeValid;

        AudioUnitRenderActionFlags actionFlags = 0;

        const auto status = AudioUnitRender (audioUnit, &actionFlags, &timeStamp, 0, numFrames, output.get());

        currentSampleTime += static_cast<Float64> (numFrames);

        return status;
    }

    static OSStatus silenceCallback (void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32 inNumberFrames, AudioBufferList* ioData)
    {
        for (UInt32 i = 0; i < ioData->mNumberBuffers; ++i)
            if (ioData->mBuffers[i].mData != nullptr)
                std::memset (ioData->mBuffers[i].mData, 0, inNumberFrames * sizeof (float));

        return noErr;
    }

    // Sidechain callback whose pull can be made to fail, and that records how
    // often it was asked for audio
    struct SidechainState
    {
        bool failPull = false;
        int callCount = 0;
        int failCount = 0;

        static OSStatus callback (void* refCon, AudioUnitRenderActionFlags* flags, const AudioTimeStamp* timeStamp, UInt32 busNumber, UInt32 inNumberFrames, AudioBufferList* ioData)
        {
            auto* state = static_cast<SidechainState*> (refCon);
            ++state->callCount;

            if (state->failPull)
            {
                ++state->failCount;
                return kAudioUnitErr_InvalidParameter;
            }

            return silenceCallback (refCon, flags, timeStamp, busNumber, inNumberFrames, ioData);
        }
    };

    static constexpr UInt32 numFrames = 64;
    static constexpr UInt32 numChannels = 2;

    ScopedProcessorLayout layoutScope { testPluginBusLayoutWithSidechain() };

    AudioUnit audioUnit = nullptr;
    yup::AudioPluginProcessorAU* wrapper = nullptr;
    TestPluginProcessor* processor = nullptr;
    Float64 currentSampleTime = 0.0;
};

TEST_F (AUSidechainViewTests, BusCountsIncludeSidechain)
{
    EXPECT_EQ (2, static_cast<int> (processor->getNumAudioInputs()));
    EXPECT_EQ (1, static_cast<int> (processor->getNumAudioOutputs()));
}

TEST_F (AUSidechainViewTests, UnfedSidechainIsExposedAsNullChannelView)
{
    SidechainState sidechain;

    // A fed sidechain shows up with real channel pointers
    ASSERT_EQ (noErr, setInputCallback (1, SidechainState::callback, &sidechain));
    ASSERT_EQ (noErr, render());

    ASSERT_EQ (1, processor->processCallCount);
    ASSERT_EQ (1, sidechain.callCount);
    ASSERT_EQ (2u, processor->lastInputBusChannels.size());
    ASSERT_EQ (2u, processor->lastInputBusChannels[0].size());
    ASSERT_EQ (1u, processor->lastInputBusChannels[1].size());
    EXPECT_NE (nullptr, processor->lastInputBusChannels[0][0]);
    EXPECT_NE (nullptr, processor->lastInputBusChannels[0][1]);
    EXPECT_NE (nullptr, processor->lastInputBusChannels[1][0]);

    // Disconnecting the sidechain (a null render callback) must leave the bus
    // reading as a null channel view, not as the pointers the previous render
    // stored there
    ASSERT_EQ (noErr, setInputCallback (1, nullptr, nullptr));
    ASSERT_EQ (noErr, render());

    ASSERT_EQ (2, processor->processCallCount) << "second render never reached processBlock";
    ASSERT_EQ (2u, processor->lastInputBusChannels.size());
    EXPECT_NE (nullptr, processor->lastInputBusChannels[0][0]);
    EXPECT_NE (nullptr, processor->lastInputBusChannels[0][1]);
    EXPECT_EQ (nullptr, processor->lastInputBusChannels[1][0]);
}

TEST_F (AUSidechainViewTests, FailedSidechainPullIsExposedAsNullChannelView)
{
    SidechainState sidechain;

    ASSERT_EQ (noErr, setInputCallback (1, SidechainState::callback, &sidechain));
    ASSERT_EQ (noErr, render());

    ASSERT_EQ (1, processor->processCallCount);
    ASSERT_EQ (1, sidechain.callCount);
    ASSERT_EQ (2u, processor->lastInputBusChannels.size());
    ASSERT_EQ (1u, processor->lastInputBusChannels[1].size());
    ASSERT_NE (nullptr, processor->lastInputBusChannels[1][0]);

    // A pull that errors leaves the bus unfed for this cycle
    sidechain.failPull = true;
    ASSERT_EQ (noErr, render());

    ASSERT_EQ (2, sidechain.callCount) << "sidechain was not pulled on the second render";
    ASSERT_EQ (1, sidechain.failCount);
    ASSERT_EQ (2, processor->processCallCount) << "second render never reached processBlock";
    ASSERT_EQ (2u, processor->lastInputBusChannels.size());
    EXPECT_EQ (nullptr, processor->lastInputBusChannels[1][0]);
}

//------------------------------------------------------------------------------
// Bypass tests
//------------------------------------------------------------------------------

class AUBypassTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        audioUnit = instantiateTestAU();
        ASSERT_NE (nullptr, audioUnit);
        ASSERT_EQ (noErr, AudioUnitInitialize (audioUnit));
    }

    void TearDown() override
    {
        if (audioUnit != nullptr)
        {
            AudioUnitUninitialize (audioUnit);
            AudioComponentInstanceDispose (audioUnit);
        }
    }

    void setBypass (bool shouldBypass)
    {
        const UInt32 bypassed = shouldBypass ? 1u : 0u;
        ASSERT_EQ (noErr, AudioUnitSetProperty (audioUnit,
                                                kAudioUnitProperty_BypassEffect,
                                                kAudioUnitScope_Global,
                                                0,
                                                &bypassed,
                                                sizeof (bypassed)));
    }

    UInt32 getBypass() const
    {
        UInt32 bypassed = 0u;
        UInt32 dataSize = sizeof (bypassed);
        EXPECT_EQ (noErr, AudioUnitGetProperty (audioUnit,
                                                kAudioUnitProperty_BypassEffect,
                                                kAudioUnitScope_Global,
                                                0,
                                                &bypassed,
                                                &dataSize));
        return bypassed;
    }

    AudioUnit audioUnit = nullptr;
};

TEST_F (AUBypassTests, BypassEffectDefaultsToOff)
{
    EXPECT_EQ (0u, getBypass());
}

TEST_F (AUBypassTests, BypassEffectPropertyRoundTrips)
{
    setBypass (true);
    EXPECT_EQ (1u, getBypass());

    setBypass (false);
    EXPECT_EQ (0u, getBypass());
}
