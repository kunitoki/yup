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
    Comprehensive Butterworth filter implementation supporting all filter modes.

    This class implements a mathematically correct Butterworth filter that supports
    all standard filter types: lowpass, highpass, bandpass, bandstop, and allpass.
    The filter is designed for realtime use with pre-allocated coefficient storage
    and stable, mathematically accurate pole placement.

    Features:
    - All filter modes with correct frequency transformations
    - Cascaded biquad implementation for higher orders
    - Allocation-free coefficient calculation using FilterDesigner
    - Proper bilinear transform with frequency prewarping
    - Mathematically correct pole placement
    - Stable across all parameter ranges

    The filter uses analog prototype design with bilinear transformation to
    ensure proper frequency response characteristics. Poles are calculated
    using the standard Butterworth equations with even angular spacing
    around the unit circle in the s-plane.

    @see FilterBase, BiquadCascade, FilterDesigner
*/
template <typename SampleType, typename CoeffType = double>
class ButterworthFilter : public BiquadCascade<SampleType, CoeffType>
{
    using BaseFilterType = BiquadCascade<SampleType, CoeffType>;

    //==============================================================================
    /** Maximum supported filter order */
    static constexpr int maxOrder = 32;

public:
    //==============================================================================
    /** Default constructor */
    ButterworthFilter()
    {
        // Pre-allocate workspace for maximum order
        workspace.reserve (maxOrder);
        coefficients.reserve (maxOrder / 2 + 1);
    }

    /** Constructor with initial parameters */
    ButterworthFilter (FilterModeType mode, int filterOrder, CoeffType freq)
        : ButterworthFilter()
    {
        setParameters (mode, filterOrder, freq, static_cast<CoeffType> (0.0), static_cast<CoeffType> (0.0), 44100.0);
    }

    //==============================================================================
    /**
        Sets the filter parameters.

        @param mode           The filter mode
        @param filterOrder    The filter order (1 to maxOrder)
        @param freq           The primary frequency (cutoff, center, etc.)
        @param freq2          Secondary frequency for bandpass/bandstop filters
        @param sampleRate     The sample rate in Hz
    */
    void setParameters (FilterModeType mode,
                        int filterOrder,
                        CoeffType freq,
                        CoeffType freq2 = static_cast<CoeffType> (0.0),
                        double sampleRate = 44100.0) noexcept
    {
        mode = resolveFilterMode (mode, getSupportedModes());

        jassert (filterOrder >= 1 && filterOrder <= maxOrder);
        jassert (freq > static_cast<CoeffType> (0.0));
        if (mode.test (FilterMode::bandpass) || mode.test (FilterMode::bandstop))
            jassert (freq2 > freq && freq2 > static_cast<CoeffType> (0.0));

        // Ensure order is valid (1 or power of 2)
        filterOrder = filterOrder == 1 ? filterOrder : jlimit (2, maxOrder, nextPowerOfTwo (filterOrder));

        if (filterMode != mode
            || order != filterOrder
            || ! approximatelyEqual (frequency, freq)
            || ! approximatelyEqual (frequency2, freq2)
            || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            filterMode = mode;
            order = filterOrder;
            frequency = freq;
            frequency2 = freq2;
            this->sampleRate = sampleRate;

            updateCoefficients();
        }
    }

    /**
        Sets the filter mode.

        @param mode  The new filter mode
    */
    void setMode (FilterModeType mode) noexcept
    {
        mode = resolveFilterMode (mode, getSupportedModes());

        if (filterMode != mode)
        {
            filterMode = mode;
            updateCoefficients();
        }
    }

    /**
        Sets the filter order.

        @param filterOrder  The new filter order (1 to maxOrder)
    */
    void setOrder (int filterOrder) noexcept
    {
        filterOrder = filterOrder == 1 ? filterOrder : jlimit (2, maxOrder, nextPowerOfTwo (filterOrder));

        if (order != filterOrder)
        {
            order = filterOrder;
            updateCoefficients();
        }
    }

    /**
        Sets the primary frequency.

        @param freq  The primary frequency in Hz
    */
    void setFrequency (CoeffType freq) noexcept
    {
        jassert (freq > static_cast<CoeffType> (0.0));

        if (! approximatelyEqual (frequency, freq))
        {
            frequency = freq;
            updateCoefficients();
        }
    }

    /**
        Sets the secondary frequency for bandpass/bandstop filters.

        @param freq2  The secondary frequency in Hz
    */
    void setSecondaryFrequency (CoeffType freq2) noexcept
    {
        jassert (freq2 > static_cast<CoeffType> (0.0));

        if (! approximatelyEqual (frequency2, freq2))
        {
            frequency2 = freq2;
            updateCoefficients();
        }
    }

    //==============================================================================
    /**
        Returns the current filter mode.
    */
    FilterModeType getMode() const noexcept { return filterMode; }

    /**
        Returns the current filter order.
    */
    int getOrder() const noexcept { return order; }

    /**
        Returns the primary frequency.
    */
    CoeffType getFrequency() const noexcept { return frequency; }

    /**
        Returns the secondary frequency.
    */
    CoeffType getSecondaryFrequency() const noexcept { return frequency2; }

    /**
        Returns the supported filter modes.
    */
    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::lowpass | FilterMode::highpass | FilterMode::bandpass |
               FilterMode::bandstop | FilterMode::allpass;
    }

    //==============================================================================
    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;

        BaseFilterType::prepare (sampleRate, maximumBlockSize);

        updateCoefficients();
    }

    /** @internal */
    void getPolesZeros (ComplexVector<CoeffType>& poles,
                        ComplexVector<CoeffType>& zeros) const override
    {
        poles = workspace.zpkPoles;
        zeros = workspace.zpkZeros;
    }

private:
    //==============================================================================
    void updateCoefficients()
    {
        if (this->sampleRate <= 0.0)
            return;

        // Use FilterDesigner to calculate coefficients
        const auto numSections = FilterDesigner<CoeffType>::designButterworth (
            filterMode,
            order,
            frequency,
            frequency2,
            this->sampleRate,
            workspace,
            coefficients
        );

        // Update the biquad cascade
        if (numSections > 0)
        {
            const bool orderChanged = BaseFilterType::getNumSections() != static_cast<size_t> (numSections);
            
            // Only resize if the number of sections has changed
            if (orderChanged)
                BaseFilterType::setNumSections (numSections);

            for (int i = 0; i < numSections; ++i)
                BaseFilterType::setSectionCoefficients (i, coefficients[i]);
                
            // Reset all sections when order changes to prevent ringing from stored energy
            if (orderChanged)
                BaseFilterType::reset();
        }
    }

    //==============================================================================
    FilterModeType filterMode = FilterMode::lowpass;
    int order = 2; // Default to 2nd order
    CoeffType frequency = static_cast<CoeffType> (1000.0);
    CoeffType frequency2 = static_cast<CoeffType> (2000.0);

    // Workspace and storage for coefficient calculation
    ButterworthWorkspace<CoeffType> workspace;
    std::vector<BiquadCoefficients<CoeffType>> coefficients;

    //==============================================================================
    YUP_LEAK_DETECTOR (ButterworthFilter)
};

} // namespace yup
