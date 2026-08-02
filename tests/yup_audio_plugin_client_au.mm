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

#define YUP_AUDIO_PLUGIN_CREATE_FUNCTION createPluginProcessorAU
#include "yup_audio_plugin_client/au/yup_audio_plugin_client_AU.mm"

extern "C" yup::AudioProcessor* createPluginProcessorAU()
{
    return new TestPluginProcessor (testPluginBusLayoutStereo());
}

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

    AudioBufferList bufferList {};
    bufferList.mNumberBuffers = numChannels;

    std::vector<float> bufferData (numFrames * numChannels, 0.0f);
    for (UInt32 i = 0; i < numChannels; ++i)
    {
        bufferList.mBuffers[i].mNumberChannels = 1;
        bufferList.mBuffers[i].mDataByteSize = numFrames * sizeof (float);
        bufferList.mBuffers[i].mData = bufferData.data() + (i * numFrames);
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
        &bufferList);

    EXPECT_EQ (noErr, status);
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
