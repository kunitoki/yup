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

#include <yup_renderer/yup_renderer.h>

using namespace yup;

TEST (TemporalAABlendWeightTests, StillFramesTakeTheTrueRunningMean)
{
    // Frame n of a still accumulation weighs 1 / (n + 1): once sampleCount frames are in, each of
    // them holds the same share of the history, so rotating noise averages out evenly
    for (int sampleCount : { 4, 16, 64 })
    {
        for (int index = 0; index < sampleCount; ++index)
            EXPECT_FLOAT_EQ (TemporalAA::getBlendWeight (index, sampleCount, false), 1.0f / static_cast<float> (index + 1)) << index << " of " << sampleCount;

        EXPECT_EQ (TemporalAA::getBlendWeight (sampleCount, sampleCount, false), 0.0f);
        EXPECT_EQ (TemporalAA::getBlendWeight (sampleCount + 5, sampleCount, false), 0.0f);
    }
}

TEST (TemporalAABlendWeightTests, ChangingFramesKeepAtLeastATenth)
{
    // While the scene changes, the current frame keeps at least a tenth, so the history fades
    // within a few frames behind moving content; the start of the ramp is the same running mean
    for (int index = 0; index < 16; ++index)
        EXPECT_FLOAT_EQ (TemporalAA::getBlendWeight (index, 16, true), jmax (1.0f / static_cast<float> (index + 1), 0.1f)) << index;

    EXPECT_EQ (TemporalAA::getBlendWeight (16, 16, true), 0.0f);
}
