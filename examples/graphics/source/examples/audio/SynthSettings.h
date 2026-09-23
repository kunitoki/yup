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

#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>

//==============================================================================
/** Sizing shared by the polyphonic engine and its user interface. */
namespace SynthExample
{
constexpr int voiceCount = 8;
constexpr int oscillatorCount = 2;
constexpr int maxHarmonics = 128;
constexpr int maxBlockSize = 2048;
constexpr int envelopeCount = 2;
constexpr int lfoCount = 2;
constexpr int modulationSlots = 8;

/** Samples between control updates: modulation, filter targets and local spectra. */
constexpr int controlChunk = 128;

/** Unison slots per oscillator, counting the one running the selected algorithm. */
constexpr int maxUnisonVoices = 5;

/** Harmonics the partial editor exposes, a subset of the maxHarmonics the engine renders. */
constexpr int editableHarmonics = 64;

/** Harmonics the waveform display sums, capped well below maxHarmonics to keep repaints cheap. */
constexpr int displayHarmonics = 64;

constexpr double levelRampSeconds = 0.01;
} // namespace SynthExample

/** @internal Item names for yup::Waveform, index aligned with the enumeration. */
inline yup::StringArray getSynthWaveformNames()
{
    return { "Sine", "Cosine", "Sawtooth", "Square", "Triangle", "Pulse" };
}

/** @internal Item names for yup::SyncMode, index aligned with the enumeration. */
inline yup::StringArray getSynthSyncModeNames()
{
    return { "None", "Hard", "Mirrored", "Pulsar" };
}

//==============================================================================
/** A plain snapshot of the amplitude envelope's controls. */
struct SynthEnvelopeValues
{
    float delay = 0.0f;
    float attack = 0.005f;
    float hold = 0.0f;
    float decay = 0.35f;
    float sustain = 0.7f;
    float release = 0.35f;
};

/** The same controls, edited from the message thread while the audio thread reads them. */
struct SynthEnvelopeSettings
{
    std::atomic<float> delay { 0.0f };
    std::atomic<float> attack { 0.005f };
    std::atomic<float> hold { 0.0f };
    std::atomic<float> decay { 0.35f };
    std::atomic<float> sustain { 0.7f };
    std::atomic<float> release { 0.35f };

    /** Takes a snapshot for one block of audio. */
    SynthEnvelopeValues read() const noexcept
    {
        return { delay.load(), attack.load(), hold.load(), decay.load(), sustain.load(), release.load() };
    }
};

//==============================================================================
/** A plain snapshot of one oscillator's controls.

    The audio thread takes one snapshot per block and compares it with the values it
    applied last time, so a control that did not move never costs a spectral
    transform or a table render.

    The edited partials are deliberately not copied here. They live behind
    harmonicGeneration, so a block only pays for them when the editor actually moved.
*/
struct SynthOscillatorValues
{
    yup::Waveform waveform = yup::Waveform::sawtooth;
    yup::SyncMode syncMode = yup::SyncMode::none;
    float syncRatio = 1.5f;
    float level = 0.5f;
    int octave = 0;
    float detuneSemitones = 0.0f;
    float ridgeSpacing = 1.5f;
    float color = 0.0f;
    float dispersion = 0.5f;
    float squeeze = 0.0f;
    float squash = 1.0f;
    float tilt = 0.0f;
    float oddEven = 0.5f;
    float formant = 0.0f;
    float formantPosition = 2.0f;
    float scatter = 0.0f;
    int unisonVoices = 1;
    float unisonDetune = 0.2f;
    float unisonSpread = 0.6f;
    float harmonicScale = 1.0f;
    bool usesCustomSeries = false;
    int harmonicGeneration = 0;
};

/** The same controls, edited from the message thread while the audio thread reads them.

    The waveform editor writes partials continuously while the mouse is down but bumps
    harmonicGeneration at most once per user interface frame. The audio thread rebuilds
    its series only when that counter moves, which keeps a drag from forcing an inverse
    FFT per mouse event on every sounding voice.

    @see SynthOscillator, WaveformEditor
*/
struct SynthOscillatorSettings
{
    SynthOscillatorSettings()
    {
        for (auto& cosine : harmonicCosines)
            cosine.store (0.0f);

        for (auto& sine : harmonicSines)
            sine.store (0.0f);
    }

    std::atomic<int> waveform { static_cast<int> (yup::Waveform::sawtooth) };
    std::atomic<int> syncMode { static_cast<int> (yup::SyncMode::none) };
    std::atomic<float> syncRatio { 1.5f };
    std::atomic<float> level { 0.5f };
    std::atomic<int> octave { 0 };
    std::atomic<float> detuneSemitones { 0.0f };
    std::atomic<float> ridgeSpacing { 1.5f };
    std::atomic<float> color { 0.0f };
    std::atomic<float> dispersion { 0.5f };
    std::atomic<float> squeeze { 0.0f };
    std::atomic<float> squash { 1.0f };
    std::atomic<float> tilt { 0.0f };
    std::atomic<float> oddEven { 0.5f };
    std::atomic<float> formant { 0.0f };
    std::atomic<float> formantPosition { 2.0f };
    std::atomic<float> scatter { 0.0f };
    std::atomic<int> unisonVoices { 1 };
    std::atomic<float> unisonDetune { 0.2f };
    std::atomic<float> unisonSpread { 0.6f };

    std::array<std::atomic<float>, SynthExample::editableHarmonics> harmonicCosines;
    std::array<std::atomic<float>, SynthExample::editableHarmonics> harmonicSines;
    std::atomic<float> harmonicScale { 1.0f };
    std::atomic<bool> usesCustomSeries { false };
    std::atomic<int> harmonicGeneration { 0 };

    /** Takes a snapshot for one block of audio. */
    SynthOscillatorValues read() const noexcept
    {
        return { static_cast<yup::Waveform> (waveform.load()),
                 static_cast<yup::SyncMode> (syncMode.load()),
                 syncRatio.load(),
                 level.load(),
                 octave.load(),
                 detuneSemitones.load(),
                 ridgeSpacing.load(),
                 color.load(),
                 dispersion.load(),
                 squeeze.load(),
                 squash.load(),
                 tilt.load(),
                 oddEven.load(),
                 formant.load(),
                 formantPosition.load(),
                 scatter.load(),
                 unisonVoices.load(),
                 unisonDetune.load(),
                 unisonSpread.load(),
                 harmonicScale.load(),
                 usesCustomSeries.load(),
                 harmonicGeneration.load() };
    }

    /** Rebuilds a prepared series from the edited partials, without allocating.

        Every partial keeps both its cosine and sine coefficient, so a drawn cycle comes
        back with the phase it was drawn with rather than as a sum of sines.

        Nothing stops the editor from asking for every harmonic at once, which would sum
        to many times full scale, so the caller passes the scale that brings the
        reconstruction back to a peak of one. WaveformEditor measures it while it redraws
        and publishes it with the same generation bump; the audio thread passes it back
        in here rather than measuring anything itself.

        @param series The prepared series to overwrite
        @param scale  The factor to apply to every coefficient
    */
    void copyHarmonicsInto (yup::FourierSeries<double>& series, float scale) const noexcept
    {
        series.clear();

        const auto count = yup::jmin (SynthExample::editableHarmonics, series.getNumHarmonics());

        for (int harmonic = 1; harmonic <= count; ++harmonic)
        {
            const auto index = static_cast<std::size_t> (harmonic - 1);

            series.setHarmonic (harmonic,
                                static_cast<double> (harmonicCosines[index].load() * scale),
                                static_cast<double> (harmonicSines[index].load() * scale));
        }
    }

    /** Seeds the edited partials from a series, so editing starts at the visible shape. */
    void seedHarmonicsFrom (const yup::FourierSeries<double>& series) noexcept
    {
        const auto count = yup::jmin (SynthExample::editableHarmonics, series.getNumHarmonics());

        for (int harmonic = 1; harmonic <= count; ++harmonic)
        {
            const auto index = static_cast<std::size_t> (harmonic - 1);

            harmonicCosines[index].store (static_cast<float> (series.getCosine (harmonic)));
            harmonicSines[index].store (static_cast<float> (series.getSine (harmonic)));
        }

        for (int harmonic = count; harmonic < SynthExample::editableHarmonics; ++harmonic)
        {
            harmonicCosines[static_cast<std::size_t> (harmonic)].store (0.0f);
            harmonicSines[static_cast<std::size_t> (harmonic)].store (0.0f);
        }
    }

    /** Sets the magnitude of one partial while keeping its phase.

        A partial that is currently silent has no phase to keep, so it starts as a sine.

        @param index     The zero based partial, 0 being the fundamental
        @param magnitude The new magnitude
    */
    void setHarmonicMagnitude (int index, float magnitude) noexcept
    {
        jassert (yup::isPositiveAndBelow (index, SynthExample::editableHarmonics));

        auto& cosine = harmonicCosines[static_cast<std::size_t> (index)];
        auto& sine = harmonicSines[static_cast<std::size_t> (index)];

        const auto currentCosine = cosine.load();
        const auto currentSine = sine.load();
        const auto currentMagnitude = std::hypot (currentCosine, currentSine);

        if (currentMagnitude < 1.0e-6f)
        {
            cosine.store (0.0f);
            sine.store (magnitude);
            return;
        }

        const auto scale = magnitude / currentMagnitude;

        cosine.store (currentCosine * scale);
        sine.store (currentSine * scale);
    }
};

//==============================================================================
/** The response the per-voice filter selects, Off skipping the stage entirely. */
enum class SynthFilterType
{
    off,
    lowpass,
    highpass,
    bandpass,
    notch,
    allpass,
    peak
};

/** @internal Item names for SynthFilterType, index aligned with the enumeration. */
inline yup::StringArray getSynthFilterTypeNames()
{
    return { "Off", "Lowpass", "Highpass", "Bandpass", "Notch", "Allpass", "Peak" };
}

/** Maps a type onto the yup::FilterMode the VAStateVariableFilter reads. */
inline yup::FilterModeType toFilterMode (SynthFilterType type) noexcept
{
    switch (type)
    {
        case SynthFilterType::highpass: return yup::FilterMode::highpass;
        case SynthFilterType::bandpass: return yup::FilterMode::bandpassCsg;
        case SynthFilterType::notch: return yup::FilterMode::bandstop;
        case SynthFilterType::allpass: return yup::FilterMode::allpass;
        case SynthFilterType::peak: return yup::FilterMode::peak;
        case SynthFilterType::off:
        case SynthFilterType::lowpass: break;
    }

    return yup::FilterMode::lowpass;
}

/** A plain snapshot of the filter section's controls. */
struct SynthFilterValues
{
    SynthFilterType type = SynthFilterType::lowpass;
    float cutoff = 8000.0f;
    float resonance = 0.2f;
    float drive = 0.0f;
    float keytrack = 0.0f;
};

/** The same controls, edited from the message thread while the audio thread reads them. */
struct SynthFilterSettings
{
    std::atomic<int> type { static_cast<int> (SynthFilterType::lowpass) };
    std::atomic<float> cutoff { 8000.0f };
    std::atomic<float> resonance { 0.2f };
    std::atomic<float> drive { 0.0f };
    std::atomic<float> keytrack { 0.0f };

    /** Takes a snapshot for one block of audio. */
    SynthFilterValues read() const noexcept
    {
        return { static_cast<SynthFilterType> (type.load()), cutoff.load(), resonance.load(), drive.load(), keytrack.load() };
    }
};

//==============================================================================
/** @internal Item names for yup::LFO::Shape, index aligned with the enumeration. */
inline yup::StringArray getSynthLFOShapeNames()
{
    return { "Sine", "Triangle", "Sawtooth", "Square", "S&H" };
}

/** A plain snapshot of one LFO's controls. */
struct SynthLFOValues
{
    yup::LFO<float>::Shape shape = yup::LFO<float>::Shape::sine;
    float rate = 1.0f;
    float phase = 0.0f;
    bool retrigger = true; /**< Restart from the phase offset on every note, or run freely. */
};

/** The same controls, edited from the message thread while the audio thread reads them. */
struct SynthLFOSettings
{
    std::atomic<int> shape { static_cast<int> (yup::LFO<float>::Shape::sine) };
    std::atomic<float> rate { 1.0f };
    std::atomic<float> phase { 0.0f };
    std::atomic<bool> retrigger { true };

    /** Takes a snapshot for one block of audio. */
    SynthLFOValues read() const noexcept
    {
        return { static_cast<yup::LFO<float>::Shape> (shape.load()), rate.load(), phase.load(), retrigger.load() };
    }
};

//==============================================================================
/** What can drive a modulation route. */
enum class SynthModulationSource
{
    env1, /**< The amplitude envelope, unipolar. */
    env2, /**< The free envelope, unipolar. */
    lfo1, /**< Bipolar. */
    lfo2
};

/** @internal Item names for SynthModulationSource, index aligned with the enumeration. */
inline yup::StringArray getSynthModulationSourceNames()
{
    return { "ENV 1", "ENV 2", "LFO 1", "LFO 2" };
}

/** Every parameter the matrix can reach; the oscillator block repeats per oscillator. */
enum class SynthModulationDestination
{
    none,
    osc1Level, osc1Cents, osc1Ridges, osc1Color, osc1Dispersion, osc1Squeeze, osc1Squash, osc1Tilt,
    osc1OddEven, osc1Formant, osc1FormantPosition, osc1Scatter, osc1SyncRatio, osc1UnisonDetune, osc1UnisonSpread,
    osc2Level, osc2Cents, osc2Ridges, osc2Color, osc2Dispersion, osc2Squeeze, osc2Squash, osc2Tilt,
    osc2OddEven, osc2Formant, osc2FormantPosition, osc2Scatter, osc2SyncRatio, osc2UnisonDetune, osc2UnisonSpread,
    filterCutoff, filterResonance, filterDrive,
    count
};

/** Destinations per oscillator in SynthModulationDestination. */
constexpr int synthOscillatorDestinations = 15;

/** @internal Item names for SynthModulationDestination, index aligned with the enumeration. */
inline yup::StringArray getSynthModulationDestinationNames()
{
    yup::StringArray names { "-" };

    for (int oscillator = 1; oscillator <= SynthExample::oscillatorCount; ++oscillator)
    {
        const auto prefix = yup::String ("OSC ") + yup::String (oscillator) + " ";

        for (const auto* name : { "Level", "Cents", "Ridges", "Color", "Dispersion", "Squeeze", "Squash", "Tilt",
                                  "Odd/Even", "Formant", "F.Pos", "Scatter", "Sync Ratio", "U.Detune", "Spread" })
            names.add (prefix + name);
    }

    names.add ("Filter Cutoff");
    names.add ("Filter Resonance");
    names.add ("Filter Drive");

    return names;
}

/** Returns which oscillator a destination belongs to, or -1 for the filter and none. */
constexpr int getDestinationOscillator (SynthModulationDestination destination) noexcept
{
    const auto index = static_cast<int> (destination) - 1;

    if (index < 0 || index >= SynthExample::oscillatorCount * synthOscillatorDestinations)
        return -1;

    return index / synthOscillatorDestinations;
}

/** True for the parameters that change the derived spectrum rather than how it is played. */
constexpr bool isSpectrumDestination (SynthModulationDestination destination) noexcept
{
    if (getDestinationOscillator (destination) < 0)
        return false;

    const auto field = (static_cast<int> (destination) - 1) % synthOscillatorDestinations;

    return field >= 2 && field <= 12;
}

/** The knob range of a destination, shared by the panel and the modulation mapping. */
struct SynthParameterRange
{
    float minimum = 0.0f;
    float maximum = 1.0f;
    bool logarithmic = false;

    /** Applies a normalized amount, one full range per unit, and clamps. */
    float modulate (float base, float amount) const noexcept
    {
        if (logarithmic)
            return yup::jlimit (minimum, maximum, base * std::exp2 (amount * std::log2 (maximum / minimum)));

        return yup::jlimit (minimum, maximum, base + amount * (maximum - minimum));
    }
};

/** Returns the range a destination's knob spans. */
inline SynthParameterRange getDestinationRange (SynthModulationDestination destination) noexcept
{
    switch (destination)
    {
        case SynthModulationDestination::filterCutoff: return { 20.0f, 20000.0f, true };
        case SynthModulationDestination::filterResonance: return { 0.0f, 1.0f };
        case SynthModulationDestination::filterDrive: return { 0.0f, 1.0f };
        case SynthModulationDestination::none:
        case SynthModulationDestination::count: return {};
        default: break;
    }

    switch ((static_cast<int> (destination) - 1) % synthOscillatorDestinations)
    {
        case 0: return { 0.0f, 1.0f };       // level
        case 1: return { -1.0f, 1.0f };      // cents, stored as semitones
        case 2: return { 0.25f, 8.0f };      // ridges
        case 3: return { 0.0f, 1.0f };       // color
        case 4: return { 0.01f, 0.99f };     // dispersion
        case 5: return { 0.0f, 0.5f };       // squeeze
        case 6: return { 0.1f, 4.0f };       // squash
        case 7: return { -4.0f, 4.0f };      // tilt
        case 8: return { 0.0f, 1.0f };       // odd/even
        case 9: return { -4.0f, 4.0f };      // formant
        case 10: return { 0.0f, 7.0f };      // formant position
        case 11: return { 0.0f, 1.0f };      // scatter
        case 12: return { 1.0f, 8.0f };      // sync ratio
        case 13: return { 0.0f, 1.0f };      // unison detune
        case 14: return { 0.0f, 1.0f };      // unison spread
        default: return {};
    }
}

/** One routing of the matrix. */
struct SynthModulationRoute
{
    SynthModulationSource source = SynthModulationSource::env1;
    SynthModulationDestination destination = SynthModulationDestination::none;
    float depth = 0.0f;
};

/** A snapshot of every routing. */
struct SynthModulationValues
{
    std::array<SynthModulationRoute, SynthExample::modulationSlots> routes;

    /** True when something is routed with depth into a spectrum parameter of this oscillator. */
    bool hasSpectrumRoute (int oscillator) const noexcept
    {
        for (const auto& route : routes)
        {
            if (route.depth != 0.0f && isSpectrumDestination (route.destination)
                && getDestinationOscillator (route.destination) == oscillator)
                return true;
        }

        return false;
    }
};

/** The routings, edited from the message thread while the audio thread reads them. */
struct SynthModulationSettings
{
    struct Slot
    {
        std::atomic<int> source { static_cast<int> (SynthModulationSource::env1) };
        std::atomic<int> destination { static_cast<int> (SynthModulationDestination::none) };
        std::atomic<float> depth { 0.0f };
    };

    std::array<Slot, SynthExample::modulationSlots> slots;

    /** Takes a snapshot for one block of audio. */
    SynthModulationValues read() const noexcept
    {
        SynthModulationValues values;

        for (std::size_t index = 0; index < slots.size(); ++index)
            values.routes[index] = { static_cast<SynthModulationSource> (slots[index].source.load()),
                                     static_cast<SynthModulationDestination> (slots[index].destination.load()),
                                     slots[index].depth.load() };

        return values;
    }
};

//==============================================================================
/** Everything a voice plays from, before or after modulation has been applied. */
struct SynthPatchValues
{
    std::array<SynthOscillatorValues, SynthExample::oscillatorCount> oscillators;
    SynthFilterValues filter;
};

/** Returns the field a destination modulates, or nullptr for none. */
inline float* getDestinationField (SynthPatchValues& patch, SynthModulationDestination destination) noexcept
{
    switch (destination)
    {
        case SynthModulationDestination::filterCutoff: return &patch.filter.cutoff;
        case SynthModulationDestination::filterResonance: return &patch.filter.resonance;
        case SynthModulationDestination::filterDrive: return &patch.filter.drive;
        default: break;
    }

    const auto oscillator = getDestinationOscillator (destination);

    if (oscillator < 0)
        return nullptr;

    auto& values = patch.oscillators[static_cast<std::size_t> (oscillator)];

    switch ((static_cast<int> (destination) - 1) % synthOscillatorDestinations)
    {
        case 0: return &values.level;
        case 1: return &values.detuneSemitones;
        case 2: return &values.ridgeSpacing;
        case 3: return &values.color;
        case 4: return &values.dispersion;
        case 5: return &values.squeeze;
        case 6: return &values.squash;
        case 7: return &values.tilt;
        case 8: return &values.oddEven;
        case 9: return &values.formant;
        case 10: return &values.formantPosition;
        case 11: return &values.scatter;
        case 12: return &values.syncRatio;
        case 13: return &values.unisonDetune;
        case 14: return &values.unisonSpread;
        default: return nullptr;
    }
}

/** Adds every route whose source has a value to the patch: one full knob range per unit of depth times source. */
inline void applyModulation (SynthPatchValues& patch,
                             const SynthModulationValues& modulation,
                             const std::array<float, 4>& sourceValues) noexcept
{
    for (const auto& route : modulation.routes)
    {
        const auto source = sourceValues[static_cast<std::size_t> (route.source)];

        if (route.depth == 0.0f || source == 0.0f)
            continue;

        if (auto* field = getDestinationField (patch, route.destination))
            *field = getDestinationRange (route.destination).modulate (*field, route.depth * source);
    }
}
