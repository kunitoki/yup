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

// Conditional includes based on available FFT backends
#if !YUP_FFT_FOUND_BACKEND && YUP_ENABLE_VDSP && (YUP_MAC || YUP_IOS) && __has_include(<Accelerate/Accelerate.h>)
#include <Accelerate/Accelerate.h>
#define YUP_FFT_USING_VDSP 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if !YUP_FFT_FOUND_BACKEND && YUP_ENABLE_INTEL_IPP && __has_include(<ipp.h>)
#include <ipp.h>
#define YUP_FFT_USING_IPP 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if !YUP_FFT_FOUND_BACKEND && YUP_ENABLE_FFTW3 && __has_include(<fftw3.h>)
#include <fftw3.h>
#define YUP_FFT_USING_FFTW3 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if !YUP_FFT_FOUND_BACKEND && YUP_ENABLE_PFFFT && YUP_MODULE_AVAILABLE_pffft_library
#include <pffft_library/pffft_library.h>
#define YUP_FFT_USING_PFFFT 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if !YUP_FFT_FOUND_BACKEND && YUP_ENABLE_OOURA
#include "yup_OouraFFT8g.h"
#define YUP_FFT_USING_OOURA 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if !defined (YUP_FFT_FOUND_BACKEND)
#error "Unable to find a proper FFT backend !"
#endif

namespace yup
{

//==============================================================================
// Base implementation class
class FFTProcessor::Engine
{
public:
    virtual ~Engine() = default;

    virtual void initialize (int fftSize) = 0;
    virtual void cleanup() = 0;

    virtual void performRealFFTForward (const float* realInput, float* complexOutput) = 0;
    virtual void performRealFFTInverse (const float* complexInput, float* realOutput) = 0;
    virtual void performComplexFFTForward (const float* complexInput, float* complexOutput) = 0;
    virtual void performComplexFFTInverse (const float* complexInput, float* complexOutput) = 0;

    virtual String getBackendName() const = 0;

protected:
    int fftSize = 0;
};

//==============================================================================
// PFFFT implementation
#if YUP_FFT_USING_PFFFT

class PFFTEngine : public FFTProcessor::Engine
{
public:
    ~PFFTEngine() override { cleanup(); }

    void initialize (int newFftSize) override
    {
        cleanup();

        fftSize = newFftSize;

        realSetup = pffft_new_setup (fftSize, PFFFT_REAL);
        complexSetup = pffft_new_setup (fftSize, PFFFT_COMPLEX);

        tempBuffer.resize (static_cast<size_t> (fftSize * 2));

        // Allocate work buffers - PFFFT uses stack for small sizes, heap for larger
        if (fftSize >= 16384)
            workBuffer.resize (static_cast<size_t> (fftSize));
    }

    void cleanup() override
    {
        if (realSetup != nullptr)
        {
            pffft_destroy_setup (realSetup);
            realSetup = nullptr;
        }

        if (complexSetup != nullptr)
        {
            pffft_destroy_setup (complexSetup);
            complexSetup = nullptr;
        }

        workBuffer.clear();
        tempBuffer.clear();
    }

    void performRealFFTForward (const float* realInput, float* complexOutput) override
    {
        float* workPtr = workBuffer.empty() ? nullptr : workBuffer.data();

        pffft_transform_ordered (realSetup, realInput, complexOutput, workPtr, PFFFT_FORWARD);

        convertFromPFFTPacked (complexOutput, fftSize);
    }

    void performRealFFTInverse (const float* complexInput, float* realOutput) override
    {
        float* workPtr = workBuffer.empty() ? nullptr : workBuffer.data();

        convertToPFFTPacked (complexInput, tempBuffer.data(), fftSize);

        pffft_transform_ordered (realSetup, tempBuffer.data(), realOutput, workPtr, PFFFT_BACKWARD);
    }

    void performComplexFFTForward (const float* complexInput, float* complexOutput) override
    {
        float* workPtr = workBuffer.empty() ? nullptr : workBuffer.data();
        pffft_transform_ordered (complexSetup, complexInput, complexOutput, workPtr, PFFFT_FORWARD);
    }

    void performComplexFFTInverse (const float* complexInput, float* complexOutput) override
    {
        float* workPtr = workBuffer.empty() ? nullptr : workBuffer.data();
        pffft_transform_ordered (complexSetup, complexInput, complexOutput, workPtr, PFFFT_BACKWARD);
    }

    String getBackendName() const override { return "PFFFT"; }

private:
    // Convert from PFFFT packed format to standard interleaved format
    void convertFromPFFTPacked (float* interleaved, int size)
    {
        // PFFFT packed: [DC_real, Nyquist_real, bin1_real, bin1_imag, bin2_real, bin2_imag, ...]
        // Standard: [DC_real, DC_imag, bin1_real, bin1_imag, ..., Nyquist_real, Nyquist_imag]

        interleaved[size] = std::exchange (interleaved[1], 0.0f);  // Nyquist real (from packed[1])
        interleaved[size + 1] = 0.0f;                              // Nyquist imaginary (always 0)
    }

    // Convert from standard interleaved format to PFFFT packed format
    void convertToPFFTPacked (const float* interleaved, float* packed, int size)
    {
        // Standard: [DC_real, DC_imag, bin1_real, bin1_imag, ..., Nyquist_real, Nyquist_imag]
        // PFFFT packed: [DC_real, Nyquist_real, bin1_real, bin1_imag, bin2_real, bin2_imag, ...]

        packed[0] = interleaved[0];      // DC real
        packed[1] = interleaved[size];   // Nyquist real (to packed[1])
        std::memcpy (&packed[2], &interleaved[2], (size - 2) * sizeof (float));
    }

    PFFFT_Setup* realSetup = nullptr;
    PFFFT_Setup* complexSetup = nullptr;
    std::vector<float> workBuffer;
    std::vector<float> tempBuffer;
};

#endif

//==============================================================================
// Ooura FFT implementation
#if YUP_FFT_USING_OOURA

class OouraEngine : public FFTProcessor::Engine
{
public:
    ~OouraEngine() override { cleanup(); }

    void initialize (int newFftSize) override
    {
        cleanup();

        fftSize = newFftSize;

        const int workSize = 2 + static_cast<int> (std::sqrt (fftSize / 2));
        workBuffer.resize (static_cast<size_t> (fftSize * 2));  // Need space for complex data
        tempBuffer.resize (static_cast<size_t> (fftSize));
        intBuffer.resize (static_cast<size_t> (workSize));
        intBuffer[0] = 0; // Initialization flag
    }

    void cleanup() override
    {
        workBuffer.clear();
        tempBuffer.clear();
        intBuffer.clear();
    }

    void performRealFFTForward (const float* realInput, float* complexOutput) override
    {
        // Copy real input to work buffer
        std::copy (realInput, realInput + fftSize, workBuffer.begin());

        // Real-to-complex forward transform
        rdft (fftSize, 1, workBuffer.data(), intBuffer.data(), tempBuffer.data());

        // Convert Ooura format to standard interleaved complex format
        // Ooura rdft output: a[0]=DC, a[1]=Nyquist, a[2k]=Re[k], a[2k+1]=Im[k] for k=1..n/2-1
        complexOutput[0] = workBuffer[0]; // DC real
        complexOutput[1] = 0.0f;          // DC imaginary

        // Nyquist frequency - Ooura stores it at position 1
        complexOutput[fftSize] = workBuffer[1];   // Nyquist real
        complexOutput[fftSize + 1] = 0.0f;        // Nyquist imaginary

        // Handle frequencies 1 to n/2-1
        // Ooura stores them as alternating real/imag starting at index 2
        for (int i = 1; i < fftSize / 2; ++i)
        {
            complexOutput[i * 2] = workBuffer[i * 2];           // real part
            complexOutput[i * 2 + 1] = -workBuffer[i * 2 + 1]; // imaginary part (negate)
        }
    }

    void performRealFFTInverse (const float* complexInput, float* realOutput) override
    {
        // Convert standard interleaved format to Ooura format
        workBuffer[0] = complexInput[0];                 // DC real
        workBuffer[1] = complexInput[fftSize];           // Nyquist real

        for (int i = 1; i < fftSize / 2; ++i)
        {
            workBuffer[i * 2] = complexInput[i * 2];           // real part
            workBuffer[i * 2 + 1] = -complexInput[i * 2 + 1]; // imaginary part (negate back)
        }

        // Complex-to-real inverse transform
        rdft (fftSize, -1, workBuffer.data(), intBuffer.data(), tempBuffer.data());

        // Apply Ooura-specific scaling for real inverse: needs 2x factor
        for (int i = 0; i < fftSize; ++i)
        {
            realOutput[i] = workBuffer[i] * 2.0f;
        }
    }

    void performComplexFFTForward (const float* complexInput, float* complexOutput) override
    {
        // Copy interleaved complex input to work buffer
        std::copy (complexInput, complexInput + fftSize * 2, workBuffer.begin());

        // Complex forward transform
        cdft (fftSize * 2, 1, workBuffer.data(), intBuffer.data(), tempBuffer.data());

        // Copy result
        std::copy (workBuffer.begin(), workBuffer.begin() + fftSize * 2, complexOutput);
    }

    void performComplexFFTInverse (const float* complexInput, float* complexOutput) override
    {
        // Copy interleaved complex input to work buffer
        std::copy (complexInput, complexInput + fftSize * 2, workBuffer.begin());

        // Complex inverse transform
        cdft (fftSize * 2, -1, workBuffer.data(), intBuffer.data(), tempBuffer.data());

        // Copy result - let framework handle scaling
        std::copy (workBuffer.begin(), workBuffer.begin() + fftSize * 2, complexOutput);
    }

    String getBackendName() const override { return "Ooura FFT"; }

private:
    std::vector<float> workBuffer;
    std::vector<int> intBuffer;
    std::vector<float> tempBuffer;
};

#endif

//==============================================================================
// Apple vDSP implementation
#if YUP_FFT_USING_VDSP

class VDSPEngine : public FFTProcessor::Engine
{
public:
    ~VDSPEngine() override { cleanup(); }

    void initialize (int newFftSize) override
    {
        cleanup();

        fftSize = newFftSize;
        order = static_cast<vDSP_Length> (std::log2 (fftSize));

        fftSetup = vDSP_create_fftsetup (order, FFT_RADIX2);

        forwardNormalisation = 0.5f;
        inverseNormalisation = 1.0f / static_cast<float> (fftSize);

        tempBuffer.resize (fftSize * 2);
    }

    void cleanup() override
    {
        if (fftSetup != nullptr)
        {
            vDSP_destroy_fftsetup (fftSetup);
            fftSetup = nullptr;
        }

        tempBuffer.clear();
    }

    void performRealFFTForward (const float* realInput, float* complexOutput) override
    {
        // Copy input to output buffer to work in-place
        std::copy_n (realInput, fftSize, complexOutput);
        complexOutput[fftSize] = 0.0f;

        // Perform vDSP real FFT
        DSPSplitComplex splitInOut = { complexOutput, complexOutput + 1 };
        vDSP_fft_zrip (fftSetup, &splitInOut, 2, order, kFFTDirection_Forward);

        // Normalize vDSP output to match other engines (vDSP outputs 2x expected)
        vDSP_vsmul (complexOutput, 1, &forwardNormalisation, complexOutput, 1, static_cast<size_t> (fftSize << 1));

        // Set Nyquist bin (real only, imaginary = 0), set DC bin (real only, imaginary = 0)
        auto* complexData = reinterpret_cast<ComplexFloat*> (complexOutput);
        complexData[fftSize >> 1] = ComplexFloat (complexData[0].imag(), 0.0f);
        complexData[0] = ComplexFloat (complexData[0].real(), 0.0f);
    }

    void performRealFFTInverse (const float* complexInput, float* realOutput) override
    {
        // Copy input to temp buffer for processing
        std::copy_n (complexInput, fftSize * 2, tempBuffer.data());

        // Pack Nyquist real into DC imaginary for vDSP
        auto* complexData = reinterpret_cast<ComplexFloat*> (tempBuffer.data());
        complexData[0] = ComplexFloat (complexData[0].real(), complexData[fftSize >> 1].real());

        // Perform vDSP real inverse FFT
        DSPSplitComplex splitInOut = { tempBuffer.data(), tempBuffer.data() + 1 };
        vDSP_fft_zrip (fftSetup, &splitInOut, 2, order, kFFTDirection_Inverse);

        // Clear upper half and extract real parts
        vDSP_vclr (tempBuffer.data() + fftSize, 1, static_cast<size_t> (fftSize));

        std::copy_n (tempBuffer.data(), fftSize, realOutput);
    }

    void performComplexFFTForward (const float* complexInput, float* complexOutput) override
    {
        std::copy_n (complexInput, fftSize * 2, tempBuffer.data());

        DSPSplitComplex splitInput = { tempBuffer.data(), tempBuffer.data() + 1 };
        DSPSplitComplex splitOutput = { complexOutput, complexOutput + 1 };

        // Perform complex FFT
        vDSP_fft_zop (fftSetup, &splitInput, 2, &splitOutput, 2, order, kFFTDirection_Forward);

        // Normalization
        float scale = forwardNormalisation * 2.0f;
        vDSP_vsmul (complexOutput, 1, &scale, complexOutput, 1, static_cast<size_t> (fftSize << 1));
    }

    void performComplexFFTInverse (const float* complexInput, float* complexOutput) override
    {
        std::memcpy (tempBuffer.data(), complexInput, fftSize * 2 * sizeof (float));

        DSPSplitComplex splitInput = { tempBuffer.data(), tempBuffer.data() + 1 };
        DSPSplitComplex splitOutput = { complexOutput, complexOutput + 1 };

        // Perform complex FFT
        vDSP_fft_zop (fftSetup, &splitInput, 2, &splitOutput, 2, order, kFFTDirection_Inverse);
    }

    String getBackendName() const override { return "Apple vDSP"; }

private:
    using ComplexFloat = std::complex<float>;

    FFTSetup fftSetup = nullptr;
    vDSP_Length order = 0;
    float forwardNormalisation = 0.5f;
    float inverseNormalisation = 1.0f;
    std::vector<float> tempBuffer;
};

#endif

//==============================================================================
// Intel IPP implementation
#if YUP_FFT_USING_IPP

class IPPEngine : public FFTProcessor::Engine
{
public:
    ~IPPEngine() override { cleanup(); }

    void initialize (int newFftSize) override
    {
        cleanup();
        fftSize = newFftSize;

        int specSizeComplex, specSizeReal, workSizeComplex, workSizeReal;

        // Get buffer sizes
        ippsFFTGetSize_C_32fc (static_cast<int> (std::log2 (fftSize)), IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSizeComplex, nullptr, &workSizeComplex);
        ippsFFTGetSize_R_32f (static_cast<int> (std::log2 (fftSize)), IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSizeReal, nullptr, &workSizeReal);

        // Allocate specification structures
        specComplex = reinterpret_cast<IppsFFTSpec_C_32fc*> (ippsMalloc_8u (specSizeComplex));
        specReal = reinterpret_cast<IppsFFTSpec_R_32f*> (ippsMalloc_8u (specSizeReal));

        // Initialize specifications
        ippsFFTInit_C_32fc (&specComplex, static_cast<int> (std::log2 (fftSize)), IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);
        ippsFFTInit_R_32f (&specReal, static_cast<int> (std::log2 (fftSize)), IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);

        // Allocate work buffer
        const int maxWorkSize = jmax (workSizeComplex, workSizeReal);
        workBuffer = reinterpret_cast<Ipp32fc*> (ippsMalloc_8u (maxWorkSize));
    }

    void cleanup() override
    {
        if (workBuffer != nullptr)
        {
            ippsFree (workBuffer);
            workBuffer = nullptr;
        }
        if (specComplex != nullptr)
        {
            ippsFFTFree_C_32fc (specComplex);
            specComplex = nullptr;
        }
        if (specReal != nullptr)
        {
            ippsFFTFree_R_32f (specReal);
            specReal = nullptr;
        }
    }

    void performRealFFTForward (const float* realInput, float* complexOutput) override
    {
        ippsFFTFwd_RToPack_32f (realInput, complexOutput, specReal, reinterpret_cast<Ipp8u*> (workBuffer));
    }

    void performRealFFTInverse (const float* complexInput, float* realOutput) override
    {
        ippsFFTInv_PackToR_32f (complexInput, realOutput, specReal, reinterpret_cast<Ipp8u*> (workBuffer));
    }

    void performComplexFFTForward (const float* complexInput, float* complexOutput) override
    {
        const auto* input = reinterpret_cast<const Ipp32fc*> (complexInput);
        auto* output = reinterpret_cast<Ipp32fc*> (complexOutput);

        ippsFFTFwd_CToC_32fc (input, output, specComplex, reinterpret_cast<Ipp8u*> (workBuffer));
    }

    void performComplexFFTInverse (const float* complexInput, float* complexOutput) override
    {
        const auto* input = reinterpret_cast<const Ipp32fc*> (complexInput);
        auto* output = reinterpret_cast<Ipp32fc*> (complexOutput);

        ippsFFTInv_CToC_32fc (input, output, specComplex, reinterpret_cast<Ipp8u*> (workBuffer));
    }

    String getBackendName() const override { return "Intel IPP"; }

private:
    Ipp32fc* workBuffer = nullptr;
    IppsFFTSpec_C_32fc* specComplex = nullptr;
    IppsFFTSpec_R_32f* specReal = nullptr;
};

#endif

//==============================================================================
// FFTW3 implementation
#if YUP_FFT_USING_FFTW3

class FFTW3Engine : public FFTProcessor::Engine
{
public:
    ~FFTW3Engine() override { cleanup(); }

    void initialize (int newFftSize) override
    {
        cleanup();

        fftSize = newFftSize;

        tempComplexBuffer = static_cast<fftwf_complex*> (fftwf_malloc (sizeof (fftwf_complex) * fftSize));
        tempRealBuffer = static_cast<float*> (fftwf_malloc (sizeof (float) * fftSize));

        auto* complexData = tempComplexBuffer;
        auto* realData = tempRealBuffer;

        planComplexForward = fftwf_plan_dft_1d (fftSize, complexData, complexData, FFTW_FORWARD, FFTW_ESTIMATE);
        planComplexInverse = fftwf_plan_dft_1d (fftSize, complexData, complexData, FFTW_BACKWARD, FFTW_ESTIMATE);
        planRealForward = fftwf_plan_dft_r2c_1d (fftSize, realData, complexData, FFTW_ESTIMATE);
        planRealInverse = fftwf_plan_dft_c2r_1d (fftSize, complexData, realData, FFTW_ESTIMATE);
    }

    void cleanup() override
    {
        if (planComplexForward != nullptr)
        {
            fftwf_destroy_plan (planComplexForward);
            planComplexForward = nullptr;
        }

        if (planComplexInverse != nullptr)
        {
            fftwf_destroy_plan (planComplexInverse);
            planComplexInverse = nullptr;
        }

        if (planRealForward != nullptr)
        {
            fftwf_destroy_plan (planRealForward);
            planRealForward = nullptr;
        }

        if (planRealInverse != nullptr)
        {
            fftwf_destroy_plan (planRealInverse);
            planRealInverse = nullptr;
        }

        if (tempComplexBuffer != nullptr)
        {
            fftwf_free (tempComplexBuffer);
            tempComplexBuffer = nullptr;
        }

        if (tempRealBuffer != nullptr)
        {
            fftwf_free (tempRealBuffer);
            tempRealBuffer = nullptr;
        }
    }

    void performRealFFTForward (const float* realInput, float* complexOutput) override
    {
        std::copy_n (realInput, fftSize, tempRealBuffer);

        fftwf_execute (planRealForward);

        const auto halfSize = fftSize / 2 + 1;
        for (int i = 0; i < halfSize; ++i)
        {
            complexOutput[i * 2] = tempComplexBuffer[i][0];     // real
            complexOutput[i * 2 + 1] = tempComplexBuffer[i][1]; // imag
        }
    }

    void performRealFFTInverse (const float* complexInput, float* realOutput) override
    {
        // Convert interleaved to FFTW format
        const auto halfSize = fftSize / 2 + 1;
        for (int i = 0; i < halfSize; ++i)
        {
            tempComplexBuffer[i][0] = complexInput[i * 2];     // real
            tempComplexBuffer[i][1] = complexInput[i * 2 + 1]; // imag
        }

        fftwf_execute (planRealInverse);

        std::copy_n (tempRealBuffer, fftSize, realOutput);
    }

    void performComplexFFTForward (const float* complexInput, float* complexOutput) override
    {
        for (int i = 0; i < fftSize; ++i)
        {
            tempComplexBuffer[i][0] = complexInput[i * 2];     // real
            tempComplexBuffer[i][1] = complexInput[i * 2 + 1]; // imag
        }

        fftwf_execute (planComplexForward);

        for (int i = 0; i < fftSize; ++i)
        {
            complexOutput[i * 2] = tempComplexBuffer[i][0];     // real
            complexOutput[i * 2 + 1] = tempComplexBuffer[i][1]; // imag
        }
    }

    void performComplexFFTInverse (const float* complexInput, float* complexOutput) override
    {
        for (int i = 0; i < fftSize; ++i)
        {
            tempComplexBuffer[i][0] = complexInput[i * 2];     // real
            tempComplexBuffer[i][1] = complexInput[i * 2 + 1]; // imag
        }

        fftwf_execute (planComplexInverse);

        for (int i = 0; i < fftSize; ++i)
        {
            complexOutput[i * 2] = tempComplexBuffer[i][0];     // real
            complexOutput[i * 2 + 1] = tempComplexBuffer[i][1]; // imag
        }
    }

    String getBackendName() const override { return "FFTW3"; }

private:
    fftwf_plan planComplexForward = nullptr;
    fftwf_plan planComplexInverse = nullptr;
    fftwf_plan planRealForward = nullptr;
    fftwf_plan planRealInverse = nullptr;
    fftwf_complex* tempComplexBuffer = nullptr;
    float* tempRealBuffer = nullptr;
};

#endif

//==============================================================================
// Factory function to create appropriate implementation
std::unique_ptr<FFTProcessor::Engine> createFFTEngine()
{
#if YUP_FFT_USING_PFFFT
    return std::make_unique<PFFTEngine>();
#elif YUP_FFT_USING_VDSP
    return std::make_unique<VDSPEngine>();
#elif YUP_FFT_USING_IPP
    return std::make_unique<IPPEngine>();
#elif YUP_FFT_USING_FFTW3
    return std::make_unique<FFTW3Engine>();
#elif YUP_FFT_USING_OOURA
    return std::make_unique<OouraEngine>();
#else
    jassertfalse; // No FFT backend available
    return nullptr;
#endif
}

//==============================================================================
// Constructor implementations
FFTProcessor::FFTProcessor()
    : engine (createFFTEngine())
{
    setSize (512);
}

FFTProcessor::FFTProcessor (int fftSize)
    : engine (createFFTEngine())
{
    setSize (fftSize);
}

FFTProcessor::~FFTProcessor()
{
    if (engine)
        engine->cleanup();
}

FFTProcessor::FFTProcessor (FFTProcessor&& other) noexcept
    : fftSize (std::exchange (other.fftSize, 0))
    , scaling (other.scaling)
    , engine (std::move (other.engine))
{
}

FFTProcessor& FFTProcessor::operator= (FFTProcessor&& other) noexcept
{
    if (this != &other)
    {
        if (engine)
            engine->cleanup();

        fftSize = std::exchange (other.fftSize, 0);
        scaling = other.scaling;
        engine = std::move (other.engine);
    }

    return *this;
}

//==============================================================================
// Public interface
void FFTProcessor::setSize (int newSize)
{
    jassert (isPowerOfTwo (newSize));
    jassert (newSize >= 32 && newSize <= 65536);

    if (newSize != fftSize)
    {
        fftSize = newSize;

        if (engine)
            engine->initialize (fftSize);
    }
}

void FFTProcessor::performRealFFTForward (const float* realInput, float* complexOutput)
{
    jassert (realInput != nullptr && complexOutput != nullptr);
    jassert (engine != nullptr);

    engine->performRealFFTForward (realInput, complexOutput);
    applyScaling (complexOutput, fftSize * 2, true);
}

void FFTProcessor::performRealFFTInverse (const float* complexInput, float* realOutput)
{
    jassert (complexInput != nullptr && realOutput != nullptr);
    jassert (engine != nullptr);

    engine->performRealFFTInverse (complexInput, realOutput);
    applyScaling (realOutput, fftSize, false);
}

void FFTProcessor::performComplexFFTForward (const float* complexInput, float* complexOutput)
{
    jassert (complexInput != nullptr && complexOutput != nullptr);
    jassert (engine != nullptr);

    engine->performComplexFFTForward (complexInput, complexOutput);
    applyScaling (complexOutput, fftSize * 2, true);
}

void FFTProcessor::performComplexFFTInverse (const float* complexInput, float* complexOutput)
{
    jassert (complexInput != nullptr && complexOutput != nullptr);
    jassert (engine != nullptr);

    engine->performComplexFFTInverse (complexInput, complexOutput);
    applyScaling (complexOutput, fftSize * 2, false);
}

String FFTProcessor::getBackendName() const
{
    return engine != nullptr ? engine->getBackendName() : "Unknown";
}

//==============================================================================
// Private implementation
void FFTProcessor::applyScaling (float* data, int numElements, bool isForward)
{
    if (scaling == FFTScaling::none)
        return;

    float scale = 1.0f;

    if (scaling == FFTScaling::unitary)
    {
        scale = 1.0f / std::sqrt (static_cast<float> (fftSize));
    }
    else if (scaling == FFTScaling::asymmetric && !isForward)
    {
        scale = 1.0f / static_cast<float> (fftSize);
    }

    if (scale != 1.0f)
        FloatVectorOperations::multiply (data, scale, numElements);
}

} // namespace yup
