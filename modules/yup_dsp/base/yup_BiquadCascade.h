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
    Cascaded biquad filter implementation.

    Allows chaining multiple biquad sections together to create higher-order filters.
    Each section processes the output of the previous section, creating an overall
    filter response that is the product of all individual section responses.

    @see Biquad
*/
template <typename SampleType, typename CoeffType = double>
class BiquadCascade : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Constructor with specified number of sections */
    explicit BiquadCascade (int numSections = 1,
                            typename Biquad<SampleType, CoeffType>::Topology topology = Biquad<SampleType, CoeffType>::Topology::directFormII)
    {
        setNumSections (numSections, topology);
    }

    //==============================================================================
    /**
        Sets the coefficients for a specific section.

        @param sectionIndex  The index of the section (0-based)
        @param coefficients  The new coefficients for this section
    */
    void setSectionCoefficients (size_t sectionIndex, const BiquadCoefficients<CoeffType>& coefficients) noexcept
    {
        if (sectionIndex < sections.size())
            sections[sectionIndex].setCoefficients (coefficients);
    }

    /**
        Gets the coefficients for a specific section.

        @param sectionIndex  The index of the section (0-based)
        @returns            The coefficients for this section
    */
    const BiquadCoefficients<CoeffType>& getSectionCoefficients (size_t sectionIndex) const noexcept
    {
        if (sectionIndex < sections.size())
            return sections[sectionIndex].getCoefficients();

        static BiquadCoefficients<CoeffType> empty;
        return empty;
    }

    /**
        Gets the number of cascaded sections.

        @returns  The number of biquad sections
    */
    size_t getNumSections() const noexcept
    {
        return sections.size();
    }

    /**
        Resizes the cascade to have a different number of sections.

        @param newNumSections  The new number of sections
        @param topology       The topology to use for new sections
    */
    void setNumSections (int newNumSections,
                         typename Biquad<SampleType, CoeffType>::Topology topology = Biquad<SampleType, CoeffType>::Topology::directFormII)
    {
        sections.clear();
        sections.resize (static_cast<size_t> (newNumSections), Biquad<SampleType, CoeffType> (topology));

        for (int i = 0; i < newNumSections; ++i)
            sections[i].prepare (this->sampleRate, this->maximumBlockSize);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        for (auto& section : sections)
            section.reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;

        for (auto& section : sections)
            section.prepare (sampleRate, maximumBlockSize);
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        auto output = inputSample;
        for (auto& section : sections)
            output = section.processSample (output);
        return output;
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        if (sections.empty())
        {
            if (inputBuffer != outputBuffer)
                std::copy_n (inputBuffer, numSamples, outputBuffer);

            return;
        }

        sections[0].processBlock (inputBuffer, outputBuffer, numSamples);

        for (size_t i = 1; i < sections.size(); ++i)
            sections[i].processInPlace (outputBuffer, numSamples);
    }

    /** @internal */
    Complex<CoeffType> getComplexResponse (CoeffType frequency) const override
    {
        auto response = Complex<CoeffType> (1.0, 0.0);
        for (const auto& section : sections)
            response = response * section.getComplexResponse (frequency);
        return response;
    }

    /** @internal */
    void getPolesZeros (
        ComplexVector<CoeffType>& poles,
        ComplexVector<CoeffType>& zeros) const override
    {
        poles.reserve (sections.size() * 2);
        zeros.reserve (sections.size() * 2);

        for (const auto& section : sections)
            section.getPolesZeros (poles, zeros);
    }

private:
    //==============================================================================
    std::vector<Biquad<SampleType, CoeffType>> sections;

    //==============================================================================
    YUP_LEAK_DETECTOR (BiquadCascade)
};

//==============================================================================
/** Type aliases for convenience */
using BiquadCascadeFloat = BiquadCascade<float>;   // float samples, double coefficients (default)
using BiquadCascadeDouble = BiquadCascade<double>; // double samples, double coefficients (default)

} // namespace yup
