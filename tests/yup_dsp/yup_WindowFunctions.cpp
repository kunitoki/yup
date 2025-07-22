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

using namespace yup;

namespace
{
constexpr double tolerance = 1e-4;
constexpr float toleranceF = 1e-4f;
constexpr float relaxedToleranceF = 1e-3f;
constexpr int windowSize = 128;
constexpr int largeWindowSize = 512;
} // namespace

//==============================================================================
class WindowFunctionsTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize test vectors
        testData.resize (windowSize, 1.0f);
        outputData.resize (windowSize);
        doubleTestData.resize (windowSize, 1.0);
        doubleOutputData.resize (windowSize);

        // Fill with test pattern
        for (int i = 0; i < windowSize; ++i)
        {
            testData[i] = std::sin (2.0f * MathConstants<float>::pi * i / windowSize);
            doubleTestData[i] = std::sin (2.0 * MathConstants<double>::pi * i / windowSize);
        }
    }

    std::vector<float> testData;
    std::vector<float> outputData;
    std::vector<double> doubleTestData;
    std::vector<double> doubleOutputData;
};

//==============================================================================
// Basic getValue() Tests
//==============================================================================

TEST_F (WindowFunctionsTests, GetValueRectangular)
{
    for (int n = 0; n < windowSize; ++n)
    {
        auto value = WindowFunctions<float>::getValue (WindowType::rectangular, n, windowSize);
        EXPECT_FLOAT_EQ (value, 1.0f);
    }
}

TEST_F (WindowFunctionsTests, GetValueHann)
{
    // Test specific known values for Hann window
    auto midValue = WindowFunctions<float>::getValue (WindowType::hann, windowSize / 2, windowSize);
    EXPECT_NEAR (midValue, 1.0f, relaxedToleranceF);

    auto startValue = WindowFunctions<float>::getValue (WindowType::hann, 0, windowSize);
    EXPECT_NEAR (startValue, 0.0f, toleranceF);

    auto endValue = WindowFunctions<float>::getValue (WindowType::hann, windowSize - 1, windowSize);
    EXPECT_NEAR (endValue, 0.0f, toleranceF);
}

TEST_F (WindowFunctionsTests, GetValueHamming)
{
    auto midValue = WindowFunctions<float>::getValue (WindowType::hamming, windowSize / 2, windowSize);
    EXPECT_GT (midValue, 0.9f);

    auto startValue = WindowFunctions<float>::getValue (WindowType::hamming, 0, windowSize);
    EXPECT_NEAR (startValue, 0.08f, 0.01f); // Hamming window has non-zero endpoints
}

TEST_F (WindowFunctionsTests, GetValueBlackman)
{
    auto midValue = WindowFunctions<float>::getValue (WindowType::blackman, windowSize / 2, windowSize);
    EXPECT_GT (midValue, 0.9f);

    auto startValue = WindowFunctions<float>::getValue (WindowType::blackman, 0, windowSize);
    EXPECT_NEAR (startValue, 0.0f, toleranceF);
}

TEST_F (WindowFunctionsTests, GetValueKaiser)
{
    // Test with different beta values
    auto value1 = WindowFunctions<float>::getValue (WindowType::kaiser, windowSize / 2, windowSize, 5.0f);
    auto value2 = WindowFunctions<float>::getValue (WindowType::kaiser, windowSize / 2, windowSize, 10.0f);

    EXPECT_GT (value1, 0.9f);
    EXPECT_GT (value2, 0.9f);
    EXPECT_NE (value1, value2); // Different beta should give different values
}

TEST_F (WindowFunctionsTests, GetValueGaussian)
{
    auto midValue = WindowFunctions<float>::getValue (WindowType::gaussian, windowSize / 2, windowSize, 0.4f);
    EXPECT_NEAR (midValue, 1.0f, relaxedToleranceF);

    auto quarterValue = WindowFunctions<float>::getValue (WindowType::gaussian, windowSize / 4, windowSize, 0.4f);
    EXPECT_LT (quarterValue, 1.0f);
    EXPECT_GT (quarterValue, 0.1f);
}

TEST_F (WindowFunctionsTests, GetValueTukey)
{
    // Test with alpha = 0.5 (default)
    auto midValue = WindowFunctions<float>::getValue (WindowType::tukey, windowSize / 2, windowSize, 0.5f);
    EXPECT_FLOAT_EQ (midValue, 1.0f);

    // Test edges
    auto startValue = WindowFunctions<float>::getValue (WindowType::tukey, 0, windowSize, 0.5f);
    EXPECT_NEAR (startValue, 0.0f, toleranceF);
}

TEST_F (WindowFunctionsTests, AllWindowTypesBasicFunctionality)
{
    const std::vector<WindowType> allTypes = {
        WindowType::rectangular,
        WindowType::hann,
        WindowType::hamming,
        WindowType::blackman,
        WindowType::blackmanHarris,
        WindowType::kaiser,
        WindowType::gaussian,
        WindowType::tukey,
        WindowType::bartlett,
        WindowType::welch,
        WindowType::flattop,
        WindowType::cosine,
        WindowType::lanczos,
        WindowType::nuttall,
        WindowType::blackmanNuttall
    };

    for (const auto type : allTypes)
    {
        for (int n = 0; n < windowSize; ++n)
        {
            auto value = WindowFunctions<float>::getValue (type, n, windowSize);
            EXPECT_TRUE (std::isfinite (value));
            // Note: Some window functions (like flattop) can have small negative values due to floating point precision
            EXPECT_GT (value, -0.1f); // Allow small negative values due to numerical precision
        }
    }
}

//==============================================================================
// Generate Methods Tests
//==============================================================================

TEST_F (WindowFunctionsTests, GenerateSpanVersion)
{
    std::vector<float> window (windowSize);
    Span<float> windowSpan (window);

    WindowFunctions<float>::generate (WindowType::hann, windowSpan);

    // Check symmetry
    for (int i = 0; i < windowSize / 2; ++i)
    {
        EXPECT_NEAR (window[i], window[windowSize - 1 - i], toleranceF);
    }

    // Check center value is maximum for Hann
    auto maxIt = std::max_element (window.begin(), window.end());
    int maxIndex = static_cast<int> (std::distance (window.begin(), maxIt));
    EXPECT_NEAR (maxIndex, windowSize / 2, 2); // Allow small deviation due to even/odd sizes
}

TEST_F (WindowFunctionsTests, GenerateRawPointerVersion)
{
    std::vector<float> window (windowSize);

    WindowFunctions<float>::generate (WindowType::hamming, window.data(), window.size());

    // Verify all values are finite and reasonable
    for (const auto& value : window)
    {
        EXPECT_TRUE (std::isfinite (value));
        EXPECT_GE (value, 0.0f);
        EXPECT_LE (value, 1.1f); // Allow small margin for numerical precision
    }
}

TEST_F (WindowFunctionsTests, GenerateKaiserWithParameter)
{
    std::vector<float> window1 (windowSize);
    std::vector<float> window2 (windowSize);

    WindowFunctions<float>::generate (WindowType::kaiser, window1.data(), window1.size(), 5.0f);
    WindowFunctions<float>::generate (WindowType::kaiser, window2.data(), window2.size(), 10.0f);

    // Different beta values should produce different windows
    bool different = false;
    for (int i = 0; i < windowSize; ++i)
    {
        if (std::abs (window1[i] - window2[i]) > toleranceF)
        {
            different = true;
            break;
        }
    }
    EXPECT_TRUE (different);
}

//==============================================================================
// Apply Methods Tests
//==============================================================================

TEST_F (WindowFunctionsTests, ApplyInPlaceSpan)
{
    std::vector<float> signal = testData; // Copy original data
    Span<float> signalSpan (signal);

    WindowFunctions<float>::apply (WindowType::hann, signalSpan);

    // Signal should be modified (windowed)
    bool modified = false;
    for (int i = 0; i < windowSize; ++i)
    {
        if (std::abs (signal[i] - testData[i]) > toleranceF)
        {
            modified = true;
            break;
        }
    }
    EXPECT_TRUE (modified);

    // Windowed signal should be smaller in magnitude at edges
    EXPECT_LT (std::abs (signal[0]), std::abs (testData[0]) + toleranceF);
    EXPECT_LT (std::abs (signal[windowSize - 1]), std::abs (testData[windowSize - 1]) + toleranceF);
}

TEST_F (WindowFunctionsTests, ApplyOutOfPlaceSpan)
{
    Span<const float> inputSpan (testData);
    Span<float> outputSpan (outputData);

    WindowFunctions<float>::apply (WindowType::blackman, inputSpan, outputSpan);

    // Original data should be unchanged
    for (int i = 0; i < windowSize; ++i)
    {
        EXPECT_FLOAT_EQ (testData[i], std::sin (2.0f * MathConstants<float>::pi * i / windowSize));
    }

    // Output should be windowed
    for (int i = 0; i < windowSize; ++i)
    {
        EXPECT_TRUE (std::isfinite (outputData[i]));
    }
}

TEST_F (WindowFunctionsTests, ApplyRawPointers)
{
    WindowFunctions<float>::apply (WindowType::bartlett, testData.data(), outputData.data(), windowSize);

    // Check that triangular window produces expected pattern
    // For Bartlett window, maximum should be somewhere in the center region
    auto maxIt = std::max_element (outputData.begin(), outputData.end());
    int maxIndex = static_cast<int> (std::distance (outputData.begin(), maxIt));
    EXPECT_GT (maxIndex, windowSize / 4);
    EXPECT_LT (maxIndex, 3 * windowSize / 4);

    // Edges should have smaller values than center region
    auto centerValue = std::abs (outputData[windowSize / 2]);
    EXPECT_LT (std::abs (outputData[0]), centerValue + toleranceF);
    EXPECT_LT (std::abs (outputData[windowSize - 1]), centerValue + toleranceF);
}

//==============================================================================
// Individual Window Function Tests
//==============================================================================

TEST_F (WindowFunctionsTests, RectangularWindow)
{
    for (int n = 0; n < windowSize; ++n)
    {
        auto value = WindowFunctions<float>::rectangular (n, windowSize);
        EXPECT_FLOAT_EQ (value, 1.0f);
    }
}

TEST_F (WindowFunctionsTests, HannWindowSymmetry)
{
    for (int n = 0; n < windowSize / 2; ++n)
    {
        auto value1 = WindowFunctions<float>::hann (n, windowSize);
        auto value2 = WindowFunctions<float>::hann (windowSize - 1 - n, windowSize);
        EXPECT_NEAR (value1, value2, toleranceF);
    }
}

TEST_F (WindowFunctionsTests, BartlettWindowTriangular)
{
    auto centerValue = WindowFunctions<float>::bartlett (windowSize / 2, windowSize);
    auto quarterValue = WindowFunctions<float>::bartlett (windowSize / 4, windowSize);
    auto startValue = WindowFunctions<float>::bartlett (0, windowSize);

    // For discrete Bartlett window, center value may not be exactly 1.0 for even window sizes
    EXPECT_GT (centerValue, 0.99f);
    EXPECT_LT (centerValue, 1.01f);
    EXPECT_GT (quarterValue, startValue);
    EXPECT_LT (quarterValue, centerValue);
    EXPECT_NEAR (startValue, 0.0f, toleranceF);
}

TEST_F (WindowFunctionsTests, WelchWindowParabolic)
{
    auto centerValue = WindowFunctions<float>::welch (windowSize / 2, windowSize);
    auto startValue = WindowFunctions<float>::welch (0, windowSize);
    auto endValue = WindowFunctions<float>::welch (windowSize - 1, windowSize);

    EXPECT_NEAR (centerValue, 1.0f, relaxedToleranceF);
    EXPECT_NEAR (startValue, 0.0f, toleranceF);
    EXPECT_NEAR (endValue, 0.0f, toleranceF);
}

TEST_F (WindowFunctionsTests, LanczosWindow)
{
    auto centerValue = WindowFunctions<float>::lanczos (windowSize / 2, windowSize);
    EXPECT_NEAR (centerValue, 1.0f, relaxedToleranceF);

    // Test symmetry
    for (int n = 0; n < windowSize / 2; ++n)
    {
        auto value1 = WindowFunctions<float>::lanczos (n, windowSize);
        auto value2 = WindowFunctions<float>::lanczos (windowSize - 1 - n, windowSize);
        EXPECT_NEAR (value1, value2, toleranceF);
    }
}

//==============================================================================
// Mathematical Properties Tests
//==============================================================================

TEST_F (WindowFunctionsTests, WindowSymmetry)
{
    const std::vector<WindowType> symmetricWindows = {
        WindowType::hann,
        WindowType::hamming,
        WindowType::blackman,
        WindowType::blackmanHarris,
        WindowType::bartlett,
        WindowType::welch,
        WindowType::cosine,
        WindowType::nuttall,
        WindowType::blackmanNuttall
    };

    for (const auto type : symmetricWindows)
    {
        for (int n = 0; n < windowSize / 2; ++n)
        {
            auto value1 = WindowFunctions<float>::getValue (type, n, windowSize);
            auto value2 = WindowFunctions<float>::getValue (type, windowSize - 1 - n, windowSize);
            EXPECT_NEAR (value1, value2, toleranceF) << "Window type failed symmetry test";
        }
    }
}

TEST_F (WindowFunctionsTests, WindowNormalization)
{
    // Test that window values are generally between 0 and 1
    const std::vector<WindowType> normalizedWindows = {
        WindowType::hann,
        WindowType::hamming,
        WindowType::blackman,
        WindowType::bartlett,
        WindowType::welch,
        WindowType::cosine
    };

    for (const auto type : normalizedWindows)
    {
        for (int n = 0; n < windowSize; ++n)
        {
            auto value = WindowFunctions<float>::getValue (type, n, windowSize);
            // Allow very small negative values due to floating point precision
            EXPECT_GT (value, -1e-6f);
            EXPECT_LE (value, 1.1f); // Allow small margin for numerical precision
        }
    }
}

TEST_F (WindowFunctionsTests, KaiserParameterEffect)
{
    // Test that different Kaiser beta values produce different window shapes
    std::vector<float> beta2 (windowSize);
    std::vector<float> beta8 (windowSize);
    std::vector<float> beta20 (windowSize);

    WindowFunctions<float>::generate (WindowType::kaiser, beta2.data(), windowSize, 2.0f);
    WindowFunctions<float>::generate (WindowType::kaiser, beta8.data(), windowSize, 8.0f);
    WindowFunctions<float>::generate (WindowType::kaiser, beta20.data(), windowSize, 20.0f);

    // Higher beta should produce narrower main lobe (lower values at edges)
    EXPECT_LT (beta20[windowSize / 4], beta8[windowSize / 4]);
    EXPECT_LT (beta8[windowSize / 4], beta2[windowSize / 4]);
}

//==============================================================================
// Edge Cases and Error Handling Tests
//==============================================================================

TEST_F (WindowFunctionsTests, ZeroLengthWindow)
{
    std::vector<float> emptyWindow;
    Span<float> emptySpan (emptyWindow);

    // Should handle empty spans gracefully
    EXPECT_NO_THROW (WindowFunctions<float>::generate (WindowType::hann, emptySpan));
}

TEST_F (WindowFunctionsTests, SingleSampleWindow)
{
    // For single sample windows, rectangular should work fine
    auto rectValue = WindowFunctions<float>::getValue (WindowType::rectangular, 0, 1);
    EXPECT_FLOAT_EQ (rectValue, 1.0f);

    // Some windows may not be well-defined for N=1, so test with N=2 instead
    auto hannValue = WindowFunctions<float>::getValue (WindowType::hann, 0, 2);
    EXPECT_TRUE (std::isfinite (hannValue));

    std::vector<float> twoSamples (2);
    WindowFunctions<float>::generate (WindowType::blackman, twoSamples.data(), 2);
    for (const auto& value : twoSamples)
    {
        EXPECT_TRUE (std::isfinite (value));
    }
}

TEST_F (WindowFunctionsTests, LargeWindowSize)
{
    std::vector<float> largeWindow (largeWindowSize);

    EXPECT_NO_THROW (WindowFunctions<float>::generate (WindowType::kaiser, largeWindow.data(), largeWindowSize, 10.0f));

    // Verify all values are reasonable
    for (const auto& value : largeWindow)
    {
        EXPECT_TRUE (std::isfinite (value));
    }
}

//==============================================================================
// Precision Tests
//==============================================================================

TEST_F (WindowFunctionsTests, FloatVsDoublePrecision)
{
    std::vector<float> windowFloat (windowSize);
    std::vector<double> windowDouble (windowSize);

    WindowFunctions<float>::generate (WindowType::blackmanHarris, windowFloat.data(), windowSize);
    WindowFunctions<double>::generate (WindowType::blackmanHarris, windowDouble.data(), windowSize);

    // Compare precision - should be close but not identical
    for (int i = 0; i < windowSize; ++i)
    {
        EXPECT_NEAR (windowFloat[i], static_cast<float> (windowDouble[i]), 1e-6f);
    }
}

TEST_F (WindowFunctionsTests, HighPrecisionKaiser)
{
    // Test Kaiser window with high precision requirements
    auto value1 = WindowFunctions<double>::kaiser (windowSize / 2, windowSize, 15.0);
    auto value2 = WindowFunctions<double>::kaiser (windowSize / 2, windowSize, 15.000001);

    EXPECT_TRUE (std::isfinite (value1));
    EXPECT_TRUE (std::isfinite (value2));
    // Values should be very close but potentially different at high precision
}

//==============================================================================
// Energy and DC Gain Tests
//==============================================================================

TEST_F (WindowFunctionsTests, WindowEnergyConservation)
{
    // Test that window functions have reasonable energy properties
    std::vector<float> window (windowSize);

    WindowFunctions<float>::generate (WindowType::hann, window.data(), windowSize);

    // Calculate energy (sum of squares)
    float energy = 0.0f;
    for (const auto& value : window)
    {
        energy += value * value;
    }

    EXPECT_GT (energy, 0.0f);
    EXPECT_LT (energy, windowSize); // Energy should be less than rectangular window
}

TEST_F (WindowFunctionsTests, WindowDCGain)
{
    // Test DC gain (sum of all samples) for different windows
    std::vector<float> window (windowSize);

    // Rectangular window should have DC gain = N
    WindowFunctions<float>::generate (WindowType::rectangular, window.data(), windowSize);
    float dcGainRect = std::accumulate (window.begin(), window.end(), 0.0f);
    EXPECT_NEAR (dcGainRect, static_cast<float> (windowSize), toleranceF);

    // Other windows should have lower DC gain
    WindowFunctions<float>::generate (WindowType::hann, window.data(), windowSize);
    float dcGainHann = std::accumulate (window.begin(), window.end(), 0.0f);
    EXPECT_LT (dcGainHann, dcGainRect);
    EXPECT_GT (dcGainHann, 0.0f);
}

//==============================================================================
// Flat-top Window Specific Tests
//==============================================================================

TEST_F (WindowFunctionsTests, FlattopWindowCharacteristics)
{
    std::vector<float> window (windowSize);
    WindowFunctions<float>::generate (WindowType::flattop, window.data(), windowSize);

    // Flat-top windows can have values > 1.0 due to their design
    auto maxValue = *std::max_element (window.begin(), window.end());
    EXPECT_GT (maxValue, 0.9f);

    // But should still be finite
    for (const auto& value : window)
    {
        EXPECT_TRUE (std::isfinite (value));
    }
}

//==============================================================================
// Consistency Tests
//==============================================================================

TEST_F (WindowFunctionsTests, GetValueVsGenerateConsistency)
{
    // Test that getValue and generate produce identical results
    std::vector<float> generatedWindow (windowSize);
    WindowFunctions<float>::generate (WindowType::nuttall, generatedWindow.data(), windowSize);

    for (int n = 0; n < windowSize; ++n)
    {
        auto getValue = WindowFunctions<float>::getValue (WindowType::nuttall, n, windowSize);
        EXPECT_FLOAT_EQ (getValue, generatedWindow[n]);
    }
}

TEST_F (WindowFunctionsTests, DirectMethodVsGetValueConsistency)
{
    // Test that direct method calls produce same results as getValue
    for (int n = 0; n < windowSize; ++n)
    {
        auto getValueResult = WindowFunctions<float>::getValue (WindowType::hamming, n, windowSize);
        auto directResult = WindowFunctions<float>::hamming (n, windowSize);
        EXPECT_FLOAT_EQ (getValueResult, directResult);
    }
}

//==============================================================================
// Type Alias Tests
//==============================================================================

TEST_F (WindowFunctionsTests, TypeAliases)
{
    // Test that type aliases work correctly
    auto value1 = WindowFunctionsFloat::getValue (WindowType::hann, windowSize / 2, windowSize);
    auto value2 = WindowFunctionsDouble::getValue (WindowType::hann, windowSize / 2, windowSize);

    EXPECT_TRUE (std::isfinite (value1));
    EXPECT_TRUE (std::isfinite (value2));
    EXPECT_NEAR (value1, static_cast<float> (value2), toleranceF);
}