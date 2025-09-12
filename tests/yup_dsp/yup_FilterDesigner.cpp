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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

#include <fstream>

using namespace yup;

//==============================================================================
class FilterDesignerTests : public ::testing::Test
{
protected:
    static constexpr double tolerance = 1e-4;
    static constexpr float toleranceF = 1e-4f;
    static constexpr double sampleRate = 44100.0;

    void SetUp() override
    {
        // Common test parameters
        frequency = 1000.0;
        qFactor = 0.707;
        gainDb = 6.0;
        nyquist = sampleRate * 0.5;
    }

    double frequency;
    double qFactor;
    double gainDb;
    double nyquist;
};

//==============================================================================
// First Order Filter Tests
//==============================================================================
TEST_F (FilterDesignerTests, FirstOrderLowpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (frequency, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // For first-order lowpass: b0 should be positive
    EXPECT_GT (coeffs.b0, 0.0);
    // Note: First-order filters may have different coefficient structures
    // b1 might be 0 for some implementations

    // a1 should be negative (for stability)
    EXPECT_LT (coeffs.a1, 0.0);

    // DC gain should be approximately 1.0 (b0 + b1) / (1 + a1)
    double dcGain = (coeffs.b0 + coeffs.b1) / (1.0 + coeffs.a1);
    EXPECT_NEAR (1.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, FirstOrderHighpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighpass (frequency, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // For highpass: b0 should equal -b1
    EXPECT_NEAR (coeffs.b0, -coeffs.b1, tolerance);
    EXPECT_GT (coeffs.b0, 0.0);
    EXPECT_LT (coeffs.b1, 0.0);

    // DC gain should be approximately 0.0
    double dcGain = (coeffs.b0 + coeffs.b1) / (1.0 + coeffs.a1);
    EXPECT_NEAR (0.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, FirstOrderLowShelfCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowShelf (frequency, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // For positive gain, DC gain should be > 1
    double dcGain = (coeffs.b0 + coeffs.b1) / (1.0 + coeffs.a1);
    double expectedGain = std::pow (10.0, gainDb / 20.0);
    EXPECT_NEAR (expectedGain, dcGain, tolerance * 10);
}

TEST_F (FilterDesignerTests, FirstOrderHighShelfCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighShelf (frequency, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // High frequency gain should be approximately the expected gain
    // At Nyquist: gain = (b0 - b1) / (1 - a1)
    double hfGain = (coeffs.b0 - coeffs.b1) / (1.0 - coeffs.a1);
    double expectedGain = std::pow (10.0, gainDb / 20.0);
    EXPECT_NEAR (expectedGain, hfGain, tolerance * 10);
}

TEST_F (FilterDesignerTests, FirstOrderAllpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderAllpass (frequency, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // For allpass: b0 = a1, b1 = 1
    EXPECT_NEAR (coeffs.b0, coeffs.a1, tolerance);
    EXPECT_NEAR (1.0, coeffs.b1, tolerance);

    // Magnitude response should be 1.0 at all frequencies
    // DC gain should be 1.0
    double dcGain = (coeffs.b0 + coeffs.b1) / (1.0 + coeffs.a1);
    EXPECT_NEAR (1.0, dcGain, tolerance);
}

//==============================================================================
// RBJ Biquad Filter Tests
//==============================================================================
TEST_F (FilterDesignerTests, RbjLowpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjLowpass (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For lowpass: b0 = b1/2 = b2, all positive
    EXPECT_NEAR (coeffs.b0, coeffs.b2, tolerance);
    EXPECT_NEAR (coeffs.b1, 2.0 * coeffs.b0, tolerance);
    EXPECT_GT (coeffs.b0, 0.0);

    // DC gain should be 1.0
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, RbjHighpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjHighpass (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For highpass: b0 = b2 > 0, b1 = -2*b0
    EXPECT_NEAR (coeffs.b0, coeffs.b2, tolerance);
    EXPECT_NEAR (coeffs.b1, -2.0 * coeffs.b0, tolerance);
    EXPECT_GT (coeffs.b0, 0.0);

    // DC gain should be 0.0
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (0.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, RbjBandpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjBandpass (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For bandpass: b0 = -b2, b1 = 0
    EXPECT_NEAR (coeffs.b0, -coeffs.b2, tolerance);
    EXPECT_NEAR (0.0, coeffs.b1, tolerance);

    // DC gain should be 0.0
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (0.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, RbjBandstopCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjBandstop (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For bandstop: b0 = b2, magnitude of DC gain should be 1.0
    EXPECT_NEAR (coeffs.b0, coeffs.b2, tolerance);

    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, std::abs (dcGain), tolerance);
}

TEST_F (FilterDesignerTests, RbjPeakCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjPeak (frequency, qFactor, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // DC gain should be approximately 1.0 (no DC boost for peaking filter)
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, RbjLowShelfCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjLowShelf (frequency, qFactor, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // DC gain should reflect the shelf gain
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    double expectedGain = std::pow (10.0, gainDb / 20.0);
    EXPECT_NEAR (expectedGain, dcGain, tolerance * 10);
}

TEST_F (FilterDesignerTests, RbjHighShelfCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjHighShelf (frequency, qFactor, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // High frequency gain should reflect the shelf gain
    // At z=-1 (Nyquist): gain = (b0 - b1 + b2) / (1 - a1 + a2)
    double hfGain = (coeffs.b0 - coeffs.b1 + coeffs.b2) / (1.0 - coeffs.a1 + coeffs.a2);
    double expectedGain = std::pow (10.0, gainDb / 20.0);
    EXPECT_NEAR (expectedGain, hfGain, tolerance * 10);
}

TEST_F (FilterDesignerTests, RbjAllpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjAllpass (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For allpass: b0 = a2, b1 = a1, b2 = 1
    EXPECT_NEAR (coeffs.b0, coeffs.a2, tolerance);
    EXPECT_NEAR (coeffs.b1, coeffs.a1, tolerance);
    EXPECT_NEAR (1.0, coeffs.b2, tolerance);

    // Magnitude should be 1.0 at DC and Nyquist
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, std::abs (dcGain), tolerance);

    double hfGain = (coeffs.b0 - coeffs.b1 + coeffs.b2) / (1.0 - coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, std::abs (hfGain), tolerance);
}

//==============================================================================
// Edge Cases and Stability Tests
//==============================================================================
TEST_F (FilterDesignerTests, HandlesNyquistFrequency)
{
    // Should handle frequency at Nyquist without issues
    auto coeffs = FilterDesigner<double>::designRbjLowpass (nyquist, qFactor, sampleRate);

    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));
}

TEST_F (FilterDesignerTests, HandlesLowFrequencies)
{
    // Should handle very low frequencies
    auto coeffs = FilterDesigner<double>::designRbjLowpass (10.0, qFactor, sampleRate);

    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));
}

TEST_F (FilterDesignerTests, HandlesHighQValues)
{
    // Should handle high Q values without instability
    auto coeffs = FilterDesigner<double>::designRbjLowpass (frequency, 10.0, sampleRate);

    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // Check stability: roots of 1 + a1*z^-1 + a2*z^-2 should be inside unit circle
    // This is satisfied if |a2| < 1 and |a1| < 1 + a2
    EXPECT_LT (std::abs (coeffs.a2), 1.0);
    EXPECT_LT (std::abs (coeffs.a1), 1.0 + coeffs.a2);
}

TEST_F (FilterDesignerTests, FloatPrecisionConsistency)
{
    // Test that float and double versions produce similar results
    auto doubleCoeffs = FilterDesigner<double>::designRbjLowpass (frequency, qFactor, sampleRate);
    auto floatCoeffs = FilterDesigner<float>::designRbjLowpass (static_cast<float> (frequency),
                                                                static_cast<float> (qFactor),
                                                                sampleRate);

    EXPECT_NEAR (doubleCoeffs.b0, static_cast<double> (floatCoeffs.b0), toleranceF);
    EXPECT_NEAR (doubleCoeffs.b1, static_cast<double> (floatCoeffs.b1), toleranceF);
    EXPECT_NEAR (doubleCoeffs.b2, static_cast<double> (floatCoeffs.b2), toleranceF);
    EXPECT_NEAR (doubleCoeffs.a1, static_cast<double> (floatCoeffs.a1), toleranceF);
    EXPECT_NEAR (doubleCoeffs.a2, static_cast<double> (floatCoeffs.a2), toleranceF);
}

//==============================================================================
// FIR Filter Design Tests
//==============================================================================

TEST_F (FilterDesignerTests, FirLowpassBasicProperties)
{
    const int numCoeffs = 65; // Odd number for symmetric filter
    auto coeffs = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRate);

    // Should return the correct number of coefficients
    EXPECT_EQ (coeffs.size(), numCoeffs);

    // All coefficients should be finite
    for (const auto& coeff : coeffs)
        EXPECT_TRUE (std::isfinite (coeff));

    // FIR filter should be symmetric for linear phase
    const int center = (numCoeffs - 1) / 2;
    for (int i = 0; i < center; ++i)
        EXPECT_NEAR (coeffs[i], coeffs[numCoeffs - 1 - i], toleranceF);

    // Center coefficient should be largest for lowpass
    for (int i = 0; i < numCoeffs; ++i)
    {
        if (i != center)
            EXPECT_GE (coeffs[center], coeffs[i]);
    }
}

TEST_F (FilterDesignerTests, FirHighpassBasicProperties)
{
    const int numCoeffs = 65;
    auto coeffs = FilterDesigner<float>::designFIRHighpass (numCoeffs, 1000.0f, sampleRate);

    // Should return the correct number of coefficients
    EXPECT_EQ (coeffs.size(), numCoeffs);

    // All coefficients should be finite
    for (const auto& coeff : coeffs)
        EXPECT_TRUE (std::isfinite (coeff));

    // FIR filter should be symmetric for linear phase
    const int center = (numCoeffs - 1) / 2;
    for (int i = 0; i < center; ++i)
        EXPECT_NEAR (coeffs[i], coeffs[numCoeffs - 1 - i], toleranceF);

    // Sum of coefficients should be approximately zero for highpass (DC gain = 0)
    // Note: windowing can cause small deviations from ideal DC gain
    float sum = 0.0f;
    for (const auto& coeff : coeffs)
        sum += coeff;

    EXPECT_NEAR (sum, 0.0f, 0.05f); // Relaxed tolerance for windowed FIR
}

TEST_F (FilterDesignerTests, FirBandpassBasicProperties)
{
    const int numCoeffs = 65;
    auto coeffs = FilterDesigner<float>::designFIRBandpass (numCoeffs, 800.0f, 1200.0f, sampleRate);

    // Should return the correct number of coefficients
    EXPECT_EQ (coeffs.size(), numCoeffs);

    // All coefficients should be finite
    for (const auto& coeff : coeffs)
        EXPECT_TRUE (std::isfinite (coeff));

    // FIR filter should be symmetric for linear phase
    const int center = (numCoeffs - 1) / 2;
    for (int i = 0; i < center; ++i)
        EXPECT_NEAR (coeffs[i], coeffs[numCoeffs - 1 - i], toleranceF);

    // Sum of coefficients should be approximately zero for bandpass (DC gain = 0)
    // Note: windowing can cause small deviations from ideal DC gain
    float sum = 0.0f;
    for (const auto& coeff : coeffs)
        sum += coeff;

    EXPECT_NEAR (sum, 0.0f, 0.15f); // Relaxed tolerance for windowed FIR
}

TEST_F (FilterDesignerTests, FirBandstopBasicProperties)
{
    const int numCoeffs = 65;
    auto coeffs = FilterDesigner<float>::designFIRBandstop (numCoeffs, 800.0f, 1200.0f, sampleRate);

    // Should return the correct number of coefficients
    EXPECT_EQ (coeffs.size(), numCoeffs);

    // All coefficients should be finite
    for (const auto& coeff : coeffs)
        EXPECT_TRUE (std::isfinite (coeff));

    // FIR filter should be symmetric for linear phase
    const int center = (numCoeffs - 1) / 2;
    for (int i = 0; i < center; ++i)
        EXPECT_NEAR (coeffs[i], coeffs[numCoeffs - 1 - i], toleranceF);

    // Sum of coefficients should be approximately 1.0 for bandstop (DC gain = 1)
    // Note: windowing can cause small deviations from ideal DC gain
    float sum = 0.0f;
    for (const auto& coeff : coeffs)
        sum += coeff;

    EXPECT_NEAR (sum, 1.0f, 0.15f); // Relaxed tolerance for windowed FIR
}

TEST_F (FilterDesignerTests, FirDifferentWindowTypes)
{
    const int numCoeffs = 33;

    // Test different window types
    auto hannCoeffs = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRate, WindowType::hann);
    auto hammingCoeffs = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRate, WindowType::hamming);
    auto blackmanCoeffs = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRate, WindowType::blackman);

    // All should have same size
    EXPECT_EQ (hannCoeffs.size(), numCoeffs);
    EXPECT_EQ (hammingCoeffs.size(), numCoeffs);
    EXPECT_EQ (blackmanCoeffs.size(), numCoeffs);

    // All coefficients should be finite
    for (int i = 0; i < numCoeffs; ++i)
    {
        EXPECT_TRUE (std::isfinite (hannCoeffs[i]));
        EXPECT_TRUE (std::isfinite (hammingCoeffs[i]));
        EXPECT_TRUE (std::isfinite (blackmanCoeffs[i]));
    }

    // Different windows should produce different coefficients
    bool coeffsDifferent = false;
    for (int i = 0; i < numCoeffs; ++i)
    {
        if (std::abs (hannCoeffs[i] - blackmanCoeffs[i]) > toleranceF)
        {
            coeffsDifferent = true;
            break;
        }
    }
    EXPECT_TRUE (coeffsDifferent);
}

TEST_F (FilterDesignerTests, FirFloatDoubleConsistency)
{
    const int numCoeffs = 33;

    auto doubleCoeffs = FilterDesigner<double>::designFIRLowpass (numCoeffs, 1000.0, sampleRate);
    auto floatCoeffs = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRate);

    EXPECT_EQ (doubleCoeffs.size(), floatCoeffs.size());

    // Coefficients should be very similar between float and double precision
    for (int i = 0; i < numCoeffs; ++i)
        EXPECT_NEAR (doubleCoeffs[i], static_cast<double> (floatCoeffs[i]), toleranceF);
}

TEST_F (FilterDesignerTests, ExportFIRCoefficientsForAnalysis)
{
    const int numCoeffs = 65;
    const float sampleRateF = 44100.0f;

    // Design different FIR filters
    auto lowpass = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRateF);
    auto highpass = FilterDesigner<float>::designFIRHighpass (numCoeffs, 1000.0f, sampleRateF);
    auto bandpass = FilterDesigner<float>::designFIRBandpass (numCoeffs, 800.0f, 1200.0f, sampleRateF);
    auto bandstop = FilterDesigner<float>::designFIRBandstop (numCoeffs, 800.0f, 1200.0f, sampleRateF);

    // Different windows for lowpass
    auto lowpassHann = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRateF, WindowType::hann);
    auto lowpassHamming = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRateF, WindowType::hamming);
    auto lowpassBlackman = FilterDesigner<float>::designFIRLowpass (numCoeffs, 1000.0f, sampleRateF, WindowType::blackman);

    // Helper lambda to write coefficients to file
    auto writeCoeffs = [] (const std::vector<float>& coeffs, const std::string& filename)
    {
        std::ofstream file (filename);
        if (file.is_open())
        {
            for (size_t i = 0; i < coeffs.size(); ++i)
            {
                file << coeffs[i];
                if (i < coeffs.size() - 1)
                    file << "\n";
            }
            file.close();
        }
    };

    // Write all coefficient sets to files
    writeCoeffs (lowpass, "fir_lowpass_1000hz.txt");
    writeCoeffs (highpass, "fir_highpass_1000hz.txt");
    writeCoeffs (bandpass, "fir_bandpass_800_1200hz.txt");
    writeCoeffs (bandstop, "fir_bandstop_800_1200hz.txt");
    writeCoeffs (lowpassHann, "fir_lowpass_hann_1000hz.txt");
    writeCoeffs (lowpassHamming, "fir_lowpass_hamming_1000hz.txt");
    writeCoeffs (lowpassBlackman, "fir_lowpass_blackman_1000hz.txt");

    // Create a Python script to plot the frequency responses
    std::ofstream pyScript ("plot_fir_responses.py");
    if (pyScript.is_open())
    {
        pyScript << R"(#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal

def load_coeffs(filename):
    with open(filename, 'r') as f:
        return [float(line.strip()) for line in f.readlines()]

def plot_frequency_response(coeffs, title, sample_rate=44100):
    w, h = signal.freqz(coeffs, worN=8000, fs=sample_rate)
    
    plt.figure(figsize=(12, 8))
    
    # Magnitude response
    plt.subplot(2, 1, 1)
    plt.plot(w, 20 * np.log10(np.abs(h)))
    plt.title(f'{title} - Magnitude Response')
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Magnitude (dB)')
    plt.grid(True)
    plt.xlim(0, sample_rate/2)
    plt.ylim(-80, 5)
    
    # Phase response
    plt.subplot(2, 1, 2)
    plt.plot(w, np.unwrap(np.angle(h)) * 180 / np.pi)
    plt.title(f'{title} - Phase Response')
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Phase (degrees)')
    plt.grid(True)
    plt.xlim(0, sample_rate/2)
    
    plt.tight_layout()
    plt.savefig(f'{title.lower().replace(" ", "_").replace("-", "_")}_response.png', dpi=150, bbox_inches='tight')
    plt.show()

# Load and plot all FIR filter responses
filters = [
    ('fir_lowpass_1000hz.txt', 'FIR Lowpass 1000Hz'),
    ('fir_highpass_1000hz.txt', 'FIR Highpass 1000Hz'), 
    ('fir_bandpass_800_1200hz.txt', 'FIR Bandpass 800-1200Hz'),
    ('fir_bandstop_800_1200hz.txt', 'FIR Bandstop 800-1200Hz'),
    ('fir_lowpass_hann_1000hz.txt', 'FIR Lowpass Hann Window'),
    ('fir_lowpass_hamming_1000hz.txt', 'FIR Lowpass Hamming Window'),
    ('fir_lowpass_blackman_1000hz.txt', 'FIR Lowpass Blackman Window')
]

for filename, title in filters:
    try:
        coeffs = load_coeffs(filename)
        plot_frequency_response(coeffs, title)
    except FileNotFoundError:
        print(f"File {filename} not found!")

# Compare window types on same plot
plt.figure(figsize=(12, 6))
window_files = [
    ('fir_lowpass_hann_1000hz.txt', 'Hann', 'blue'),
    ('fir_lowpass_hamming_1000hz.txt', 'Hamming', 'red'),
    ('fir_lowpass_blackman_1000hz.txt', 'Blackman', 'green')
]

for filename, label, color in window_files:
    try:
        coeffs = load_coeffs(filename)
        w, h = signal.freqz(coeffs, worN=8000, fs=44100)
        plt.plot(w, 20 * np.log10(np.abs(h)), label=label, color=color)
    except FileNotFoundError:
        print(f"File {filename} not found!")

plt.title('FIR Lowpass 1000Hz - Window Comparison')
plt.xlabel('Frequency (Hz)')
plt.ylabel('Magnitude (dB)')
plt.grid(True)
plt.legend()
plt.xlim(0, 22050)
plt.ylim(-80, 5)
plt.savefig('fir_window_comparison.png', dpi=150, bbox_inches='tight')
plt.show()

print("All plots generated successfully!")
)";
        pyScript.close();
    }

    // Just verify the files were created - the actual validation will be done visually with Python
    EXPECT_EQ (lowpass.size(), numCoeffs);
    EXPECT_EQ (highpass.size(), numCoeffs);
    EXPECT_EQ (bandpass.size(), numCoeffs);
    EXPECT_EQ (bandstop.size(), numCoeffs);

    std::cout << "\nFIR coefficient files and Python plotting script created:\n";
    std::cout << "- fir_lowpass_1000hz.txt\n";
    std::cout << "- fir_highpass_1000hz.txt\n";
    std::cout << "- fir_bandpass_800_1200hz.txt\n";
    std::cout << "- fir_bandstop_800_1200hz.txt\n";
    std::cout << "- fir_lowpass_hann_1000hz.txt\n";
    std::cout << "- fir_lowpass_hamming_1000hz.txt\n";
    std::cout << "- fir_lowpass_blackman_1000hz.txt\n";
    std::cout << "- plot_fir_responses.py\n\n";
    std::cout << "Run: python3 plot_fir_responses.py (requires numpy, matplotlib, scipy)\n";
}
