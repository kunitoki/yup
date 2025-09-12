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

#pragma once

namespace yup
{

//==============================================================================
/**
    Direct-form FIR (Finite Impulse Response) filter processor optimized for real-time audio.

    Implements a time-domain FIR filter using direct convolution with SIMD optimizations.
    This class is ideal for low-latency applications where the number of coefficients is relatively
    small (typically < 512 coefficients), as it provides zero algorithmic delay.

    Features:
    - Zero algorithmic latency (only processing delay)
    - SIMD-optimized convolution (AVX2, SSE, ARM NEON, vDSP)
    - Circular buffer implementation for efficient sample history management
    - Real-time safe processing (no heap allocations during process())
    - Support for arbitrary block sizes
    - Inherits FilterBase interface for frequency response analysis

    Example usage:
    @code
    DirectFIR<float> fir;

    // Set filter coefficients (e.g., lowpass filter)
    auto coeffs = FilterDesigner<float>::designFIRLowpass(64, 1000.0f, 44100.0);
    fir.setCoefficients(coeffs);

    // Prepare for processing
    fir.prepare(44100.0, 512);

    // In audio callback:
    fir.processBlock(inputBuffer, outputBuffer, numSamples);
    @endcode

    @tparam SampleType  Type for audio samples (float or double)
    @tparam CoeffType   Type for internal coefficients (defaults to double)

    @see PartitionedConvolver for longer impulse responses using FFT-based convolution
    @see FilterBase for frequency response methods
*/
template <typename SampleType, typename CoeffType = double>
class DirectFIR : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Default constructor */
    DirectFIR() = default;

    /** Destructor */
    ~DirectFIR() override = default;

    /** Move constructor */
    DirectFIR (DirectFIR&& other) noexcept
        : coefficientsReversed (std::move (other.coefficientsReversed))
        , history (std::move (other.history))
        , numCoefficients (std::exchange (other.numCoefficients, 0))
        , paddedLen (std::exchange (other.paddedLen, 0))
        , writeIndex (std::exchange (other.writeIndex, 0))
        , currentScaling (std::exchange (other.currentScaling, CoeffType (1)))
    {
    }

    /** Move assignment */
    DirectFIR& operator= (DirectFIR&& other) noexcept
    {
        if (this != &other)
        {
            coefficientsReversed = std::move (other.coefficientsReversed);
            history = std::move (other.history);
            numCoefficients = std::exchange (other.numCoefficients, 0);
            paddedLen = std::exchange (other.paddedLen, 0);
            writeIndex = std::exchange (other.writeIndex, 0);
            currentScaling = std::exchange (other.currentScaling, CoeffType (1));
        }
        return *this;
    }

    //==============================================================================
    /**
        Set the FIR filter coefficients.

        @param coefficients  Vector containing the FIR coefficients in time order
        @param scaling       Scaling factor to apply to all coefficients

        @note This method is not real-time safe and should be called during initialization
              or when audio processing is paused.
    */
    void setCoefficients (std::vector<CoeffType> coefficients, CoeffType scaling = CoeffType (1))
    {
        currentScaling = scaling;
        if (! approximatelyEqual (currentScaling, 1.0f))
            FloatVectorOperations::multiply (coefficients.data(), scaling, coefficients.size());

        coefficientsReversed = std::move (coefficients);
        std::reverse (coefficientsReversed.begin(), coefficientsReversed.end());

        numCoefficients = coefficientsReversed.size();
        paddedLen = (numCoefficients + 3u) & ~3u; // Round up to multiple of 4 for SIMD
        coefficientsReversed.resize (paddedLen, 0.0f);

        history.assign (2 * numCoefficients, 0.0f);
        reset();
    }

    /**
        Set the FIR filter coefficients from a raw pointer.

        @param coefficients     Pointer to FIR coefficients array
        @param numCoefficients  Number of coefficients
        @param scaling          Scaling factor to apply to all coefficients

        @note This method is not real-time safe and should be called during initialization
              or when audio processing is paused.
    */
    void setCoefficients (const CoeffType* coefficients, std::size_t numCoefficientsIn, CoeffType scaling = CoeffType (1))
    {
        if (coefficients == nullptr || numCoefficientsIn == 0)
        {
            reset();
            numCoefficients = 0;
            return;
        }

        std::vector<float> coefficientsVector (coefficients, coefficients + numCoefficientsIn);
        setCoefficients (std::move (coefficientsVector), scaling);
    }

    /**
        Get the number of filter coefficients.

        @return Number of coefficients in the current filter
    */
    std::size_t getNumCoefficients() const noexcept
    {
        return numCoefficients;
    }

    /**
        Check if the filter has been configured with coefficients.

        @return True if coefficients have been set, false otherwise
    */
    bool hasCoefficients() const noexcept
    {
        return numCoefficients > 0;
    }

    /**
        Get the current filter coefficients.

        @return Vector containing the current coefficients (time-reversed for processing)
    */
    const std::vector<CoeffType>& getCoefficients() const noexcept
    {
        return coefficientsReversed;
    }

    /**
        Get the current scaling factor applied to coefficients.

        @return Current scaling factor
    */
    CoeffType getScaling() const noexcept
    {
        return currentScaling;
    }

    //==============================================================================
    /**
        Reset all internal processing state (clears sample history).
        Filter coefficients are preserved.
    */
    void reset() noexcept override
    {
        std::fill (history.begin(), history.end(), 0.0f);
        writeIndex = 0;
    }

    /**
        Prepares the filter for processing with the given sample rate and block size.

        @param sampleRate     The sample rate in Hz
        @param maximumBlockSize  The maximum number of samples that will be processed at once
    */
    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
    }

    /**
        Processes a single sample.

        @param inputSample  The input sample to process
        @returns           The filtered output sample
    */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        const std::size_t M = numCoefficients;
        const CoeffType* h = coefficientsReversed.data();

        // Update circular buffer with current input sample
        history[writeIndex] = inputSample;
        history[writeIndex + M] = inputSample; // Duplicate for efficient circular access

        // Point to the start of the delay line for this sample
        const SampleType* w = history.data() + writeIndex + 1;

        // Advance circular buffer write pointer
        if (++writeIndex == M)
            writeIndex = 0;

        return dotProduct (w, h, M);
    }

    /**
        Processes a block of samples.

        @param inputBuffer   Pointer to the input samples
        @param outputBuffer  Pointer to the output buffer
        @param numSamples    Number of samples to process
    */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        if (numCoefficients == 0 || inputBuffer == nullptr || outputBuffer == nullptr)
            return;

        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] += processSample (inputBuffer[i]);
    }

    /**
        Returns the complex frequency response at the given frequency.

        @param frequency  The frequency in Hz
        @returns         The complex frequency response
    */
    Complex<CoeffType> getComplexResponse (CoeffType frequency) const override
    {
        if (numCoefficients == 0)
            return Complex<CoeffType> (0, 0);

        const CoeffType omega = MathConstants<CoeffType>::twoPi * frequency / static_cast<CoeffType> (this->sampleRate);

        Complex<CoeffType> response (0, 0);

        // H(e^jω) = Σ h[n] * e^(-jωn) for n = 0 to N-1
        for (std::size_t n = 0; n < numCoefficients; ++n)
        {
            const CoeffType angle = -omega * static_cast<CoeffType> (n);
            Complex<CoeffType> exponential (std::cos (angle), std::sin (angle));
            response += static_cast<CoeffType> (coefficientsReversed[numCoefficients - 1 - n]) * exponential;
        }

        return response;
    }

    /**
        Process audio samples through the FIR filter (legacy method).

        @param input       Input audio buffer
        @param output      Output audio buffer (results are accumulated)
        @param numSamples  Number of samples to process

        @note Results are accumulated into the output buffer. Clear it first if needed.
        @note This method is real-time safe with no heap allocations.
        @note Use processBlock() for new code
    */
    void process (const SampleType* input, SampleType* output, std::size_t numSamples) noexcept
    {
        processBlock (input, output, static_cast<int> (numSamples));
    }

private:
    std::vector<CoeffType> coefficientsReversed;
    std::vector<SampleType> history;
    std::size_t numCoefficients = 0;
    std::size_t paddedLen = 0;
    std::size_t writeIndex = 0;
    CoeffType currentScaling = CoeffType (1);

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectFIR)
};

//==============================================================================
/** Type aliases for backward compatibility and convenience */
using DirectFIRFloat = DirectFIR<float, float>;
using DirectFIRDouble = DirectFIR<double, double>;

} // namespace yup
