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

namespace yup
{
namespace detail
{

//==============================================================================
// Base implementation class
template <typename SampleType>
class FFTEngine
{
public:
    virtual ~FFTEngine() = default;

    virtual void initialize (int fftSize) = 0;
    virtual void cleanup() = 0;

    virtual void performRealFFTForward (const SampleType* realInput, SampleType* complexOutput) = 0;
    virtual void performRealFFTInverse (const SampleType* complexInput, SampleType* realOutput) = 0;
    virtual void performComplexFFTForward (const SampleType* complexInput, SampleType* complexOutput) = 0;
    virtual void performComplexFFTInverse (const SampleType* complexInput, SampleType* complexOutput) = 0;

    virtual String getBackendName() const = 0;

protected:
    int fftSize = 0;
};

} // namespace detail

//==============================================================================
// PFFFT implementation
#if YUP_FFT_USING_PFFFT

template <typename SampleType>
class PFFTEngine : public detail::FFTEngine<SampleType>
{
public:
    ~PFFTEngine() override { this->cleanup(); }

    void initialize (int newFftSize) override
    {
        this->cleanup();

        this->fftSize = newFftSize;

        if constexpr (std::is_same_v<SampleType, double>)
        {
            realSetupD = pffftd_new_setup (this->fftSize, PFFFT_REAL);
            complexSetupD = pffftd_new_setup (this->fftSize, PFFFT_COMPLEX);
        }
        else
        {
            realSetup = pffft_new_setup (this->fftSize, PFFFT_REAL);
            complexSetup = pffft_new_setup (this->fftSize, PFFFT_COMPLEX);
        }

        tempBuffer.resize (static_cast<size_t> (this->fftSize * 2));

        // Allocate work buffers - PFFFT uses stack for small sizes, heap for larger
        if (this->fftSize >= 16384)
            workBuffer.resize (static_cast<size_t> (this->fftSize));
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

        if (realSetupD != nullptr)
        {
            pffftd_destroy_setup (realSetupD);
            realSetupD = nullptr;
        }

        if (complexSetupD != nullptr)
        {
            pffftd_destroy_setup (complexSetupD);
            complexSetupD = nullptr;
        }

        workBuffer.clear();
        tempBuffer.clear();
    }

    void performRealFFTForward (const SampleType* realInput, SampleType* complexOutput) override
    {
        SampleType* workPtr = workBuffer.empty() ? nullptr : workBuffer.data();

        if constexpr (std::is_same_v<SampleType, double>)
            pffftd_transform_ordered (realSetupD, realInput, complexOutput, workPtr, PFFFT_FORWARD);
        else
            pffft_transform_ordered (realSetup, realInput, complexOutput, workPtr, PFFFT_FORWARD);

        convertFromPFFTPacked (complexOutput, this->fftSize);
    }

    void performRealFFTInverse (const SampleType* complexInput, SampleType* realOutput) override
    {
        SampleType* workPtr = workBuffer.empty() ? nullptr : workBuffer.data();

        convertToPFFTPacked (complexInput, tempBuffer.data(), this->fftSize);

        if constexpr (std::is_same_v<SampleType, double>)
            pffftd_transform_ordered (realSetupD, tempBuffer.data(), realOutput, workPtr, PFFFT_BACKWARD);
        else
            pffft_transform_ordered (realSetup, tempBuffer.data(), realOutput, workPtr, PFFFT_BACKWARD);
    }

    void performComplexFFTForward (const SampleType* complexInput, SampleType* complexOutput) override
    {
        SampleType* workPtr = workBuffer.empty() ? nullptr : workBuffer.data();

        if constexpr (std::is_same_v<SampleType, double>)
            pffftd_transform_ordered (complexSetupD, complexInput, complexOutput, workPtr, PFFFT_FORWARD);
        else
            pffft_transform_ordered (complexSetup, complexInput, complexOutput, workPtr, PFFFT_FORWARD);
    }

    void performComplexFFTInverse (const SampleType* complexInput, SampleType* complexOutput) override
    {
        SampleType* workPtr = workBuffer.empty() ? nullptr : workBuffer.data();

        if constexpr (std::is_same_v<SampleType, double>)
            pffftd_transform_ordered (complexSetupD, complexInput, complexOutput, workPtr, PFFFT_BACKWARD);
        else
            pffft_transform_ordered (complexSetup, complexInput, complexOutput, workPtr, PFFFT_BACKWARD);
    }

    String getBackendName() const override { return "PFFFT"; }

private:
    // Convert from PFFFT packed format to standard interleaved format
    void convertFromPFFTPacked (SampleType* interleaved, int size)
    {
        // PFFFT packed: [DC_real, Nyquist_real, bin1_real, bin1_imag, bin2_real, bin2_imag, ...]
        // Standard: [DC_real, DC_imag, bin1_real, bin1_imag, ..., Nyquist_real, Nyquist_imag]

        interleaved[size] = std::exchange (interleaved[1], SampleType (0)); // Nyquist real (from packed[1])
        interleaved[size + 1] = SampleType (0);                             // Nyquist imaginary (always 0)
    }

    // Convert from standard interleaved format to PFFFT packed format
    void convertToPFFTPacked (const SampleType* interleaved, SampleType* packed, int size)
    {
        // Standard: [DC_real, DC_imag, bin1_real, bin1_imag, ..., Nyquist_real, Nyquist_imag]
        // PFFFT packed: [DC_real, Nyquist_real, bin1_real, bin1_imag, bin2_real, bin2_imag, ...]

        packed[0] = interleaved[0];    // DC real
        packed[1] = interleaved[size]; // Nyquist real (to packed[1])
        std::memcpy (&packed[2], &interleaved[2], static_cast<size_t> (size - 2) * sizeof (SampleType));
    }

    PFFFT_Setup* realSetup = nullptr;
    PFFFT_Setup* complexSetup = nullptr;
    PFFFTD_Setup* realSetupD = nullptr;
    PFFFTD_Setup* complexSetupD = nullptr;
    std::vector<SampleType> workBuffer;
    std::vector<SampleType> tempBuffer;
};

#endif

//==============================================================================
// Ooura FFT implementation
#if YUP_FFT_USING_OOURA

template <typename SampleType>
class OouraEngine : public detail::FFTEngine<SampleType>
{
public:
    ~OouraEngine() override { this->cleanup(); }

    void initialize (int newFftSize) override
    {
        this->cleanup();

        this->fftSize = newFftSize;

        // The complex transforms operate on 2 * fftSize values, which needs a larger
        // bit-reversal table than the real transform (Ooura requires 2 + sqrt (n / 2)).
        const int workSize = 2 + static_cast<int> (std::sqrt (static_cast<double> (this->fftSize)));
        workBuffer.resize (static_cast<size_t> (this->fftSize * 2)); // Need space for complex data
        tempBuffer.resize (static_cast<size_t> (this->fftSize));
        intBuffer.resize (static_cast<size_t> (workSize));
        intBuffer[0] = 0; // Initialization flag
    }

    void cleanup() override
    {
        workBuffer.clear();
        tempBuffer.clear();
        intBuffer.clear();
    }

    void performRealFFTForward (const SampleType* realInput, SampleType* complexOutput) override
    {
        // Copy real input to work buffer
        std::copy (realInput, realInput + this->fftSize, workBuffer.begin());

        // Real-to-complex forward transform
        rdft (this->fftSize, 1, workBuffer.data(), intBuffer.data(), tempBuffer.data());

        // Convert Ooura format to standard interleaved complex format
        // Ooura rdft output: a[0]=DC, a[1]=Nyquist, a[2k]=Re[k], a[2k+1]=Im[k] for k=1..n/2-1
        complexOutput[0] = workBuffer[0];  // DC real
        complexOutput[1] = SampleType (0); // DC imaginary

        // Nyquist frequency - Ooura stores it at position 1
        complexOutput[this->fftSize] = workBuffer[1];      // Nyquist real
        complexOutput[this->fftSize + 1] = SampleType (0); // Nyquist imaginary

        // Handle frequencies 1 to n/2-1
        // Ooura stores them as alternating real/imag starting at index 2
        for (int i = 1; i < this->fftSize / 2; ++i)
        {
            complexOutput[i * 2] = workBuffer[i * 2];          // real part
            complexOutput[i * 2 + 1] = -workBuffer[i * 2 + 1]; // imaginary part (negate)
        }
    }

    void performRealFFTInverse (const SampleType* complexInput, SampleType* realOutput) override
    {
        // Convert standard interleaved format to Ooura format
        workBuffer[0] = complexInput[0];             // DC real
        workBuffer[1] = complexInput[this->fftSize]; // Nyquist real

        for (int i = 1; i < this->fftSize / 2; ++i)
        {
            workBuffer[i * 2] = complexInput[i * 2];          // real part
            workBuffer[i * 2 + 1] = -complexInput[i * 2 + 1]; // imaginary part (negate back)
        }

        // Complex-to-real inverse transform
        rdft (this->fftSize, -1, workBuffer.data(), intBuffer.data(), tempBuffer.data());

        // Apply Ooura-specific scaling for real inverse: needs 2x factor
        for (int i = 0; i < this->fftSize; ++i)
        {
            realOutput[i] = workBuffer[i] * SampleType (2);
        }
    }

    void performComplexFFTForward (const SampleType* complexInput, SampleType* complexOutput) override
    {
        // Copy interleaved complex input to work buffer
        std::copy (complexInput, complexInput + this->fftSize * 2, workBuffer.begin());

        // Complex forward transform
        cdft (this->fftSize * 2, 1, workBuffer.data(), intBuffer.data(), tempBuffer.data());

        // Copy result
        std::copy (workBuffer.begin(), workBuffer.begin() + this->fftSize * 2, complexOutput);
    }

    void performComplexFFTInverse (const SampleType* complexInput, SampleType* complexOutput) override
    {
        // Copy interleaved complex input to work buffer
        std::copy (complexInput, complexInput + this->fftSize * 2, workBuffer.begin());

        // Complex inverse transform
        cdft (this->fftSize * 2, -1, workBuffer.data(), intBuffer.data(), tempBuffer.data());

        // Copy result - let framework handle scaling
        std::copy (workBuffer.begin(), workBuffer.begin() + this->fftSize * 2, complexOutput);
    }

    String getBackendName() const override { return "Ooura FFT"; }

private:
    std::vector<SampleType> workBuffer;
    std::vector<int> intBuffer;
    std::vector<SampleType> tempBuffer;
};

#endif

//==============================================================================
// Apple vDSP implementation
#if YUP_FFT_USING_VDSP

template <typename SampleType>
class VDSPEngine : public detail::FFTEngine<SampleType>
{
public:
    ~VDSPEngine() override { this->cleanup(); }

    void initialize (int newFftSize) override
    {
        this->cleanup();

        this->fftSize = newFftSize;
        order = static_cast<vDSP_Length> (std::log2 (this->fftSize));

        if constexpr (std::is_same_v<SampleType, double>)
            fftSetup = vDSP_create_fftsetupD (order, FFT_RADIX2);
        else
            fftSetup = vDSP_create_fftsetup (order, FFT_RADIX2);

        forwardNormalisation = SampleType (0.5);
        inverseNormalisation = SampleType (1) / static_cast<SampleType> (this->fftSize);

        tempBuffer.resize (static_cast<size_t> (this->fftSize * 2));
    }

    void cleanup() override
    {
        if (fftSetup != nullptr)
        {
            if constexpr (std::is_same_v<SampleType, double>)
                vDSP_destroy_fftsetupD (fftSetup);
            else
                vDSP_destroy_fftsetup (fftSetup);

            fftSetup = nullptr;
        }

        tempBuffer.clear();
    }

    void performRealFFTForward (const SampleType* realInput, SampleType* complexOutput) override
    {
        // Copy input to output buffer to work in-place
        std::copy_n (realInput, this->fftSize, complexOutput);
        complexOutput[this->fftSize] = SampleType (0);

        // Perform vDSP real FFT
        SplitComplex splitInOut = { complexOutput, complexOutput + 1 };

        if constexpr (std::is_same_v<SampleType, double>)
            vDSP_fft_zripD (fftSetup, &splitInOut, 2, order, kFFTDirection_Forward);
        else
            vDSP_fft_zrip (fftSetup, &splitInOut, 2, order, kFFTDirection_Forward);

        // Normalize vDSP output to match other engines (vDSP outputs 2x expected)
        if constexpr (std::is_same_v<SampleType, double>)
            vDSP_vsmulD (complexOutput, 1, &forwardNormalisation, complexOutput, 1, static_cast<vDSP_Length> (this->fftSize << 1));
        else
            vDSP_vsmul (complexOutput, 1, &forwardNormalisation, complexOutput, 1, static_cast<vDSP_Length> (this->fftSize << 1));

        // Set Nyquist bin (real only, imaginary = 0), set DC bin (real only, imaginary = 0)
        auto* complexData = reinterpret_cast<Complex*> (complexOutput);
        complexData[this->fftSize >> 1] = Complex (complexData[0].imag(), SampleType (0));
        complexData[0] = Complex (complexData[0].real(), SampleType (0));
    }

    void performRealFFTInverse (const SampleType* complexInput, SampleType* realOutput) override
    {
        // Copy input to temp buffer for processing
        std::copy_n (complexInput, this->fftSize * 2, tempBuffer.data());

        // Pack Nyquist real into DC imaginary for vDSP
        auto* complexData = reinterpret_cast<Complex*> (tempBuffer.data());
        complexData[0] = Complex (complexData[0].real(), complexData[this->fftSize >> 1].real());

        // Perform vDSP real inverse FFT
        SplitComplex splitInOut = { tempBuffer.data(), tempBuffer.data() + 1 };

        if constexpr (std::is_same_v<SampleType, double>)
            vDSP_fft_zripD (fftSetup, &splitInOut, 2, order, kFFTDirection_Inverse);
        else
            vDSP_fft_zrip (fftSetup, &splitInOut, 2, order, kFFTDirection_Inverse);

        // Clear upper half and extract real parts
        if constexpr (std::is_same_v<SampleType, double>)
            vDSP_vclrD (tempBuffer.data() + this->fftSize, 1, static_cast<vDSP_Length> (this->fftSize));
        else
            vDSP_vclr (tempBuffer.data() + this->fftSize, 1, static_cast<vDSP_Length> (this->fftSize));

        std::copy_n (tempBuffer.data(), this->fftSize, realOutput);
    }

    void performComplexFFTForward (const SampleType* complexInput, SampleType* complexOutput) override
    {
        std::copy_n (complexInput, this->fftSize * 2, tempBuffer.data());

        SplitComplex splitInput = { tempBuffer.data(), tempBuffer.data() + 1 };
        SplitComplex splitOutput = { complexOutput, complexOutput + 1 };

        // Perform complex FFT
        if constexpr (std::is_same_v<SampleType, double>)
            vDSP_fft_zopD (fftSetup, &splitInput, 2, &splitOutput, 2, order, kFFTDirection_Forward);
        else
            vDSP_fft_zop (fftSetup, &splitInput, 2, &splitOutput, 2, order, kFFTDirection_Forward);

        // Normalization
        SampleType scale = forwardNormalisation * SampleType (2);

        if constexpr (std::is_same_v<SampleType, double>)
            vDSP_vsmulD (complexOutput, 1, &scale, complexOutput, 1, static_cast<vDSP_Length> (this->fftSize << 1));
        else
            vDSP_vsmul (complexOutput, 1, &scale, complexOutput, 1, static_cast<vDSP_Length> (this->fftSize << 1));
    }

    void performComplexFFTInverse (const SampleType* complexInput, SampleType* complexOutput) override
    {
        std::memcpy (tempBuffer.data(), complexInput, static_cast<size_t> (this->fftSize * 2) * sizeof (SampleType));

        SplitComplex splitInput = { tempBuffer.data(), tempBuffer.data() + 1 };
        SplitComplex splitOutput = { complexOutput, complexOutput + 1 };

        // Perform complex FFT
        if constexpr (std::is_same_v<SampleType, double>)
            vDSP_fft_zopD (fftSetup, &splitInput, 2, &splitOutput, 2, order, kFFTDirection_Inverse);
        else
            vDSP_fft_zop (fftSetup, &splitInput, 2, &splitOutput, 2, order, kFFTDirection_Inverse);
    }

    String getBackendName() const override { return "Apple vDSP"; }

private:
    using Complex = std::complex<SampleType>;
    using SplitComplex = std::conditional_t<std::is_same_v<SampleType, double>, DSPDoubleSplitComplex, DSPSplitComplex>;
    using SetupType = std::conditional_t<std::is_same_v<SampleType, double>, FFTSetupD, FFTSetup>;

    SetupType fftSetup = nullptr;
    vDSP_Length order = 0;
    SampleType forwardNormalisation = SampleType (0.5);
    SampleType inverseNormalisation = SampleType (1);
    std::vector<SampleType> tempBuffer;
};

#endif

//==============================================================================
// Intel IPP implementation
#if YUP_FFT_USING_IPP

template <typename SampleType>
class IPPEngine : public detail::FFTEngine<SampleType>
{
public:
    ~IPPEngine() override { this->cleanup(); }

    void initialize (int newFftSize) override
    {
        this->cleanup();
        this->fftSize = newFftSize;

        const int order = static_cast<int> (std::log2 (this->fftSize));
        int specSizeComplex, specSizeReal, workSizeComplex, workSizeReal;

        if constexpr (std::is_same_v<SampleType, double>)
        {
            // Get buffer sizes
            ippsFFTGetSize_C_64fc (order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSizeComplex, nullptr, &workSizeComplex);
            ippsFFTGetSize_R_64f (order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSizeReal, nullptr, &workSizeReal);

            // Allocate specification structures
            specComplex = reinterpret_cast<SpecComplex*> (ippsMalloc_8u (specSizeComplex));
            specReal = reinterpret_cast<SpecReal*> (ippsMalloc_8u (specSizeReal));

            // Initialize specifications
            ippsFFTInit_C_64fc (&specComplex, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);
            ippsFFTInit_R_64f (&specReal, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);
        }
        else
        {
            // Get buffer sizes
            ippsFFTGetSize_C_32fc (order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSizeComplex, nullptr, &workSizeComplex);
            ippsFFTGetSize_R_32f (order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast, &specSizeReal, nullptr, &workSizeReal);

            // Allocate specification structures
            specComplex = reinterpret_cast<SpecComplex*> (ippsMalloc_8u (specSizeComplex));
            specReal = reinterpret_cast<SpecReal*> (ippsMalloc_8u (specSizeReal));

            // Initialize specifications
            ippsFFTInit_C_32fc (&specComplex, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);
            ippsFFTInit_R_32f (&specReal, order, IPP_FFT_NODIV_BY_ANY, ippAlgHintFast);
        }

        // Allocate work buffer
        workBuffer = reinterpret_cast<Ipp8u*> (ippsMalloc_8u (jmax (workSizeComplex, workSizeReal)));
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
            if constexpr (std::is_same_v<SampleType, double>)
                ippsFFTFree_C_64fc (specComplex);
            else
                ippsFFTFree_C_32fc (specComplex);

            specComplex = nullptr;
        }

        if (specReal != nullptr)
        {
            if constexpr (std::is_same_v<SampleType, double>)
                ippsFFTFree_R_64f (specReal);
            else
                ippsFFTFree_R_32f (specReal);

            specReal = nullptr;
        }
    }

    void performRealFFTForward (const SampleType* realInput, SampleType* complexOutput) override
    {
        if constexpr (std::is_same_v<SampleType, double>)
            ippsFFTFwd_RToPack_64f (realInput, complexOutput, specReal, workBuffer);
        else
            ippsFFTFwd_RToPack_32f (realInput, complexOutput, specReal, workBuffer);
    }

    void performRealFFTInverse (const SampleType* complexInput, SampleType* realOutput) override
    {
        if constexpr (std::is_same_v<SampleType, double>)
            ippsFFTInv_PackToR_64f (complexInput, realOutput, specReal, workBuffer);
        else
            ippsFFTInv_PackToR_32f (complexInput, realOutput, specReal, workBuffer);
    }

    void performComplexFFTForward (const SampleType* complexInput, SampleType* complexOutput) override
    {
        if constexpr (std::is_same_v<SampleType, double>)
        {
            const auto* input = reinterpret_cast<const Ipp64fc*> (complexInput);
            auto* output = reinterpret_cast<Ipp64fc*> (complexOutput);
            ippsFFTFwd_CToC_64fc (input, output, specComplex, workBuffer);
        }
        else
        {
            const auto* input = reinterpret_cast<const Ipp32fc*> (complexInput);
            auto* output = reinterpret_cast<Ipp32fc*> (complexOutput);
            ippsFFTFwd_CToC_32fc (input, output, specComplex, workBuffer);
        }
    }

    void performComplexFFTInverse (const SampleType* complexInput, SampleType* complexOutput) override
    {
        if constexpr (std::is_same_v<SampleType, double>)
        {
            const auto* input = reinterpret_cast<const Ipp64fc*> (complexInput);
            auto* output = reinterpret_cast<Ipp64fc*> (complexOutput);
            ippsFFTInv_CToC_64fc (input, output, specComplex, workBuffer);
        }
        else
        {
            const auto* input = reinterpret_cast<const Ipp32fc*> (complexInput);
            auto* output = reinterpret_cast<Ipp32fc*> (complexOutput);
            ippsFFTInv_CToC_32fc (input, output, specComplex, workBuffer);
        }
    }

    String getBackendName() const override { return "Intel IPP"; }

private:
    using SpecComplex = std::conditional_t<std::is_same_v<SampleType, double>, IppsFFTSpec_C_64fc, IppsFFTSpec_C_32fc>;
    using SpecReal = std::conditional_t<std::is_same_v<SampleType, double>, IppsFFTSpec_R_64f, IppsFFTSpec_R_32f>;

    Ipp8u* workBuffer = nullptr;
    SpecComplex* specComplex = nullptr;
    SpecReal* specReal = nullptr;
};

#endif

//==============================================================================
// FFTW3 implementation
#if YUP_FFT_USING_FFTW3

template <typename SampleType>
class FFTW3Engine : public detail::FFTEngine<SampleType>
{
public:
    ~FFTW3Engine() override { this->cleanup(); }

    void initialize (int newFftSize) override
    {
        this->cleanup();

        this->fftSize = newFftSize;

        if constexpr (std::is_same_v<SampleType, double>)
        {
            tempComplexBuffer = static_cast<Complex*> (fftw_malloc (sizeof (Complex) * static_cast<size_t> (this->fftSize)));
            tempRealBuffer = static_cast<SampleType*> (fftw_malloc (sizeof (SampleType) * static_cast<size_t> (this->fftSize)));

            auto* complexData = tempComplexBuffer;
            auto* realData = tempRealBuffer;

            planComplexForward = fftw_plan_dft_1d (this->fftSize, complexData, complexData, FFTW_FORWARD, FFTW_ESTIMATE);
            planComplexInverse = fftw_plan_dft_1d (this->fftSize, complexData, complexData, FFTW_BACKWARD, FFTW_ESTIMATE);
            planRealForward = fftw_plan_dft_r2c_1d (this->fftSize, realData, complexData, FFTW_ESTIMATE);
            planRealInverse = fftw_plan_dft_c2r_1d (this->fftSize, complexData, realData, FFTW_ESTIMATE);
        }
        else
        {
            tempComplexBuffer = static_cast<Complex*> (fftwf_malloc (sizeof (Complex) * static_cast<size_t> (this->fftSize)));
            tempRealBuffer = static_cast<SampleType*> (fftwf_malloc (sizeof (SampleType) * static_cast<size_t> (this->fftSize)));

            auto* complexData = tempComplexBuffer;
            auto* realData = tempRealBuffer;

            planComplexForward = fftwf_plan_dft_1d (this->fftSize, complexData, complexData, FFTW_FORWARD, FFTW_ESTIMATE);
            planComplexInverse = fftwf_plan_dft_1d (this->fftSize, complexData, complexData, FFTW_BACKWARD, FFTW_ESTIMATE);
            planRealForward = fftwf_plan_dft_r2c_1d (this->fftSize, realData, complexData, FFTW_ESTIMATE);
            planRealInverse = fftwf_plan_dft_c2r_1d (this->fftSize, complexData, realData, FFTW_ESTIMATE);
        }
    }

    void cleanup() override
    {
        destroyPlan (planComplexForward);
        destroyPlan (planComplexInverse);
        destroyPlan (planRealForward);
        destroyPlan (planRealInverse);

        if (tempComplexBuffer != nullptr)
        {
            freeBuffer (tempComplexBuffer);
            tempComplexBuffer = nullptr;
        }

        if (tempRealBuffer != nullptr)
        {
            freeBuffer (tempRealBuffer);
            tempRealBuffer = nullptr;
        }
    }

    void performRealFFTForward (const SampleType* realInput, SampleType* complexOutput) override
    {
        std::copy_n (realInput, this->fftSize, tempRealBuffer);

        execute (planRealForward);

        const auto halfSize = this->fftSize / 2 + 1;
        for (int i = 0; i < halfSize; ++i)
        {
            complexOutput[i * 2] = tempComplexBuffer[i][0];     // real
            complexOutput[i * 2 + 1] = tempComplexBuffer[i][1]; // imag
        }
    }

    void performRealFFTInverse (const SampleType* complexInput, SampleType* realOutput) override
    {
        // Convert interleaved to FFTW format
        const auto halfSize = this->fftSize / 2 + 1;
        for (int i = 0; i < halfSize; ++i)
        {
            tempComplexBuffer[i][0] = complexInput[i * 2];     // real
            tempComplexBuffer[i][1] = complexInput[i * 2 + 1]; // imag
        }

        execute (planRealInverse);

        std::copy_n (tempRealBuffer, this->fftSize, realOutput);
    }

    void performComplexFFTForward (const SampleType* complexInput, SampleType* complexOutput) override
    {
        for (int i = 0; i < this->fftSize; ++i)
        {
            tempComplexBuffer[i][0] = complexInput[i * 2];     // real
            tempComplexBuffer[i][1] = complexInput[i * 2 + 1]; // imag
        }

        execute (planComplexForward);

        for (int i = 0; i < this->fftSize; ++i)
        {
            complexOutput[i * 2] = tempComplexBuffer[i][0];     // real
            complexOutput[i * 2 + 1] = tempComplexBuffer[i][1]; // imag
        }
    }

    void performComplexFFTInverse (const SampleType* complexInput, SampleType* complexOutput) override
    {
        for (int i = 0; i < this->fftSize; ++i)
        {
            tempComplexBuffer[i][0] = complexInput[i * 2];     // real
            tempComplexBuffer[i][1] = complexInput[i * 2 + 1]; // imag
        }

        execute (planComplexInverse);

        for (int i = 0; i < this->fftSize; ++i)
        {
            complexOutput[i * 2] = tempComplexBuffer[i][0];     // real
            complexOutput[i * 2 + 1] = tempComplexBuffer[i][1]; // imag
        }
    }

    String getBackendName() const override { return "FFTW3"; }

private:
    using Complex = std::conditional_t<std::is_same_v<SampleType, double>, fftw_complex, fftwf_complex>;
    using Plan = std::conditional_t<std::is_same_v<SampleType, double>, fftw_plan, fftwf_plan>;

    void execute (Plan plan)
    {
        if constexpr (std::is_same_v<SampleType, double>)
            fftw_execute (plan);
        else
            fftwf_execute (plan);
    }

    void destroyPlan (Plan& plan)
    {
        if (plan == nullptr)
            return;

        if constexpr (std::is_same_v<SampleType, double>)
            fftw_destroy_plan (plan);
        else
            fftwf_destroy_plan (plan);

        plan = nullptr;
    }

    template <typename BufferType>
    void freeBuffer (BufferType*& buffer)
    {
        if constexpr (std::is_same_v<SampleType, double>)
            fftw_free (buffer);
        else
            fftwf_free (buffer);
    }

    Plan planComplexForward = nullptr;
    Plan planComplexInverse = nullptr;
    Plan planRealForward = nullptr;
    Plan planRealInverse = nullptr;
    Complex* tempComplexBuffer = nullptr;
    SampleType* tempRealBuffer = nullptr;
};

#endif

//==============================================================================
// Factory function to create appropriate implementation
template <typename SampleType>
std::unique_ptr<detail::FFTEngine<SampleType>> createFFTEngine()
{
#if YUP_FFT_USING_PFFFT
    return std::make_unique<PFFTEngine<SampleType>>();
#elif YUP_FFT_USING_VDSP
    return std::make_unique<VDSPEngine<SampleType>>();
#elif YUP_FFT_USING_IPP
    return std::make_unique<IPPEngine<SampleType>>();
#elif YUP_FFT_USING_FFTW3
    return std::make_unique<FFTW3Engine<SampleType>>();
#elif YUP_FFT_USING_OOURA
    return std::make_unique<OouraEngine<SampleType>>();
#else
    jassertfalse; // No FFT backend available
    return nullptr;
#endif
}

//==============================================================================
// Constructor implementations
template <typename SampleType>
FFTProcessor<SampleType>::FFTProcessor()
    : engine (createFFTEngine<SampleType>())
{
    setSize (512);
}

template <typename SampleType>
FFTProcessor<SampleType>::FFTProcessor (int fftSize)
    : engine (createFFTEngine<SampleType>())
{
    setSize (fftSize);
}

template <typename SampleType>
FFTProcessor<SampleType>::~FFTProcessor()
{
    if (engine)
        engine->cleanup();
}

template <typename SampleType>
FFTProcessor<SampleType>::FFTProcessor (FFTProcessor&& other) noexcept
    : fftSize (std::exchange (other.fftSize, 0))
    , scaling (other.scaling)
    , scalingFactor (other.scalingFactor)
    , engine (std::move (other.engine))
{
}

template <typename SampleType>
FFTProcessor<SampleType>& FFTProcessor<SampleType>::operator= (FFTProcessor&& other) noexcept
{
    if (this != &other)
    {
        if (engine)
            engine->cleanup();

        fftSize = std::exchange (other.fftSize, 0);
        scaling = other.scaling;
        scalingFactor = other.scalingFactor;
        engine = std::move (other.engine);
    }

    return *this;
}

//==============================================================================

template <typename SampleType>
void FFTProcessor<SampleType>::setScaling (FFTScaling newScaling) noexcept
{
    if (scaling != newScaling)
    {
        scaling = newScaling;

        updateScalingFactor();
    }
}

template <typename SampleType>
void FFTProcessor<SampleType>::setSize (int newSize)
{
    jassert (isPowerOfTwo (newSize) && newSize >= 64 && newSize <= 65536);

    if (newSize != fftSize)
    {
        fftSize = newSize;

        updateScalingFactor();

        if (engine)
            engine->initialize (fftSize);
    }
}

template <typename SampleType>
void FFTProcessor<SampleType>::performRealFFTForward (const SampleType* realInput, SampleType* complexOutput)
{
    jassert (realInput != nullptr && complexOutput != nullptr);
    jassert (engine != nullptr);

    engine->performRealFFTForward (realInput, complexOutput);

    applyScaling (complexOutput, fftSize * 2, true);
}

template <typename SampleType>
void FFTProcessor<SampleType>::performRealFFTInverse (const SampleType* complexInput, SampleType* realOutput)
{
    jassert (complexInput != nullptr && realOutput != nullptr);
    jassert (engine != nullptr);

    engine->performRealFFTInverse (complexInput, realOutput);

    applyScaling (realOutput, fftSize, false);
}

template <typename SampleType>
void FFTProcessor<SampleType>::performComplexFFTForward (const SampleType* complexInput, SampleType* complexOutput)
{
    jassert (complexInput != nullptr && complexOutput != nullptr);
    jassert (engine != nullptr);

    engine->performComplexFFTForward (complexInput, complexOutput);

    applyScaling (complexOutput, fftSize * 2, true);
}

template <typename SampleType>
void FFTProcessor<SampleType>::performComplexFFTInverse (const SampleType* complexInput, SampleType* complexOutput)
{
    jassert (complexInput != nullptr && complexOutput != nullptr);
    jassert (engine != nullptr);

    engine->performComplexFFTInverse (complexInput, complexOutput);

    applyScaling (complexOutput, fftSize * 2, false);
}

template <typename SampleType>
String FFTProcessor<SampleType>::getBackendName() const
{
    return engine != nullptr ? engine->getBackendName() : "Unknown";
}

//==============================================================================

template <typename SampleType>
void FFTProcessor<SampleType>::updateScalingFactor()
{
    if (scaling == FFTScaling::unitary)
        scalingFactor = SampleType (1) / std::sqrt (static_cast<SampleType> (fftSize));

    else if (scaling == FFTScaling::asymmetric)
        scalingFactor = SampleType (1) / static_cast<SampleType> (fftSize);

    else
        scalingFactor = SampleType (1);
}

template <typename SampleType>
void FFTProcessor<SampleType>::applyScaling (SampleType* data, int numElements, bool isForward) const
{
    if (scaling == FFTScaling::none || (scaling == FFTScaling::asymmetric && ! isForward))
        return;

    FloatVectorOperationsBase<SampleType, int>::multiply (data, scalingFactor, numElements);
}

//==============================================================================

template class FFTProcessor<float>;
template class FFTProcessor<double>;

} // namespace yup
