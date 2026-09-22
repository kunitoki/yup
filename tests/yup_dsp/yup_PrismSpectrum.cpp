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

using namespace yup;

class PrismSpectrumTests : public ::testing::Test
{
protected:
    static constexpr int numHarmonics = 64;

    using Shape = PrismSpectrum<double>::Shape;

    PrismSpectrum<double> spectrum;
    FourierSeries<double> source { FourierSeries<double>::create (Waveform::sawtooth, numHarmonics) };
    FourierSeries<double> shaped { numHarmonics };

    void SetUp() override { spectrum.prepare (numHarmonics); }

    static double sumOfMagnitudes (const FourierSeries<double>& series) noexcept
    {
        auto sum = 0.0;

        for (int harmonic = 1; harmonic <= series.getNumHarmonics(); ++harmonic)
            sum += series.getMagnitude (harmonic);

        return sum;
    }

    /** Every shape the sweeping tests walk.

        A full cross product of nine controls would be enormous, so the core four are
        gridded and the modifiers are then swept over a fixed base. Out-of-range values
        are deliberate: the clamps are part of what is being tested.
    */
    static std::vector<Shape> shapeGrid()
    {
        std::vector<Shape> shapes;

        for (const auto spacing : { 0.1, 0.5, 1.0, 3.0, 12.0 })
            for (const auto dispersion : { 0.0, 0.3, 0.5, 1.0 })
                for (const auto squeeze : { 0.0, 0.02, 0.17, 0.5 })
                    for (const auto squash : { 0.05, 0.5, 1.0, 2.0, 6.0 })
                        shapes.push_back ({ spacing, dispersion, squeeze, squash });

        for (const auto tilt : { -6.0, -1.0, 0.0, 1.0, 6.0 })
            for (const auto oddEven : { -1.0, 0.0, 0.5, 1.0, 2.0 })
                for (const auto formant : { -6.0, 0.0, 2.5, 6.0 })
                    for (const auto position : { 0.0, 2.0, 9.0 })
                        for (const auto scatter : { 0.0, 0.4, 1.0, 3.0 })
                            shapes.push_back ({ 1.5, 0.4, 0.1, 1.2, tilt, oddEven, formant, position, scatter });

        return shapes;
    }
};

TEST_F (PrismSpectrumTests, PrecomputedLogTablesMatchTheStandardLibrary)
{
    // The tables exist only to delete a log2 per harmonic, so they have to be exact.
    FourierSeries<double> impulse { numHarmonics };
    FourierSeries<double> rotated { numHarmonics };

    for (int harmonic = 1; harmonic <= numHarmonics; ++harmonic)
    {
        impulse.clear();
        impulse.setHarmonic (harmonic, 1.0, 0.0);

        // dispersion = 1 rotates harmonic h by 0.5 * log2 (h) ^ 2, and the ridge and
        // normalization stages cancel for a lone harmonic, so the angle is readable.
        spectrum.process (impulse, rotated, { 1.0, 1.0, 0.0, 1.0 }, 0.0);

        const auto u = std::log2 (static_cast<double> (harmonic));
        const auto expected = 0.5 * u * u;

        EXPECT_NEAR (std::cos (expected), rotated.getCosine (harmonic), 1e-15);
        EXPECT_NEAR (std::sin (expected), rotated.getSine (harmonic), 1e-15);
    }
}

TEST_F (PrismSpectrumTests, NeverCreatesAHarmonicAboveTheSourceLimit)
{
    // The whole antialiasing claim: every stage scales or rotates, none synthesizes.
    source.clear();
    for (int harmonic = 1; harmonic <= 9; ++harmonic)
        source.setHarmonic (harmonic, 0.3 / harmonic, 0.7 / harmonic);

    for (const auto& shape : shapeGrid())
    {
        for (const auto color : { 0.0, 0.31, 0.5, 0.87, 1.0 })
        {
            spectrum.process (source, shaped, shape, color);

            for (int harmonic = 10; harmonic <= numHarmonics; ++harmonic)
            {
                EXPECT_EQ (0.0, shaped.getCosine (harmonic));
                EXPECT_EQ (0.0, shaped.getSine (harmonic));
            }

            EXPECT_EQ (0.0, shaped.getDC());
        }
    }
}

TEST_F (PrismSpectrumTests, PreservesTheSumOfMagnitudes)
{
    const auto expected = sumOfMagnitudes (source);

    for (const auto& shape : shapeGrid())
    {
        for (const auto color : { 0.0, 0.4, 0.75 })
        {
            spectrum.process (source, shaped, shape, color);

            EXPECT_NEAR (expected, sumOfMagnitudes (shaped), 1e-9 * expected);
        }
    }
}

TEST_F (PrismSpectrumTests, AllZeroAndSparseSourcesDoNotProduceNonFiniteCoefficients)
{
    // A sparse source can be annihilated outright - harmonic 2 alone at squeeze 0.5 -
    // and the normalization must not divide by the resulting zero.
    FourierSeries<double> sparse { numHarmonics };

    for (const auto& shape : shapeGrid())
    {
        for (const auto harmonic : { 0, 1, 2, 4 })
        {
            sparse.clear();
            if (harmonic > 0)
                sparse.setHarmonic (harmonic, 1.0, 0.0);

            spectrum.process (sparse, shaped, shape, 0.25);

            for (int index = 1; index <= numHarmonics; ++index)
            {
                EXPECT_TRUE (std::isfinite (shaped.getCosine (index)));
                EXPECT_TRUE (std::isfinite (shaped.getSine (index)));
            }
        }
    }
}

TEST_F (PrismSpectrumTests, BypassShapeLeavesTheFundamentalDominant)
{
    // A ridge period far wider than the source's bandwidth is flat over it.
    spectrum.process (source, shaped, { 8.0, 0.5, 0.0, 1.0 }, 0.0);

    for (int harmonic = 2; harmonic <= numHarmonics; ++harmonic)
        EXPECT_LT (shaped.getMagnitude (harmonic), shaped.getMagnitude (1));

    EXPECT_NEAR (sumOfMagnitudes (source), sumOfMagnitudes (shaped), 1e-12);
}

TEST_F (PrismSpectrumTests, SquashCompressesOrExpandsTheHarmonicRange)
{
    const auto spread = [this] (double squash)
    {
        spectrum.process (source, shaped, { 8.0, 0.5, 0.0, squash }, 0.0);

        auto weakest = std::numeric_limits<double>::max();
        auto strongest = 0.0;

        for (int harmonic = 1; harmonic <= numHarmonics; ++harmonic)
        {
            weakest = jmin (weakest, shaped.getMagnitude (harmonic));
            strongest = jmax (strongest, shaped.getMagnitude (harmonic));
        }

        return weakest / strongest;
    };

    const auto neutral = spread (1.0);

    EXPECT_GT (spread (0.5), neutral);
    EXPECT_LT (spread (2.0), neutral);
}

TEST_F (PrismSpectrumTests, SqueezeAtHalfRemovesEveryEvenHarmonic)
{
    // 1 - cis (-pi h) is zero for even h and 2 for odd, the square-from-saw identity.
    spectrum.process (source, shaped, { 8.0, 0.5, 0.5, 1.0 }, 0.0);

    for (int harmonic = 2; harmonic <= numHarmonics; harmonic += 2)
    {
        EXPECT_NEAR (0.0, shaped.getCosine (harmonic), 1e-14);
        EXPECT_NEAR (0.0, shaped.getSine (harmonic), 1e-14);
    }

    for (int harmonic = 1; harmonic <= numHarmonics; harmonic += 2)
        EXPECT_GT (shaped.getMagnitude (harmonic), 0.0);
}

TEST_F (PrismSpectrumTests, NeutralShapeOnALoneHarmonicIsTheIdentity)
{
    // Every stage but the ridge is off, and the ridge is a real scalar the
    // normalization undoes exactly, so a single harmonic must come back untouched.
    FourierSeries<double> lone { numHarmonics };
    lone.setHarmonic (5, 0.6, -0.8);

    spectrum.process (lone, shaped, { 1.0, 0.5, 0.0, 1.0 }, 0.3);

    EXPECT_NEAR (0.6, shaped.getCosine (5), 1e-15);
    EXPECT_NEAR (-0.8, shaped.getSine (5), 1e-15);
}

TEST_F (PrismSpectrumTests, SqueezeRotatesTheSameLoneHarmonicAwayFromTheIdentity)
{
    FourierSeries<double> lone { numHarmonics };
    lone.setHarmonic (5, 0.6, -0.8);

    spectrum.process (lone, shaped, { 1.0, 0.5, 0.17, 1.0 }, 0.3);

    // 1 - cis (-2 pi h w) has unit magnitude nowhere, so normalization restores the
    // magnitude and leaves the rotation, which is what makes squeeze audible at all.
    EXPECT_NEAR (1.0, shaped.getMagnitude (5), 1e-14);
    EXPECT_GT (std::abs (shaped.getCosine (5) - 0.6), 1e-3);
}

TEST_F (PrismSpectrumTests, SqueezePassesContinuouslyThroughZero)
{
    // The raw factor tends to a differentiator rather than to 1 as the width falls, so
    // the depth is faded in instead. Without that fade, stepping off zero is a jump:
    // the unfaded factor at w = 0.0005 reweights harmonic h by h and rotates it by 90
    // degrees, which renormalization then scales straight back up to full level.
    FourierSeries<double> bypassed { numHarmonics };
    spectrum.process (source, bypassed, { 2.0, 0.5, 0.0, 1.0 }, 0.3);

    for (const auto squeeze : { 0.0001, 0.0005, 0.001 })
    {
        spectrum.process (source, shaped, { 2.0, 0.5, squeeze, 1.0 }, 0.3);

        auto difference = 0.0;

        for (int harmonic = 1; harmonic <= numHarmonics; ++harmonic)
            difference += std::abs (shaped.getCosine (harmonic) - bypassed.getCosine (harmonic))
                        + std::abs (shaped.getSine (harmonic) - bypassed.getSine (harmonic));

        // Depth reaches only squeeze/squeezeFadeWidth here, so the whole series should
        // still sit within a couple of percent of the bypass.
        EXPECT_LT (difference, 0.05 * sumOfMagnitudes (source)) << "at squeeze " << squeeze;
    }
}

TEST_F (PrismSpectrumTests, HarmonicsBeyondThePreparedCountAreDropped)
{
    PrismSpectrum<double> narrow;
    narrow.prepare (8);

    narrow.process (source, shaped, { 1.0, 0.5, 0.0, 1.0 }, 0.0);

    for (int harmonic = 9; harmonic <= numHarmonics; ++harmonic)
        EXPECT_EQ (0.0, shaped.getMagnitude (harmonic));

    EXPECT_GT (shaped.getMagnitude (1), 0.0);
}

TEST_F (PrismSpectrumTests, TiltSlopesTheSpectrumAndIsFlatAtZero)
{
    const auto ratioOfHarmonics = [this] (double tilt)
    {
        Shape shape;
        shape.ridgeSpacing = 8.0;
        shape.tilt = tilt;
        spectrum.process (source, shaped, shape, 0.0);

        return shaped.getMagnitude (32) / shaped.getMagnitude (2);
    };

    const auto flat = ratioOfHarmonics (0.0);

    EXPECT_LT (ratioOfHarmonics (1.0), flat);
    EXPECT_GT (ratioOfHarmonics (-1.0), flat);

    // Four octaves apart, one gain octave per harmonic octave is a factor of sixteen.
    EXPECT_NEAR (flat / 16.0, ratioOfHarmonics (1.0), 1e-12);
}

TEST_F (PrismSpectrumTests, OddEvenBalanceIsolatesEitherHalfAndIsNeutralAtTheCenter)
{
    Shape shape;
    shape.ridgeSpacing = 8.0;

    shape.oddEven = 0.0;
    spectrum.process (source, shaped, shape, 0.0);
    for (int harmonic = 2; harmonic <= numHarmonics; harmonic += 2)
        EXPECT_EQ (0.0, shaped.getMagnitude (harmonic));
    EXPECT_GT (shaped.getMagnitude (1), 0.0);

    shape.oddEven = 1.0;
    spectrum.process (source, shaped, shape, 0.0);
    for (int harmonic = 1; harmonic <= numHarmonics; harmonic += 2)
        EXPECT_EQ (0.0, shaped.getMagnitude (harmonic));
    EXPECT_GT (shaped.getMagnitude (2), 0.0);

    shape.oddEven = 0.5;
    spectrum.process (source, shaped, shape, 0.0);
    FourierSeries<double> neutral { numHarmonics };
    spectrum.process (source, neutral, { 8.0, 0.5, 0.0, 1.0 }, 0.0);
    for (int harmonic = 1; harmonic <= numHarmonics; ++harmonic)
        EXPECT_DOUBLE_EQ (neutral.getCosine (harmonic), shaped.getCosine (harmonic));
}

TEST_F (PrismSpectrumTests, FormantPeaksAtItsPositionAndBypassesAtZero)
{
    Shape shape;
    shape.ridgeSpacing = 8.0;
    shape.formantPosition = 3.0; // harmonic 8
    shape.formant = 3.0;

    FourierSeries<double> withoutFormant { numHarmonics };
    spectrum.process (source, withoutFormant, { 8.0, 0.5, 0.0, 1.0 }, 0.0);
    spectrum.process (source, shaped, shape, 0.0);

    // Relative to the unresonated spectrum, harmonic 8 must gain on its neighbours.
    const auto boosted = shaped.getMagnitude (8) / shaped.getMagnitude (1);
    const auto plain = withoutFormant.getMagnitude (8) / withoutFormant.getMagnitude (1);
    EXPECT_GT (boosted, plain);

    shape.formant = -3.0;
    spectrum.process (source, shaped, shape, 0.0);
    EXPECT_LT (shaped.getMagnitude (8) / shaped.getMagnitude (1), plain);

    shape.formant = 0.0;
    spectrum.process (source, shaped, shape, 0.0);
    for (int harmonic = 1; harmonic <= numHarmonics; ++harmonic)
        EXPECT_DOUBLE_EQ (withoutFormant.getCosine (harmonic), shaped.getCosine (harmonic));
}

TEST_F (PrismSpectrumTests, ScatterRotatesWithoutChangingAnyMagnitude)
{
    Shape shape;
    shape.ridgeSpacing = 8.0;

    FourierSeries<double> unscattered { numHarmonics };
    spectrum.process (source, unscattered, shape, 0.0);

    shape.scatter = 1.0;
    spectrum.process (source, shaped, shape, 0.0);

    auto rotated = 0;

    for (int harmonic = 1; harmonic <= numHarmonics; ++harmonic)
    {
        EXPECT_NEAR (unscattered.getMagnitude (harmonic), shaped.getMagnitude (harmonic), 1e-12);

        if (std::abs (unscattered.getSine (harmonic) - shaped.getSine (harmonic)) > 1e-6)
            ++rotated;
    }

    EXPECT_GT (rotated, numHarmonics / 2);
}

TEST_F (PrismSpectrumTests, ScatterIsReproducibleAcrossInstances)
{
    // The angles are a property of the shape, so two shapers must agree - otherwise
    // re-deriving a block would re-scatter and sound like noise.
    PrismSpectrum<double> other;
    other.prepare (numHarmonics);

    FourierSeries<double> otherResult { numHarmonics };
    const Shape shape { 8.0, 0.5, 0.0, 1.0, 0.0, 0.5, 0.0, 2.0, 0.8 };

    spectrum.process (source, shaped, shape, 0.2);
    other.process (source, otherResult, shape, 0.2);

    for (int harmonic = 1; harmonic <= numHarmonics; ++harmonic)
        EXPECT_DOUBLE_EQ (shaped.getSine (harmonic), otherResult.getSine (harmonic));
}
