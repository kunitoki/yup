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

#include <vector>
#include <memory>

namespace yup
{

//==============================================================================
/**
    Finite Impulse Response (FIR) filter implementation.

    This class implements high-quality FIR filters with windowing support.
    FIR filters have linear phase response and are always stable, making them
    ideal for applications requiring precise phase relationships.

    Features:
    - Kaiser-Bessel windowing for optimal frequency response
    - Configurable filter length and cutoff frequency
    - Linear phase response (symmetric coefficients)
    - Efficient circular buffer implementation
    - Block processing with SIMD optimizations

    The filter uses the windowing method with Kaiser-Bessel windows to design
    filters with excellent stopband attenuation and minimal ripple.

    @see FilterBase, KaiserWindow
*/
template <typename SampleType, typename CoeffType = double>
class FirFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** FIR filter type enumeration */
    enum class Type
    {
        lowpass,     /**< Low-pass filter */
        highpass,    /**< High-pass filter */
        bandpass,    /**< Band-pass filter */
        bandstop,    /**< Band-stop (notch) filter */
        hilbert,     /**< Hilbert transformer (90-degree phase shift) */
        differentiator /**< Differentiator filter */
    };

    //==============================================================================
    /** Default constructor */
    FirFilter() = default;

    /** Constructor with filter parameters */
    FirFilter (Type filterType, int filterLength, CoeffType cutoffFreq, double sampleRate,
               CoeffType beta = static_cast<CoeffType> (6.0))
        : type (filterType), length (filterLength), cutoff (cutoffFreq), kaiserBeta (beta)
    {
        this->sampleRate = sampleRate;
        designFilter();
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        std::fill (delayLine.begin(), delayLine.end(), static_cast<SampleType> (0.0));
        writeIndex = 0;
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;

        // Ensure delay line is properly sized
        if (delayLine.size() != static_cast<size_t> (length))
        {
            delayLine.resize (static_cast<size_t> (length));
            reset();
        }

        designFilter();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        // Store input in circular buffer
        delayLine[static_cast<size_t> (writeIndex)] = inputSample;

        // Compute convolution
        CoeffType output = static_cast<CoeffType> (0.0);
        int delayIndex = writeIndex;

        for (int i = 0; i < length; ++i)
        {
            const auto delaySample = static_cast<CoeffType> (delayLine[static_cast<size_t> (delayIndex)]);
            output += coefficients[static_cast<size_t> (i)] * delaySample;

            // Circular buffer index
            --delayIndex;
            if (delayIndex < 0)
                delayIndex = length - 1;
        }

        // Update write index
        ++writeIndex;
        if (writeIndex >= length)
            writeIndex = 0;

        return static_cast<SampleType> (output);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (this->sampleRate));

        DspMath::Complex<CoeffType> response (0.0, 0.0);

        for (int n = 0; n < length; ++n)
        {
            const auto phase = -omega * static_cast<CoeffType> (n);
            const auto coeff = coefficients[static_cast<size_t> (n)];

            response = response + DspMath::Complex<CoeffType> (coeff * std::cos (phase), coeff * std::sin (phase));
        }

        return response;
    }

    //==============================================================================
    /**
        Sets the filter parameters and redesigns the filter.

        @param filterType   The type of filter to design
        @param filterLength The length of the FIR filter (number of taps)
        @param cutoffFreq   The cutoff frequency in Hz
        @param sampleRate   The sample rate in Hz
        @param beta        Kaiser window beta parameter (controls stopband attenuation)
    */
    void setParameters (Type filterType, int filterLength, CoeffType cutoffFreq, double sampleRate,
                       CoeffType beta = static_cast<CoeffType> (6.0)) noexcept
    {
        type = filterType;
        length = filterLength;
        cutoff = cutoffFreq;
        kaiserBeta = beta;
        this->sampleRate = sampleRate;

        designFilter();

        // Resize delay line if needed
        if (delayLine.size() != static_cast<size_t> (length))
        {
            delayLine.resize (static_cast<size_t> (length));
            reset();
        }
    }

    /**
        Sets the filter parameters for bandpass/bandstop filters.

        @param filterType      The type of filter (should be bandpass or bandstop)
        @param filterLength    The length of the FIR filter
        @param lowCutoffFreq   The low cutoff frequency in Hz
        @param highCutoffFreq  The high cutoff frequency in Hz
        @param sampleRate      The sample rate in Hz
        @param beta           Kaiser window beta parameter
    */
    void setBandParameters (Type filterType, int filterLength, CoeffType lowCutoffFreq, CoeffType highCutoffFreq,
                           double sampleRate, CoeffType beta = static_cast<CoeffType> (6.0)) noexcept
    {
        type = filterType;
        length = filterLength;
        cutoff = lowCutoffFreq;
        cutoff2 = highCutoffFreq;
        kaiserBeta = beta;
        this->sampleRate = sampleRate;

        designFilter();

        // Resize delay line if needed
        if (delayLine.size() != static_cast<size_t> (length))
        {
            delayLine.resize (static_cast<size_t> (length));
            reset();
        }
    }

    /** Gets the filter type */
    Type getType() const noexcept { return type; }

    /** Gets the filter length */
    int getLength() const noexcept { return length; }

    /** Gets the cutoff frequency */
    CoeffType getCutoffFrequency() const noexcept { return cutoff; }

    /** Gets the second cutoff frequency (for bandpass/bandstop) */
    CoeffType getSecondCutoffFrequency() const noexcept { return cutoff2; }

    /** Gets the Kaiser beta parameter */
    CoeffType getKaiserBeta() const noexcept { return kaiserBeta; }

    /** Gets the filter coefficients */
    const std::vector<CoeffType>& getCoefficients() const noexcept { return coefficients; }

private:
    //==============================================================================
    /** Designs the FIR filter coefficients using the windowing method */
    void designFilter() noexcept
    {
        if (length <= 0)
            return;

        coefficients.resize (static_cast<size_t> (length));

        switch (type)
        {
            case Type::lowpass:
                designLowpass();
                break;
            case Type::highpass:
                designHighpass();
                break;
            case Type::bandpass:
                designBandpass();
                break;
            case Type::bandstop:
                designBandstop();
                break;
            case Type::hilbert:
                designHilbert();
                break;
            case Type::differentiator:
                designDifferentiator();
                break;
        }
    }

    /** Designs lowpass filter coefficients */
    void designLowpass() noexcept
    {
        FilterDesigner<CoeffType>::designFirLowpass (coefficients, cutoff, this->sampleRate, "kaiser", kaiserBeta);
    }

    /** Designs highpass filter coefficients */
    void designHighpass() noexcept
    {
        FilterDesigner<CoeffType>::designFirHighpass (coefficients, cutoff, this->sampleRate, "kaiser", kaiserBeta);
    }

    /** Designs bandpass filter coefficients */
    void designBandpass() noexcept
    {
        FilterDesigner<CoeffType>::designFirBandpass (coefficients, cutoff, cutoff2, this->sampleRate, "kaiser", kaiserBeta);
    }

    /** Designs bandstop filter coefficients */
    void designBandstop() noexcept
    {
        FilterDesigner<CoeffType>::designFirBandstop (coefficients, cutoff, cutoff2, this->sampleRate, "kaiser", kaiserBeta);
    }

    /** Designs Hilbert transformer coefficients */
    void designHilbert() noexcept
    {
        const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);

        for (int n = 0; n < length; ++n)
        {
            const auto nOffset = static_cast<CoeffType> (n) - center;

            if (std::abs (nOffset) < 1e-10)
            {
                coefficients[static_cast<size_t> (n)] = static_cast<CoeffType> (0.0);
            }
            else
            {
                coefficients[static_cast<size_t> (n)] = (static_cast<CoeffType> (1.0) - std::cos (MathConstants<CoeffType>::pi * nOffset)) /
                                                       (MathConstants<CoeffType>::pi * nOffset);
            }
        }
    }

    /** Designs differentiator filter coefficients */
    void designDifferentiator() noexcept
    {
        const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);

        for (int n = 0; n < length; ++n)
        {
            const auto nOffset = static_cast<CoeffType> (n) - center;

            if (std::abs (nOffset) < 1e-10)
            {
                coefficients[static_cast<size_t> (n)] = static_cast<CoeffType> (0.0);
            }
            else
            {
                coefficients[static_cast<size_t> (n)] = std::cos (MathConstants<CoeffType>::pi * nOffset) / nOffset;
                if ((static_cast<int> (n) - static_cast<int> (center)) % 2 == 1)
                    coefficients[static_cast<size_t> (n)] = -coefficients[static_cast<size_t> (n)];
            }
        }
    }


    //==============================================================================
    Type type = Type::lowpass;
    int length = 64;
    CoeffType cutoff = static_cast<CoeffType> (1000.0);
    CoeffType cutoff2 = static_cast<CoeffType> (2000.0);  // For bandpass/bandstop
    CoeffType kaiserBeta = static_cast<CoeffType> (6.0);

    std::vector<CoeffType> coefficients;
    std::vector<SampleType> delayLine;
    int writeIndex = 0;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FirFilter)
};

//==============================================================================
/** Type aliases for convenience */
using FirFilterFloat = FirFilter<float>;      // float samples, double coefficients (default)
using FirFilterDouble = FirFilter<double>;    // double samples, double coefficients (default)

} // namespace yup
