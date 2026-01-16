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
