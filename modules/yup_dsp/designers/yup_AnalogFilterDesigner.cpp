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

namespace yup
{

//==============================================================================

template <typename CoeffType>
AnalogSaturatorCoefficients<CoeffType> AnalogFilterDesigner<CoeffType>::designSaturator (
    CoeffType drive) noexcept
{
    drive = sanitizeNormalized (drive);

    AnalogSaturatorCoefficients<CoeffType> coefficients;
    coefficients.drive = drive;

    if (drive > static_cast<CoeffType> (0))
    {
        coefficients.preScale = square (drive * static_cast<CoeffType> (5));
        coefficients.postScale = static_cast<CoeffType> (1) / std::atan (coefficients.preScale)
                               * std::pow (static_cast<CoeffType> (4), -drive);
    }

    return coefficients;
}

//==============================================================================

template <typename CoeffType>
AnalogTwoPoleCoefficients<CoeffType> AnalogFilterDesigner<CoeffType>::designTwoPole (
    FilterModeType mode,
    CoeffType frequency,
    double sampleRate,
    CoeffType normalizedResonance) noexcept
{
    const auto fs = sanitizeSampleRate (sampleRate);
    frequency = sanitizeFrequency (frequency, fs);
    normalizedResonance = sanitizeNormalized (normalizedResonance);

    constexpr auto maxQ = static_cast<CoeffType> (16);

    AnalogTwoPoleCoefficients<CoeffType> coefficients;
    coefficients.g = std::tan (MathConstants<CoeffType>::pi * frequency / fs);

    if (mode.test (FilterMode::bandpassCpg))
    {
        const auto k = static_cast<CoeffType> (0.075)
                     + square (static_cast<CoeffType> (1) - normalizedResonance)
                           * (maxQ / static_cast<CoeffType> (4) - static_cast<CoeffType> (0.075));
        const auto lowerFrequency = frequency * std::pow (static_cast<CoeffType> (2), -k / static_cast<CoeffType> (2));
        const auto lowerG = std::tan (MathConstants<CoeffType>::pi * lowerFrequency / fs);
        const auto ratio = lowerG / coefficients.g;
        const auto ratioTerm = (static_cast<CoeffType> (1) - ratio * ratio)
                             * (static_cast<CoeffType> (1) - ratio * ratio)
                             / (static_cast<CoeffType> (4) * ratio * ratio);

        coefficients.r2 = static_cast<CoeffType> (2) * std::sqrt (jmax (static_cast<CoeffType> (0), ratioTerm));
        coefficients.h = static_cast<CoeffType> (1)
                       / (static_cast<CoeffType> (1) + coefficients.r2 * coefficients.g + coefficients.g * coefficients.g);
        coefficients.lowOut = static_cast<CoeffType> (0);
        coefficients.bandOut = coefficients.r2;
        coefficients.highOut = static_cast<CoeffType> (0);
        coefficients.gainCorrection = static_cast<CoeffType> (1);
        return coefficients;
    }

    if (mode.test (FilterMode::bandstop))
    {
        const auto k = static_cast<CoeffType> (0.075)
                     + square (static_cast<CoeffType> (1) - normalizedResonance)
                           * (maxQ / static_cast<CoeffType> (4) - static_cast<CoeffType> (0.075));

        coefficients.r2 = static_cast<CoeffType> (1) / k;
        coefficients.h = static_cast<CoeffType> (1)
                       / (static_cast<CoeffType> (1) + coefficients.r2 * coefficients.g + coefficients.g * coefficients.g);
        coefficients.lowOut = static_cast<CoeffType> (1);
        coefficients.bandOut = static_cast<CoeffType> (0);
        coefficients.highOut = static_cast<CoeffType> (1);
        coefficients.gainCorrection = static_cast<CoeffType> (1);
        return coefficients;
    }

    if (mode.test (FilterMode::peak))
    {
        const auto k = static_cast<CoeffType> (1)
                     + square (normalizedResonance) * (static_cast<CoeffType> (2) * maxQ - static_cast<CoeffType> (1));

        coefficients.r2 = static_cast<CoeffType> (1) / k;
        coefficients.h = static_cast<CoeffType> (1)
                       / (static_cast<CoeffType> (1) + coefficients.r2 * coefficients.g + coefficients.g * coefficients.g);
        coefficients.lowOut = static_cast<CoeffType> (1);
        coefficients.bandOut = static_cast<CoeffType> (1);
        coefficients.highOut = static_cast<CoeffType> (1);
        coefficients.gainCorrection = static_cast<CoeffType> (1)
                                    / (static_cast<CoeffType> (1) + (k - static_cast<CoeffType> (1)) / static_cast<CoeffType> (8));
        return coefficients;
    }

    const auto k = static_cast<CoeffType> (0.35)
                 + square (normalizedResonance) * (maxQ - static_cast<CoeffType> (0.35));

    coefficients.r2 = static_cast<CoeffType> (1) / k;
    coefficients.h = static_cast<CoeffType> (1)
                   / (static_cast<CoeffType> (1) + coefficients.r2 * coefficients.g + coefficients.g * coefficients.g);
    coefficients.lowOut = mode.test (FilterMode::lowpass) ? static_cast<CoeffType> (1) : static_cast<CoeffType> (0);
    coefficients.bandOut = mode.test (FilterMode::bandpassCsg) ? static_cast<CoeffType> (1) : static_cast<CoeffType> (0);
    coefficients.highOut = mode.test (FilterMode::highpass) ? static_cast<CoeffType> (1) : static_cast<CoeffType> (0);

    if (mode.test (FilterMode::bandpassCsg))
        coefficients.gainCorrection = static_cast<CoeffType> (2)
                                    / (static_cast<CoeffType> (1) + (k - static_cast<CoeffType> (0.7)) / static_cast<CoeffType> (8));
    else
        coefficients.gainCorrection = static_cast<CoeffType> (1)
                                    / (static_cast<CoeffType> (1) + (k - static_cast<CoeffType> (0.7)) / static_cast<CoeffType> (8));

    return coefficients;
}

//==============================================================================

template <typename CoeffType>
AnalogVowelCoefficients<CoeffType> AnalogFilterDesigner<CoeffType>::designVowel (
    CoeffType vowel,
    double sampleRate,
    CoeffType normalizedResonance) noexcept
{
    static constexpr std::array<std::array<CoeffType, 3>, 10> vowelFrequencies { { { { static_cast<CoeffType> (570), static_cast<CoeffType> (840), static_cast<CoeffType> (2410) } },
                                                                                   { { static_cast<CoeffType> (300), static_cast<CoeffType> (870), static_cast<CoeffType> (2240) } },
                                                                                   { { static_cast<CoeffType> (440), static_cast<CoeffType> (1020), static_cast<CoeffType> (2240) } },
                                                                                   { { static_cast<CoeffType> (730), static_cast<CoeffType> (1090), static_cast<CoeffType> (2440) } },
                                                                                   { { static_cast<CoeffType> (520), static_cast<CoeffType> (1190), static_cast<CoeffType> (2390) } },
                                                                                   { { static_cast<CoeffType> (490), static_cast<CoeffType> (1350), static_cast<CoeffType> (1690) } },
                                                                                   { { static_cast<CoeffType> (660), static_cast<CoeffType> (1720), static_cast<CoeffType> (2410) } },
                                                                                   { { static_cast<CoeffType> (530), static_cast<CoeffType> (1840), static_cast<CoeffType> (2480) } },
                                                                                   { { static_cast<CoeffType> (390), static_cast<CoeffType> (1990), static_cast<CoeffType> (2550) } },
                                                                                   { { static_cast<CoeffType> (270), static_cast<CoeffType> (2290), static_cast<CoeffType> (3010) } } } };

    vowel = sanitizeNormalized (vowel);
    normalizedResonance = sanitizeNormalized (normalizedResonance);

    const auto scaledVowel = vowel * static_cast<CoeffType> (vowelFrequencies.size() - 1);
    const auto baseIndex = jmin (static_cast<int> (scaledVowel), static_cast<int> (vowelFrequencies.size() - 1));
    const auto nextIndex = jmin (baseIndex + 1, static_cast<int> (vowelFrequencies.size() - 1));
    const auto fraction = scaledVowel - static_cast<CoeffType> (baseIndex);
    const auto currentWeight = square (square (static_cast<CoeffType> (1) - fraction));
    const auto nextWeight = static_cast<CoeffType> (1) - currentWeight;

    AnalogVowelCoefficients<CoeffType> coefficients;

    for (std::size_t i = 0; i < coefficients.formants.size(); ++i)
    {
        const auto formantFrequency = vowelFrequencies[static_cast<std::size_t> (baseIndex)][i] * currentWeight
                                    + vowelFrequencies[static_cast<std::size_t> (nextIndex)][i] * nextWeight;
        coefficients.formants[i] = designTwoPole (FilterMode::peak, formantFrequency, sampleRate, normalizedResonance);
    }

    coefficients.gainCompensation = static_cast<CoeffType> (1)
                                  + square (square (normalizedResonance)) * static_cast<CoeffType> (18);

    return coefficients;
}

//==============================================================================

template <typename CoeffType>
AnalogKorg35Coefficients<CoeffType> AnalogFilterDesigner<CoeffType>::designKorg35 (
    FilterModeType mode,
    CoeffType frequency,
    double sampleRate,
    CoeffType resonance,
    CoeffType saturation) noexcept
{
    const auto fs = sanitizeSampleRate (sampleRate);
    frequency = sanitizeFrequency (frequency, fs);
    resonance = sanitizeNormalized (resonance);
    saturation = sanitizeNormalized (saturation);
    mode = resolveFilterMode (mode, FilterMode::lowpass | FilterMode::bandpassCsg | FilterMode::highpass);

    const auto t2 = static_cast<CoeffType> (0.5) / fs;
    const auto wa = static_cast<CoeffType> (2) * fs * std::tan (MathConstants<CoeffType>::twoPi * frequency * t2);
    const auto g = wa * t2;
    const auto gI = static_cast<CoeffType> (1) / (static_cast<CoeffType> (1) + g);
    const auto G = g * gI;

    AnalogKorg35Coefficients<CoeffType> coefficients;
    coefficients.poles[0].alpha = G;
    coefficients.poles[1].alpha = G;
    coefficients.poles[2].alpha = G;
    coefficients.feedback = static_cast<CoeffType> (0.01)
                          + resonance * static_cast<CoeffType> (1.99)
                          - (static_cast<CoeffType> (1) - saturation) * static_cast<CoeffType> (0.001);

    coefficients.poles[0].beta = static_cast<CoeffType> (1);

    if (mode.test (FilterMode::lowpass))
    {
        coefficients.poles[1].beta = (coefficients.feedback - coefficients.feedback * G) * gI;
        coefficients.poles[2].beta = -gI;
        coefficients.gainCorrection = (static_cast<CoeffType> (1) / coefficients.feedback)
                                    / (static_cast<CoeffType> (1) + square (resonance) * static_cast<CoeffType> (1.5));
    }
    else if (mode.test (FilterMode::bandpass))
    {
        coefficients.poles[1].beta = gI;
        coefficients.poles[2].beta = -G * gI;
        coefficients.gainCorrection = (static_cast<CoeffType> (2.2) / coefficients.feedback)
                                    / (static_cast<CoeffType> (1) + square (resonance));
    }
    else
    {
        coefficients.poles[1].beta = gI;
        coefficients.poles[2].beta = -G * gI;
        coefficients.gainCorrection = (static_cast<CoeffType> (1) / coefficients.feedback)
                                    / (static_cast<CoeffType> (1) + square (resonance));
    }

    coefficients.alpha0 = static_cast<CoeffType> (1)
                        / (static_cast<CoeffType> (1) - coefficients.feedback * G + coefficients.feedback * G * G);

    return coefficients;
}

//==============================================================================

template <typename CoeffType>
AnalogMoogLadderCoefficients<CoeffType> AnalogFilterDesigner<CoeffType>::designMoogLadder (
    AnalogMoogLadderMode mode,
    CoeffType frequency,
    double sampleRate,
    CoeffType resonance,
    CoeffType saturation) noexcept
{
    static constexpr std::array<std::array<CoeffType, 5>, 10> outputGains { { { { static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (1) } },
                                                                              { { static_cast<CoeffType> (1), static_cast<CoeffType> (-4), static_cast<CoeffType> (6), static_cast<CoeffType> (-4), static_cast<CoeffType> (1) } },
                                                                              { { static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (1), static_cast<CoeffType> (0) } },
                                                                              { { static_cast<CoeffType> (1), static_cast<CoeffType> (-3), static_cast<CoeffType> (3), static_cast<CoeffType> (-1), static_cast<CoeffType> (0) } },
                                                                              { { static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (1), static_cast<CoeffType> (0), static_cast<CoeffType> (0) } },
                                                                              { { static_cast<CoeffType> (1), static_cast<CoeffType> (-2), static_cast<CoeffType> (1), static_cast<CoeffType> (0), static_cast<CoeffType> (0) } },
                                                                              { { static_cast<CoeffType> (0), static_cast<CoeffType> (1), static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (0) } },
                                                                              { { static_cast<CoeffType> (1), static_cast<CoeffType> (-1), static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (0) } },
                                                                              { { static_cast<CoeffType> (0), static_cast<CoeffType> (0), static_cast<CoeffType> (1), static_cast<CoeffType> (-2), static_cast<CoeffType> (1) } },
                                                                              { { static_cast<CoeffType> (0), static_cast<CoeffType> (1), static_cast<CoeffType> (-1), static_cast<CoeffType> (0), static_cast<CoeffType> (0) } } } };

    const auto fs = sanitizeSampleRate (sampleRate);
    frequency = sanitizeFrequency (frequency, fs);
    resonance = sanitizeNormalized (resonance);
    saturation = sanitizeNormalized (saturation);

    const auto t2 = static_cast<CoeffType> (0.5) / fs;
    const auto wa = static_cast<CoeffType> (2) * fs * std::tan (MathConstants<CoeffType>::twoPi * frequency * t2);
    const auto g = wa * t2;
    const auto gI = static_cast<CoeffType> (1) / (static_cast<CoeffType> (1) + g);
    const auto G = g * gI;

    AnalogMoogLadderCoefficients<CoeffType> coefficients;

    for (auto& pole : coefficients.poles)
        pole.alpha = G;

    coefficients.poles[0].beta = G * G * G * gI;
    coefficients.poles[1].beta = G * G * gI;
    coefficients.poles[2].beta = G * gI;
    coefficients.poles[3].beta = gI;

    coefficients.outputs = outputGains[static_cast<std::size_t> (mode)];
    coefficients.feedback = resonance * (static_cast<CoeffType> (4) - static_cast<CoeffType> (0.02) * (static_cast<CoeffType> (1) - saturation * static_cast<CoeffType> (0.98)));
    coefficients.alpha0 = static_cast<CoeffType> (1)
                        / (static_cast<CoeffType> (1) + coefficients.feedback * G * G * G * G);

    switch (mode)
    {
        case AnalogMoogLadderMode::lowpass24:
        case AnalogMoogLadderMode::lowpass18:
        case AnalogMoogLadderMode::lowpass12:
        case AnalogMoogLadderMode::lowpass6:
            coefficients.gainCorrection = std::pow (static_cast<CoeffType> (1) + resonance * static_cast<CoeffType> (4), static_cast<CoeffType> (0.45));
            break;

        case AnalogMoogLadderMode::highpass24:
        case AnalogMoogLadderMode::highpass18:
        case AnalogMoogLadderMode::highpass12:
        case AnalogMoogLadderMode::highpass6:
            coefficients.gainCorrection = static_cast<CoeffType> (1) - square (resonance * static_cast<CoeffType> (0.6));
            break;

        case AnalogMoogLadderMode::bandpass12:
            coefficients.gainCorrection = static_cast<CoeffType> (4);
            break;

        case AnalogMoogLadderMode::bandpass6:
            coefficients.gainCorrection = static_cast<CoeffType> (2);
            break;
    }

    return coefficients;
}

//==============================================================================

template <typename CoeffType>
AnalogRolandDiodeCoefficients<CoeffType> AnalogFilterDesigner<CoeffType>::designRolandDiode (
    CoeffType frequency,
    double sampleRate,
    CoeffType resonance,
    CoeffType saturation) noexcept
{
    const auto fs = sanitizeSampleRate (sampleRate);
    frequency = sanitizeFrequency (frequency, fs);
    resonance = sanitizeNormalized (resonance);
    saturation = sanitizeNormalized (saturation);

    AnalogRolandDiodeCoefficients<CoeffType> coefficients;
    coefficients.cutoff = frequency / (static_cast<CoeffType> (2) * fs) * MathConstants<CoeffType>::sqrt2;
    coefficients.feedback = resonance * (static_cast<CoeffType> (17.1) - static_cast<CoeffType> (0.1) * (static_cast<CoeffType> (1) - saturation * static_cast<CoeffType> (0.99)));
    coefficients.gainCorrection = static_cast<CoeffType> (1.05) + static_cast<CoeffType> (0.8) * coefficients.feedback - square (resonance) * static_cast<CoeffType> (7);

    const auto highpassFc = static_cast<CoeffType> (50) / (static_cast<CoeffType> (2) * fs);
    const auto kh = highpassFc * MathConstants<CoeffType>::pi;
    const auto khp2Inv = static_cast<CoeffType> (1) / (kh + static_cast<CoeffType> (2));
    coefficients.highpassA = (kh - static_cast<CoeffType> (2)) * khp2Inv;
    coefficients.highpassB = static_cast<CoeffType> (2) * khp2Inv;

    coefficients.a = MathConstants<CoeffType>::pi * coefficients.cutoff;
    coefficients.a2 = coefficients.a * coefficients.a;
    coefficients.aInv = static_cast<CoeffType> (1) / coefficients.a;
    coefficients.b = static_cast<CoeffType> (2) * coefficients.a + static_cast<CoeffType> (1);
    coefficients.b2 = coefficients.b * coefficients.b;
    coefficients.c = static_cast<CoeffType> (1)
                   / (static_cast<CoeffType> (2) * coefficients.a2 * coefficients.a2
                      - static_cast<CoeffType> (4) * coefficients.a2 * coefficients.b2
                      + coefficients.b2 * coefficients.b2);
    coefficients.g0 = static_cast<CoeffType> (2) * coefficients.a2 * coefficients.a2 * coefficients.c;
    coefficients.g = coefficients.g0 * coefficients.highpassB;
    coefficients.fg = static_cast<CoeffType> (1) / (static_cast<CoeffType> (1) + coefficients.g * coefficients.feedback);

    return coefficients;
}

//==============================================================================

template <typename CoeffType>
CoeffType AnalogFilterDesigner<CoeffType>::sanitizeFrequency (CoeffType frequency, double sampleRate) noexcept
{
    const auto fs = sanitizeSampleRate (sampleRate);
    const auto maxFrequency = jmax (static_cast<CoeffType> (20), fs * static_cast<CoeffType> (0.5) / (static_cast<CoeffType> (22050) / static_cast<CoeffType> (18000)));

    if (! std::isfinite (frequency))
        return static_cast<CoeffType> (20);

    return jlimit (static_cast<CoeffType> (20), maxFrequency, frequency);
}

template <typename CoeffType>
CoeffType AnalogFilterDesigner<CoeffType>::sanitizeSampleRate (double sampleRate) noexcept
{
    if (! std::isfinite (sampleRate))
        return static_cast<CoeffType> (44100);

    return jmax (static_cast<CoeffType> (11025), static_cast<CoeffType> (sampleRate));
}

template <typename CoeffType>
CoeffType AnalogFilterDesigner<CoeffType>::sanitizeNormalized (CoeffType value) noexcept
{
    if (! std::isfinite (value))
        return static_cast<CoeffType> (0);

    return jlimit (static_cast<CoeffType> (0), static_cast<CoeffType> (1), value);
}

//==============================================================================

template class AnalogFilterDesigner<float>;
template class AnalogFilterDesigner<double>;

} // namespace yup
