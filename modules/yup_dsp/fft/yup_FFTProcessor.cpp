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

namespace yup {

//==============================================================================
// Implementation structure to hide backend-specific details
struct FFTProcessor::Impl
{
#if YUP_FFT_USING_OOURA
    std::vector<float> workBuffer;
    std::vector<int> intBuffer;
    std::vector<float> tempBuffer;
#elif YUP_FFT_USING_VDSP
    FFTSetup fftSetup = nullptr;
    std::vector<float> tempBuffer;
#elif YUP_FFT_USING_IPP
    Ipp32fc* workBuffer = nullptr;
    IppsFFTSpec_C_32fc* specComplex = nullptr;
    IppsFFTSpec_R_32f* specReal = nullptr;
#elif YUP_FFT_USING_FFTW3
    fftwf_plan planComplexForward = nullptr;
    fftwf_plan planComplexInverse = nullptr;
    fftwf_plan planRealForward = nullptr;
    fftwf_plan planRealInverse = nullptr;
    std::vector<fftwf_complex> tempComplexBuffer;
    std::vector<float> tempRealBuffer;
#endif
};

//==============================================================================
// Constructor implementations
FFTProcessor::FFTProcessor() : pImpl (std::make_unique<Impl>())
{
    setSize (512);
}

FFTProcessor::FFTProcessor (int fftSize) : pImpl (std::make_unique<Impl>())
{
    setSize (fftSize);
}

FFTProcessor::~FFTProcessor()
{
    cleanup();
}

FFTProcessor::FFTProcessor (FFTProcessor&& other) noexcept
    : fftSize (other.fftSize), scaling (other.scaling), pImpl (std::move (other.pImpl))
{
    other.fftSize = 0;
}

FFTProcessor& FFTProcessor::operator= (FFTProcessor&& other) noexcept
{
    if (this != &other)
    {
        cleanup();
        fftSize = other.fftSize;
        scaling = other.scaling;
        pImpl = std::move (other.pImpl);
        other.fftSize = 0;
    }
    return *this;
}

//==============================================================================
// Public interface
void FFTProcessor::setSize (int newSize)
{
    jassert (isPowerOfTwo (newSize));
    jassert (newSize >= 2 && newSize <= 65536);
    
    if (newSize != fftSize)
    {
        cleanup();
        fftSize = newSize;
        initialize();
    }
}

void FFTProcessor::performRealFFT (const float* realInput, float* complexOutput, FFTDirection direction)
{
    jassert (realInput != nullptr && complexOutput != nullptr);
    
#if YUP_FFT_USING_OOURA
    performRealFFTOoura (realInput, complexOutput, direction);
#elif YUP_FFT_USING_VDSP
    performRealFFTVDSP (realInput, complexOutput, direction);
#elif YUP_FFT_USING_IPP
    performRealFFTIPP (realInput, complexOutput, direction);
#elif YUP_FFT_USING_FFTW3
    performRealFFTFFTW3 (realInput, complexOutput, direction);
#endif
    
    applyScaling (complexOutput, fftSize * 2, direction);
}

void FFTProcessor::performComplexFFT (const float* complexInput, float* complexOutput, FFTDirection direction)
{
    jassert (complexInput != nullptr && complexOutput != nullptr);
    
#if YUP_FFT_USING_OOURA
    performComplexFFTOoura (complexInput, complexOutput, direction);
#elif YUP_FFT_USING_VDSP
    performComplexFFTVDSP (complexInput, complexOutput, direction);
#elif YUP_FFT_USING_IPP
    performComplexFFTIPP (complexInput, complexOutput, direction);
#elif YUP_FFT_USING_FFTW3
    performComplexFFTFFTW3 (complexInput, complexOutput, direction);
#endif
    
    applyScaling (complexOutput, fftSize * 2, direction);
}

const char* FFTProcessor::getBackendName()
{
#if YUP_FFT_USING_VDSP
    return "Apple vDSP";
#elif YUP_FFT_USING_IPP
    return "Intel IPP";
#elif YUP_FFT_USING_FFTW3
    return "FFTW3";
#elif YUP_FFT_USING_OOURA
    return "Ooura FFT";
#else
    return "Unknown";
#endif
}

//==============================================================================
// Private implementation
void FFTProcessor::initialize()
{
#if YUP_FFT_USING_OOURA
    initializeOoura();
#elif YUP_FFT_USING_VDSP
    initializeVDSP();
#elif YUP_FFT_USING_IPP
    initializeIPP();
#elif YUP_FFT_USING_FFTW3
    initializeFFTW3();
#endif
}

void FFTProcessor::cleanup()
{
#if YUP_FFT_USING_VDSP
    if (pImpl && pImpl->fftSetup != nullptr)
    {
        vDSP_destroy_fftsetup (pImpl->fftSetup);
        pImpl->fftSetup = nullptr;
    }
#elif YUP_FFT_USING_IPP
    if (pImpl)
    {
        if (pImpl->workBuffer != nullptr)
        {
            ippsFree (pImpl->workBuffer);
            pImpl->workBuffer = nullptr;
        }
        if (pImpl->specComplex != nullptr)
        {
            ippsFFTFree_C_32fc (pImpl->specComplex);
            pImpl->specComplex = nullptr;
        }
        if (pImpl->specReal != nullptr)
        {
            ippsFFTFree_R_32f (pImpl->specReal);
            pImpl->specReal = nullptr;
        }
    }
#elif YUP_FFT_USING_FFTW3
    if (pImpl)
    {
        if (pImpl->planComplexForward != nullptr)
        {
            fftwf_destroy_plan (pImpl->planComplexForward);
            pImpl->planComplexForward = nullptr;
        }
        if (pImpl->planComplexInverse != nullptr)
        {
            fftwf_destroy_plan (pImpl->planComplexInverse);
            pImpl->planComplexInverse = nullptr;
        }
        if (pImpl->planRealForward != nullptr)
        {
            fftwf_destroy_plan (pImpl->planRealForward);
            pImpl->planRealForward = nullptr;
        }
        if (pImpl->planRealInverse != nullptr)
        {
            fftwf_destroy_plan (pImpl->planRealInverse);
            pImpl->planRealInverse = nullptr;
        }
    }
#endif
}

void FFTProcessor::applyScaling (float* data, int numElements, FFTDirection direction)
{
    if (scaling == FFTScaling::none)
        return;
    
    float scale = 1.0f;
    
    if (scaling == FFTScaling::unitary)
    {
        scale = 1.0f / std::sqrt (static_cast<float> (fftSize));
    }
    else if (scaling == FFTScaling::asymmetric && direction == FFTDirection::inverse)
    {
        scale = 1.0f / static_cast<float> (fftSize);
    }
    
    if (scale != 1.0f)
    {
        for (int i = 0; i < numElements; ++i)
            data[i] *= scale;
    }
}

//==============================================================================
// Ooura FFT implementation
#if YUP_FFT_USING_OOURA

void FFTProcessor::initializeOoura()
{
    const int workSize = 2 + static_cast<int> (std::sqrt (fftSize / 2));
    pImpl->workBuffer.resize (static_cast<size_t> (fftSize));
    pImpl->tempBuffer.resize (static_cast<size_t> (fftSize));
    pImpl->intBuffer.resize (static_cast<size_t> (workSize));
    pImpl->intBuffer[0] = 0; // Initialization flag
}

void FFTProcessor::performRealFFTOoura (const float* realInput, float* complexOutput, FFTDirection direction)
{
    // Copy real input to work buffer
    std::copy (realInput, realInput + fftSize, pImpl->workBuffer.begin());
    
    if (direction == FFTDirection::forward)
    {
        // Real-to-complex forward transform
        rdft (fftSize, 1, pImpl->workBuffer.data(), pImpl->intBuffer.data(), pImpl->tempBuffer.data());
    }
    else
    {
        // Complex-to-real inverse transform
        rdft (fftSize, -1, pImpl->workBuffer.data(), pImpl->intBuffer.data(), pImpl->tempBuffer.data());
    }
    
    // Convert Ooura format to standard interleaved complex format
    complexOutput[0] = pImpl->workBuffer[0]; // DC real
    complexOutput[1] = 0.0f;                 // DC imag
    
    for (int i = 1; i < fftSize / 2; ++i)
    {
        complexOutput[i * 2] = pImpl->workBuffer[i];                    // real
        complexOutput[i * 2 + 1] = pImpl->workBuffer[fftSize - i];      // imag
    }
    
    complexOutput[fftSize] = pImpl->workBuffer[fftSize / 2]; // Nyquist real
    complexOutput[fftSize + 1] = 0.0f;                       // Nyquist imag
}

void FFTProcessor::performComplexFFTOoura (const float* complexInput, float* complexOutput, FFTDirection direction)
{
    // Copy interleaved complex input to work buffer
    std::copy (complexInput, complexInput + fftSize * 2, pImpl->workBuffer.begin());
    
    if (direction == FFTDirection::forward)
    {
        cdft (fftSize * 2, 1, pImpl->workBuffer.data(), pImpl->intBuffer.data(), pImpl->tempBuffer.data());
    }
    else
    {
        cdft (fftSize * 2, -1, pImpl->workBuffer.data(), pImpl->intBuffer.data(), pImpl->tempBuffer.data());
    }
    
    // Copy result
    std::copy (pImpl->workBuffer.begin(), pImpl->workBuffer.begin() + fftSize * 2, complexOutput);
}

#endif

//==============================================================================
// Apple vDSP implementation
#if YUP_FFT_USING_VDSP

void FFTProcessor::initializeVDSP()
{
    const auto log2N = static_cast<vDSP_Length> (std::log2 (fftSize));
    pImpl->fftSetup = vDSP_create_fftsetup (log2N, FFT_RADIX2);
    pImpl->tempBuffer.resize (static_cast<size_t> (fftSize));
}

void FFTProcessor::performRealFFTVDSP (const float* realInput, float* complexOutput, FFTDirection direction)
{
    const auto halfSize = fftSize / 2;
    
    if (direction == FFTDirection::forward)
    {
        // Copy input to temp buffer
        std::copy (realInput, realInput + fftSize, pImpl->tempBuffer.begin());
        
        // Set up split complex structure
        DSPSplitComplex splitComplex;
        splitComplex.realp = pImpl->tempBuffer.data();
        splitComplex.imagp = pImpl->tempBuffer.data() + halfSize;
        
        // Perform real forward FFT
        vDSP_fft_zrip (pImpl->fftSetup, &splitComplex, 2, static_cast<vDSP_Length> (std::log2 (fftSize)), kFFTDirection_Forward);
        
        // Convert split format to interleaved format
        complexOutput[0] = splitComplex.realp[0]; // DC real
        complexOutput[1] = 0.0f;                  // DC imag
        
        for (int i = 1; i < halfSize; ++i)
        {
            complexOutput[i * 2] = splitComplex.realp[i];     // real
            complexOutput[i * 2 + 1] = splitComplex.imagp[i]; // imag
        }
        
        complexOutput[fftSize] = splitComplex.imagp[0]; // Nyquist real (stored in DC imag)
        complexOutput[fftSize + 1] = 0.0f;              // Nyquist imag
    }
    else
    {
        // Inverse transform: interleaved to real
        DSPSplitComplex splitComplex;
        splitComplex.realp = pImpl->tempBuffer.data();
        splitComplex.imagp = pImpl->tempBuffer.data() + halfSize;
        
        // Convert interleaved to split format
        splitComplex.realp[0] = complexInput[0];      // DC real
        splitComplex.imagp[0] = complexInput[fftSize]; // Nyquist real
        
        for (int i = 1; i < halfSize; ++i)
        {
            splitComplex.realp[i] = complexInput[i * 2];     // real
            splitComplex.imagp[i] = complexInput[i * 2 + 1]; // imag
        }
        
        // Perform real inverse FFT
        vDSP_fft_zrip (pImpl->fftSetup, &splitComplex, 2, static_cast<vDSP_Length> (std::log2 (fftSize)), kFFTDirection_Inverse);
        
        // Copy result
        std::copy (pImpl->tempBuffer.begin(), pImpl->tempBuffer.begin() + fftSize, complexOutput);
    }
}

void FFTProcessor::performComplexFFTVDSP (const float* complexInput, float* complexOutput, FFTDirection direction)
{
    const auto halfSize = fftSize / 2;
    
    // Set up split complex structure
    DSPSplitComplex splitInput, splitOutput;
    splitInput.realp = const_cast<float*> (complexInput);
    splitInput.imagp = const_cast<float*> (complexInput) + 1;
    splitOutput.realp = complexOutput;
    splitOutput.imagp = complexOutput + 1;
    
    // Perform complex FFT
    const auto fftDirection = (direction == FFTDirection::forward) ? kFFTDirection_Forward : kFFTDirection_Inverse;
    vDSP_fft_zop (pImpl->fftSetup, &splitInput, 2, &splitOutput, 2, 
                  static_cast<vDSP_Length> (std::log2 (fftSize)), fftDirection);
}

#endif

//==============================================================================
// Intel IPP implementation
#if YUP_FFT_USING_IPP

void FFTProcessor::initializeIPP()
{
    int specSizeComplex, specSizeReal, workSizeComplex, workSizeReal;
    
    // Get buffer sizes
    ippsFFTGetSize_C_32fc (static_cast<int> (std::log2 (fftSize)), IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, 
                           &specSizeComplex, nullptr, &workSizeComplex);
    ippsFFTGetSize_R_32f (static_cast<int> (std::log2 (fftSize)), IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, 
                          &specSizeReal, nullptr, &workSizeReal);
    
    // Allocate specification structures
    pImpl->specComplex = reinterpret_cast<IppsFFTSpec_C_32fc*> (ippsMalloc_8u (specSizeComplex));
    pImpl->specReal = reinterpret_cast<IppsFFTSpec_R_32f*> (ippsMalloc_8u (specSizeReal));
    
    // Initialize specifications
    ippsFFTInit_C_32fc (&pImpl->specComplex, static_cast<int> (std::log2 (fftSize)), IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);
    ippsFFTInit_R_32f (&pImpl->specReal, static_cast<int> (std::log2 (fftSize)), IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);
    
    // Allocate work buffer
    const int maxWorkSize = jmax (workSizeComplex, workSizeReal);
    pImpl->workBuffer = reinterpret_cast<Ipp32fc*> (ippsMalloc_8u (maxWorkSize));
}

void FFTProcessor::performRealFFTIPP (const float* realInput, float* complexOutput, FFTDirection direction)
{
    if (direction == FFTDirection::forward)
    {
        ippsFFTFwd_RToPack_32f (realInput, complexOutput, pImpl->specReal, reinterpret_cast<Ipp8u*> (pImpl->workBuffer));
    }
    else
    {
        ippsFFTInv_PackToR_32f (realInput, complexOutput, pImpl->specReal, reinterpret_cast<Ipp8u*> (pImpl->workBuffer));
    }
}

void FFTProcessor::performComplexFFTIPP (const float* complexInput, float* complexOutput, FFTDirection direction)
{
    const auto* input = reinterpret_cast<const Ipp32fc*> (complexInput);
    auto* output = reinterpret_cast<Ipp32fc*> (complexOutput);
    
    if (direction == FFTDirection::forward)
    {
        ippsFFTFwd_CToC_32fc (input, output, pImpl->specComplex, reinterpret_cast<Ipp8u*> (pImpl->workBuffer));
    }
    else
    {
        ippsFFTInv_CToC_32fc (input, output, pImpl->specComplex, reinterpret_cast<Ipp8u*> (pImpl->workBuffer));
    }
}

#endif

//==============================================================================
// FFTW3 implementation
#if YUP_FFT_USING_FFTW3

void FFTProcessor::initializeFFTW3()
{
    // Allocate buffers
    pImpl->tempComplexBuffer.resize (static_cast<size_t> (fftSize));
    pImpl->tempRealBuffer.resize (static_cast<size_t> (fftSize));
    
    // Create plans
    auto* complexData = pImpl->tempComplexBuffer.data();
    auto* realData = pImpl->tempRealBuffer.data();
    
    pImpl->planComplexForward = fftwf_plan_dft_1d (fftSize, complexData, complexData, FFTW_FORWARD, FFTW_ESTIMATE);
    pImpl->planComplexInverse = fftwf_plan_dft_1d (fftSize, complexData, complexData, FFTW_BACKWARD, FFTW_ESTIMATE);
    pImpl->planRealForward = fftwf_plan_dft_r2c_1d (fftSize, realData, complexData, FFTW_ESTIMATE);
    pImpl->planRealInverse = fftwf_plan_dft_c2r_1d (fftSize, complexData, realData, FFTW_ESTIMATE);
}

void FFTProcessor::performRealFFTFFTW3 (const float* realInput, float* complexOutput, FFTDirection direction)
{
    if (direction == FFTDirection::forward)
    {
        std::copy (realInput, realInput + fftSize, pImpl->tempRealBuffer.begin());
        fftwf_execute (pImpl->planRealForward);
        
        // Convert FFTW format to interleaved format
        const auto halfSize = fftSize / 2 + 1;
        for (int i = 0; i < halfSize; ++i)
        {
            complexOutput[i * 2] = pImpl->tempComplexBuffer[i][0];     // real
            complexOutput[i * 2 + 1] = pImpl->tempComplexBuffer[i][1]; // imag
        }
    }
    else
    {
        // Convert interleaved to FFTW format
        const auto halfSize = fftSize / 2 + 1;
        for (int i = 0; i < halfSize; ++i)
        {
            pImpl->tempComplexBuffer[i][0] = complexInput[i * 2];     // real
            pImpl->tempComplexBuffer[i][1] = complexInput[i * 2 + 1]; // imag
        }
        
        fftwf_execute (pImpl->planRealInverse);
        std::copy (pImpl->tempRealBuffer.begin(), pImpl->tempRealBuffer.begin() + fftSize, complexOutput);
    }
}

void FFTProcessor::performComplexFFTFFTW3 (const float* complexInput, float* complexOutput, FFTDirection direction)
{
    // Convert interleaved to FFTW format
    for (int i = 0; i < fftSize; ++i)
    {
        pImpl->tempComplexBuffer[i][0] = complexInput[i * 2];     // real
        pImpl->tempComplexBuffer[i][1] = complexInput[i * 2 + 1]; // imag
    }
    
    if (direction == FFTDirection::forward)
    {
        fftwf_execute (pImpl->planComplexForward);
    }
    else
    {
        fftwf_execute (pImpl->planComplexInverse);
    }
    
    // Convert FFTW format to interleaved
    for (int i = 0; i < fftSize; ++i)
    {
        complexOutput[i * 2] = pImpl->tempComplexBuffer[i][0];     // real
        complexOutput[i * 2 + 1] = pImpl->tempComplexBuffer[i][1]; // imag
    }
}

#endif

} // namespace yup
