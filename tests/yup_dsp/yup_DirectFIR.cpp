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

#include <random>
#include <cmath>

namespace yup::test
{

//==============================================================================
class DirectFIRTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        generator.seed (42); // Fixed seed for reproducible tests
    }

    float randomFloat (float min = -1.0f, float max = 1.0f)
    {
        std::uniform_real_distribution<float> dist (min, max);
        return dist (generator);
    }

    void fillWithRandomData (std::vector<float>& buffer)
    {
        for (auto& sample : buffer)
            sample = randomFloat();
    }

    void fillWithSine (std::vector<float>& buffer, float frequency, float sampleRate)
    {
        for (size_t i = 0; i < buffer.size(); ++i)
            buffer[i] = std::sin (2.0f * MathConstants<float>::pi * frequency * static_cast<float> (i) / sampleRate);
    }

    void clearBuffer (std::vector<float>& buffer)
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
    }

    float calculateRMS (const std::vector<float>& buffer)
    {
        if (buffer.empty())
            return 0.0f;

        float sum = 0.0f;
        for (float sample : buffer)
            sum += sample * sample;

        return std::sqrt (sum / static_cast<float> (buffer.size()));
    }

    float findPeak (const std::vector<float>& buffer)
    {
        if (buffer.empty())
            return 0.0f;

        float peak = 0.0f;
        for (float sample : buffer)
            peak = std::max (peak, std::abs (sample));

        return peak;
    }

    std::vector<float> createLowpassCoefficients (int numCoefficients, float cutoffFreq, float sampleRate)
    {
        std::vector<float> coefficients (numCoefficients);
        float fc = cutoffFreq / sampleRate;
        int center = numCoefficients / 2;

        for (int i = 0; i < numCoefficients; ++i)
        {
            if (i == center)
                coefficients[i] = 2.0f * fc;
            else
            {
                float x = 2.0f * MathConstants<float>::pi * fc * (i - center);
                coefficients[i] = std::sin (x) / x;
            }

            // Apply Hanning window
            float w = 0.5f - 0.5f * std::cos (2.0f * MathConstants<float>::pi * i / (numCoefficients - 1));
            coefficients[i] *= w;
        }

        return coefficients;
    }

    std::mt19937 generator;
};

//==============================================================================
// Basic API Tests
//==============================================================================

TEST_F (DirectFIRTest, DefaultConstruction)
{
    DirectFIR<float, float> fir;

    // Default state should be safe
    EXPECT_EQ (fir.getNumCoefficients(), 0);
    EXPECT_FALSE (fir.hasCoefficients());
    EXPECT_EQ (fir.getScaling(), 1.0f);

    // Should handle empty processing gracefully
    std::vector<float> input (256, 0.0f);
    std::vector<float> output (256, 0.0f);
    EXPECT_NO_THROW (fir.processBlock (input.data(), output.data(), static_cast<int> (input.size())));

    // Output should remain zero without coefficients
    for (float sample : output)
        EXPECT_EQ (sample, 0.0f);
}

TEST_F (DirectFIRTest, MoveSemantics)
{
    DirectFIR<float, float> fir1;
    std::vector<float> coefficients = { 1.0f, 0.5f, 0.25f };
    fir1.setCoefficients (coefficients, 2.0f);

    // Move constructor
    DirectFIR<float, float> fir2 = std::move (fir1);

    // Verify moved filter works
    EXPECT_EQ (fir2.getNumCoefficients(), 3);
    EXPECT_TRUE (fir2.hasCoefficients());
    EXPECT_EQ (fir2.getScaling(), 2.0f);

    // Original should be in valid but unspecified state
    EXPECT_EQ (fir1.getNumCoefficients(), 0);

    // Test processing with moved filter
    std::vector<float> input (10, 0.0f);
    input[0] = 1.0f;
    std::vector<float> output (10, 0.0f);

    EXPECT_NO_THROW (fir2.processBlock (input.data(), output.data(), static_cast<int> (static_cast<int> (input.size()))));

    // Should produce scaled output
    float outputSum = 0.0f;
    for (float sample : output)
        outputSum += std::abs (sample);
    EXPECT_GT (outputSum, 1.0f); // Should be > 1 due to scaling

    // Move assignment
    DirectFIR<float, float> fir3;
    fir3 = std::move (fir2);

    EXPECT_EQ (fir3.getNumCoefficients(), 3);
    EXPECT_TRUE (fir3.hasCoefficients());
    EXPECT_EQ (fir3.getScaling(), 2.0f);
}

//==============================================================================
// Coefficient Setting Tests
//==============================================================================

TEST_F (DirectFIRTest, SetCoefficientsVector)
{
    DirectFIR<float, float> fir;
    std::vector<float> coefficients = { 0.1f, 0.5f, 1.0f, 0.5f, 0.1f };

    fir.setCoefficients (coefficients, 1.0f);

    EXPECT_EQ (fir.getNumCoefficients(), 5);
    EXPECT_TRUE (fir.hasCoefficients());
    EXPECT_EQ (fir.getScaling(), 1.0f);

    // Coefficients should be available
    const auto& coeffs = fir.getCoefficients();
    EXPECT_EQ (coeffs.size(), 8); // Padded to multiple of 4
}

TEST_F (DirectFIRTest, SetCoefficientsPointer)
{
    DirectFIR<float, float> fir;
    float coefficients[] = { 0.2f, 0.4f, 0.6f, 0.8f };

    fir.setCoefficients (coefficients, 4, 2.0f);

    EXPECT_EQ (fir.getNumCoefficients(), 4);
    EXPECT_TRUE (fir.hasCoefficients());
    EXPECT_EQ (fir.getScaling(), 2.0f);
}

TEST_F (DirectFIRTest, SetCoefficientsNullptr)
{
    DirectFIR<float, float> fir;

    // First set some valid coefficients
    std::vector<float> coefficients = { 1.0f, 0.5f };
    fir.setCoefficients (coefficients);
    EXPECT_TRUE (fir.hasCoefficients());

    // Setting nullptr should clear the filter
    fir.setCoefficients (nullptr, 0, 1.0f);
    EXPECT_FALSE (fir.hasCoefficients());
    EXPECT_EQ (fir.getNumCoefficients(), 0);
}

TEST_F (DirectFIRTest, SetCoefficientsWithScaling)
{
    DirectFIR<float, float> fir;
    std::vector<float> coefficients = { 1.0f, 1.0f, 1.0f };

    fir.setCoefficients (coefficients, 0.5f);

    // Test impulse response
    std::vector<float> input (10, 0.0f);
    input[0] = 2.0f; // Unit impulse scaled by 2
    std::vector<float> output (10, 0.0f);

    fir.processBlock (input.data(), output.data(), static_cast<int> (input.size()));

    // Output should reflect the coefficient scaling
    // Each coefficient was originally 1.0, scaled by 0.5, so output per coefficient = 2.0 * 0.5 = 1.0
    float expectedSum = 3.0f; // 3 coefficients * 1.0 each
    float actualSum = 0.0f;
    for (size_t i = 0; i < 5; ++i) // Check first 5 samples
        actualSum += output[i];

    EXPECT_NEAR (actualSum, expectedSum, 0.001f);
}

//==============================================================================
// Processing Tests
//==============================================================================

TEST_F (DirectFIRTest, ImpulseResponse)
{
    DirectFIR<float, float> fir;
    std::vector<float> coefficients = { 1.0f, 0.5f, 0.25f };
    fir.setCoefficients (coefficients);

    // Test with unit impulse
    std::vector<float> input (10, 0.0f);
    input[0] = 1.0f;
    std::vector<float> output (10, 0.0f);

    fir.processBlock (input.data(), output.data(), static_cast<int> (input.size()));

    // Should get the impulse response (coefficients in original order)
    EXPECT_NEAR (output[0], 1.0f, 0.001f);  // First coefficient h0
    EXPECT_NEAR (output[1], 0.5f, 0.001f);  // Second coefficient h1
    EXPECT_NEAR (output[2], 0.25f, 0.001f); // Third coefficient h2

    // Rest should be zero
    for (size_t i = 3; i < output.size(); ++i)
        EXPECT_NEAR (output[i], 0.0f, 0.001f);
}

TEST_F (DirectFIRTest, AccumulativeOutput)
{
    DirectFIR<float, float> fir;
    std::vector<float> coefficients = { 0.5f, 0.5f };
    fir.setCoefficients (coefficients);

    std::vector<float> input (5, 1.0f);
    std::vector<float> output (5);

    // Pre-populate output buffer
    std::fill (output.begin(), output.end(), 1.0f);
    std::vector<float> originalOutput = output;

    fir.processBlock (input.data(), output.data(), static_cast<int> (input.size()));

    // Output should contain original data plus filter result
    for (size_t i = 0; i < output.size(); ++i)
        EXPECT_GT (output[i], originalOutput[i]);
}

TEST_F (DirectFIRTest, Linearity)
{
    DirectFIR<float, float> fir;
    auto coefficients = FilterDesigner<float>::designFIRLowpass (32, 1000.0f, 44100.0f);
    fir.setCoefficients (coefficients);

    std::vector<float> input (512);
    fillWithRandomData (input);

    // Scale input by 2 and test linearity
    std::vector<float> input2 = input;
    FloatVectorOperations::multiply (input2.data(), 2.0f, input2.size());

    std::vector<float> output1 (512, 0.0f);
    std::vector<float> output2 (512, 0.0f);

    fir.reset();
    fir.processBlock (input.data(), output1.data(), static_cast<int> (input.size()));

    fir.reset();
    fir.processBlock (input2.data(), output2.data(), input2.size());

    // output2 should be approximately 2x output1
    for (size_t i = 0; i < output1.size(); ++i)
    {
        if (std::abs (output1[i]) > 0.001f) // Avoid division by near-zero
            EXPECT_NEAR (output2[i] / output1[i], 2.0f, 0.01f);
    }
}

TEST_F (DirectFIRTest, Reset)
{
    DirectFIR<float, float> fir;
    std::vector<float> coefficients = { 1.0f, 0.8f, 0.6f, 0.4f, 0.2f };
    fir.setCoefficients (coefficients);

    std::vector<float> input (20);
    fillWithRandomData (input);
    std::vector<float> output1 (20, 0.0f);

    // Process some data to build up internal state
    fir.processBlock (input.data(), output1.data(), static_cast<int> (input.size()));

    // Reset and process same input
    fir.reset();
    std::vector<float> output2 (20, 0.0f);
    fir.processBlock (input.data(), output2.data(), static_cast<int> (input.size()));

    // Outputs should be identical after reset
    for (size_t i = 0; i < output1.size(); ++i)
        EXPECT_NEAR (output1[i], output2[i], 0.0001f);
}

//==============================================================================
// Signal Processing Tests
//==============================================================================

TEST_F (DirectFIRTest, LowpassFiltering)
{
    DirectFIR<float, float> fir;

    // Create lowpass filter coefficients
    auto coefficients = FilterDesigner<float>::designFIRLowpass (64, 1000.0f, 44100.0);
    fir.setCoefficients (coefficients);

    const float sampleRate = 44100.0f;
    const size_t bufferSize = 2048;

    // Test with low frequency (should pass)
    std::vector<float> lowFreqInput (bufferSize);
    fillWithSine (lowFreqInput, 500.0f, sampleRate);
    std::vector<float> lowFreqOutput (bufferSize, 0.0f);

    fir.processBlock (lowFreqInput.data(), lowFreqOutput.data(), bufferSize);

    // Test with high frequency (should be attenuated)
    fir.reset();
    std::vector<float> highFreqInput (bufferSize);
    fillWithSine (highFreqInput, 5000.0f, sampleRate);
    std::vector<float> highFreqOutput (bufferSize, 0.0f);

    fir.processBlock (highFreqInput.data(), highFreqOutput.data(), bufferSize);

    // Compare RMS levels (skip first samples due to transient)
    const size_t skipSamples = 100;
    float lowFreqRMS = 0.0f, highFreqRMS = 0.0f;

    for (size_t i = skipSamples; i < bufferSize; ++i)
    {
        lowFreqRMS += lowFreqOutput[i] * lowFreqOutput[i];
        highFreqRMS += highFreqOutput[i] * highFreqOutput[i];
    }

    lowFreqRMS = std::sqrt (lowFreqRMS / (bufferSize - skipSamples));
    highFreqRMS = std::sqrt (highFreqRMS / (bufferSize - skipSamples));

    // Low frequency should have higher RMS than high frequency
    EXPECT_GT (lowFreqRMS, highFreqRMS * 2.0f);
}

TEST_F (DirectFIRTest, BlockSizeIndependence)
{
    DirectFIR<float, float> fir;
    auto coefficients = FilterDesigner<float>::designFIRLowpass (48, 2000.0f, 44100.0);
    fir.setCoefficients (coefficients);

    const size_t totalSamples = 1024;
    std::vector<float> input (totalSamples);
    fillWithRandomData (input);

    // Process in one big block
    fir.reset();
    std::vector<float> output1 (totalSamples, 0.0f);
    fir.processBlock (input.data(), output1.data(), totalSamples);

    // Process in smaller blocks
    fir.reset();
    std::vector<float> output2 (totalSamples, 0.0f);
    const std::vector<size_t> blockSizes = { 32, 64, 128, 256, 32, 128, 64 };
    size_t processed = 0;

    for (size_t blockSize : blockSizes)
    {
        if (processed >= totalSamples)
            break;

        if (processed + blockSize > totalSamples)
            blockSize = totalSamples - processed;

        if (blockSize == 0)
            break;

        fir.processBlock (input.data() + processed, output2.data() + processed, blockSize);
        processed += blockSize;
    }

    // Process any remaining samples
    while (processed < totalSamples)
    {
        size_t remaining = totalSamples - processed;
        size_t blockSize = std::min (remaining, size_t (128)); // Process in chunks of 128
        fir.processBlock (input.data() + processed, output2.data() + processed, blockSize);
        processed += blockSize;
    }

    // Outputs should be identical regardless of block size
    for (size_t i = 0; i < totalSamples; ++i)
        EXPECT_NEAR (output1[i], output2[i], 0.0001f);
}

//==============================================================================
// Edge Cases and Error Handling
//==============================================================================

TEST_F (DirectFIRTest, ZeroSamples)
{
    DirectFIR<float, float> fir;
    std::vector<float> coefficients = { 1.0f, 0.5f };
    fir.setCoefficients (coefficients);

    std::vector<float> input (10, 1.0f);
    std::vector<float> output (10, 0.0f);

    // Processing zero samples should be safe
    EXPECT_NO_THROW (fir.processBlock (input.data(), output.data(), 0));

    // Output should remain unchanged
    for (float sample : output)
        EXPECT_EQ (sample, 0.0f);
}

TEST_F (DirectFIRTest, NullPointers)
{
    DirectFIR<float, float> fir;
    std::vector<float> coefficients = { 1.0f };
    fir.setCoefficients (coefficients);

    std::vector<float> buffer (10, 0.0f);

    // Null input pointer should be handled gracefully
    EXPECT_NO_THROW (fir.processBlock (nullptr, buffer.data(), 10));

    // Null output pointer should be handled gracefully
    EXPECT_NO_THROW (fir.processBlock (buffer.data(), nullptr, 10));

    // Both null should be handled gracefully
    EXPECT_NO_THROW (fir.processBlock (nullptr, nullptr, 10));
}

TEST_F (DirectFIRTest, LargeTapCounts)
{
    DirectFIR<float, float> fir;

    // Test with relatively large number of coefficients
    std::vector<float> coefficients (512);
    for (size_t i = 0; i < coefficients.size(); ++i)
        coefficients[i] = std::exp (-static_cast<float> (i) / 100.0f) * std::sin (2.0f * MathConstants<float>::pi * i / 16.0f);

    EXPECT_NO_THROW (fir.setCoefficients (coefficients));
    EXPECT_EQ (fir.getNumCoefficients(), 512);

    // Should process without issues
    std::vector<float> input (1024);
    std::vector<float> output (1024, 0.0f);
    fillWithRandomData (input);

    EXPECT_NO_THROW (fir.processBlock (input.data(), output.data(), static_cast<int> (input.size())));

    // Should produce reasonable output
    float rms = calculateRMS (output);
    EXPECT_GT (rms, 0.001f);
    EXPECT_LT (rms, 10.0f);
}

TEST_F (DirectFIRTest, SingleTap)
{
    DirectFIR<float, float> fir;
    std::vector<float> coefficients = { 0.75f };
    fir.setCoefficients (coefficients);

    EXPECT_EQ (fir.getNumCoefficients(), 1);

    // Single coefficient should act as a simple gain
    std::vector<float> input = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    std::vector<float> output (5, 0.0f);

    fir.processBlock (input.data(), output.data(), static_cast<int> (input.size()));

    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_NEAR (output[i], input[i] * 0.75f, 0.001f);
}

//==============================================================================
// Performance and Memory Tests
//==============================================================================

TEST_F (DirectFIRTest, MemoryAlignment)
{
    DirectFIR<float, float> fir;

    // Coefficient count that's not a multiple of 4
    std::vector<float> coefficients (37);
    std::fill (coefficients.begin(), coefficients.end(), 0.1f);
    fir.setCoefficients (coefficients);

    // Coefficients should be padded for SIMD alignment
    const auto& coeffs = fir.getCoefficients();
    EXPECT_EQ (coeffs.size() % 4, 0); // Should be multiple of 4
    EXPECT_GE (coeffs.size(), 37);    // Should be at least original size

    // Padded elements should be zero
    for (size_t i = 37; i < coeffs.size(); ++i)
        EXPECT_EQ (coeffs[i], 0.0f);
}

TEST_F (DirectFIRTest, StressTest)
{
    DirectFIR<float, float> fir;

    // Create complex impulse response
    std::vector<float> coefficients (256);
    for (size_t i = 0; i < coefficients.size(); ++i)
    {
        float t = static_cast<float> (i) / 256.0f;
        coefficients[i] = std::exp (-t * 5.0f) * std::cos (20.0f * MathConstants<float>::pi * t);
    }
    fir.setCoefficients (coefficients);

    // Process multiple blocks of varying sizes
    const std::vector<size_t> blockSizes = { 1, 7, 32, 63, 128, 255, 512, 1023 };

    for (size_t blockSize : blockSizes)
    {
        SCOPED_TRACE (testing::Message() << "Block size: " << blockSize);

        std::vector<float> input (blockSize);
        std::vector<float> output (blockSize, 0.0f);
        fillWithRandomData (input);

        EXPECT_NO_THROW (fir.processBlock (input.data(), output.data(), blockSize));

        // Verify output quality
        for (float sample : output)
        {
            EXPECT_TRUE (std::isfinite (sample));
            EXPECT_LT (std::abs (sample), 100.0f); // Reasonable bounds
        }
    }
}

} // namespace yup::test
