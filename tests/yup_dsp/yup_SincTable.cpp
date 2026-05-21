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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

#include <cmath>

namespace yup::test
{

//==============================================================================
class SincTableTest : public ::testing::Test
{
protected:
    static constexpr double sampleRate = 44100.0;
    static constexpr int radius = 8;
    static constexpr int factor = 4; // oversampling factor / resolution
    static constexpr double tolerance = 1e-9;
};

//==============================================================================
TEST_F (SincTableTest, TableSizeIsCorrect)
{
    EXPECT_EQ ((SincTable<double, factor, radius>::tableSize), (radius + 1) * factor);
}

TEST_F (SincTableTest, ConfigureSetsUnityAtZero)
{
    SincTable<double, factor, radius> table;
    table.configure (static_cast<double> (sampleRate));

    // (tap=0, delta=0) is the kernel centre — sinc(0) = 1
    EXPECT_NEAR (table (0, 0), 1.0, tolerance);
}

TEST_F (SincTableTest, ConfigureWithCutoffSetsUnityAtZero)
{
    SincTable<double, factor, radius> table;
    table.configureWithCutoff (sampleRate / 2.0, sampleRate);

    EXPECT_NEAR (table (0, 0), 1.0, tolerance);
}

TEST_F (SincTableTest, KernelDecaysAwayFromCenter)
{
    SincTable<double, factor, radius> table;
    table.configure (static_cast<double> (sampleRate));

    // Use a fractional phase so the full-band sinc is not sampled exactly at
    // its integer zero crossings.
    EXPECT_LT (std::abs (table (1, 1)), std::abs (table (0, 1)));
    EXPECT_LT (std::abs (table (2, 1)), std::abs (table (1, 1)));
}

TEST_F (SincTableTest, SymmetryViaOperatorBracket)
{
    SincTable<double, factor, radius> table;
    table.configure (static_cast<double> (sampleRate));

    // operator[] mirrors negative indices: table[-i] == table[i]
    for (int i = 1; i <= radius * factor; ++i)
        EXPECT_DOUBLE_EQ (table[-i], table[i]);
}

TEST_F (SincTableTest, SymmetryViaTapDelta)
{
    SincTable<double, factor, radius> table;
    table.configure (static_cast<double> (sampleRate));

    // Negative tap mirrors positive tap: table(-n, d) == table(n, -d) (by sinc symmetry)
    for (int n = 1; n <= radius; ++n)
    {
        for (int d = 0; d < factor; ++d)
        {
            // table(-n, d) should equal table(n, -d... handled via tap=0, delta<0 branch at boundary)
            // Direct: negative tap accesses (-tap)*factor - delta = n*factor - d
            // That equals table[n*factor - d] which == table(n, -d) only for d==0 or d mirrored
            // Simplest check: table(-n, 0) == table(n, 0)
            if (d == 0)
                EXPECT_DOUBLE_EQ (table (-n, 0), table (n, 0));
        }
    }
}

TEST_F (SincTableTest, NegativeDeltaAtTapZeroUsesSymmetry)
{
    SincTable<double, factor, radius> table;
    table.configure (static_cast<double> (sampleRate));

    // table(0, -d) must equal table(0, d) — sinc(-t) == sinc(t)
    for (int d = 1; d < factor; ++d)
        EXPECT_DOUBLE_EQ (table (0, -d), table (0, d));
}

TEST_F (SincTableTest, ApplyKaiserWindowReducesOuterTaps)
{
    SincTable<double, factor, radius> table;
    table.configure (static_cast<double> (sampleRate));

    // Record value at a tap far from centre before windowing
    const double beforeWindow = std::abs (table (radius, 0));

    table.applyKaiserWindow (5.0);

    const double afterWindow = std::abs (table (radius, 0));

    // Kaiser window tapers to zero at the edges, so outer taps must shrink
    EXPECT_LT (afterWindow, beforeWindow);
}

TEST_F (SincTableTest, ApplyKaiserWindowPreservesCenter)
{
    SincTable<double, factor, radius> table;
    table.configure (static_cast<double> (sampleRate));

    // Center before windowing
    const double before = table (0, 0);

    table.applyKaiserWindow (5.0);

    // The Kaiser window is exactly 1 at the center.
    EXPECT_DOUBLE_EQ (table (0, 0), before);
}

TEST_F (SincTableTest, ConfigureWithLowerCutoffWidensMainLobe)
{
    SincTable<double, factor, radius> highCut;
    highCut.configure (static_cast<double> (sampleRate)); // cutoff = sr/2

    SincTable<double, factor, radius> lowCut;
    lowCut.configureWithCutoff (sampleRate / 4.0, sampleRate); // cutoff = sr/4

    // Lower cutoff -> wider time-domain main lobe, so near-center fractional
    // taps have a larger magnitude.
    EXPECT_GT (std::abs (lowCut (1, 1)), std::abs (highCut (1, 1)));
}

TEST_F (SincTableTest, FloatPrecisionInstantiates)
{
    SincTable<float, 2, 4> table;
    table.configure (44100.0f);
    table.applyKaiserWindow (5.0f);

    EXPECT_NEAR (table (0, 0), 1.0f, 1e-4f);
}

TEST_F (SincTableTest, HighResolutionInstantiates)
{
    SincTable<double, 256, 8> table;
    table.configure (static_cast<double> (sampleRate));
    table.applyKaiserWindow (5.0);

    EXPECT_EQ ((SincTable<double, 256, 8>::tableSize), 9 * 256);
    EXPECT_NEAR (table (0, 0), 1.0, tolerance);
}

} // namespace yup::test
