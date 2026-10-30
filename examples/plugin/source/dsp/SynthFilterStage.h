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

#include "SynthFilterParameters.h"

#include <array>

//==============================================================================
/** One voice filter: an antialiased drive into the selected filter, per channel.

    The state variable types run one VAStateVariableFilter per channel, or two cascaded
    for the 24 dB slopes. The analog models and the comb come from yup_dsp's filters;
    their own saturation is not antialiased, so it is kept mild and follows the drive.

    Drive is a tanh through yup::AaIirAntialiaser, so it does not alias at the host rate.
    It is blended in over the clean signal below driveFadeRange, and the blend moves by at
    most a full swing per enable fade, so neither a slow nor a sudden drive change from or
    to zero switches it in or out at once.
    Cutoff, drive and mix are ramped across each control chunk; resonance glides toward
    its target over resonanceSmoothingSeconds, because the analog models scale their
    output with it. The coefficients are refreshed every few samples, not on every sample.

    @see SynthFilterParameters
*/
class SynthFilterStage
{
public:
    /** Prepares every filter and the parameter ramps. */
    void prepare (double sampleRate);

    /** Clears the filter memory, for a fresh note. */
    void reset() noexcept;

    /** Filters a stereo chunk in place at the note's keytracked cutoff.

        A disabled filter fades its mix to dry across the chunk, then leaves the audio
        untouched and does no work until it is enabled again.
    */
    void process (float* left, float* right, int numSamples, const SynthFilterValues& values, double midiNote) noexcept;

    /** Sets the stage to a filter's controls without processing, for a response display.
        Use a stage of its own, never one that plays.
    */
    void configureForDisplay (const SynthFilterValues& values) noexcept;

    /** Returns the configured filter's linear magnitude at a frequency, drive aside. */
    float getMagnitudeResponse (float frequency) const noexcept;

    /** Returns the spacing in Hz of the comb's teeth, or 0 when the filter is not a comb. */
    float getToothSpacing() const noexcept;

    /** Returns the magnitudes the comb's troughs and teeth reach around a frequency, drive aside:
        smooth envelopes for a display where they are too dense to resolve.
    */
    yup::Range<float> getToothMagnitudeRange (float frequency) const noexcept;

private:
    //==============================================================================
    static constexpr int coefficientInterval = 8;
    static constexpr float driveFadeRange = 0.05f;
    static constexpr double resonanceSmoothingSeconds = 0.01;

    using Antialiaser = yup::AaIirAntialiaser<float, yup::TanhClipperTraits>;

    void updateCoefficients (SynthFilterType type, float frequency, float resonance, float saturation) noexcept;
    float filterSample (int channel, SynthFilterType type, float input) noexcept;

    //==============================================================================
    std::array<std::array<yup::VAStateVariableFilter<float>, 2>, 2> stateVariable; // [stage][channel]
    std::array<yup::AnalogMoogLadderFilter<float>, 2> moog;
    std::array<yup::AnalogKorg35Filter<float>, 2> korg;
    std::array<yup::AnalogRolandDiodeFilter<float>, 2> diode;
    std::array<yup::AnalogVowelFilter<float>, 2> vowel;
    std::array<yup::CombFilter<float>, 2> comb;
    std::array<Antialiaser, 2> drivers;

    double sampleRate = 44100.0;
    SynthFilterType activeType = SynthFilterType::lowpass12;
    float cutoff = 8000.0f;
    float resonance = 0.2f;
    float drive = 0.0f;
    float driveFade = 0.0f;
    float mix = 1.0f;
    bool hasState = false;
};
