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

#include "SynthParameterFactory.h"
#include "SynthTypes.h"

#include <vector>

//==============================================================================
/** The master effects. Their order here sets their host IDs, not where they run in the chain. */
enum class SynthEffectType
{
    bitcrusher,
    chorus,
    phaser,
    flanger,
    delay,
    reverb,
    distortion,
    equalizer,
    compressor,
    surround,
    waveshaper
};

/** The effects from the first to process to the last, each present once. */
using SynthEffectOrder = std::array<SynthEffectType, SynthExample::effectCount>;

/** @internal Display names for SynthEffectType, index aligned with the enumeration. */
inline yup::StringArray getSynthEffectNames()
{
    return { "Bitcrusher", "Chorus", "Phaser", "Flanger", "Delay", "Reverb", "Distortion", "EQ", "Compressor", "Surround", "Waveshaper" };
}

/** How the chorus moves: free controls, or the fixed settings of the Juno-60 buttons. */
enum class SynthChorusMode
{
    generic,
    junoI,
    junoII,
    junoBoth
};

/** @internal Item names for SynthChorusMode, index aligned with the enumeration. */
inline yup::StringArray getSynthChorusModeNames()
{
    return { "Generic", "Juno I", "Juno II", "Juno I+II" };
}

/** The curve the distortion shapes with. */
enum class SynthDistortionType
{
    soft,
    cubic,
    hard
};

/** @internal Item names for SynthDistortionType, index aligned with the enumeration. */
inline yup::StringArray getSynthDistortionTypeNames()
{
    return { "Soft", "Cubic", "Hard" };
}

//==============================================================================
/** A plain snapshot of the bitcrusher. */
struct SynthBitcrusherValues
{
    float bits = 8.0f;     /**< Quantization depth, 1 to 16, fractional values included. */
    float rate = 11025.0f; /**< The rate samples are held at, in Hz. */
    float dither = 0.0f;
    float mix = 1.0f;

    bool operator== (const SynthBitcrusherValues&) const = default;
};

/** A plain snapshot of the chorus. Rate, depth, delay and feedback only apply to the generic mode. */
struct SynthChorusValues
{
    SynthChorusMode mode = SynthChorusMode::generic;
    float rate = 0.8f;
    float depth = 0.5f;
    float delayMs = 7.0f;
    float feedback = 0.0f;
    float mix = 0.5f;

    bool operator== (const SynthChorusValues&) const = default;
};

/** A plain snapshot of the phaser. */
struct SynthPhaserValues
{
    int stages = 8; /**< 4, 8 or 12 allpass stages. */
    float rate = 0.4f;
    float depth = 0.7f;
    float center = 800.0f;
    float feedback = 0.5f;
    float stereo = 0.5f; /**< LFO phase offset of the right channel, 0 to half a cycle. */
    float mix = 0.5f;

    bool operator== (const SynthPhaserValues&) const = default;
};

/** A plain snapshot of the flanger. */
struct SynthFlangerValues
{
    float rate = 0.25f;
    float depth = 0.7f;
    float delayMs = 2.5f;
    float feedback = 0.5f; /**< Bipolar: negative feedback hollows the comb out. */
    float stereo = 0.5f;
    float mix = 0.5f;

    bool operator== (const SynthFlangerValues&) const = default;
};

/** A plain snapshot of the delay. */
struct SynthDelayValues
{
    static constexpr double fallbackTempo = 120.0;

    bool tempoSync = true;
    float timeMs = 375.0f;
    int division = 13; /**< Index into getSynthLFODivisions(), 1/8 D by default. */
    float feedback = 0.35f;
    bool pingPong = true;
    float lowCut = 100.0f;
    float highCut = 8000.0f;
    float mix = 0.3f;

    bool operator== (const SynthDelayValues&) const = default;

    /** Returns the delay time in seconds, following the division when synced.

        @param bpm  The host tempo, or zero when unknown, which plays at fallbackTempo
    */
    double getSeconds (double bpm) const noexcept
    {
        if (! tempoSync)
            return timeMs * 0.001;

        const auto& divisions = getSynthLFODivisions();
        const auto beats = divisions[static_cast<std::size_t> (yup::jlimit (0, static_cast<int> (divisions.size()) - 1, division))].beats;

        return 60.0 * beats / (bpm > 0.0 ? bpm : fallbackTempo);
    }
};

/** A plain snapshot of the plate reverb. */
struct SynthReverbValues
{
    float predelayMs = 20.0f;
    float decay = 0.6f;
    float damping = 0.4f;
    float width = 1.0f;
    float mix = 0.3f;

    bool operator== (const SynthReverbValues&) const = default;
};

/** A plain snapshot of the distortion. */
struct SynthDistortionValues
{
    SynthDistortionType type = SynthDistortionType::soft;
    float driveDb = 12.0f;
    float bias = 0.0f;       /**< Bipolar offset before the curve, for even harmonics. */
    float tone = 12000.0f;   /**< Lowpass after the curve, in Hz. */
    float outputDb = 0.0f;   /**< Gain after the automatic level match. */
    float mix = 1.0f;

    bool operator== (const SynthDistortionValues&) const = default;
};

/** A plain snapshot of the three band equalizer: a low shelf, a peak and a high shelf. */
struct SynthEqualizerValues
{
    float lowGainDb = 0.0f;
    float lowFrequency = 120.0f;
    float midGainDb = 0.0f;
    float midFrequency = 1000.0f;
    float midQ = 0.7f;
    float highGainDb = 0.0f;
    float highFrequency = 6000.0f;

    bool operator== (const SynthEqualizerValues&) const = default;
};

/** A plain snapshot of the compressor. */
struct SynthCompressorValues
{
    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 150.0f;
    float makeupDb = 0.0f;
    float mix = 1.0f; /**< Below one, parallel compression. */

    bool operator== (const SynthCompressorValues&) const = default;
};

/** A plain snapshot of the surround: a mid/side widener with a delayed ambience. */
struct SynthSurroundValues
{
    float width = 1.0f;      /**< Side level, 0 for mono to 2 for twice as wide. */
    float depth = 0.5f;      /**< Level of the delayed mid sent to the sides. */
    float timeMs = 12.0f;    /**< Delay of that ambience. */
    float monoBelow = 120.0f; /**< The sides are cut below this frequency, keeping the bass centered. */
    float mix = 1.0f;

    bool operator== (const SynthSurroundValues&) const = default;
};

/** A plain snapshot of the waveshaper; its transfer curve is patch state, see SynthCurveState. */
struct SynthWaveshaperValues
{
    float driveDb = 0.0f;  /**< Gain into the curve. */
    float bias = 0.0f;     /**< Bipolar offset before the curve, for even harmonics. */
    float outputDb = 0.0f; /**< Gain after the curve. */
    float mix = 1.0f;

    bool operator== (const SynthWaveshaperValues&) const = default;
};

/** A plain snapshot of every effect's controls; the chain order is SynthEffectChain's. */
struct SynthEffectsValues
{
    std::array<bool, SynthExample::effectCount> enabled {};

    SynthBitcrusherValues bitcrusher;
    SynthChorusValues chorus;
    SynthPhaserValues phaser;
    SynthFlangerValues flanger;
    SynthDelayValues delay;
    SynthReverbValues reverb;
    SynthDistortionValues distortion;
    SynthEqualizerValues equalizer;
    SynthCompressorValues compressor;
    SynthSurroundValues surround;
    SynthWaveshaperValues waveshaper;

    bool operator== (const SynthEffectsValues&) const = default;

    /** True when an effect is switched on. */
    bool isEnabled (SynthEffectType type) const noexcept { return enabled[static_cast<std::size_t> (type)]; }
};

//==============================================================================
/** The parameters of the master effects, IDs prefixed "fx.".

    Each effect has its own switch, all off by default. The chain order is not a parameter
    but patch state, see SynthEffectChain.

    Host IDs are 100 per effect from 8100, in SynthEffectType order.

    @see SynthEffectsRack, SynthEffectChain
*/
class SynthEffectsParameters
{
public:
    /** Creates every parameter at its default. */
    SynthEffectsParameters();

    /** Registers every parameter with the processor. */
    void addTo (yup::AudioProcessor& processor) const;

    /** Returns every parameter, in registration order. */
    std::vector<yup::AudioParameter::Ptr> getParameters() const;

    /** Takes a snapshot. */
    SynthEffectsValues read (SynthParameterReader reader = {}) const noexcept;

    /** Returns the switch of an effect. */
    const yup::AudioParameter::Ptr& getEnabled (SynthEffectType type) const noexcept { return enabled[static_cast<std::size_t> (type)]; }

    struct Bitcrusher
    {
        yup::AudioParameter::Ptr bits, rate, dither, mix;
    };

    struct Chorus
    {
        yup::AudioParameter::Ptr mode, rate, depth, delay, feedback, mix;
    };

    struct Phaser
    {
        yup::AudioParameter::Ptr stages, rate, depth, center, feedback, stereo, mix;
    };

    struct Flanger
    {
        yup::AudioParameter::Ptr rate, depth, delay, feedback, stereo, mix;
    };

    struct Delay
    {
        yup::AudioParameter::Ptr tempoSync, time, division, feedback, pingPong, lowCut, highCut, mix;
    };

    struct Reverb
    {
        yup::AudioParameter::Ptr predelay, decay, damping, width, mix;
    };

    struct Distortion
    {
        yup::AudioParameter::Ptr type, drive, bias, tone, output, mix;
    };

    struct Equalizer
    {
        yup::AudioParameter::Ptr lowGain, lowFrequency, midGain, midFrequency, midQ, highGain, highFrequency;
    };

    struct Compressor
    {
        yup::AudioParameter::Ptr threshold, ratio, attack, release, makeup, mix;
    };

    struct Surround
    {
        yup::AudioParameter::Ptr width, depth, time, monoBelow, mix;
    };

    struct Waveshaper
    {
        yup::AudioParameter::Ptr drive, bias, output, mix;
    };

    std::array<yup::AudioParameter::Ptr, SynthExample::effectCount> enabled;

    Bitcrusher bitcrusher;
    Chorus chorus;
    Phaser phaser;
    Flanger flanger;
    Delay delay;
    Reverb reverb;
    Distortion distortion;
    Equalizer equalizer;
    Compressor compressor;
    Surround surround;
    Waveshaper waveshaper;
};
