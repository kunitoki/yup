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

/** Shared validation fixture, instantiated once per sample precision. */
template <typename SampleTypeT>
class FFTProcessorValidationT : public ::testing::Test
{
protected:
    using SampleType = SampleTypeT;
    using ProcessorType = FFTProcessor<SampleTypeT>;

    void SetUp() override
    {
        generator.seed (42); // Fixed seed for reproducible tests
    }

    // Generate random sample in range [-1, 1]
    SampleType randomSample()
    {
        std::uniform_real_distribution<SampleType> dist (SampleType (-1), SampleType (1));
        return dist (generator);
    }

    // Fill buffer with random real values
    void generateRandomReal (SampleType* buffer, int size)
    {
        for (int i = 0; i < size; ++i)
            buffer[i] = randomSample();
    }

    // Fill buffer with random complex values (interleaved real/imag)
    void generateRandomComplex (SampleType* buffer, int size)
    {
        for (int i = 0; i < size * 2; ++i)
            buffer[i] = randomSample();
    }

    // Reference discrete Fourier transform for real input (produces full spectrum)
    void computeReferenceDFT (const SampleType* realInput, SampleType* complexOutput, int size, bool inverse = false)
    {
        const SampleType sign = inverse ? SampleType (1) : SampleType (-1);
        const SampleType twoPi = SampleType (2) * MathConstants<SampleType>::pi;

        for (int k = 0; k < size; ++k)
        {
            SampleType realSum = SampleType (0);
            SampleType imagSum = SampleType (0);

            for (int n = 0; n < size; ++n)
            {
                const SampleType angle = sign * twoPi * static_cast<SampleType> (k * n) / static_cast<SampleType> (size);
                const SampleType cosVal = std::cos (angle);
                const SampleType sinVal = std::sin (angle);

                realSum += realInput[n] * cosVal;
                imagSum += realInput[n] * sinVal;
            }

            complexOutput[k * 2] = realSum;
            complexOutput[k * 2 + 1] = imagSum;
        }
    }

    // Reference DFT for real input producing standard interleaved format
    void computeReferenceRealDFT (const SampleType* realInput, SampleType* interleavedOutput, int size)
    {
        const SampleType twoPi = SampleType (2) * MathConstants<SampleType>::pi;
        const int numBins = size / 2 + 1;

        // Compute all frequency bins (k=0 to size/2)
        for (int k = 0; k < numBins; ++k)
        {
            SampleType realSum = SampleType (0);
            SampleType imagSum = SampleType (0);

            for (int n = 0; n < size; ++n)
            {
                const SampleType angle = -twoPi * static_cast<SampleType> (k * n) / static_cast<SampleType> (size);
                const SampleType cosVal = std::cos (angle);
                const SampleType sinVal = std::sin (angle);

                realSum += realInput[n] * cosVal;
                imagSum += realInput[n] * sinVal;
            }

            interleavedOutput[k * 2] = realSum;
            interleavedOutput[k * 2 + 1] = imagSum;
        }
    }

    // Reference inverse DFT for hermitian-symmetric input producing real output
    void computeReferenceRealIDFT (const SampleType* complexInput, SampleType* realOutput, int size)
    {
        const SampleType twoPi = SampleType (2) * MathConstants<SampleType>::pi;
        const int numBins = size / 2 + 1;

        for (int n = 0; n < size; ++n)
        {
            SampleType sum = SampleType (0);

            // DC component
            sum += complexInput[0];

            // Other frequencies (except Nyquist)
            for (int k = 1; k < numBins - 1; ++k)
            {
                const SampleType angle = twoPi * static_cast<SampleType> (k * n) / static_cast<SampleType> (size);
                const SampleType cosVal = std::cos (angle);
                const SampleType sinVal = std::sin (angle);

                const SampleType real = complexInput[k * 2];
                const SampleType imag = complexInput[k * 2 + 1];

                sum += SampleType (2) * (real * cosVal + imag * sinVal);
            }

            // Nyquist component (if size is even)
            if (size % 2 == 0)
            {
                const int nyquistBin = size / 2;
                const SampleType nyquistAngle = twoPi * static_cast<SampleType> (nyquistBin * n) / static_cast<SampleType> (size);
                sum += complexInput[nyquistBin * 2] * std::cos (nyquistAngle);
            }

            realOutput[n] = sum / static_cast<SampleType> (size);
        }
    }

    // Reference DFT for complex input (interleaved format)
    void computeReferenceComplexDFT (const SampleType* complexInput, SampleType* complexOutput, int size, bool inverse = false)
    {
        const SampleType sign = inverse ? SampleType (1) : SampleType (-1);
        const SampleType twoPi = SampleType (2) * MathConstants<SampleType>::pi;

        for (int k = 0; k < size; ++k)
        {
            SampleType realSum = SampleType (0);
            SampleType imagSum = SampleType (0);

            for (int n = 0; n < size; ++n)
            {
                const SampleType angle = sign * twoPi * static_cast<SampleType> (k * n) / static_cast<SampleType> (size);
                const SampleType cosVal = std::cos (angle);
                const SampleType sinVal = std::sin (angle);

                const SampleType inputReal = complexInput[n * 2];
                const SampleType inputImag = complexInput[n * 2 + 1];

                realSum += inputReal * cosVal - inputImag * sinVal;
                imagSum += inputReal * sinVal + inputImag * cosVal;
            }

            complexOutput[k * 2] = realSum;
            complexOutput[k * 2 + 1] = imagSum;
        }
    }

    // Check if two arrays are approximately equal
    bool areArraysClose (const SampleType* a, const SampleType* b, int size, SampleType tolerance = defaultTolerance)
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
    static constexpr SampleType defaultTolerance = SampleType (1e-3);
    static constexpr SampleType tightTolerance = std::is_same_v<SampleType, double> ? SampleType (1e-9) : SampleType (1e-4);
};

using FFTProcessorValidation = FFTProcessorValidationT<float>;
using FFTProcessorDoubleValidation = FFTProcessorValidationT<double>;

//==============================================================================
TEST_F (FFTProcessorValidation, StandardFormatValidation)
{
    const int size = 64;
    ProcessorType processor (size);

    // Test 1: Impulse should produce flat spectrum
    {
        std::vector<SampleType> impulse (size, SampleType (0));
        impulse[0] = SampleType (1);

        std::vector<SampleType> output (size * 2);
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
        std::vector<SampleType> dcSignal (size, SampleType (1));

        std::vector<SampleType> output (size * 2);
        processor.performRealFFTForward (dcSignal.data(), output.data());

        EXPECT_NEAR (output[0], static_cast<SampleType> (size), defaultTolerance) << "DC real should equal sum";
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
        std::vector<SampleType> alternating (size);
        for (int i = 0; i < size; ++i)
            alternating[i] = (i % 2 == 0) ? SampleType (1) : SampleType (-1);

        std::vector<SampleType> output (size * 2);
        processor.performRealFFTForward (alternating.data(), output.data());

        EXPECT_NEAR (output[0], 0.0f, defaultTolerance) << "DC real should be 0.0 for alternating";
        EXPECT_NEAR (output[1], 0.0f, defaultTolerance) << "DC imaginary should be 0.0";
        EXPECT_NEAR (output[size], static_cast<SampleType> (size), defaultTolerance) << "Nyquist should equal size";
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
        ProcessorType processor (size);

        std::vector<SampleType> input (size);
        std::vector<SampleType> fftOutput (size * 2);
        std::vector<SampleType> referenceOutput (size * 2);

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
        ProcessorType processor (size);

        // Test roundtrip: original -> forward -> inverse -> should equal original
        std::vector<SampleType> originalInput (size);
        std::vector<SampleType> complexData (size * 2);
        std::vector<SampleType> reconstructed (size);

        generateRandomReal (originalInput.data(), size);

        // Forward transform
        processor.performRealFFTForward (originalInput.data(), complexData.data());

        // Inverse transform
        processor.performRealFFTInverse (complexData.data(), reconstructed.data());

        // For roundtrip test, we need to handle scaling
        processor.setScaling (ProcessorType::FFTScaling::asymmetric);
        processor.performRealFFTForward (originalInput.data(), complexData.data());
        processor.performRealFFTInverse (complexData.data(), reconstructed.data());

        EXPECT_TRUE (areArraysClose (originalInput.data(), reconstructed.data(), size))
            << "Real inverse FFT roundtrip failed for size " << size << " (order " << order << ")";

        // Reset scaling
        processor.setScaling (ProcessorType::FFTScaling::none);
    }
}

TEST_F (FFTProcessorValidation, ComplexForwardTransformAccuracy)
{
    // Test with simple known cases first
    const int size = 64;
    ProcessorType processor (size);

    // Test with impulse
    std::vector<SampleType> impulse (size * 2, SampleType (0));
    impulse[0] = SampleType (1); // Real part of first sample
    impulse[1] = SampleType (0); // Imag part of first sample

    std::vector<SampleType> output (size * 2);
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
    ProcessorType processor (size);
    processor.setScaling (ProcessorType::FFTScaling::asymmetric);

    std::vector<SampleType> originalInput (size * 2);
    std::vector<SampleType> transformed (size * 2);
    std::vector<SampleType> reconstructed (size * 2);

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
        ProcessorType processor (size);
        processor.setScaling (ProcessorType::FFTScaling::asymmetric);

        std::vector<SampleType> original (size);
        std::vector<SampleType> frequency (size * 2);
        std::vector<SampleType> restored (size);

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
        ProcessorType processor (size);
        processor.setScaling (ProcessorType::FFTScaling::asymmetric);

        std::vector<SampleType> original (size * 2);
        std::vector<SampleType> frequency (size * 2);
        std::vector<SampleType> restored (size * 2);

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
    ProcessorType processor (size);

    // Test DC component
    {
        std::vector<SampleType> dcInput (size, SampleType (1)); // All ones
        std::vector<SampleType> output (size * 2);

        processor.performRealFFTForward (dcInput.data(), output.data());

        // DC should have magnitude of size, other bins should be near zero
        EXPECT_NEAR (output[0], static_cast<SampleType> (size), defaultTolerance) << "DC component incorrect";
        EXPECT_NEAR (output[1], 0.0f, defaultTolerance) << "DC imaginary should be zero";

        for (int i = 1; i < size / 2; ++i)
        {
            EXPECT_NEAR (output[i * 2], 0.0f, defaultTolerance) << "Non-DC bin " << i << " real should be zero";
            EXPECT_NEAR (output[i * 2 + 1], 0.0f, defaultTolerance) << "Non-DC bin " << i << " imag should be zero";
        }
    }

    // Test Nyquist frequency (alternating pattern)
    {
        std::vector<SampleType> nyquistInput (size);
        for (int i = 0; i < size; ++i)
            nyquistInput[i] = (i % 2 == 0) ? SampleType (1) : SampleType (-1);

        std::vector<SampleType> output (size * 2);
        processor.performRealFFTForward (nyquistInput.data(), output.data());

        // In standard format, Nyquist is stored at output[size]
        SampleType nyquistMagnitude = std::abs (output[size]);
        EXPECT_GT (nyquistMagnitude, 1.0f) << "Nyquist component should be significant for alternating pattern";

        // The DC component should be zero for alternating pattern
        EXPECT_NEAR (output[0], 0.0f, defaultTolerance) << "DC should be zero for alternating pattern";
    }
}

TEST_F (FFTProcessorValidation, LinearityProperty)
{
    const int size = 128;
    ProcessorType processor (size);

    std::vector<SampleType> signal1 (size);
    std::vector<SampleType> signal2 (size);
    std::vector<SampleType> combined (size);

    generateRandomReal (signal1.data(), size);
    generateRandomReal (signal2.data(), size);

    for (int i = 0; i < size; ++i)
        combined[i] = signal1[i] + signal2[i];

    std::vector<SampleType> fft1 (size * 2);
    std::vector<SampleType> fft2 (size * 2);
    std::vector<SampleType> fftCombined (size * 2);
    std::vector<SampleType> fftSum (size * 2);

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
    for (auto scaling : { ProcessorType::FFTScaling::none,
                          ProcessorType::FFTScaling::unitary,
                          ProcessorType::FFTScaling::asymmetric })
    {
        ProcessorType processor (size);
        processor.setScaling (scaling);

        std::vector<SampleType> input (size);
        std::vector<SampleType> frequency (size * 2);
        std::vector<SampleType> restored (size);

        generateRandomReal (input.data(), size);

        processor.performRealFFTForward (input.data(), frequency.data());
        processor.performRealFFTInverse (frequency.data(), restored.data());

        // With proper scaling, we should get back the original
        SampleType tolerance = (scaling == ProcessorType::FFTScaling::none) ? SampleType (1) : defaultTolerance;

        if (scaling == ProcessorType::FFTScaling::none)
        {
            // Without scaling, result should be multiplied by size
            for (int i = 0; i < size; ++i)
                restored[i] /= static_cast<SampleType> (size);
        }

        EXPECT_TRUE (areArraysClose (input.data(), restored.data(), size, tolerance))
            << "Scaling behavior incorrect for scaling mode " << static_cast<int> (scaling);
    }
}

TEST_F (FFTProcessorValidation, BackendIdentification)
{
    ProcessorType processor (64);
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
            ProcessorType processor (size);

            std::vector<SampleType> input (size);
            std::vector<SampleType> output (size * 2);

            generateRandomReal (input.data(), size);
            processor.performRealFFTForward (input.data(), output.data());
        }) << "FFT failed for edge case size "
           << size;
    }
}

//==============================================================================
// Double precision validation - the same public interface at a higher precision
//==============================================================================

TEST_F (FFTProcessorDoubleValidation, BackendIdentification)
{
    ProcessorType processor (64);
    const String backendName = processor.getBackendName();

    EXPECT_FALSE (backendName.isEmpty()) << "Backend name should not be empty";
    EXPECT_NE (backendName, "Unknown") << "Backend should be identified";
}

TEST_F (FFTProcessorDoubleValidation, RealForwardTransformAccuracy)
{
    for (int order = 6; order <= 9; ++order)
    {
        const int size = 1 << order;
        ProcessorType processor (size);

        std::vector<SampleType> input (size);
        std::vector<SampleType> fftOutput (size * 2);
        std::vector<SampleType> referenceOutput (size * 2);

        generateRandomReal (input.data(), size);
        computeReferenceRealDFT (input.data(), referenceOutput.data(), size);

        processor.performRealFFTForward (input.data(), fftOutput.data());

        const int numBins = size / 2 + 1;
        EXPECT_TRUE (areArraysClose (fftOutput.data(), referenceOutput.data(), numBins * 2, tightTolerance))
            << "Double real forward FFT failed for size " << size;
    }
}

TEST_F (FFTProcessorDoubleValidation, RealRoundtripConsistency)
{
    for (int order = 6; order <= 9; ++order)
    {
        const int size = 1 << order;
        ProcessorType processor (size);
        processor.setScaling (ProcessorType::FFTScaling::asymmetric);

        std::vector<SampleType> original (size);
        std::vector<SampleType> frequency (size * 2);
        std::vector<SampleType> restored (size);

        generateRandomReal (original.data(), size);

        processor.performRealFFTForward (original.data(), frequency.data());
        processor.performRealFFTInverse (frequency.data(), restored.data());

        EXPECT_TRUE (areArraysClose (original.data(), restored.data(), size, tightTolerance))
            << "Double real roundtrip failed for size " << size;
    }
}

TEST_F (FFTProcessorDoubleValidation, ComplexRoundtripConsistency)
{
    for (int order = 6; order <= 9; ++order)
    {
        const int size = 1 << order;
        ProcessorType processor (size);
        processor.setScaling (ProcessorType::FFTScaling::asymmetric);

        std::vector<SampleType> original (size * 2);
        std::vector<SampleType> frequency (size * 2);
        std::vector<SampleType> restored (size * 2);

        generateRandomComplex (original.data(), size);

        processor.performComplexFFTForward (original.data(), frequency.data());
        processor.performComplexFFTInverse (frequency.data(), restored.data());

        EXPECT_TRUE (areArraysClose (original.data(), restored.data(), size * 2, tightTolerance))
            << "Double complex roundtrip failed for size " << size;
    }
}

TEST_F (FFTProcessorDoubleValidation, DcAndNyquistBinsAreExact)
{
    const int size = 64;
    ProcessorType processor (size);

    // An impulse carries unit energy in every bin
    {
        std::vector<SampleType> impulse (size, SampleType (0));
        impulse[0] = SampleType (1);

        std::vector<SampleType> output (size * 2);
        processor.performRealFFTForward (impulse.data(), output.data());

        EXPECT_NEAR (output[0], 1.0, tightTolerance);
        EXPECT_NEAR (output[1], 0.0, tightTolerance);
        EXPECT_NEAR (output[size], 1.0, tightTolerance);
        EXPECT_NEAR (output[size + 1], 0.0, tightTolerance);
    }

    // An alternating pattern carries all of its energy at Nyquist
    {
        std::vector<SampleType> alternating (size);
        for (int i = 0; i < size; ++i)
            alternating[i] = (i % 2 == 0) ? SampleType (1) : SampleType (-1);

        std::vector<SampleType> output (size * 2);
        processor.performRealFFTForward (alternating.data(), output.data());

        EXPECT_NEAR (output[0], 0.0, tightTolerance);
        EXPECT_NEAR (output[size], static_cast<SampleType> (size), tightTolerance);
        EXPECT_NEAR (output[size + 1], 0.0, tightTolerance);
    }
}

TEST_F (FFTProcessorDoubleValidation, ScalingBehavior)
{
    const int size = 64;

    for (auto scaling : { ProcessorType::FFTScaling::none,
                          ProcessorType::FFTScaling::unitary,
                          ProcessorType::FFTScaling::asymmetric })
    {
        ProcessorType processor (size);
        processor.setScaling (scaling);
        EXPECT_EQ (processor.getScaling(), scaling);

        std::vector<SampleType> input (size);
        std::vector<SampleType> frequency (size * 2);
        std::vector<SampleType> restored (size);

        generateRandomReal (input.data(), size);

        processor.performRealFFTForward (input.data(), frequency.data());
        processor.performRealFFTInverse (frequency.data(), restored.data());

        SampleType tolerance = (scaling == ProcessorType::FFTScaling::none) ? SampleType (1000) : tightTolerance;

        if (scaling == ProcessorType::FFTScaling::none)
        {
            // Without scaling, the roundtrip result is scaled by the FFT size
            for (int i = 0; i < size; ++i)
                restored[i] /= static_cast<SampleType> (size);
        }

        EXPECT_TRUE (areArraysClose (input.data(), restored.data(), size, tolerance))
            << "Double scaling behavior incorrect for scaling mode " << static_cast<int> (scaling);
    }
}

TEST_F (FFTProcessorDoubleValidation, EdgeCaseSizes)
{
    for (int size : { 64, 128, 1024, 2048, 4096 })
    {
        EXPECT_NO_THROW ({
            ProcessorType processor (size);

            std::vector<SampleType> input (size);
            std::vector<SampleType> output (size * 2);

            generateRandomReal (input.data(), size);
            processor.performRealFFTForward (input.data(), output.data());
        }) << "Double FFT failed for edge case size "
           << size;
    }
}

} // namespace yup::test
