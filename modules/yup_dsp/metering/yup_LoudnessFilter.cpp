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
LoudnessFilter::LoudnessFilter()
{
    updateCoefficients();
}

LoudnessFilter::~LoudnessFilter()
{
}

//==============================================================================
void LoudnessFilter::prepare (double newSampleRate, int maxBlockSize)
{
    ignoreUnused (maxBlockSize);

    if (newSampleRate > 0.0)
    {
        if (newSampleRate != sampleRate)
        {
            sampleRate = newSampleRate;
            updateCoefficients();
        }
    }

    reset();
}

void LoudnessFilter::reset() noexcept
{
    preFilter.reset();
    highpassFilter.reset();
}

//==============================================================================
float LoudnessFilter::processSample (float sample) noexcept
{
    // Cascade: pre-filter -> highpass
    float output = preFilter.processSample (sample);
    output = highpassFilter.processSample (output);
    return output;
}

void LoudnessFilter::processBlock (float* samples, int numSamples) noexcept
{
    jassert (samples != nullptr);
    jassert (numSamples >= 0);

    // Process through filter cascade
    for (int i = 0; i < numSamples; ++i)
        samples[i] = processSample (samples[i]);
}

//==============================================================================
void LoudnessFilter::calculatePreFilterCoefficients (double sampleRate,
                                                     double& b0,
                                                     double& b1,
                                                     double& b2,
                                                     double& a0,
                                                     double& a1,
                                                     double& a2)
{
    // ITU-R BS.1770-4 Stage 1: High-shelf filter
    // Center frequency: 1681 Hz
    // Gain: +4 dB above fc
    //
    // This approximates the head-related transfer function, emphasizing
    // high frequencies similar to how the ear perceives loudness.

    constexpr double fc = 1681.0;                // Center frequency (Hz)
    constexpr double gainDb = 3.999843853973347; // Exact gain from ITU spec
    constexpr double Q = 0.7071752369554193;     // Q factor from ITU spec

    const double K = std::tan (MathConstants<double>::pi * fc / sampleRate);
    const double V = std::pow (10.0, gainDb / 20.0); // Linear gain
    const double norm = 1.0 / (1.0 + K / Q + K * K);

    // High-shelf biquad coefficients
    b0 = (V + std::sqrt (V) * K / Q + K * K) * norm;
    b1 = 2.0 * (K * K - V) * norm;
    b2 = (V - std::sqrt (V) * K / Q + K * K) * norm;
    a0 = 1.0;
    a1 = 2.0 * (K * K - 1.0) * norm;
    a2 = (1.0 - K / Q + K * K) * norm;
}

void LoudnessFilter::calculateHighpassCoefficients (double sampleRate,
                                                    double& b0,
                                                    double& b1,
                                                    double& b2,
                                                    double& a0,
                                                    double& a1,
                                                    double& a2)
{
    // ITU-R BS.1770-4 Stage 2: Highpass filter
    // Cutoff frequency: 38 Hz
    // Order: 2nd (Butterworth)
    // Q: 0.5 (Butterworth characteristic)
    //
    // This removes DC offset and low-frequency rumble that should not
    // contribute to perceived loudness.

    constexpr double fc = 38.13547087613982; // Cutoff frequency from ITU spec (Hz)
    constexpr double Q = 0.5003270373253953; // Q factor from ITU spec

    const double K = std::tan (MathConstants<double>::pi * fc / sampleRate);
    const double norm = 1.0 / (1.0 + K / Q + K * K);

    // 2nd order highpass biquad coefficients
    b0 = 1.0 * norm;
    b1 = -2.0 * norm;
    b2 = 1.0 * norm;
    a0 = 1.0;
    a1 = 2.0 * (K * K - 1.0) * norm;
    a2 = (1.0 - K / Q + K * K) * norm;
}

//==============================================================================
void LoudnessFilter::updateCoefficients()
{
    // Calculate and set pre-filter coefficients
    double b0_pre, b1_pre, b2_pre, a0_pre, a1_pre, a2_pre;
    calculatePreFilterCoefficients (sampleRate, b0_pre, b1_pre, b2_pre, a0_pre, a1_pre, a2_pre);

    BiquadCoefficients<double> preCoeffs (b0_pre, b1_pre, b2_pre, a0_pre, a1_pre, a2_pre);
    preFilter.setCoefficients (preCoeffs);

    // Calculate and set highpass filter coefficients
    double b0_hp, b1_hp, b2_hp, a0_hp, a1_hp, a2_hp;
    calculateHighpassCoefficients (sampleRate, b0_hp, b1_hp, b2_hp, a0_hp, a1_hp, a2_hp);

    BiquadCoefficients<double> hpCoeffs (b0_hp, b1_hp, b2_hp, a0_hp, a1_hp, a2_hp);
    highpassFilter.setCoefficients (hpCoeffs);
}

} // namespace yup
