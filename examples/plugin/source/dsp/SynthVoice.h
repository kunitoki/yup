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

#include "SynthBlockContext.h"
#include "SynthEnvelope.h"
#include "SynthFilterStage.h"
#include "SynthOscillator.h"
#include "SynthSound.h"

#include <vector>

//==============================================================================
/** A polyphonic voice: two oscillators into a filter, with its own envelopes and LFOs. */
class SynthVoice : public yup::SynthesiserVoice
{
public:
    SynthVoice (const std::array<SynthPartials, SynthExample::oscillatorCount>& oscillatorPartials,
                const SynthOscillatorResources& oscillatorResources,
                const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& sharedSlots,
                const SynthBlockContext& blockContext);

    /** Allocates every oscillator backend. Must run outside the audio callback. */
    void prepare (double sampleRate, int maxBlockSize);

    //==============================================================================
    bool canPlaySound (yup::SynthesiserSound* sound) override;

    /** Configures the next mono transition. Legato retains the envelope and phases.
        Glide is measured in seconds and interpolates pitch in semitones. */
    void setTransition (double seconds, bool legato) noexcept;

    void startNote (int midiNoteNumber, float velocity, yup::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote (float, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int, int) override {}

    /** Includes a recycled voice's short continuation in the activity meter. */
    bool isSounding() const noexcept { return isVoiceActive() || tailPosition < tailLength; }

    /** Returns the phase of one of this voice's LFOs, for the display. */
    float getLFOPhase (int lfoIndex) const noexcept { return lfos[static_cast<std::size_t> (lfoIndex)].getPhase(); }

    //==============================================================================
    void renderNextBlock (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    //==============================================================================
    void renderChunk (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) noexcept;

    static double midiNoteToFrequency (double midiNoteNumber) noexcept
    {
        return 440.0 * std::pow (2.0, (midiNoteNumber - 69) / 12.0);
    }

    //==============================================================================
    static constexpr double pitchWheelRangeSemitones = 2.0;

    const std::array<SynthPartials, SynthExample::oscillatorCount>& partials;
    const SynthOscillatorResources& resources;
    const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& slots;
    const SynthBlockContext& context;

    std::array<SynthOscillator, SynthExample::oscillatorCount> oscillators;
    std::array<yup::SmoothedValue<float>, SynthExample::oscillatorCount> levels;

    SynthEnvelope envelope;
    SynthEnvelope modulationEnvelope;
    std::array<yup::LFO<float>, SynthExample::lfoCount> lfos;
    SynthFilterStage filter;

    std::vector<float> oscLeft;
    std::vector<float> oscRight;
    std::vector<float> mixLeft;
    std::vector<float> mixRight;

    yup::SmoothedValue<double> pitch;
    yup::SmoothedValue<double> bend;
    yup::AudioBuffer<float> tailBuffer;
    yup::AudioBuffer<float> tailScratch;
    double playbackRate = 44100.0;
    double glideSeconds = 0.0;
    int tailLength = 0;
    int tailPosition = 0;
    bool preserveNote = false;
    bool hasPlayed = false;
    float velocityGain = 1.0f;
};
