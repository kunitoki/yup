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

#include <yup_audio_gui/yup_audio_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace yup
{
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GpuDevice::Options, yup::GpuDevice::Ptr);
} // namespace yup

//==============================================================================
TEST (KMeterComponentTests, DefaultsAreConfigured)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);

    EXPECT_TRUE (meter.getShowPeak());
    EXPECT_TRUE (meter.getShowPeakHold());
    EXPECT_EQ (KMeterComponent::ScaleMapping::segmented, meter.getScaleMapping());
}

TEST (KMeterComponentTests, SettersUpdateState)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);

    meter.setShowPeak (false);
    meter.setShowPeakHold (false);
    meter.setScaleMapping (KMeterComponent::ScaleMapping::linear);
    meter.setAverageIntegrationTime (0.2);
    meter.setAverageFallTime (0.9);
    meter.setPeakHoldTime (1.5);
    meter.setOverCounterMode (KMeterState::OverCounterMode::total);

    EXPECT_FALSE (meter.getShowPeak());
    EXPECT_FALSE (meter.getShowPeakHold());
    EXPECT_EQ (KMeterComponent::ScaleMapping::linear, meter.getScaleMapping());
    EXPECT_DOUBLE_EQ (0.2, meterState.getIntegrationTime());
    EXPECT_DOUBLE_EQ (0.9, meterState.getAverageFallTime());
    EXPECT_DOUBLE_EQ (1.5, meterState.getPeakHoldTime());
    EXPECT_EQ (KMeterState::OverCounterMode::total, meter.getOverCounterMode());
}

TEST (KMeterComponentTests, PaintWithThemeDoesNotCrash)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setBounds (0.0f, 0.0f, 60.0f, 240.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (120, 240);
    Graphics g (*context, *renderer);

    meter.paint (g);

    EXPECT_TRUE (true);
}

TEST (KMeterComponentTests, GetChannelReturnsConstructorArg)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter0 (meterState, 0);
    KMeterComponent meter1 (meterState, 1);
    KMeterComponent meterAll (meterState, -1);

    EXPECT_EQ (0, meter0.getChannel());
    EXPECT_EQ (1, meter1.getChannel());
    EXPECT_EQ (-1, meterAll.getChannel());
}

TEST (KMeterComponentTests, RefreshRateDefaultIs30)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    EXPECT_EQ (30, meter.getRefreshRate());
}

TEST (KMeterComponentTests, SetRefreshRateUpdatesValue)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setRefreshRate (60);
    EXPECT_EQ (60, meter.getRefreshRate());
}

TEST (KMeterComponentTests, PaintLinearScaleDoesNotCrash)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setScaleMapping (KMeterComponent::ScaleMapping::linear);
    meter.setBounds (0.0f, 0.0f, 30.0f, 120.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (30, 120);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({ meter.paint (g); });
}

TEST (KMeterComponentTests, PaintWithShowPeakFalseDoesNotCrash)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setShowPeak (false);
    meter.setShowPeakHold (false);
    meter.setBounds (0.0f, 0.0f, 30.0f, 120.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (30, 120);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({ meter.paint (g); });
}
