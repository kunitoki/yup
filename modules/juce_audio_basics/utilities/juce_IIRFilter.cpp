/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

constexpr auto minimumDecibels = -300.0f;

IIRCoefficients::IIRCoefficients() noexcept
{
    zeromem (coefficients, sizeof (coefficients));
}

IIRCoefficients::~IIRCoefficients() noexcept {}

IIRCoefficients::IIRCoefficients (const IIRCoefficients& other) noexcept
{
    memcpy (coefficients, other.coefficients, sizeof (coefficients));
}

IIRCoefficients& IIRCoefficients::operator= (const IIRCoefficients& other) noexcept
{
    memcpy (coefficients, other.coefficients, sizeof (coefficients));
    return *this;
}

IIRCoefficients::IIRCoefficients (double c1, double c2, double c3, double c4, double c5, double c6) noexcept
{
    const auto a = 1.0 / c4;

    coefficients[0] = (float) (c1 * a);
    coefficients[1] = (float) (c2 * a);
    coefficients[2] = (float) (c3 * a);
    coefficients[3] = (float) (c5 * a);
    coefficients[4] = (float) (c6 * a);
}

IIRCoefficients IIRCoefficients::makeLowPass (double sampleRate,
                                              double frequency) noexcept
{
    return makeLowPass (sampleRate, frequency, 1.0 / MathConstants<double>::sqrt2);
}

IIRCoefficients IIRCoefficients::makeLowPass (double sampleRate,
                                              double frequency,
                                              double Q) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (frequency > 0.0 && frequency <= sampleRate * 0.5);
    jassert (Q > 0.0);

    const auto n = 1.0 / std::tan (MathConstants<double>::pi * frequency / sampleRate);
    const auto nSquared = n * n;
    const auto c1 = 1.0 / (1.0 + 1.0 / Q * n + nSquared);

    return IIRCoefficients (c1,
                            c1 * 2.0,
                            c1,
                            1.0,
                            c1 * 2.0 * (1.0 - nSquared),
                            c1 * (1.0 - 1.0 / Q * n + nSquared));
}

IIRCoefficients IIRCoefficients::makeHighPass (double sampleRate,
                                               double frequency) noexcept
{
    return makeHighPass (sampleRate, frequency, 1.0 / std::sqrt (2.0));
}

IIRCoefficients IIRCoefficients::makeHighPass (double sampleRate,
                                               double frequency,
                                               double Q) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (frequency > 0.0 && frequency <= sampleRate * 0.5);
    jassert (Q > 0.0);

    const auto n = std::tan (MathConstants<double>::pi * frequency / sampleRate);
    const auto nSquared = n * n;
    const auto c1 = 1.0 / (1.0 + 1.0 / Q * n + nSquared);

    return IIRCoefficients (c1,
                            c1 * -2.0,
                            c1,
                            1.0,
                            c1 * 2.0 * (nSquared - 1.0),
                            c1 * (1.0 - 1.0 / Q * n + nSquared));
}

IIRCoefficients IIRCoefficients::makeBandPass (double sampleRate,
                                               double frequency) noexcept
{
    return makeBandPass (sampleRate, frequency, 1.0 / MathConstants<double>::sqrt2);
}

IIRCoefficients IIRCoefficients::makeBandPass (double sampleRate,
                                               double frequency,
                                               double Q) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (frequency > 0.0 && frequency <= sampleRate * 0.5);
    jassert (Q > 0.0);

    const auto n = 1.0 / std::tan (MathConstants<double>::pi * frequency / sampleRate);
    const auto nSquared = n * n;
    const auto c1 = 1.0 / (1.0 + 1.0 / Q * n + nSquared);

    return IIRCoefficients (c1 * n / Q,
                            0.0,
                            -c1 * n / Q,
                            1.0,
                            c1 * 2.0 * (1.0 - nSquared),
                            c1 * (1.0 - 1.0 / Q * n + nSquared));
}

IIRCoefficients IIRCoefficients::makeNotchFilter (double sampleRate,
                                                  double frequency) noexcept
{
    return makeNotchFilter (sampleRate, frequency, 1.0 / MathConstants<double>::sqrt2);
}

IIRCoefficients IIRCoefficients::makeNotchFilter (double sampleRate,
                                                  double frequency,
                                                  double Q) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (frequency > 0.0 && frequency <= sampleRate * 0.5);
    jassert (Q > 0.0);

    const auto n = 1.0 / std::tan (MathConstants<double>::pi * frequency / sampleRate);
    const auto nSquared = n * n;
    const auto c1 = 1.0 / (1.0 + n / Q + nSquared);

    return IIRCoefficients (c1 * (1.0 + nSquared),
                            2.0 * c1 * (1.0 - nSquared),
                            c1 * (1.0 + nSquared),
                            1.0,
                            c1 * 2.0 * (1.0 - nSquared),
                            c1 * (1.0 - n / Q + nSquared));
}

IIRCoefficients IIRCoefficients::makeAllPass (double sampleRate,
                                              double frequency) noexcept
{
    return makeAllPass (sampleRate, frequency, 1.0 / MathConstants<double>::sqrt2);
}

IIRCoefficients IIRCoefficients::makeAllPass (double sampleRate,
                                              double frequency,
                                              double Q) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (frequency > 0.0 && frequency <= sampleRate * 0.5);
    jassert (Q > 0.0);

    const auto n = 1.0 / std::tan (MathConstants<double>::pi * frequency / sampleRate);
    const auto nSquared = n * n;
    const auto c1 = 1.0 / (1.0 + 1.0 / Q * n + nSquared);

    return IIRCoefficients (c1 * (1.0 - n / Q + nSquared),
                            c1 * 2.0 * (1.0 - nSquared),
                            1.0,
                            1.0,
                            c1 * 2.0 * (1.0 - nSquared),
                            c1 * (1.0 - n / Q + nSquared));
}

IIRCoefficients IIRCoefficients::makeLowShelf (double sampleRate,
                                               double cutOffFrequency,
                                               double Q,
                                               float gainFactor) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (cutOffFrequency > 0.0 && cutOffFrequency <= sampleRate * 0.5);
    jassert (Q > 0.0);

    const auto A = std::sqrt (Decibels::gainWithLowerBound (gainFactor, minimumDecibels));
    const auto aminus1 = A - 1.0;
    const auto aplus1 = A + 1.0;
    const auto omega = (MathConstants<double>::twoPi * jmax (cutOffFrequency, 2.0)) / sampleRate;
    const auto coso = std::cos (omega);
    const auto beta = std::sin (omega) * std::sqrt (A) / Q;
    const auto aminus1TimesCoso = aminus1 * coso;

    return IIRCoefficients (A * (aplus1 - aminus1TimesCoso + beta),
                            A * 2.0 * (aminus1 - aplus1 * coso),
                            A * (aplus1 - aminus1TimesCoso - beta),
                            aplus1 + aminus1TimesCoso + beta,
                            -2.0 * (aminus1 + aplus1 * coso),
                            aplus1 + aminus1TimesCoso - beta);
}

IIRCoefficients IIRCoefficients::makeHighShelf (double sampleRate,
                                                double cutOffFrequency,
                                                double Q,
                                                float gainFactor) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (cutOffFrequency > 0.0 && cutOffFrequency <= sampleRate * 0.5);
    jassert (Q > 0.0);

    const auto A = std::sqrt (Decibels::gainWithLowerBound (gainFactor, minimumDecibels));
    const auto aminus1 = A - 1.0;
    const auto aplus1 = A + 1.0;
    const auto omega = (MathConstants<double>::twoPi * jmax (cutOffFrequency, 2.0)) / sampleRate;
    const auto coso = std::cos (omega);
    const auto beta = std::sin (omega) * std::sqrt (A) / Q;
    const auto aminus1TimesCoso = aminus1 * coso;

    return IIRCoefficients (A * (aplus1 + aminus1TimesCoso + beta),
                            A * -2.0 * (aminus1 + aplus1 * coso),
                            A * (aplus1 + aminus1TimesCoso - beta),
                            aplus1 - aminus1TimesCoso + beta,
                            2.0 * (aminus1 - aplus1 * coso),
                            aplus1 - aminus1TimesCoso - beta);
}

IIRCoefficients IIRCoefficients::makePeakFilter (double sampleRate,
                                                 double frequency,
                                                 double Q,
                                                 float gainFactor) noexcept
{
    jassert (sampleRate > 0.0);
    jassert (frequency > 0.0 && frequency <= sampleRate * 0.5);
    jassert (Q > 0.0);

    const auto A = std::sqrt (Decibels::gainWithLowerBound (gainFactor, minimumDecibels));
    const auto omega = (MathConstants<double>::twoPi * jmax (frequency, 2.0)) / sampleRate;
    const auto alpha = 0.5 * std::sin (omega) / Q;
    const auto c2 = -2.0 * std::cos (omega);
    const auto alphaTimesA = alpha * A;
    const auto alphaOverA = alpha / A;

    return IIRCoefficients (1.0 + alphaTimesA,
                            c2,
                            1.0 - alphaTimesA,
                            1.0 + alphaOverA,
                            c2,
                            1.0 - alphaOverA);
}

//==============================================================================
template <typename Mutex>
IIRFilterBase<Mutex>::IIRFilterBase() noexcept = default;

template <typename Mutex>
IIRFilterBase<Mutex>::IIRFilterBase (const IIRFilterBase& other) noexcept
    : active (other.active)
{
    const typename Mutex::ScopedLockType sl (other.processLock);
    coefficients = other.coefficients;
}

//==============================================================================
template <typename Mutex>
void IIRFilterBase<Mutex>::makeInactive() noexcept
{
    const typename Mutex::ScopedLockType sl (processLock);
    active = false;
}

template <typename Mutex>
void IIRFilterBase<Mutex>::setCoefficients (const IIRCoefficients& newCoefficients) noexcept
{
    const typename Mutex::ScopedLockType sl (processLock);
    coefficients = newCoefficients;
    active = true;
}

//==============================================================================
template <typename Mutex>
void IIRFilterBase<Mutex>::reset() noexcept
{
    const typename Mutex::ScopedLockType sl (processLock);
    v1 = v2 = 0.0;
}

template <typename Mutex>
float IIRFilterBase<Mutex>::processSingleSampleRaw (float in) noexcept
{
    auto out = coefficients.coefficients[0] * in + v1;

    JUCE_SNAP_TO_ZERO (out);

    v1 = coefficients.coefficients[1] * in - coefficients.coefficients[3] * out + v2;
    v2 = coefficients.coefficients[2] * in - coefficients.coefficients[4] * out;

    return out;
}

template <typename Mutex>
void IIRFilterBase<Mutex>::processSamples (float* const samples, const int numSamples) noexcept
{
    const typename Mutex::ScopedLockType sl (processLock);

    if (active)
    {
        auto c0 = coefficients.coefficients[0];
        auto c1 = coefficients.coefficients[1];
        auto c2 = coefficients.coefficients[2];
        auto c3 = coefficients.coefficients[3];
        auto c4 = coefficients.coefficients[4];
        auto lv1 = v1, lv2 = v2;

        for (int i = 0; i < numSamples; ++i)
        {
            auto in = samples[i];
            auto out = c0 * in + lv1;
            samples[i] = out;

            lv1 = c1 * in - c3 * out + lv2;
            lv2 = c2 * in - c4 * out;
        }

        JUCE_SNAP_TO_ZERO (lv1);
        v1 = lv1;
        JUCE_SNAP_TO_ZERO (lv2);
        v2 = lv2;
    }
}

template class IIRFilterBase<SpinLock>;
template class IIRFilterBase<DummyCriticalSection>;

} // namespace juce
