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
    Multi-backend FFT processor that provides a unified interface for different
    FFT implementations.

    Supports the following backends (in order of preference):
    - PFFFT (cross-platform, SIMD optimized)
    - Apple vDSP (macOS/iOS)
    - Intel IPP
    - FFTW3
    - Ooura FFT (fallback)

    The class automatically selects the best available backend at compile time
    based on preprocessor definitions and platform availability.

    @note This class only works with float buffers for optimal performance.

    Example usage:
    @code
    FFTProcessor fft (512);  // 512-point FFT

    std::vector<float> realInput (512);
    std::vector<float> complexOutput (1024);  // 512 complex pairs = 1024 floats

    // Fill realInput with your audio data...

    fft.performRealFFTForward (realInput.data(), complexOutput.data());
    @endcode
*/
class FFTProcessor
{
public:
    //==============================================================================
    /** FFT scaling options */
    enum class FFTScaling
    {
        none,      /**< No scaling applied */
        unitary,   /**< Unitary scaling (1/sqrt(N)) */
        asymmetric /**< Asymmetric scaling (1/N for inverse only) */
    };

    //==============================================================================
    /** Constructor - initializes with default size of 512 */
    FFTProcessor();

    /** Constructor with specific FFT size
        @param fftSize  The FFT size (must be power of 2)
    */
    explicit FFTProcessor (int fftSize);

    /** Destructor */
    ~FFTProcessor();

    // Non-copyable but movable
    FFTProcessor (FFTProcessor&& other) noexcept;
    FFTProcessor& operator= (FFTProcessor&& other) noexcept;

    //==============================================================================
    /** Sets the FFT size (must be power of 2) */
    void setSize (int newSize);

    /** Gets the current FFT size */
    int getSize() const noexcept { return fftSize; }

    /** Sets the FFT scaling mode */
    void setScaling (FFTScaling newScaling) noexcept { scaling = newScaling; }

    /** Gets the current scaling mode */
    FFTScaling getScaling() const noexcept { return scaling; }

    //==============================================================================
    /**
        Performs a forward real-to-complex FFT.

        @param realInput     Input buffer containing real samples (fftSize elements)
        @param complexOutput Output buffer for complex data (fftSize * 2 elements, interleaved real/imag)
    */
    void performRealFFTForward (const float* realInput, float* complexOutput);

    /**
        Performs an inverse complex-to-real FFT.

        @param complexInput  Input buffer containing complex data (fftSize * 2 elements, interleaved real/imag)
        @param realOutput    Output buffer for real data (fftSize elements)
    */
    void performRealFFTInverse (const float* complexInput, float* realOutput);

    /**
        Performs a forward complex-to-complex FFT.

        @param complexInput  Input buffer containing complex data (fftSize * 2 elements, interleaved real/imag)
        @param complexOutput Output buffer for complex data (fftSize * 2 elements, interleaved real/imag)
    */
    void performComplexFFTForward (const float* complexInput, float* complexOutput);

    /**
        Performs an inverse complex-to-complex FFT.

        @param complexInput  Input buffer containing complex data (fftSize * 2 elements, interleaved real/imag)
        @param complexOutput Output buffer for complex data (fftSize * 2 elements, interleaved real/imag)
    */
    void performComplexFFTInverse (const float* complexInput, float* complexOutput);

    //==============================================================================
    /** Returns a string describing the active FFT backend */
    String getBackendName() const;

    //==============================================================================
    #ifndef DOXYGEN
    /** @internal */
    class Engine;
    #endif

private:
    //==============================================================================
    void applyScaling (float* data, int numElements, bool isForward);

    //==============================================================================
    int fftSize = -1;
    FFTScaling scaling = FFTScaling::none;

    std::unique_ptr<Engine> engine;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFTProcessor)
};

} // namespace yup
