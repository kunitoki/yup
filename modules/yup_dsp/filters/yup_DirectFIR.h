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

    Example usage:
    @code
    DirectFIR fir;

    // Set filter coefficients (e.g., lowpass filter)
    std::vector<float> coeffs = calculateLowpassCoeffs(44100.0f, 1000.0f, 64);
    fir.setTaps(coeffs, 1.0f); // coeffs with 1.0x scaling

    // Prepare for processing
    fir.prepare(512); // Maximum 512 samples per process() call

    // In audio callback:
    fir.process(inputBuffer, outputBuffer, numSamples); // Accumulates into output
    @endcode

    @note The process() method accumulates results into the output buffer.
          Clear the output buffer first if overwrite behavior is desired.

    @see PartitionedConvolver for longer impulse responses using FFT-based convolution
*/
class DirectFIR
{
public:
    //==============================================================================
    /** Default constructor */
    DirectFIR();

    /** Destructor */
    ~DirectFIR();

    // Non-copyable but movable
    DirectFIR (DirectFIR&& other) noexcept;
    DirectFIR& operator= (DirectFIR&& other) noexcept;

    //==============================================================================
    /**
        Set the FIR filter coefficients.

        @param coefficients  Vector containing the FIR coefficients in time order
        @param scaling       Scaling factor to apply to all coefficients

        @note This method is not real-time safe and should be called during initialization
              or when audio processing is paused.
    */
    void setCoefficients (std::vector<float> coefficients, float scaling = 1.0f);

    /**
        Set the FIR filter coefficients from a raw pointer.

        @param coefficients     Pointer to FIR coefficients array
        @param numCoefficients  Number of coefficients
        @param scaling          Scaling factor to apply to all coefficients

        @note This method is not real-time safe and should be called during initialization
              or when audio processing is paused.
    */
    void setCoefficients (const float* coefficients, std::size_t numCoefficients, float scaling = 1.0f);

    /**
        Get the number of filter coefficients.

        @return Number of coefficients in the current filter
    */
    std::size_t getNumCoefficients() const noexcept;

    /**
        Check if the filter has been configured with coefficients.

        @return True if coefficients have been set, false otherwise
    */
    bool hasCoefficients() const noexcept;

    /**
        Get the current filter coefficients.

        @return Vector containing the current coefficients (time-reversed for processing)
    */
    const std::vector<float>& getCoefficients() const noexcept;

    /**
        Get the current scaling factor applied to coefficients.

        @return Current scaling factor
    */
    float getScaling() const noexcept;

    //==============================================================================
    /**
        Reset all internal processing state (clears sample history).
        Filter coefficients are preserved.
    */
    void reset();

    /**
        Process audio samples through the FIR filter.

        @param input       Input audio buffer
        @param output      Output audio buffer (results are accumulated)
        @param numSamples  Number of samples to process

        @note Results are accumulated into the output buffer. Clear it first if needed.
        @note This method is real-time safe with no heap allocations.
    */
    void process (const float* input, float* output, std::size_t numSamples) noexcept;

private:
    std::vector<float> coefficientsReversed;
    std::vector<float> history;
    std::size_t numCoefficients = 0;
    std::size_t paddedLen = 0;
    std::size_t writeIndex = 0;
    float currentScaling = 1.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DirectFIR)
};

} // namespace yup
