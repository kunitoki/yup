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

#include "SynthBitcrusher.h"
#include "SynthChorus.h"
#include "SynthCompressor.h"
#include "SynthDelay.h"
#include "SynthDistortion.h"
#include "SynthEqualizer.h"
#include "SynthFlanger.h"
#include "SynthPhaser.h"
#include "SynthPlateReverb.h"
#include "SynthSurround.h"
#include "SynthWaveshaper.h"

#include <vector>

//==============================================================================
/** The master effects chain, run on the summed voices.

    Effects run in the given order. Switching one on or off crossfades it in or out
    over SynthExample::enableFadeSeconds; once faded out it is cleared and skipped, so a
    disabled effect costs nothing and comes back without an old tail.

    @see SynthEffectsParameters, SynthEffectChain
*/
class SynthEffectsRack
{
public:
    /** Prepares every effect. Must run outside the audio callback.

        @param maxBlockSize  The longest chunk process() will be given
    */
    void prepare (double sampleRate, int maxBlockSize);

    /** Clears every effect's memory, cutting the tails. */
    void reset() noexcept;

    /** Picks up the drawn curves the effects shape with when they changed. Audio thread, never waits. */
    void updateCurves (const SynthCurveState& curves) noexcept { waveshaper.updateCurve (curves); }

    /** Processes a stereo chunk in place.

        @param order  The effects from first to last, see SynthEffectChain
        @param bpm    The host tempo, or zero when unknown
    */
    void process (float* left, float* right, int numSamples, const SynthEffectOrder& order, const SynthEffectsValues& values, double bpm) noexcept;

private:
    void processEffect (SynthEffectType type, float* left, float* right, int numSamples, const SynthEffectsValues& values, double bpm) noexcept;
    void resetEffect (SynthEffectType type) noexcept;

    SynthBitcrusher bitcrusher;
    SynthChorus chorus;
    SynthPhaser phaser;
    SynthFlanger flanger;
    SynthDelay delay;
    SynthPlateReverb reverb;
    SynthDistortion distortion;
    SynthEqualizer equalizer;
    SynthCompressor compressor;
    SynthSurround surround;
    SynthWaveshaper waveshaper;

    float fadeStep = 1.0f;
    std::array<float, SynthExample::effectCount> enableGains {};
    std::vector<float> wetLeft, wetRight;
};
