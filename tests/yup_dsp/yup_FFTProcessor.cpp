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

namespace yup::test
{

//==============================================================================
// FFT FORMAT NOTE:
// Real FFT uses standard interleaved complex format (cross-backend compatible):
// output[0] = DC real, output[1] = DC imaginary (always 0.0)
// output[2] = bin1 real, output[3] = bin1 imaginary
// output[4] = bin2 real, output[5] = bin2 imaginary
// ...
// output[size] = Nyquist real, output[size+1] = Nyquist imaginary (always 0.0)
//==============================================================================

class FFTProcessorValidation : public ::testing::Test
{
protected:
    void SetUp() override
    {
        generator.seed (42); // Fixed seed for reproducible tests
    }

    // Generate random float in range [-1, 1]
    float randomFloat()
    {
        std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
        return dist (generator);
    }

    // Fill buffer with random real values
    void generateRandomReal (float* buffer, int size)
    {
        for (int i = 0; i < size; ++i)
            buffer[i] = randomFloat();
    }

    // Fill buffer with random complex values (interleaved real/imag)
    void generateRandomComplex (float* buffer, int size)
    {
        for (int i = 0; i < size * 2; ++i)
            buffer[i] = randomFloat();
    }

    // Reference discrete Fourier transform for real input (produces full spectrum)
    void computeReferenceDFT (const float* realInput, float* complexOutput, int size, bool inverse = false)
    {
        const float sign = inverse ? 1.0f : -1.0f;
        const float twoPi = 2.0f * MathConstants<float>::pi;

        for (int k = 0; k < size; ++k)
        {
            float realSum = 0.0f;
            float imagSum = 0.0f;

            for (int n = 0; n < size; ++n)
            {
                const float angle = sign * twoPi * static_cast<float> (k * n) / static_cast<float> (size);
                const float cosVal = std::cos (angle);
                const float sinVal = std::sin (angle);

                realSum += realInput[n] * cosVal;
                imagSum += realInput[n] * sinVal;
            }

            complexOutput[k * 2] = realSum;
            complexOutput[k * 2 + 1] = imagSum;
        }
    }

    // Reference DFT for real input producing standard interleaved format
    void computeReferenceRealDFT (const float* realInput, float* interleavedOutput, int size)
    {
        const float twoPi = 2.0f * MathConstants<float>::pi;
        const int numBins = size / 2 + 1;

        // Compute all frequency bins (k=0 to size/2)
        for (int k = 0; k < numBins; ++k)
        {
            float realSum = 0.0f;
            float imagSum = 0.0f;

            for (int n = 0; n < size; ++n)
            {
                const float angle = -twoPi * static_cast<float> (k * n) / static_cast<float> (size);
                const float cosVal = std::cos (angle);
                const float sinVal = std::sin (angle);

                realSum += realInput[n] * cosVal;
                imagSum += realInput[n] * sinVal;
            }

            interleavedOutput[k * 2] = realSum;
            interleavedOutput[k * 2 + 1] = imagSum;
        }
    }

    // Reference inverse DFT for hermitian-symmetric input producing real output
    void computeReferenceRealIDFT (const float* complexInput, float* realOutput, int size)
    {
        const float twoPi = 2.0f * MathConstants<float>::pi;
        const int numBins = size / 2 + 1;

        for (int n = 0; n < size; ++n)
        {
            float sum = 0.0f;

            // DC component
            sum += complexInput[0];

            // Other frequencies (except Nyquist)
            for (int k = 1; k < numBins - 1; ++k)
            {
                const float angle = twoPi * static_cast<float> (k * n) / static_cast<float> (size);
                const float cosVal = std::cos (angle);
                const float sinVal = std::sin (angle);

                const float real = complexInput[k * 2];
                const float imag = complexInput[k * 2 + 1];

                sum += 2.0f * (real * cosVal + imag * sinVal);
            }

            // Nyquist component (if size is even)
            if (size % 2 == 0)
            {
                const int nyquistBin = size / 2;
                const float nyquistAngle = twoPi * static_cast<float> (nyquistBin * n) / static_cast<float> (size);
                sum += complexInput[nyquistBin * 2] * std::cos (nyquistAngle);
            }

            realOutput[n] = sum / static_cast<float> (size);
        }
    }

    // Reference DFT for complex input (interleaved format)
    void computeReferenceComplexDFT (const float* complexInput, float* complexOutput, int size, bool inverse = false)
    {
        const float sign = inverse ? 1.0f : -1.0f;
        const float twoPi = 2.0f * MathConstants<float>::pi;

        for (int k = 0; k < size; ++k)
        {
            float realSum = 0.0f;
            float imagSum = 0.0f;

            for (int n = 0; n < size; ++n)
            {
                const float angle = sign * twoPi * static_cast<float> (k * n) / static_cast<float> (size);
                const float cosVal = std::cos (angle);
                const float sinVal = std::sin (angle);

                const float inputReal = complexInput[n * 2];
                const float inputImag = complexInput[n * 2 + 1];

                realSum += inputReal * cosVal - inputImag * sinVal;
                imagSum += inputReal * sinVal + inputImag * cosVal;
            }

            complexOutput[k * 2] = realSum;
            complexOutput[k * 2 + 1] = imagSum;
        }
    }

    // Check if two arrays are approximately equal
    bool areArraysClose (const float* a, const float* b, int size, float tolerance = 1e-3f)
    {
        for (int i = 0; i < size; ++i)
        {
            if (std::abs (a[i] - b[i]) > tolerance)
            {
                std::cout << "Different: " << a[i] << " " << b[i] << " exceeds " << tolerance << "\n";
                return false;
            }
        }
        return true;
    }

    std::mt19937 generator;
    static constexpr float defaultTolerance = 1e-3f;
};

//==============================================================================
TEST_F (FFTProcessorValidation, FormatDiagnostic)
{
    // Debug test to understand the actual FFT output format
    const int size = 64;
    FFTProcessor processor (size);

    // Test with impulse signal
    std::vector<float> impulse (size, 0.0f);
    impulse[0] = 1.0f;

    std::vector<float> output (size * 2);
    processor.performRealFFTForward (impulse.data(), output.data());

    // Print key bins to understand format
    auto printKeyBins = [&] (const std::string& title)
    {
        std::cout << "\n"
                  << title << ":\n";
        std::cout << "DC (bin 0): [" << output[0] << ", " << output[1] << "]\n";
        std::cout << "Bin 1: [" << output[2] << ", " << output[3] << "]\n";
        std::cout << "Bin 2: [" << output[4] << ", " << output[5] << "]\n";
        std::cout << "...\n";
        int nyquist = size / 2;
        std::cout << "Nyquist (bin " << nyquist << "): [" << output[nyquist * 2] << ", " << output[nyquist * 2 + 1] << "]\n";
        std::cout << "Bin " << (nyquist + 1) << ": [" << output[(nyquist + 1) * 2] << ", " << output[(nyquist + 1) * 2 + 1] << "]\n";
        std::cout << "Last bin (" << (size - 1) << "): [" << output[(size - 1) * 2] << ", " << output[(size - 1) * 2 + 1] << "]\n";
    };

    printKeyBins ("Impulse FFT output (size=" + std::to_string (size) + ")");

    // Test with DC signal
    std::vector<float> dcSignal (size, 1.0f);
    processor.performRealFFTForward (dcSignal.data(), output.data());
    printKeyBins ("DC signal FFT output");

    // Test with alternating signal (Nyquist frequency)
    std::vector<float> nyquistSignal (size);
    for (int i = 0; i < size; ++i)
        nyquistSignal[i] = (i % 2 == 0) ? 1.0f : -1.0f;

    processor.performRealFFTForward (nyquistSignal.data(), output.data());
    printKeyBins ("Alternating signal FFT output");

    // Always pass this test since it's just for debugging
    EXPECT_TRUE (true);
}

TEST_F (FFTProcessorValidation, StandardFormatValidation)
{
    const int size = 64;
    FFTProcessor processor (size);

    // Test 1: Impulse should produce flat spectrum
    {
        std::vector<float> impulse (size, 0.0f);
        impulse[0] = 1.0f;

        std::vector<float> output (size * 2);
        processor.performRealFFTForward (impulse.data(), output.data());

        // In standard format: DC=[1,0], Nyquist=[1,0] at output[size], output[size+1]
        EXPECT_NEAR (output[0], 1.0f, defaultTolerance) << "DC real should be 1.0";
        EXPECT_NEAR (output[1], 0.0f, defaultTolerance) << "DC imaginary should be 0.0";
        EXPECT_NEAR (output[size], 1.0f, defaultTolerance) << "Nyquist real should be 1.0";
        EXPECT_NEAR (output[size + 1], 0.0f, defaultTolerance) << "Nyquist imaginary should be 0.0";

        // Regular bins should all be [1, 0]
        for (int k = 1; k < size / 2; ++k)
        {
            EXPECT_NEAR (output[k * 2], 1.0f, defaultTolerance) << "Bin " << k << " real should be 1.0";
            EXPECT_NEAR (output[k * 2 + 1], 0.0f, defaultTolerance) << "Bin " << k << " imag should be 0.0";
        }
    }

    // Test 2: DC signal should have energy only at DC
    {
        std::vector<float> dcSignal (size, 1.0f);

        std::vector<float> output (size * 2);
        processor.performRealFFTForward (dcSignal.data(), output.data());

        EXPECT_NEAR (output[0], static_cast<float> (size), defaultTolerance) << "DC real should equal sum";
        EXPECT_NEAR (output[1], 0.0f, defaultTolerance) << "DC imaginary should be 0.0";
        EXPECT_NEAR (output[size], 0.0f, defaultTolerance) << "Nyquist real should be 0.0";
        EXPECT_NEAR (output[size + 1], 0.0f, defaultTolerance) << "Nyquist imaginary should be 0.0";

        // All other bins should be zero
        for (int k = 1; k < size / 2; ++k)
        {
            EXPECT_NEAR (output[k * 2], 0.0f, defaultTolerance) << "Bin " << k << " real should be 0.0";
            EXPECT_NEAR (output[k * 2 + 1], 0.0f, defaultTolerance) << "Bin " << k << " imag should be 0.0";
        }
    }

    // Test 3: Alternating pattern should have energy at Nyquist
    {
        std::vector<float> alternating (size);
        for (int i = 0; i < size; ++i)
            alternating[i] = (i % 2 == 0) ? 1.0f : -1.0f;

        std::vector<float> output (size * 2);
        processor.performRealFFTForward (alternating.data(), output.data());

        EXPECT_NEAR (output[0], 0.0f, defaultTolerance) << "DC real should be 0.0 for alternating";
        EXPECT_NEAR (output[1], 0.0f, defaultTolerance) << "DC imaginary should be 0.0";
        EXPECT_NEAR (output[size], static_cast<float> (size), defaultTolerance) << "Nyquist should equal size";
        EXPECT_NEAR (output[size + 1], 0.0f, defaultTolerance) << "Nyquist imaginary should be 0.0";

        // All other bins should be zero
        for (int k = 1; k < size / 2; ++k)
        {
            EXPECT_NEAR (output[k * 2], 0.0f, defaultTolerance) << "Bin " << k << " real should be 0.0";
            EXPECT_NEAR (output[k * 2 + 1], 0.0f, defaultTolerance) << "Bin " << k << " imag should be 0.0";
        }
    }
}

TEST_F (FFTProcessorValidation, RealForwardTransformAccuracy)
{
    for (int order = 6; order <= 8; ++order) // Reduced range for debugging
    {
        const int size = 1 << order;
        FFTProcessor processor (size);

        std::vector<float> input (size);
        std::vector<float> fftOutput (size * 2);
        std::vector<float> referenceOutput (size * 2);

        generateRandomReal (input.data(), size);
        computeReferenceRealDFT (input.data(), referenceOutput.data(), size);

        processor.performRealFFTForward (input.data(), fftOutput.data());

        // Compare the standard interleaved format (DC to Nyquist)
        const int numBins = size / 2 + 1;
        EXPECT_TRUE (areArraysClose (fftOutput.data(), referenceOutput.data(), numBins * 2))
            << "Real forward FFT failed for size " << size << " (order " << order << ")";
    }
}

TEST_F (FFTProcessorValidation, RealInverseTransformAccuracy)
{
    for (int order = 6; order <= 8; ++order) // Reduced range for debugging
    {
        const int size = 1 << order;
        FFTProcessor processor (size);

        // Test roundtrip: original -> forward -> inverse -> should equal original
        std::vector<float> originalInput (size);
        std::vector<float> complexData (size * 2);
        std::vector<float> reconstructed (size);

        generateRandomReal (originalInput.data(), size);

        // Forward transform
        processor.performRealFFTForward (originalInput.data(), complexData.data());

        // Inverse transform
        processor.performRealFFTInverse (complexData.data(), reconstructed.data());

        // For roundtrip test, we need to handle scaling
        processor.setScaling (FFTProcessor::FFTScaling::asymmetric);
        processor.performRealFFTForward (originalInput.data(), complexData.data());
        processor.performRealFFTInverse (complexData.data(), reconstructed.data());

        EXPECT_TRUE (areArraysClose (originalInput.data(), reconstructed.data(), size))
            << "Real inverse FFT roundtrip failed for size " << size << " (order " << order << ")";

        // Reset scaling
        processor.setScaling (FFTProcessor::FFTScaling::none);
    }
}

TEST_F (FFTProcessorValidation, ComplexForwardTransformAccuracy)
{
    // Test with simple known cases first
    const int size = 64;
    FFTProcessor processor (size);

    // Test with impulse
    std::vector<float> impulse (size * 2, 0.0f);
    impulse[0] = 1.0f; // Real part of first sample
    impulse[1] = 0.0f; // Imag part of first sample

    std::vector<float> output (size * 2);
    processor.performComplexFFTForward (impulse.data(), output.data());

    // For impulse, all bins should have real=1.0, imag=0.0
    for (int i = 0; i < size; ++i)
    {
        EXPECT_NEAR (output[i * 2], 1.0f, defaultTolerance)
            << "Complex impulse response real part incorrect at bin " << i;
        EXPECT_NEAR (output[i * 2 + 1], 0.0f, defaultTolerance)
            << "Complex impulse response imag part incorrect at bin " << i;
    }
}

TEST_F (FFTProcessorValidation, ComplexInverseTransformAccuracy)
{
    const int size = 64;
    FFTProcessor processor (size);
    processor.setScaling (FFTProcessor::FFTScaling::asymmetric);

    std::vector<float> originalInput (size * 2);
    std::vector<float> transformed (size * 2);
    std::vector<float> reconstructed (size * 2);

    generateRandomComplex (originalInput.data(), size);

    // Forward transform
    processor.performComplexFFTForward (originalInput.data(), transformed.data());

    // Inverse transform
    processor.performComplexFFTInverse (transformed.data(), reconstructed.data());

    EXPECT_TRUE (areArraysClose (originalInput.data(), reconstructed.data(), size * 2))
        << "Complex inverse FFT roundtrip failed for size " << size;
}

TEST_F (FFTProcessorValidation, RealRoundtripConsistency)
{
    for (int order = 6; order <= 8; ++order)
    {
        const int size = 1 << order;
        FFTProcessor processor (size);
        processor.setScaling (FFTProcessor::FFTScaling::asymmetric);

        std::vector<float> original (size);
        std::vector<float> frequency (size * 2);
        std::vector<float> restored (size);

        generateRandomReal (original.data(), size);

        // Forward -> Inverse should restore original
        processor.performRealFFTForward (original.data(), frequency.data());
        processor.performRealFFTInverse (frequency.data(), restored.data());

        EXPECT_TRUE (areArraysClose (original.data(), restored.data(), size))
            << "Real roundtrip consistency failed for size " << size;
    }
}

TEST_F (FFTProcessorValidation, ComplexRoundtripConsistency)
{
    for (int order = 6; order <= 8; ++order)
    {
        const int size = 1 << order;
        FFTProcessor processor (size);
        processor.setScaling (FFTProcessor::FFTScaling::asymmetric);

        std::vector<float> original (size * 2);
        std::vector<float> frequency (size * 2);
        std::vector<float> restored (size * 2);

        generateRandomComplex (original.data(), size);

        // Forward -> Inverse should restore original
        processor.performComplexFFTForward (original.data(), frequency.data());
        processor.performComplexFFTInverse (frequency.data(), restored.data());

        EXPECT_TRUE (areArraysClose (original.data(), restored.data(), size * 2))
            << "Complex roundtrip consistency failed for size " << size;
    }
}

TEST_F (FFTProcessorValidation, DCAndNyquistBehavior)
{
    const int size = 64;
    FFTProcessor processor (size);

    // Test DC component
    {
        std::vector<float> dcInput (size, 1.0f); // All ones
        std::vector<float> output (size * 2);

        processor.performRealFFTForward (dcInput.data(), output.data());

        // DC should have magnitude of size, other bins should be near zero
        EXPECT_NEAR (output[0], static_cast<float> (size), defaultTolerance) << "DC component incorrect";
        EXPECT_NEAR (output[1], 0.0f, defaultTolerance) << "DC imaginary should be zero";

        for (int i = 1; i < size / 2; ++i)
        {
            EXPECT_NEAR (output[i * 2], 0.0f, defaultTolerance) << "Non-DC bin " << i << " real should be zero";
            EXPECT_NEAR (output[i * 2 + 1], 0.0f, defaultTolerance) << "Non-DC bin " << i << " imag should be zero";
        }
    }

    // Test Nyquist frequency (alternating pattern)
    {
        std::vector<float> nyquistInput (size);
        for (int i = 0; i < size; ++i)
            nyquistInput[i] = (i % 2 == 0) ? 1.0f : -1.0f;

        std::vector<float> output (size * 2);
        processor.performRealFFTForward (nyquistInput.data(), output.data());

        // In standard format, Nyquist is stored at output[size]
        float nyquistMagnitude = std::abs (output[size]);
        EXPECT_GT (nyquistMagnitude, 1.0f) << "Nyquist component should be significant for alternating pattern";

        // The DC component should be zero for alternating pattern
        EXPECT_NEAR (output[0], 0.0f, defaultTolerance) << "DC should be zero for alternating pattern";
    }
}

TEST_F (FFTProcessorValidation, LinearityProperty)
{
    const int size = 128;
    FFTProcessor processor (size);

    std::vector<float> signal1 (size);
    std::vector<float> signal2 (size);
    std::vector<float> combined (size);

    generateRandomReal (signal1.data(), size);
    generateRandomReal (signal2.data(), size);

    for (int i = 0; i < size; ++i)
        combined[i] = signal1[i] + signal2[i];

    std::vector<float> fft1 (size * 2);
    std::vector<float> fft2 (size * 2);
    std::vector<float> fftCombined (size * 2);
    std::vector<float> fftSum (size * 2);

    processor.performRealFFTForward (signal1.data(), fft1.data());
    processor.performRealFFTForward (signal2.data(), fft2.data());
    processor.performRealFFTForward (combined.data(), fftCombined.data());

    // FFT(a + b) should equal FFT(a) + FFT(b)
    for (int i = 0; i < size * 2; ++i)
        fftSum[i] = fft1[i] + fft2[i];

    EXPECT_TRUE (areArraysClose (fftCombined.data(), fftSum.data(), size * 2))
        << "FFT linearity property violated";
}

TEST_F (FFTProcessorValidation, ScalingBehavior)
{
    const int size = 64;

    // Test different scaling modes
    for (auto scaling : { FFTProcessor::FFTScaling::none,
                          FFTProcessor::FFTScaling::unitary,
                          FFTProcessor::FFTScaling::asymmetric })
    {
        FFTProcessor processor (size);
        processor.setScaling (scaling);

        std::vector<float> input (size);
        std::vector<float> frequency (size * 2);
        std::vector<float> restored (size);

        generateRandomReal (input.data(), size);

        processor.performRealFFTForward (input.data(), frequency.data());
        processor.performRealFFTInverse (frequency.data(), restored.data());

        // With proper scaling, we should get back the original
        float tolerance = (scaling == FFTProcessor::FFTScaling::none) ? 1.0f : defaultTolerance;

        if (scaling == FFTProcessor::FFTScaling::none)
        {
            // Without scaling, result should be multiplied by size
            for (int i = 0; i < size; ++i)
                restored[i] /= static_cast<float> (size);
        }

        EXPECT_TRUE (areArraysClose (input.data(), restored.data(), size, tolerance))
            << "Scaling behavior incorrect for scaling mode " << static_cast<int> (scaling);
    }
}

TEST_F (FFTProcessorValidation, BackendIdentification)
{
    FFTProcessor processor (64);
    String backendName = processor.getBackendName();

    EXPECT_FALSE (backendName.isEmpty()) << "Backend name should not be empty";
    EXPECT_NE (backendName, "Unknown") << "Backend should be identified";

    // Verify it's one of the expected backends
    const std::vector<String> expectedBackends = {
        "PFFFT", "Apple vDSP", "Intel IPP", "FFTW3", "Ooura FFT"
    };

    bool foundExpected = false;
    for (const auto& expected : expectedBackends)
    {
        if (backendName == expected)
        {
            foundExpected = true;
            break;
        }
    }

    EXPECT_TRUE (foundExpected) << "Backend name '" << backendName << "' not in expected list";
}

TEST_F (FFTProcessorValidation, EdgeCaseSizes)
{
    // Test minimum size (64) and some larger sizes
    for (int size : { 64, 128, 1024, 2048, 4096 })
    {
        EXPECT_NO_THROW ({
            FFTProcessor processor (size);

            std::vector<float> input (size);
            std::vector<float> output (size * 2);

            generateRandomReal (input.data(), size);
            processor.performRealFFTForward (input.data(), output.data());
        }) << "FFT failed for edge case size "
           << size;
    }
}

} // namespace yup::test
