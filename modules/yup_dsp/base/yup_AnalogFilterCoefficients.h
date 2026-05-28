/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
    Coefficients for the saturator used by analog-model filters.

    The coefficients describe a symmetric atan waveshaper. A drive of zero keeps
    the processor transparent.
*/
template <typename CoeffType = double>
struct AnalogSaturatorCoefficients
{
    CoeffType drive = static_cast<CoeffType> (0);
    CoeffType preScale = static_cast<CoeffType> (1);
    CoeffType postScale = static_cast<CoeffType> (1);
};

//==============================================================================
/**
    Coefficients for a trapezoidal-integrator two-pole state-variable filter.

    The output gains select low-pass, high-pass, band-pass, peak, or notch
    responses without changing the integrator state update.
*/
template <typename CoeffType = double>
struct AnalogTwoPoleCoefficients
{
    CoeffType g = static_cast<CoeffType> (0);
    CoeffType h = static_cast<CoeffType> (1);
    CoeffType r2 = static_cast<CoeffType> (0);
    CoeffType gainCorrection = static_cast<CoeffType> (1);
    CoeffType lowOut = static_cast<CoeffType> (1);
    CoeffType bandOut = static_cast<CoeffType> (0);
    CoeffType highOut = static_cast<CoeffType> (0);
};

//==============================================================================
/**
    Coefficients for one trapezoidal one-pole section used inside ladder models.
*/
template <typename CoeffType = double>
struct AnalogOnePoleCoefficients
{
    CoeffType alpha = static_cast<CoeffType> (0);
    CoeffType beta = static_cast<CoeffType> (1);
};

//==============================================================================
/**
    Coefficients for a Korg35-inspired analog-model filter.
*/
template <typename CoeffType = double>
struct AnalogKorg35Coefficients
{
    std::array<AnalogOnePoleCoefficients<CoeffType>, 3> poles;
    CoeffType alpha0 = static_cast<CoeffType> (1);
    CoeffType feedback = static_cast<CoeffType> (0);
    CoeffType gainCorrection = static_cast<CoeffType> (1);
};

//==============================================================================
/**
    Moog ladder output mode.
*/
enum class AnalogMoogLadderMode
{
    lowpass24,
    highpass24,
    lowpass18,
    highpass18,
    lowpass12,
    highpass12,
    lowpass6,
    highpass6,
    bandpass12,
    bandpass6
};

//==============================================================================
/**
    Coefficients for a four-pole Moog ladder-style analog-model filter.
*/
template <typename CoeffType = double>
struct AnalogMoogLadderCoefficients
{
    std::array<AnalogOnePoleCoefficients<CoeffType>, 4> poles;
    std::array<CoeffType, 5> outputs {
        static_cast<CoeffType> (0),
        static_cast<CoeffType> (0),
        static_cast<CoeffType> (0),
        static_cast<CoeffType> (0),
        static_cast<CoeffType> (1)
    };
    CoeffType alpha0 = static_cast<CoeffType> (1);
    CoeffType feedback = static_cast<CoeffType> (0);
    CoeffType gainCorrection = static_cast<CoeffType> (1);
};

//==============================================================================
/**
    Coefficients for a Roland diode-ladder-inspired low-pass filter.
*/
template <typename CoeffType = double>
struct AnalogRolandDiodeCoefficients
{
    CoeffType cutoff = static_cast<CoeffType> (0);
    CoeffType feedback = static_cast<CoeffType> (0);
    CoeffType gainCorrection = static_cast<CoeffType> (1);
    CoeffType a = static_cast<CoeffType> (0);
    CoeffType a2 = static_cast<CoeffType> (0);
    CoeffType aInv = static_cast<CoeffType> (1);
    CoeffType b = static_cast<CoeffType> (1);
    CoeffType b2 = static_cast<CoeffType> (1);
    CoeffType c = static_cast<CoeffType> (1);
    CoeffType g0 = static_cast<CoeffType> (0);
    CoeffType g = static_cast<CoeffType> (0);
    CoeffType fg = static_cast<CoeffType> (1);
    CoeffType highpassA = static_cast<CoeffType> (0);
    CoeffType highpassB = static_cast<CoeffType> (1);
};

//==============================================================================
/**
    Coefficients for the vowel/formant analog-model filter.
*/
template <typename CoeffType = double>
struct AnalogVowelCoefficients
{
    std::array<AnalogTwoPoleCoefficients<CoeffType>, 3> formants;
    CoeffType gainCompensation = static_cast<CoeffType> (1);
};

} // namespace yup
