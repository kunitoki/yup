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

#include <array>

namespace yup
{

//==============================================================================
/** 
    Vowel Formant Filter implementation.
    
    This filter simulates vocal formants to create vowel-like sounds from any
    input signal. It uses multiple parallel bandpass filters tuned to the
    characteristic resonant frequencies (formants) of human vowels.
    
    The filter supports morphing between different vowel sounds and provides
    controls for formant strength, bandwidth, and vocal tract modeling.
    
    Key features:
    - Authentic vowel formant frequencies for A, E, I, O, U
    - Smooth morphing between vowel sounds
    - Configurable number of formants (2-5)
    - Formant strength and bandwidth controls
    - Gender-specific formant frequencies (male/female)
    - Real-time vowel modulation capabilities
    
    The filter uses a dual-precision architecture where:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal calculations (defaults to double for precision)
    
    @see FilterBase, RbjFilter, BiquadCascade
*/
template <typename SampleType, typename CoeffType = double>
class VowelFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    /** Vowel type enumeration */
    enum class Vowel
    {
        A,   /**< Vowel "A" (as in "father") */
        E,   /**< Vowel "E" (as in "bed") */
        I,   /**< Vowel "I" (as in "see") */
        O,   /**< Vowel "O" (as in "law") */
        U    /**< Vowel "U" (as in "boot") */
    };

    /** Gender for formant frequency selection */
    enum class Gender
    {
        Male,    /**< Male vocal tract characteristics */
        Female,  /**< Female vocal tract characteristics */
        Child    /**< Child vocal tract characteristics */
    };

    /** Formant data structure */
    struct FormantData
    {
        CoeffType frequency;  /**< Formant center frequency in Hz */
        CoeffType amplitude;  /**< Formant amplitude (0-1) */
        CoeffType bandwidth;  /**< Formant bandwidth in Hz */
    };

    //==============================================================================
    /** Constructor with optional parameters */
    explicit VowelFilter (Vowel vowel = Vowel::A, 
                         Gender gender = Gender::Male,
                         int numFormants = 3)
        : currentVowel (vowel), voiceGender (gender), formantCount (numFormants)
    {
        // Initialize formant filters
        for (int i = 0; i < maxFormants; ++i)
            formantFilters[i] = std::make_unique<Biquad<SampleType>>();
        
        setParameters (vowel, gender, numFormants);
    }

    //==============================================================================
    /** @internal */
    void reset() noexcept override
    {
        for (int i = 0; i < formantCount; ++i)
            formantFilters[i]->reset();
    }

    /** @internal */
    void prepare (double sampleRate, int maximumBlockSize) noexcept override
    {
        this->sampleRate = sampleRate;
        this->maximumBlockSize = maximumBlockSize;
        
        for (int i = 0; i < formantCount; ++i)
            formantFilters[i]->prepare (sampleRate, maximumBlockSize);
        
        updateCoefficients();
    }

    /** @internal */
    SampleType processSample (SampleType inputSample) noexcept override
    {
        CoeffType output = static_cast<CoeffType> (0.0);
        
        // Process through each formant filter
        for (int i = 0; i < formantCount; ++i)
        {
            const auto formantOutput = formantFilters[i]->processSample (inputSample);
            output += static_cast<CoeffType> (formantOutput) * currentFormants[i].amplitude;
        }
        
        return static_cast<SampleType> (output * outputGain);
    }

    /** @internal */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        // Clear output buffer
        std::fill (outputBuffer, outputBuffer + numSamples, static_cast<SampleType> (0.0));
        
        // Process each formant and accumulate
        for (int i = 0; i < formantCount; ++i)
        {
            // Use temporary buffer for this formant
            std::vector<SampleType> tempBuffer (numSamples);
            formantFilters[i]->processBlock (inputBuffer, tempBuffer.data(), numSamples);
            
            // Add to output with formant amplitude scaling
            const auto amplitude = currentFormants[i].amplitude * outputGain;
            for (int j = 0; j < numSamples; ++j)
            {
                outputBuffer[j] += tempBuffer[j] * static_cast<SampleType> (amplitude);
            }
        }
    }

    /** @internal */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept override
    {
        DspMath::Complex<CoeffType> totalResponse (0.0, 0.0);
        
        // Sum responses from all formant filters
        for (int i = 0; i < formantCount; ++i)
        {
            const auto formantResponse = formantFilters[i]->getComplexResponse (frequency);
            totalResponse = totalResponse + formantResponse * currentFormants[i].amplitude;
        }
        
        return totalResponse;
    }

    //==============================================================================
    /** 
        Sets the vowel parameters.
        
        @param vowel       The target vowel sound
        @param gender      The gender for formant frequencies
        @param numFormants The number of formants to use (2-5)
    */
    void setParameters (Vowel vowel, Gender gender, int numFormants) noexcept
    {
        currentVowel = vowel;
        voiceGender = gender;
        formantCount = jlimit (2, maxFormants, numFormants);
        
        loadVowelFormants (vowel, gender);
        updateCoefficients();
    }

    /** 
        Sets just the vowel type.
        
        @param vowel  The target vowel sound
    */
    void setVowel (Vowel vowel) noexcept
    {
        if (currentVowel != vowel)
        {
            currentVowel = vowel;
            loadVowelFormants (vowel, voiceGender);
            updateCoefficients();
        }
    }

    /** 
        Sets the gender for formant frequencies.
        
        @param gender  The gender type
    */
    void setGender (Gender gender) noexcept
    {
        if (voiceGender != gender)
        {
            voiceGender = gender;
            loadVowelFormants (currentVowel, gender);
            updateCoefficients();
        }
    }

    /** 
        Sets the number of active formants.
        
        @param numFormants  Number of formants (2-5)
    */
    void setFormantCount (int numFormants) noexcept
    {
        const auto newCount = jlimit (2, maxFormants, numFormants);
        if (formantCount != newCount)
        {
            formantCount = newCount;
            updateCoefficients();
        }
    }

    /** 
        Morphs between two vowel sounds.
        
        @param vowelA  The first vowel
        @param vowelB  The second vowel
        @param morph   Morph amount (0.0 = vowelA, 1.0 = vowelB)
    */
    void morphVowels (Vowel vowelA, Vowel vowelB, CoeffType morph) noexcept
    {
        morph = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (1.0), morph);
        
        std::array<FormantData, maxFormants> formantsA, formantsB;
        loadVowelFormantsInto (vowelA, voiceGender, formantsA);
        loadVowelFormantsInto (vowelB, voiceGender, formantsB);
        
        // Interpolate between formants
        for (int i = 0; i < formantCount; ++i)
        {
            currentFormants[i].frequency = formantsA[i].frequency * (static_cast<CoeffType> (1.0) - morph) + 
                                         formantsB[i].frequency * morph;
            currentFormants[i].amplitude = formantsA[i].amplitude * (static_cast<CoeffType> (1.0) - morph) + 
                                         formantsB[i].amplitude * morph;
            currentFormants[i].bandwidth = formantsA[i].bandwidth * (static_cast<CoeffType> (1.0) - morph) + 
                                         formantsB[i].bandwidth * morph;
        }
        
        updateCoefficients();
    }

    /** 
        Sets formant strength multiplier.
        
        @param strength  The formant strength (0.0 to 2.0)
    */
    void setFormantStrength (CoeffType strength) noexcept
    {
        formantStrength = jlimit (static_cast<CoeffType> (0.0), static_cast<CoeffType> (2.0), strength);
        
        // Update amplitudes
        for (int i = 0; i < formantCount; ++i)
        {
            // Preserve relative formant amplitudes while scaling overall strength
            const auto baseAmplitude = getBaseFormantAmplitude (currentVowel, voiceGender, i);
            currentFormants[i].amplitude = baseAmplitude * formantStrength;
        }
        
        updateCoefficients();
    }

    //==============================================================================
    /** Gets the current vowel */
    Vowel getVowel() const noexcept { return currentVowel; }
    
    /** Gets the current gender */
    Gender getGender() const noexcept { return voiceGender; }
    
    /** Gets the number of active formants */
    int getFormantCount() const noexcept { return formantCount; }
    
    /** Gets the formant strength */
    CoeffType getFormantStrength() const noexcept { return formantStrength; }

    /** 
        Gets formant data for a specific formant.
        
        @param formantIndex  The formant index (0-4)
        @returns            The formant data
    */
    FormantData getFormantData (int formantIndex) const noexcept
    {
        if (formantIndex >= 0 && formantIndex < formantCount)
            return currentFormants[formantIndex];
        
        return FormantData { 0, 0, 0 };
    }

private:
    //==============================================================================
    static constexpr int maxFormants = 5;
    
    Vowel currentVowel = Vowel::A;
    Gender voiceGender = Gender::Male;
    int formantCount = 3;
    CoeffType formantStrength = static_cast<CoeffType> (1.0);
    CoeffType outputGain = static_cast<CoeffType> (0.5);
    
    std::array<std::unique_ptr<Biquad<SampleType>>, maxFormants> formantFilters;
    std::array<FormantData, maxFormants> currentFormants;

    //==============================================================================
    /** Loads vowel formant data */
    void loadVowelFormants (Vowel vowel, Gender gender) noexcept
    {
        loadVowelFormantsInto (vowel, gender, currentFormants);
    }
    
    /** Loads vowel formant data into specified array */
    void loadVowelFormantsInto (Vowel vowel, Gender gender, std::array<FormantData, maxFormants>& formants) const noexcept
    {
        // Formant frequencies and amplitudes based on research data
        // Values are approximate and may vary between individuals
        
        switch (vowel)
        {
            case Vowel::A: // "father"
                if (gender == Gender::Male)
                {
                    formants[0] = { 730,  1.0,  60 };   // F1
                    formants[1] = { 1090, 0.7,  70 };   // F2
                    formants[2] = { 2440, 0.4,  110 };  // F3
                    formants[3] = { 3200, 0.2,  120 };  // F4
                    formants[4] = { 4000, 0.1,  130 };  // F5
                }
                else if (gender == Gender::Female)
                {
                    formants[0] = { 850,  1.0,  60 };
                    formants[1] = { 1220, 0.7,  70 };
                    formants[2] = { 2810, 0.4,  110 };
                    formants[3] = { 3800, 0.2,  120 };
                    formants[4] = { 4950, 0.1,  130 };
                }
                else // Child
                {
                    formants[0] = { 1030, 1.0,  60 };
                    formants[1] = { 1370, 0.7,  70 };
                    formants[2] = { 3170, 0.4,  110 };
                    formants[3] = { 4500, 0.2,  120 };
                    formants[4] = { 5500, 0.1,  130 };
                }
                break;
                
            case Vowel::E: // "bed"
                if (gender == Gender::Male)
                {
                    formants[0] = { 530,  1.0,  60 };
                    formants[1] = { 1840, 0.8,  80 };
                    formants[2] = { 2480, 0.4,  100 };
                    formants[3] = { 3500, 0.2,  120 };
                    formants[4] = { 4200, 0.1,  130 };
                }
                else if (gender == Gender::Female)
                {
                    formants[0] = { 610,  1.0,  60 };
                    formants[1] = { 2330, 0.8,  80 };
                    formants[2] = { 2990, 0.4,  100 };
                    formants[3] = { 4000, 0.2,  120 };
                    formants[4] = { 5100, 0.1,  130 };
                }
                else // Child
                {
                    formants[0] = { 690,  1.0,  60 };
                    formants[1] = { 2610, 0.8,  80 };
                    formants[2] = { 3570, 0.4,  100 };
                    formants[3] = { 4500, 0.2,  120 };
                    formants[4] = { 5500, 0.1,  130 };
                }
                break;
                
            case Vowel::I: // "see"
                if (gender == Gender::Male)
                {
                    formants[0] = { 270,  1.0,  40 };
                    formants[1] = { 2290, 0.9,  90 };
                    formants[2] = { 3010, 0.3,  100 };
                    formants[3] = { 3500, 0.2,  120 };
                    formants[4] = { 4200, 0.1,  130 };
                }
                else if (gender == Gender::Female)
                {
                    formants[0] = { 310,  1.0,  40 };
                    formants[1] = { 2790, 0.9,  90 };
                    formants[2] = { 3310, 0.3,  100 };
                    formants[3] = { 4000, 0.2,  120 };
                    formants[4] = { 5100, 0.1,  130 };
                }
                else // Child
                {
                    formants[0] = { 370,  1.0,  40 };
                    formants[1] = { 3200, 0.9,  90 };
                    formants[2] = { 3730, 0.3,  100 };
                    formants[3] = { 4500, 0.2,  120 };
                    formants[4] = { 5500, 0.1,  130 };
                }
                break;
                
            case Vowel::O: // "law"
                if (gender == Gender::Male)
                {
                    formants[0] = { 570,  1.0,  70 };
                    formants[1] = { 840,  0.6,  80 };
                    formants[2] = { 2410, 0.4,  100 };
                    formants[3] = { 3200, 0.2,  120 };
                    formants[4] = { 4000, 0.1,  130 };
                }
                else if (gender == Gender::Female)
                {
                    formants[0] = { 590,  1.0,  70 };
                    formants[1] = { 920,  0.6,  80 };
                    formants[2] = { 2710, 0.4,  100 };
                    formants[3] = { 3800, 0.2,  120 };
                    formants[4] = { 4950, 0.1,  130 };
                }
                else // Child
                {
                    formants[0] = { 680,  1.0,  70 };
                    formants[1] = { 1060, 0.6,  80 };
                    formants[2] = { 3180, 0.4,  100 };
                    formants[3] = { 4500, 0.2,  120 };
                    formants[4] = { 5500, 0.1,  130 };
                }
                break;
                
            case Vowel::U: // "boot"
                if (gender == Gender::Male)
                {
                    formants[0] = { 300,  1.0,  50 };
                    formants[1] = { 870,  0.5,  70 };
                    formants[2] = { 2240, 0.3,  100 };
                    formants[3] = { 3200, 0.2,  120 };
                    formants[4] = { 4000, 0.1,  130 };
                }
                else if (gender == Gender::Female)
                {
                    formants[0] = { 370,  1.0,  50 };
                    formants[1] = { 950,  0.5,  70 };
                    formants[2] = { 2670, 0.3,  100 };
                    formants[3] = { 3800, 0.2,  120 };
                    formants[4] = { 4950, 0.1,  130 };
                }
                else // Child
                {
                    formants[0] = { 430,  1.0,  50 };
                    formants[1] = { 1170, 0.5,  70 };
                    formants[2] = { 3260, 0.3,  100 };
                    formants[3] = { 4500, 0.2,  120 };
                    formants[4] = { 5500, 0.1,  130 };
                }
                break;
        }
    }
    
    /** Gets base formant amplitude for a specific vowel/gender/formant combination */
    CoeffType getBaseFormantAmplitude (Vowel vowel, Gender gender, int formantIndex) const noexcept
    {
        std::array<FormantData, maxFormants> tempFormants;
        loadVowelFormantsInto (vowel, gender, tempFormants);
        
        if (formantIndex >= 0 && formantIndex < maxFormants)
            return tempFormants[formantIndex].amplitude;
        
        return static_cast<CoeffType> (0.0);
    }

    /** Updates all formant filter coefficients */
    void updateCoefficients() noexcept
    {
        for (int i = 0; i < formantCount; ++i)
        {
            const auto& formant = currentFormants[i];
            const auto q = formant.frequency / jmax (formant.bandwidth, static_cast<CoeffType> (10.0));
            
            // Design bandpass filter for this formant
            const auto coeffs = FilterDesigner<CoeffType>::designRbjBandpass (formant.frequency, q, this->sampleRate);
            
            formantFilters[i]->setCoefficients (coeffs);
        }
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VowelFilter)
};

//==============================================================================
/** Type aliases for convenience */
using VowelFilterFloat = VowelFilter<float>;       // float samples, double coefficients (default)
using VowelFilterDouble = VowelFilter<double>;     // double samples, double coefficients (default)

} // namespace yup
