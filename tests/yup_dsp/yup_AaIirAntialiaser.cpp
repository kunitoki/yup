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

#include "yup_core/yup_core.h"
#include "yup_dsp/yup_dsp.h"

#include <gtest/gtest.h>

#include <cmath>
#include <numeric>
#include <vector>

template <typename FloatType>
class AaIirAntialiaserTests : public ::testing::Test
{
public:
    using Clipper = yup::HardClipper<FloatType>;
    using Config = typename Clipper::PoleConfig;

    static constexpr double kSampleRate = 44100.0;
    static constexpr int kBlockSize = 512;
    static constexpr FloatType kTolerance = FloatType (1e-6);

    Clipper clipper;

    void SetUp() override
    {
        clipper = Clipper {};
        clipper.prepare (kSampleRate, kBlockSize);
    }

    //==============================================================================
    void testHardClipperTraitsIdentity()
    {
        EXPECT_NEAR (yup::HardClipperTraits::f (FloatType (0.5)), FloatType (0.5), kTolerance);
        EXPECT_NEAR (yup::HardClipperTraits::f (FloatType (-0.3)), FloatType (-0.3), kTolerance);
        EXPECT_NEAR (yup::HardClipperTraits::f (FloatType (0.0)), FloatType (0.0), kTolerance);
        EXPECT_NEAR (yup::HardClipperTraits::f (FloatType (1.0)), FloatType (1.0), kTolerance);
        EXPECT_NEAR (yup::HardClipperTraits::f (FloatType (-1.0)), FloatType (-1.0), kTolerance);
    }

    void testHardClipperTraitsSaturation()
    {
        EXPECT_NEAR (yup::HardClipperTraits::f (FloatType (2.0)), FloatType (1.0), kTolerance);
        EXPECT_NEAR (yup::HardClipperTraits::f (FloatType (-1.5)), FloatType (-1.0), kTolerance);
        EXPECT_NEAR (yup::HardClipperTraits::f (FloatType (10.0)), FloatType (1.0), kTolerance);
    }

    void testDefaultButterworthConfig()
    {
        const auto config = Clipper::makeButterworthOrder2();

        EXPECT_EQ (config.complexPairs.size(), std::size_t (1));
        EXPECT_EQ (config.realPoles.size(), std::size_t (0));

        // Poles at wc * (-1 +/- j) / sqrt(2), wc = 2*pi*0.45
        const auto wc = FloatType (2) * yup::MathConstants<FloatType>::pi * FloatType (0.45);
        const auto sq = std::sqrt (FloatType (2));

        const auto& pair = config.complexPairs[0];
        EXPECT_NEAR (pair.poleReal, -wc / sq, FloatType (1e-5));
        EXPECT_NEAR (pair.poleImag, +wc / sq, FloatType (1e-5));
        EXPECT_NEAR (pair.residueReal, FloatType (0), FloatType (1e-5));
        EXPECT_NEAR (pair.residueImag, -wc / sq, FloatType (1e-5));
    }

    void testResetClearsState()
    {
        // Push a large signal through to build up significant IIR state.
        for (int i = 0; i < 200; ++i)
            clipper.processSample (FloatType (5.0) * std::sin (FloatType (i) * FloatType (0.1)));

        clipper.reset();

        // After reset the state is zero, so the first output must equal what a
        // fresh instance produces for the same input — verify by comparing to
        // an independent freshly-prepared clipper.
        Clipper fresh;
        fresh.prepare (kSampleRate, kBlockSize);

        const FloatType testInput = FloatType (0.1);
        const FloatType afterReset = clipper.processSample (testInput);
        const FloatType fromFresh = fresh.processSample (testInput);

        EXPECT_NEAR (afterReset, fromFresh, kTolerance);
    }

    void testLowAmplitudeIsApproximatelyLinear()
    {
        // For |x| << 1, f(x) = x so the antialiased output should closely track the input.
        // The IIR filter introduces phase shift but not amplitude error at DC.
        // Feed a DC value and wait for the IIR to settle, then check convergence.
        const FloatType dc = FloatType (0.1);

        for (int i = 0; i < 2000; ++i)
            clipper.processSample (dc);

        // After settling the output should be close to dc (the filter has unity DC gain
        // for a proper LP filter, and the hard clipper is linear for |x| < 1).
        const FloatType out = clipper.processSample (dc);
        EXPECT_NEAR (out, dc, FloatType (0.02));
    }

    void testOutputIsBoundedForLargeInput()
    {
        // The antialiased output should remain bounded even for very large inputs.
        bool bounded = true;
        for (int i = 0; i < 500; ++i)
        {
            const FloatType x = FloatType (10) * std::sin (FloatType (i) * FloatType (0.3));
            const FloatType out = clipper.processSample (x);
            if (std::abs (out) > FloatType (2))
                bounded = false;
        }
        EXPECT_TRUE (bounded);
    }

    void testProcessBlockMatchesProcessSample()
    {
        // Both paths must produce bit-identical results.
        constexpr int N = 64;
        FloatType input[N], outBlock[N], outSample[N];

        for (int i = 0; i < N; ++i)
            input[i] = FloatType (0.3) * std::sin (FloatType (i) * FloatType (0.2));

        clipper.reset();
        clipper.processBlock (input, outBlock, N);

        clipper.reset();
        for (int i = 0; i < N; ++i)
            outSample[i] = clipper.processSample (input[i]);

        for (int i = 0; i < N; ++i)
            EXPECT_EQ (outBlock[i], outSample[i]);
    }

    void testProcessInPlaceMatchesProcessBlock()
    {
        constexpr int N = 32;
        FloatType input[N], outBlock[N], outInPlace[N];

        for (int i = 0; i < N; ++i)
            input[i] = FloatType (0.5) * std::sin (FloatType (i) * FloatType (0.15));

        clipper.reset();
        clipper.processBlock (input, outBlock, N);

        clipper.reset();
        std::copy (input, input + N, outInPlace);
        clipper.processInPlace (outInPlace, N);

        for (int i = 0; i < N; ++i)
            EXPECT_EQ (outInPlace[i], outBlock[i]);
    }

    void testChebyshevTypeIIOrder10Config()
    {
        const auto config = Clipper::makeChebyshevTypeIIOrder10();

        // 10th-order → 5 complex conjugate pairs, no real poles.
        EXPECT_EQ (config.complexPairs.size(), std::size_t (5));
        EXPECT_EQ (config.realPoles.size(), std::size_t (0));

        // All poles must be stable (Re < 0) and upper-half-plane (Im > 0).
        for (const auto& pair : config.complexPairs)
        {
            EXPECT_LT (pair.poleReal, FloatType (0));
            EXPECT_GT (pair.poleImag, FloatType (0));
        }

        // Constant term must be positive (it equals the biproper gain K).
        EXPECT_GT (config.constantTerm, FloatType (0));
    }

    void testChebyshevTypeIIOrder10ConvergesToDC()
    {
        // Feed a low-amplitude DC signal through the Chebyshev II config and verify
        // that the output converges to the input (unity DC gain, linear region).
        Clipper cheb (Clipper::makeChebyshevTypeIIOrder10());
        cheb.prepare (kSampleRate, kBlockSize);

        const FloatType dc = FloatType (0.1);

        for (int i = 0; i < 5000; ++i)
            cheb.processSample (dc);

        const FloatType out = cheb.processSample (dc);
        EXPECT_NEAR (out, dc, FloatType (0.05));
    }

    void testChebyshevTypeIIOrder10BoundedOutput()
    {
        // With large input the output must remain finite and bounded.
        Clipper cheb (Clipper::makeChebyshevTypeIIOrder10());
        cheb.prepare (kSampleRate, kBlockSize);

        bool bounded = true;
        for (int i = 0; i < 500; ++i)
        {
            const FloatType x = FloatType (10) * std::sin (FloatType (i) * FloatType (0.3));
            const FloatType out = cheb.processSample (x);
            if (! std::isfinite (static_cast<double> (out)) || std::abs (out) > FloatType (3))
                bounded = false;
        }
        EXPECT_TRUE (bounded);
    }

    void testCustomPoleConfigWorks()
    {
        // A first-order real-pole configuration (simple RC lowpass).
        // pole = -pi (cutoff at 0.5*Fs), residue = pi (unity DC gain: A/(-alpha) = 1).
        Config config;
        typename Config::RealPole rp;
        rp.pole = FloatType (-yup::MathConstants<FloatType>::pi);
        rp.residue = FloatType (yup::MathConstants<FloatType>::pi);
        config.realPoles.push_back (rp);

        Clipper custom (config);
        custom.prepare (kSampleRate, kBlockSize);

        // Should not crash and should produce bounded output.
        for (int i = 0; i < 100; ++i)
        {
            const FloatType out = custom.processSample (FloatType (0.5) * std::sin (FloatType (i) * FloatType (0.1)));
            EXPECT_TRUE (std::isfinite (static_cast<double> (out)));
        }
    }
};

using TestTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE (AaIirAntialiaserTests, TestTypes);

TYPED_TEST (AaIirAntialiaserTests, HardClipperTraitsIdentity) { this->testHardClipperTraitsIdentity(); }

TYPED_TEST (AaIirAntialiaserTests, HardClipperTraitsSaturation) { this->testHardClipperTraitsSaturation(); }

TYPED_TEST (AaIirAntialiaserTests, DefaultButterworthConfig) { this->testDefaultButterworthConfig(); }

TYPED_TEST (AaIirAntialiaserTests, ResetClearsState) { this->testResetClearsState(); }

TYPED_TEST (AaIirAntialiaserTests, LowAmplitudeIsApproximatelyLinear) { this->testLowAmplitudeIsApproximatelyLinear(); }

TYPED_TEST (AaIirAntialiaserTests, OutputIsBoundedForLargeInput) { this->testOutputIsBoundedForLargeInput(); }

TYPED_TEST (AaIirAntialiaserTests, ProcessBlockMatchesProcessSample) { this->testProcessBlockMatchesProcessSample(); }

TYPED_TEST (AaIirAntialiaserTests, ProcessInPlaceMatchesProcessBlock) { this->testProcessInPlaceMatchesProcessBlock(); }

TYPED_TEST (AaIirAntialiaserTests, ChebyshevTypeIIOrder10Config) { this->testChebyshevTypeIIOrder10Config(); }

TYPED_TEST (AaIirAntialiaserTests, ChebyshevTypeIIOrder10ConvergesToDC) { this->testChebyshevTypeIIOrder10ConvergesToDC(); }

TYPED_TEST (AaIirAntialiaserTests, ChebyshevTypeIIOrder10BoundedOutput) { this->testChebyshevTypeIIOrder10BoundedOutput(); }

TYPED_TEST (AaIirAntialiaserTests, CustomPoleConfigWorks) { this->testCustomPoleConfigWorks(); }

//==============================================================================
// Mathematical correctness: verify the mean-integral recursion against the
// closed-form from Appendix B of the paper (f(x)=x, first-order real-pole LP).
// These tests are not templated — they require double precision.

namespace
{

// Identity nonlinearity — no breakpoints, no clipping. Used to isolate
// the integral computation from the nonlinear function's piecewise structure.
struct LinearTraits
{
    template <typename T>
    static T f (T x) noexcept
    {
        return x;
    }
};

// Compute Goertzel power estimate at a single frequency for a block of samples.
// Equivalent to |DFT[k]|^2 / N^2 but at one bin, in O(N) time.
double goertzelPower (const std::vector<double>& signal, double freq, double sampleRate)
{
    const int N = static_cast<int> (signal.size());
    const double omega = 2.0 * yup::MathConstants<double>::pi * freq / sampleRate;
    const double coeff = 2.0 * std::cos (omega);
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;

    for (double x : signal)
    {
        s0 = x + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    return (s1 * s1 + s2 * s2 - coeff * s1 * s2) / (static_cast<double> (N) * N);
}

} // namespace

// --- Appendix B ground-truth test -------------------------------------------

TEST (AaIirCorrectnessTests, LinearFunctionMatchesAppendixBFormula)
{
    // Appendix B derives the exact recursion for f(x)=x with H(s)=-α/(s-α):
    //
    //   y_{n+1} = e^α · y_n
    //           − ((e^α − α − 1) / α) · x_{n+1}
    //           − ((α − 1)·e^α + 1) / α · x_n
    //
    // Verify that our AA-IIR implementation reproduces this formula exactly.

    using LinearClipper = yup::AaIirAntialiaser<double, LinearTraits>;

    // First-order LP: H(s) = -α/(s-α), pole α=-1, residue A=-α=1, H(0)=1.
    LinearClipper::PoleConfig config;
    {
        typename LinearClipper::PoleConfig::RealPole rp;
        rp.pole = -1.0;
        rp.residue = 1.0; // -alpha = 1.0
        config.realPoles.push_back (rp);
    }

    LinearClipper aaIir (config);
    aaIir.prepare (44100.0, 512);

    const double alpha = -1.0;
    const double eA = std::exp (alpha);                   // e^{-1}
    const double k1 = (eA - alpha - 1.0) / alpha;         // coeff for x_{n+1}
    const double k2 = ((alpha - 1.0) * eA + 1.0) / alpha; // coeff for x_n

    const double inputs[] = { 0.3, -0.5, 0.7, 0.1, -0.4, 0.2, 0.6, -0.8, 0.0, 0.9 };

    double yRef = 0.0;
    double xPrev = 0.0;

    for (double xCurr : inputs)
    {
        const double yComputed = aaIir.processSample (xCurr);
        yRef = eA * yRef - k1 * xCurr - k2 * xPrev;
        EXPECT_NEAR (yComputed, yRef, 1e-10);
        xPrev = xCurr;
    }
}

// --- Custom nonlinearity test ------------------------------------------------

TEST (AaIirCorrectnessTests, TanhTraitsRequiresNoBreakpoints)
{
    // TanhClipperTraits is smooth — it must work without fillBreakpoints.
    // Verify it compiles, produces bounded output, and converges to tanh(dc) at DC.
    using TanhClipper = yup::AaIirAntialiaser<double, yup::TanhClipperTraits>;

    TanhClipper clipper (TanhClipper::makeChebyshevTypeIIOrder10());
    clipper.prepare (44100.0, 512);

    const double dc = 2.0; // well into saturation: tanh(2) ≈ 0.964

    for (int i = 0; i < 5000; ++i)
        clipper.processSample (dc);

    const double out = clipper.processSample (dc);
    EXPECT_NEAR (out, std::tanh (dc), 0.05);
}

// --- SNR quality comparison --------------------------------------------------

TEST (AaIirQualityTests, ButterworthReducesAliasingVsTrivialClipper)
{
    // Feed a 1 kHz sine at gain 10 (heavy clipping). The trivial clipper generates
    // odd harmonics that fold above Nyquist and alias back into the audible band.
    // For example, the 23 kHz harmonic aliases to 44100-23000 = 21100 Hz — a
    // frequency that is NOT an odd harmonic of 1 kHz.
    //
    // Measure the power at 21100 Hz (a known aliasing target).
    // AA-IIR-1 should have significantly less alias power there.

    constexpr double Fs = 44100.0;
    constexpr double f0 = 1000.0;
    constexpr double gain = 10.0;
    constexpr int N = 8192;
    constexpr int warmup = 2000;
    constexpr double aliasHz = 21100.0; // 44100 - 23000 (alias of 23rd harmonic)

    std::vector<double> input (N + warmup);
    for (int i = 0; i < N + warmup; ++i)
        input[i] = gain * std::sin (2.0 * yup::MathConstants<double>::pi * f0 * i / Fs);

    // Trivial hard clipper
    std::vector<double> trivialOut (N);
    for (int i = 0; i < N + warmup; ++i)
    {
        const double y = std::clamp (input[i], -1.0, 1.0);
        if (i >= warmup)
            trivialOut[i - warmup] = y;
    }

    // AA-IIR-1 (Butterworth order 2)
    yup::HardClipperDouble aaIir1 (yup::HardClipperDouble::makeButterworthOrder2());
    aaIir1.prepare (Fs, N);
    std::vector<double> aaIir1Out (N);
    for (int i = 0; i < warmup; ++i)
        aaIir1.processSample (input[i]);
    for (int i = 0; i < N; ++i)
        aaIir1Out[i] = aaIir1.processSample (input[i + warmup]);

    // AA-IIR-2 (Chebyshev II order 10)
    yup::HardClipperDouble aaIir2 (yup::HardClipperDouble::makeChebyshevTypeIIOrder10());
    aaIir2.prepare (Fs, N);
    std::vector<double> aaIir2Out (N);
    for (int i = 0; i < warmup; ++i)
        aaIir2.processSample (input[i]);
    for (int i = 0; i < N; ++i)
        aaIir2Out[i] = aaIir2.processSample (input[i + warmup]);

    const double trivialAlias = goertzelPower (trivialOut, aliasHz, Fs);
    const double aaIir1Alias = goertzelPower (aaIir1Out, aliasHz, Fs);
    const double aaIir2Alias = goertzelPower (aaIir2Out, aliasHz, Fs);

    // Both AA-IIR methods must have less aliasing power than trivial clipping.
    EXPECT_LT (aaIir1Alias, trivialAlias) << "AA-IIR-1 should reduce aliasing at " << aliasHz << " Hz";
    EXPECT_LT (aaIir2Alias, aaIir1Alias) << "AA-IIR-2 should reduce aliasing more than AA-IIR-1";
}
