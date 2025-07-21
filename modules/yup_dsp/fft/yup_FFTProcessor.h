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

// Conditional includes based on available FFT backends
#if YUP_USE_OOURA_FFT || !defined(YUP_USE_INTEL_IPP) && !defined(YUP_USE_APPLE_VDSP) && !defined(YUP_USE_FFTW3)
 #include "yup_OouraFFT8g.h"
 #define YUP_FFT_USING_OOURA 1
#endif

#if defined(YUP_USE_APPLE_VDSP) && YUP_MAC
 #include <Accelerate/Accelerate.h>
 #define YUP_FFT_USING_VDSP 1
#endif

#if defined(YUP_USE_INTEL_IPP)
 #include <ipp.h>
 #define YUP_FFT_USING_IPP 1
#endif

#if defined(YUP_USE_FFTW3)
 #include <fftw3.h>
 #define YUP_FFT_USING_FFTW3 1
#endif

namespace yup {

//==============================================================================
/**
    Multi-backend FFT processor that provides a unified interface for different
    FFT implementations.
    
    Supports the following backends (in order of preference):
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
    
    fft.performRealFFT (realInput.data(), complexOutput.data(), FFTDirection::forward);
    @endcode
*/
class FFTProcessor
{
public:
    //==============================================================================
    /** FFT direction enumeration */
    enum class FFTDirection
    {
        forward = 1,   /**< Forward transform (time domain to frequency domain) */
        inverse = -1   /**< Inverse transform (frequency domain to time domain) */
    };
    
    /** FFT scaling options */
    enum class FFTScaling
    {
        none,          /**< No scaling applied */
        unitary,       /**< Unitary scaling (1/sqrt(N)) */
        asymmetric     /**< Asymmetric scaling (1/N for inverse only) */
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
        Performs a real-to-complex FFT.
        
        @param realInput     Input buffer containing real samples (fftSize elements)
        @param complexOutput Output buffer for complex data (fftSize * 2 elements, interleaved real/imag)
        @param direction     Transform direction
    */
    void performRealFFT (const float* realInput, float* complexOutput, FFTDirection direction);
    
    /** 
        Performs a complex-to-complex FFT.
        
        @param complexInput  Input buffer containing complex data (fftSize * 2 elements, interleaved real/imag)
        @param complexOutput Output buffer for complex data (fftSize * 2 elements, interleaved real/imag)
        @param direction     Transform direction
    */
    void performComplexFFT (const float* complexInput, float* complexOutput, FFTDirection direction);
    
    //==============================================================================
    /** Returns a string describing the active FFT backend */
    static const char* getBackendName();
    
private:
    //==============================================================================
    void initialize();
    void cleanup();
    void applyScaling (float* data, int numElements, FFTDirection direction);
    
    // Backend-specific initialization and processing
    void initializeOoura();
    void initializeVDSP();
    void initializeIPP();
    void initializeFFTW3();
    
    void performRealFFTOoura (const float* realInput, float* complexOutput, FFTDirection direction);
    void performComplexFFTOoura (const float* complexInput, float* complexOutput, FFTDirection direction);
    
    void performRealFFTVDSP (const float* realInput, float* complexOutput, FFTDirection direction);
    void performComplexFFTVDSP (const float* complexInput, float* complexOutput, FFTDirection direction);
    
    void performRealFFTIPP (const float* realInput, float* complexOutput, FFTDirection direction);
    void performComplexFFTIPP (const float* complexInput, float* complexOutput, FFTDirection direction);
    
    void performRealFFTFFTW3 (const float* realInput, float* complexOutput, FFTDirection direction);
    void performComplexFFTFFTW3 (const float* complexInput, float* complexOutput, FFTDirection direction);
    
    //==============================================================================
    int fftSize = 512;
    FFTScaling scaling = FFTScaling::none;
    
    // Backend-specific data (implementation details hidden in .cpp)
    struct Impl;
    std::unique_ptr<Impl> pImpl;
    
    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFTProcessor)
};

} // namespace yup
