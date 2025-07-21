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

#include <memory>

namespace yup
{

//==============================================================================
/** 
    Second-order IIR filter implementation (biquad).
    
    This class implements a general-purpose biquad filter supporting multiple
    topologies including Direct Form I, Direct Form II, and Transposed Direct Form II.
    It provides both per-sample and block processing with SIMD optimizations.
    
    The filter implements the difference equation:
    y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    
    @see FilterBase, BiquadCoefficients, BiquadState
*/
template <typename SampleType, typename CoeffType = double>
class Biquad : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Filter topology enumeration */
    enum class Topology
    {
        directFormI,           /**< Direct Form I - separate input and output delay lines */
        directFormII,          /**< Direct Form II - shared delay line (canonical form) */
        transposedDirectFormII /**< Transposed Direct Form II - parallel structure */
    };

    //==============================================================================
    /** Constructor with optional topology selection */
    explicit Biquad (Topology topology = Topology::directFormII) noexcept
        : filterTopology (topology)
    {
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        topologyState.reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;

        reset();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        switch (filterTopology)
        {
            case Topology::directFormI:
                return processDirectFormI (inputSample);

            case Topology::directFormII:
                return processDirectFormII (inputSample);

            case Topology::transposedDirectFormII:
                return processTransposedDirectFormII (inputSample);

            default:
                return inputSample;
        }
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        switch (filterTopology)
        {
            case Topology::directFormI:
                processBlockDirectFormI (inputBuffer, outputBuffer, numSamples);
                break;

            case Topology::directFormII:
                processBlockDirectFormII (inputBuffer, outputBuffer, numSamples);
                break;

            case Topology::transposedDirectFormII:
                processBlockTransposedDirectFormII (inputBuffer, outputBuffer, numSamples);
                break;
        }
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        return coefficients.getComplexResponse (frequency, this->sampleRate);
    }

    //==============================================================================
    /** 
        Sets the filter coefficients.
        
        @param newCoefficients  The new biquad coefficients
    */
    void setCoefficients (const BiquadCoefficients<CoeffType>& newCoefficients) noexcept
    {
        coefficients = newCoefficients;
        coefficients.normalize();
    }

    /** 
        Gets the current filter coefficients.
        
        @returns  The current biquad coefficients
    */
    const BiquadCoefficients<CoeffType>& getCoefficients() const noexcept
    {
        return coefficients;
    }

    /** 
        Sets the filter topology.
        
        @param newTopology  The new filter topology
    */
    void setTopology (Topology newTopology) noexcept
    {
        if (filterTopology != newTopology)
        {
            filterTopology = newTopology;
            reset();
        }
    }

    /** 
        Gets the current filter topology.
        
        @returns  The current filter topology
    */
    Topology getTopology() const noexcept
    {
        return filterTopology;
    }

private:
    //==============================================================================
    /** State structures for different topologies - using CoeffType for precision

        DirectFormIState: uses x1, x2, y1, y2
        DirectFormIIState: uses x1 = w1 and x2 = w2
        TransposedDirectFormIIState: uses x1 = s1 and x2 = s2
    */
    struct TopologyState
    {
        CoeffType x1 = 0, x2 = 0;  // Input delay line
        CoeffType y1 = 0, y2 = 0;  // Output delay line

        void reset() noexcept
        {
            x1 = x2 = y1 = y2 = static_cast<CoeffType> (0.0);
        }
    };

    //==============================================================================
    /** Direct Form I processing */
    SampleType processDirectFormI (SampleType input) noexcept
    {
        // Promote input to CoeffType precision
        const auto inputCoeff = static_cast<CoeffType> (input);
        
        const auto outputCoeff = coefficients.b0 * inputCoeff + coefficients.b1 * topologyState.x1 + coefficients.b2 * topologyState.x2
                               - coefficients.a1 * topologyState.y1 - coefficients.a2 * topologyState.y2;

        // Update state in CoeffType precision
        topologyState.x2 = topologyState.x1;
        topologyState.x1 = inputCoeff;
        topologyState.y2 = topologyState.y1;
        topologyState.y1 = outputCoeff;

        // Convert back to SampleType for return
        return static_cast<SampleType> (outputCoeff);
    }

    /** Direct Form II processing */
    SampleType processDirectFormII (SampleType input) noexcept
    {
        // Promote input to CoeffType precision
        const auto inputCoeff = static_cast<CoeffType> (input);
        
        const auto w = inputCoeff - coefficients.a1 * topologyState.x1 - coefficients.a2 * topologyState.x2;
        const auto outputCoeff = coefficients.b0 * w + coefficients.b1 * topologyState.x1 + coefficients.b2 * topologyState.x2;

        // Update state in CoeffType precision
        topologyState.x2 = topologyState.x1;
        topologyState.x1 = w;

        // Convert back to SampleType for return
        return static_cast<SampleType> (outputCoeff);
    }

    /** Transposed Direct Form II processing */
    SampleType processTransposedDirectFormII (SampleType input) noexcept
    {
        // Promote input to CoeffType precision
        const auto inputCoeff = static_cast<CoeffType> (input);
        
        const auto outputCoeff = coefficients.b0 * inputCoeff + topologyState.x1;

        // Update state in CoeffType precision
        topologyState.x1 = coefficients.b1 * inputCoeff - coefficients.a1 * outputCoeff + topologyState.x2;
        topologyState.x2 = coefficients.b2 * inputCoeff - coefficients.a2 * outputCoeff;

        // Convert back to SampleType for return
        return static_cast<SampleType> (outputCoeff);
    }

    //==============================================================================
    /** Block processing implementations */
    void processBlockDirectFormI (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processDirectFormI (input[i]);
    }

    void processBlockDirectFormII (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto w1 = topologyState.x1;
        auto w2 = topologyState.x2;
        const auto b0 = coefficients.b0;
        const auto b1 = coefficients.b1;
        const auto b2 = coefficients.b2;
        const auto a1 = coefficients.a1;
        const auto a2 = coefficients.a2;

        for (int i = 0; i < numSamples; ++i)
        {
            // Promote input to CoeffType precision
            const auto inputCoeff = static_cast<CoeffType> (input[i]);
            
            const auto w = inputCoeff - a1 * w1 - a2 * w2;
            const auto outputCoeff = b0 * w + b1 * w1 + b2 * w2;

            // Convert back to SampleType for output
            output[i] = static_cast<SampleType> (outputCoeff);

            w2 = w1;
            w1 = w;
        }

        topologyState.x1 = w1;
        topologyState.x2 = w2;
    }

    void processBlockTransposedDirectFormII (const SampleType* input, SampleType* output, int numSamples) noexcept
    {
        auto s1 = topologyState.x1;
        auto s2 = topologyState.x2;
        const auto b0 = coefficients.b0;
        const auto b1 = coefficients.b1;
        const auto b2 = coefficients.b2;
        const auto a1 = coefficients.a1;
        const auto a2 = coefficients.a2;

        for (int i = 0; i < numSamples; ++i)
        {
            // Promote input to CoeffType precision
            const auto inputCoeff = static_cast<CoeffType> (input[i]);
            
            const auto outputCoeff = b0 * inputCoeff + s1;
            
            // Convert back to SampleType for output
            output[i] = static_cast<SampleType> (outputCoeff);

            s1 = b1 * inputCoeff - a1 * outputCoeff + s2;
            s2 = b2 * inputCoeff - a2 * outputCoeff;
        }

        topologyState.x1 = s1;
        topologyState.x2 = s2;
    }

    //==============================================================================
    BiquadCoefficients<CoeffType> coefficients;
    TopologyState topologyState;
    Topology filterTopology = Topology::directFormII;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Biquad)
};

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
    /** @internal */
    void reset() noexcept override
    {
        for (auto& section : sections)
            section->reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;

        for (auto& section : sections)
            section->prepare (sampleRate, maximumBlockSize);
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        auto output = inputSample;
        for (auto& section : sections)
            output = section->processSample (output);
        return output;
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        if (sections.empty())
        {
            if (inputBuffer != outputBuffer)
                std::copy (inputBuffer, inputBuffer + numSamples, outputBuffer);
            return;
        }

        sections[0]->processBlock (inputBuffer, outputBuffer, numSamples);

        for (size_t i = 1; i < sections.size(); ++i)
            sections[i]->processInPlace (outputBuffer, numSamples);
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        auto response = DspMath::Complex<CoeffType> (1.0, 0.0);
        for (const auto& section : sections)
            response = response * section->getComplexResponse (frequency);
        return response;
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
            sections[sectionIndex]->setCoefficients (coefficients);
    }

    /** 
        Gets the coefficients for a specific section.
        
        @param sectionIndex  The index of the section (0-based)
        @returns            The coefficients for this section
    */
    const BiquadCoefficients<CoeffType>& getSectionCoefficients (size_t sectionIndex) const noexcept
    {
        if (sectionIndex < sections.size())
            return sections[sectionIndex]->getCoefficients();
        
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
        sections.reserve (static_cast<size_t> (newNumSections));
        
        for (int i = 0; i < newNumSections; ++i)
        {
            sections.emplace_back (std::make_unique<Biquad<SampleType, CoeffType>> (topology));
            sections.back()->prepare (this->sampleRate, this->maximumBlockSize);
        }
    }

private:
    //==============================================================================
    std::vector<std::unique_ptr<Biquad<SampleType, CoeffType>>> sections;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BiquadCascade)
};

//==============================================================================
/** Type aliases for convenience */
using BiquadFloat = Biquad<float>;                    // float samples, double coefficients (default)
using BiquadDouble = Biquad<double>;                  // double samples, double coefficients (default)
using BiquadCascadeFloat = BiquadCascade<float>;      // float samples, double coefficients (default)
using BiquadCascadeDouble = BiquadCascade<double>;    // double samples, double coefficients (default)

} // namespace yup
