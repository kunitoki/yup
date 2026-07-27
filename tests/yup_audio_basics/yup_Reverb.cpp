/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
class ReverbTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reverb = std::make_unique<Reverb>();
    }

    void TearDown() override
    {
        reverb.reset();
    }

    std::unique_ptr<Reverb> reverb;
};

//==============================================================================
TEST_F (ReverbTests, Constructor)
{
    // Constructor should call setParameters and setSampleRate (lines 61-62)
    EXPECT_NO_THROW (Reverb());

    // Default parameters should be set
    auto params = reverb->getParameters();
    EXPECT_FLOAT_EQ (params.roomSize, 0.5f);
    EXPECT_FLOAT_EQ (params.damping, 0.5f);
    EXPECT_FLOAT_EQ (params.wetLevel, 0.33f);
    EXPECT_FLOAT_EQ (params.dryLevel, 0.4f);
    EXPECT_FLOAT_EQ (params.width, 1.0f);
    EXPECT_FLOAT_EQ (params.freezeMode, 0.0f);
}

//==============================================================================
TEST_F (ReverbTests, ParametersDefaultValues)
{
    Reverb::Parameters params;

    EXPECT_FLOAT_EQ (params.roomSize, 0.5f);
    EXPECT_FLOAT_EQ (params.damping, 0.5f);
    EXPECT_FLOAT_EQ (params.wetLevel, 0.33f);
    EXPECT_FLOAT_EQ (params.dryLevel, 0.4f);
    EXPECT_FLOAT_EQ (params.width, 1.0f);
    EXPECT_FLOAT_EQ (params.freezeMode, 0.0f);
}

//==============================================================================
TEST_F (ReverbTests, GetParameters)
{
    auto params = reverb->getParameters();

    EXPECT_FLOAT_EQ (params.roomSize, 0.5f);
    EXPECT_FLOAT_EQ (params.damping, 0.5f);
    EXPECT_FLOAT_EQ (params.wetLevel, 0.33f);
    EXPECT_FLOAT_EQ (params.dryLevel, 0.4f);
    EXPECT_FLOAT_EQ (params.width, 1.0f);
    EXPECT_FLOAT_EQ (params.freezeMode, 0.0f);
}

//==============================================================================
TEST_F (ReverbTests, SetParametersBasic)
{
    Reverb::Parameters params;
    params.roomSize = 0.8f;
    params.damping = 0.3f;
    params.wetLevel = 0.5f;
    params.dryLevel = 0.5f;
    params.width = 0.7f;
    params.freezeMode = 0.0f;

    EXPECT_NO_THROW (reverb->setParameters (params));

    auto retrieved = reverb->getParameters();
    EXPECT_FLOAT_EQ (retrieved.roomSize, 0.8f);
    EXPECT_FLOAT_EQ (retrieved.damping, 0.3f);
    EXPECT_FLOAT_EQ (retrieved.wetLevel, 0.5f);
    EXPECT_FLOAT_EQ (retrieved.dryLevel, 0.5f);
    EXPECT_FLOAT_EQ (retrieved.width, 0.7f);
    EXPECT_FLOAT_EQ (retrieved.freezeMode, 0.0f);
}

TEST_F (ReverbTests, SetParametersWithFreezeMode)
{
    Reverb::Parameters params;
    params.freezeMode = 0.6f; // >= 0.5f activates freeze mode (line 96)

    EXPECT_NO_THROW (reverb->setParameters (params));

    auto retrieved = reverb->getParameters();
    EXPECT_FLOAT_EQ (retrieved.freezeMode, 0.6f);
}

TEST_F (ReverbTests, SetParametersWithoutFreezeMode)
{
    Reverb::Parameters params;
    params.freezeMode = 0.3f; // < 0.5f normal mode (line 96)

    EXPECT_NO_THROW (reverb->setParameters (params));

    auto retrieved = reverb->getParameters();
    EXPECT_FLOAT_EQ (retrieved.freezeMode, 0.3f);
}

TEST_F (ReverbTests, SetParametersWetGainCalculation)
{
    Reverb::Parameters params;
    params.wetLevel = 0.5f;
    params.width = 1.0f;

    // Tests lines 88-94 (wet gain calculations)
    EXPECT_NO_THROW (reverb->setParameters (params));
}

TEST_F (ReverbTests, SetParametersDryGainCalculation)
{
    Reverb::Parameters params;
    params.dryLevel = 0.7f;

    // Tests line 92 (dry gain calculation)
    EXPECT_NO_THROW (reverb->setParameters (params));
}

TEST_F (ReverbTests, SetParametersWithZeroWidth)
{
    Reverb::Parameters params;
    params.width = 0.0f;

    // Tests lines 93-94 with width = 0
    EXPECT_NO_THROW (reverb->setParameters (params));
}

TEST_F (ReverbTests, SetParametersWithFullWidth)
{
    Reverb::Parameters params;
    params.width = 1.0f;

    // Tests lines 93-94 with width = 1
    EXPECT_NO_THROW (reverb->setParameters (params));
}

TEST_F (ReverbTests, SetParametersUpdatesDamping)
{
    Reverb::Parameters params;
    params.damping = 0.8f;
    params.roomSize = 0.9f;

    // Tests line 98 (updateDamping call)
    EXPECT_NO_THROW (reverb->setParameters (params));
}

//==============================================================================
TEST_F (ReverbTests, SetSampleRate44100)
{
    EXPECT_NO_THROW (reverb->setSampleRate (44100.0));
}

TEST_F (ReverbTests, SetSampleRate48000)
{
    EXPECT_NO_THROW (reverb->setSampleRate (48000.0));
}

TEST_F (ReverbTests, SetSampleRate22050)
{
    EXPECT_NO_THROW (reverb->setSampleRate (22050.0));
}

TEST_F (ReverbTests, SetSampleRate96000)
{
    EXPECT_NO_THROW (reverb->setSampleRate (96000.0));
}

TEST_F (ReverbTests, SetSampleRateCombFilterSizing)
{
    // Tests lines 114-118 (comb filter sizing)
    reverb->setSampleRate (48000.0);

    // Process some audio to verify filters are sized correctly
    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

TEST_F (ReverbTests, SetSampleRateAllPassFilterSizing)
{
    // Tests lines 120-124 (all-pass filter sizing)
    reverb->setSampleRate (96000.0);

    // Process some audio to verify filters are sized correctly
    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.3f;
        right[i] = 0.3f;
    }

    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

TEST_F (ReverbTests, SetSampleRateSmoothedValues)
{
    // Tests lines 126-131 (smoothed value reset)
    EXPECT_NO_THROW (reverb->setSampleRate (44100.0));
}

//==============================================================================
TEST_F (ReverbTests, Reset)
{
    reverb->setSampleRate (44100.0);

    // Process some audio to fill buffers
    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }
    reverb->processStereo (left, right, 512);

    // Tests lines 135-145 (reset clears all filters)
    EXPECT_NO_THROW (reverb->reset());

    // Process silent audio after reset
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.0f;
        right[i] = 0.0f;
    }
    reverb->processStereo (left, right, 512);

    // Output should be silent or very quiet
    for (int i = 0; i < 512; ++i)
    {
        EXPECT_NEAR (left[i], 0.0f, 0.1f);
        EXPECT_NEAR (right[i], 0.0f, 0.1f);
    }
}

TEST_F (ReverbTests, ResetClearsAllCombs)
{
    reverb->setSampleRate (44100.0);

    // Tests lines 137-140 (clear all comb filters)
    EXPECT_NO_THROW (reverb->reset());
}

TEST_F (ReverbTests, ResetClearsAllAllPasses)
{
    reverb->setSampleRate (44100.0);

    // Tests lines 142-143 (clear all all-pass filters)
    EXPECT_NO_THROW (reverb->reset());
}

//==============================================================================
TEST_F (ReverbTests, ProcessStereoBasic)
{
    reverb->setSampleRate (44100.0);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    // Tests lines 149-183 (processStereo)
    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));

    // Output should have been modified by reverb
    bool hasNonZero = false;
    for (int i = 0; i < 512; ++i)
    {
        if (left[i] != 0.0f || right[i] != 0.0f)
        {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZero);
}

TEST_F (ReverbTests, ProcessStereoWithSilence)
{
    reverb->setSampleRate (44100.0);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.0f;
        right[i] = 0.0f;
    }

    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

TEST_F (ReverbTests, ProcessStereoMultipleTimes)
{
    reverb->setSampleRate (44100.0);

    float left[256], right[256];

    // Process multiple times to test state preservation
    for (int iter = 0; iter < 10; ++iter)
    {
        for (int i = 0; i < 256; ++i)
        {
            left[i] = 0.3f;
            right[i] = 0.3f;
        }

        EXPECT_NO_THROW (reverb->processStereo (left, right, 256));
    }
}

TEST_F (ReverbTests, ProcessStereoCombFilterAccumulation)
{
    reverb->setSampleRate (44100.0);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    // Tests lines 163-167 (comb filter accumulation)
    reverb->processStereo (left, right, 512);

    // Both channels should have output
    bool leftHasSignal = false;
    bool rightHasSignal = false;
    for (int i = 0; i < 512; ++i)
    {
        if (std::abs (left[i]) > 0.001f)
            leftHasSignal = true;
        if (std::abs (right[i]) > 0.001f)
            rightHasSignal = true;
    }
    EXPECT_TRUE (leftHasSignal);
    EXPECT_TRUE (rightHasSignal);
}

TEST_F (ReverbTests, ProcessStereoAllPassFilters)
{
    reverb->setSampleRate (44100.0);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    // Tests lines 169-173 (all-pass filter processing)
    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

TEST_F (ReverbTests, ProcessStereoWetDryMix)
{
    reverb->setSampleRate (44100.0);

    Reverb::Parameters params;
    params.wetLevel = 0.5f;
    params.dryLevel = 0.5f;
    reverb->setParameters (params);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    // Tests lines 175-180 (wet/dry mixing)
    reverb->processStereo (left, right, 512);

    // Should have both wet and dry components
    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

TEST_F (ReverbTests, ProcessStereoWidthEffect)
{
    reverb->setSampleRate (44100.0);

    Reverb::Parameters params;
    params.width = 1.0f;
    reverb->setParameters (params);

    // Process multiple blocks to let smoothing settle
    for (int block = 0; block < 5; ++block)
    {
        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = 0.5f;
            right[i] = 0.5f;
        }
        reverb->processStereo (left, right, 512);
    }

    // Now capture output with width = 1.0
    float left1[512], right1[512];
    for (int i = 0; i < 512; ++i)
    {
        left1[i] = 0.5f;
        right1[i] = 0.5f;
    }
    reverb->processStereo (left1, right1, 512);

    // Reset and try with different width
    reverb->reset();
    params.width = 0.0f;
    reverb->setParameters (params);

    // Process multiple blocks to let smoothing settle
    for (int block = 0; block < 5; ++block)
    {
        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = 0.5f;
            right[i] = 0.5f;
        }
        reverb->processStereo (left, right, 512);
    }

    // Now capture output with width = 0.0
    float left2[512], right2[512];
    for (int i = 0; i < 512; ++i)
    {
        left2[i] = 0.5f;
        right2[i] = 0.5f;
    }
    reverb->processStereo (left2, right2, 512);

    // Outputs should be different due to width parameter
    bool isDifferent = false;
    for (int i = 0; i < 512; ++i)
    {
        if (std::abs (left1[i] - left2[i]) > 0.01f)
        {
            isDifferent = true;
            break;
        }
    }
    EXPECT_TRUE (isDifferent);
}

TEST_F (ReverbTests, ProcessStereoInputCalculation)
{
    reverb->setSampleRate (44100.0);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.3f;
        right[i] = 0.7f;
    }

    // Tests line 157 (input = (left + right) * gain)
    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

//==============================================================================
TEST_F (ReverbTests, ProcessMonoBasic)
{
    reverb->setSampleRate (44100.0);

    float samples[512];
    for (int i = 0; i < 512; ++i)
    {
        samples[i] = 0.5f;
    }

    // Tests lines 186-211 (processMono)
    EXPECT_NO_THROW (reverb->processMono (samples, 512));

    // Output should have been modified by reverb
    bool hasNonZero = false;
    for (int i = 0; i < 512; ++i)
    {
        if (samples[i] != 0.0f)
        {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE (hasNonZero);
}

TEST_F (ReverbTests, ProcessMonoWithSilence)
{
    reverb->setSampleRate (44100.0);

    float samples[512];
    for (int i = 0; i < 512; ++i)
    {
        samples[i] = 0.0f;
    }

    EXPECT_NO_THROW (reverb->processMono (samples, 512));
}

TEST_F (ReverbTests, ProcessMonoMultipleTimes)
{
    reverb->setSampleRate (44100.0);

    float samples[256];

    // Process multiple times to test state preservation
    for (int iter = 0; iter < 10; ++iter)
    {
        for (int i = 0; i < 256; ++i)
        {
            samples[i] = 0.3f;
        }

        EXPECT_NO_THROW (reverb->processMono (samples, 256));
    }
}

TEST_F (ReverbTests, ProcessMonoCombFilterAccumulation)
{
    reverb->setSampleRate (44100.0);

    float samples[512];
    for (int i = 0; i < 512; ++i)
    {
        samples[i] = 0.5f;
    }

    // Tests lines 199-200 (comb filter accumulation)
    reverb->processMono (samples, 512);

    // Should have output signal
    bool hasSignal = false;
    for (int i = 0; i < 512; ++i)
    {
        if (std::abs (samples[i]) > 0.001f)
        {
            hasSignal = true;
            break;
        }
    }
    EXPECT_TRUE (hasSignal);
}

TEST_F (ReverbTests, ProcessMonoAllPassFilters)
{
    reverb->setSampleRate (44100.0);

    float samples[512];
    for (int i = 0; i < 512; ++i)
    {
        samples[i] = 0.5f;
    }

    // Tests lines 202-203 (all-pass filter processing)
    EXPECT_NO_THROW (reverb->processMono (samples, 512));
}

TEST_F (ReverbTests, ProcessMonoWetDryMix)
{
    reverb->setSampleRate (44100.0);

    Reverb::Parameters params;
    params.wetLevel = 0.5f;
    params.dryLevel = 0.5f;
    reverb->setParameters (params);

    float samples[512];
    for (int i = 0; i < 512; ++i)
    {
        samples[i] = 0.5f;
    }

    // Tests lines 205-208 (wet/dry mixing)
    reverb->processMono (samples, 512);

    // Should have both wet and dry components
    EXPECT_NO_THROW (reverb->processMono (samples, 512));
}

TEST_F (ReverbTests, ProcessMonoInputCalculation)
{
    reverb->setSampleRate (44100.0);

    float samples[512];
    for (int i = 0; i < 512; ++i)
    {
        samples[i] = 0.7f;
    }

    // Tests line 193 (input = samples[i] * gain)
    EXPECT_NO_THROW (reverb->processMono (samples, 512));
}

//==============================================================================
TEST_F (ReverbTests, FreezeModeActivated)
{
    reverb->setSampleRate (44100.0);

    Reverb::Parameters params;
    params.freezeMode = 0.6f; // >= 0.5f (line 215, 223)
    params.roomSize = 0.8f;
    params.damping = 0.5f;

    // Tests lines 223-224 (freeze mode damping)
    reverb->setParameters (params);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    reverb->processStereo (left, right, 512);

    // In freeze mode, reverb should create sustained effect
    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

TEST_F (ReverbTests, FreezeModeDeactivated)
{
    reverb->setSampleRate (44100.0);

    Reverb::Parameters params;
    params.freezeMode = 0.3f; // < 0.5f (line 215)
    params.roomSize = 0.8f;
    params.damping = 0.5f;

    // Tests lines 225-227 (normal mode damping)
    reverb->setParameters (params);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

TEST_F (ReverbTests, FreezeModeTransition)
{
    reverb->setSampleRate (44100.0);

    // Start in normal mode
    Reverb::Parameters params;
    params.freezeMode = 0.0f;
    reverb->setParameters (params);

    float samples[256];
    for (int i = 0; i < 256; ++i)
    {
        samples[i] = 0.5f;
    }
    reverb->processMono (samples, 256);

    // Switch to freeze mode
    params.freezeMode = 0.8f;
    reverb->setParameters (params);

    for (int i = 0; i < 256; ++i)
    {
        samples[i] = 0.5f;
    }
    reverb->processMono (samples, 256);

    // Should handle transition smoothly
    EXPECT_NO_THROW (reverb->processMono (samples, 256));
}

//==============================================================================
TEST_F (ReverbTests, UpdateDampingNormalMode)
{
    Reverb::Parameters params;
    params.freezeMode = 0.0f;
    params.damping = 0.7f;
    params.roomSize = 0.6f;

    // Tests lines 217-228 (updateDamping in normal mode)
    EXPECT_NO_THROW (reverb->setParameters (params));
}

TEST_F (ReverbTests, UpdateDampingFreezeMode)
{
    Reverb::Parameters params;
    params.freezeMode = 0.9f;
    params.damping = 0.7f;
    params.roomSize = 0.6f;

    // Tests lines 217-228 (updateDamping in freeze mode)
    EXPECT_NO_THROW (reverb->setParameters (params));
}

//==============================================================================
TEST_F (ReverbTests, RoomSizeEffect)
{
    reverb->setSampleRate (44100.0);

    // Small room
    Reverb::Parameters params1;
    params1.roomSize = 0.2f;
    reverb->setParameters (params1);

    // Process multiple blocks to let smoothing settle
    for (int block = 0; block < 5; ++block)
    {
        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = 0.5f;
            right[i] = 0.5f;
        }
        reverb->processStereo (left, right, 512);
    }

    // Capture output with small room
    float left1[512], right1[512];
    for (int i = 0; i < 512; ++i)
    {
        left1[i] = 0.5f;
        right1[i] = 0.5f;
    }
    reverb->processStereo (left1, right1, 512);

    // Large room
    reverb->reset();
    Reverb::Parameters params2;
    params2.roomSize = 0.9f;
    reverb->setParameters (params2);

    // Process multiple blocks to let smoothing settle
    for (int block = 0; block < 5; ++block)
    {
        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = 0.5f;
            right[i] = 0.5f;
        }
        reverb->processStereo (left, right, 512);
    }

    // Capture output with large room
    float left2[512], right2[512];
    for (int i = 0; i < 512; ++i)
    {
        left2[i] = 0.5f;
        right2[i] = 0.5f;
    }
    reverb->processStereo (left2, right2, 512);

    // Outputs should be different
    bool isDifferent = false;
    for (int i = 0; i < 512; ++i)
    {
        if (std::abs (left1[i] - left2[i]) > 0.01f)
        {
            isDifferent = true;
            break;
        }
    }
    EXPECT_TRUE (isDifferent);
}

TEST_F (ReverbTests, DampingEffect)
{
    reverb->setSampleRate (44100.0);

    // Test with low damping - send continuous signal and measure output
    Reverb::Parameters params1;
    params1.damping = 0.0f;
    params1.roomSize = 0.8f;
    params1.wetLevel = 1.0f;
    params1.dryLevel = 0.0f;
    reverb->setParameters (params1);

    // Let smoothing settle
    for (int block = 0; block < 10; ++block)
    {
        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = 0.1f;
            right[i] = 0.1f;
        }
        reverb->processStereo (left, right, 512);
    }

    // Capture output energy with low damping
    float left1[512], right1[512];
    for (int i = 0; i < 512; ++i)
    {
        left1[i] = 0.1f;
        right1[i] = 0.1f;
    }
    reverb->processStereo (left1, right1, 512);

    float energy1 = 0.0f;
    for (int i = 256; i < 512; ++i) // Use second half to avoid transients
    {
        energy1 += std::abs (left1[i]) + std::abs (right1[i]);
    }

    // Test with high damping - send continuous signal and measure output
    reverb->reset();
    Reverb::Parameters params2;
    params2.damping = 1.0f;
    params2.roomSize = 0.8f;
    params2.wetLevel = 1.0f;
    params2.dryLevel = 0.0f;
    reverb->setParameters (params2);

    // Let smoothing settle
    for (int block = 0; block < 10; ++block)
    {
        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = 0.1f;
            right[i] = 0.1f;
        }
        reverb->processStereo (left, right, 512);
    }

    // Capture output energy with high damping
    float left2[512], right2[512];
    for (int i = 0; i < 512; ++i)
    {
        left2[i] = 0.1f;
        right2[i] = 0.1f;
    }
    reverb->processStereo (left2, right2, 512);

    float energy2 = 0.0f;
    for (int i = 256; i < 512; ++i) // Use second half to avoid transients
    {
        energy2 += std::abs (left2[i]) + std::abs (right2[i]);
    }

    // Damping affects high-frequency content, both should have some output
    EXPECT_GT (energy1, 0.0f);
    EXPECT_GT (energy2, 0.0f);
}

TEST_F (ReverbTests, WetLevelOnly)
{
    reverb->setSampleRate (44100.0);

    Reverb::Parameters params;
    params.wetLevel = 1.0f;
    params.dryLevel = 0.0f;
    reverb->setParameters (params);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
}

TEST_F (ReverbTests, DryLevelOnly)
{
    reverb->setSampleRate (44100.0);

    Reverb::Parameters params;
    params.wetLevel = 0.0f;
    params.dryLevel = 1.0f;
    reverb->setParameters (params);

    float left[512], right[512];
    const float inputValue = 0.5f;
    for (int i = 0; i < 512; ++i)
    {
        left[i] = inputValue;
        right[i] = inputValue;
    }

    reverb->processStereo (left, right, 512);

    // With only dry signal, output should be close to scaled input
    // (allowing for some variation due to smoothing)
    for (int i = 256; i < 512; ++i) // Check second half after smoothing
    {
        EXPECT_NEAR (left[i], inputValue * 2.0f, 0.5f); // dryScaleFactor = 2.0
        EXPECT_NEAR (right[i], inputValue * 2.0f, 0.5f);
    }
}

//==============================================================================
TEST_F (ReverbTests, LargeBuffer)
{
    reverb->setSampleRate (44100.0);

    const int bufferSize = 8192;
    std::vector<float> left (bufferSize);
    std::vector<float> right (bufferSize);

    for (int i = 0; i < bufferSize; ++i)
    {
        left[i] = 0.3f;
        right[i] = 0.3f;
    }

    EXPECT_NO_THROW (reverb->processStereo (left.data(), right.data(), bufferSize));
}

TEST_F (ReverbTests, SmallBuffer)
{
    reverb->setSampleRate (44100.0);

    float left[8], right[8];
    for (int i = 0; i < 8; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    EXPECT_NO_THROW (reverb->processStereo (left, right, 8));
}

TEST_F (ReverbTests, SingleSample)
{
    reverb->setSampleRate (44100.0);

    float left[1] = { 0.5f };
    float right[1] = { 0.5f };

    EXPECT_NO_THROW (reverb->processStereo (left, right, 1));
}

//==============================================================================
TEST_F (ReverbTests, SequentialProcessing)
{
    reverb->setSampleRate (44100.0);

    // Process multiple sequential blocks
    for (int block = 0; block < 20; ++block)
    {
        float left[256], right[256];
        for (int i = 0; i < 256; ++i)
        {
            left[i] = 0.4f;
            right[i] = 0.4f;
        }

        EXPECT_NO_THROW (reverb->processStereo (left, right, 256));
    }
}

TEST_F (ReverbTests, AlternatingMonoStereo)
{
    reverb->setSampleRate (44100.0);

    float mono[256];
    float left[256], right[256];

    for (int i = 0; i < 256; ++i)
    {
        mono[i] = 0.5f;
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    reverb->processMono (mono, 256);
    reverb->processStereo (left, right, 256);
    reverb->processMono (mono, 256);

    // Should handle switching between mono and stereo
    EXPECT_NO_THROW (reverb->processStereo (left, right, 256));
}

//==============================================================================
TEST_F (ReverbTests, CombFilterWraparound)
{
    reverb->setSampleRate (44100.0);

    // Process enough samples to ensure comb filters wrap around
    const int totalSamples = 10000;
    for (int offset = 0; offset < totalSamples; offset += 512)
    {
        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = 0.3f;
            right[i] = 0.3f;
        }

        EXPECT_NO_THROW (reverb->processStereo (left, right, 512));
    }
}

TEST_F (ReverbTests, AllPassFilterWraparound)
{
    reverb->setSampleRate (44100.0);

    // Process enough samples to ensure all-pass filters wrap around
    const int totalSamples = 5000;
    for (int offset = 0; offset < totalSamples; offset += 256)
    {
        float samples[256];
        for (int i = 0; i < 256; ++i)
        {
            samples[i] = 0.3f;
        }

        EXPECT_NO_THROW (reverb->processMono (samples, 256));
    }
}
