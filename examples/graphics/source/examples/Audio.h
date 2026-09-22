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
#include <memory>
#include <vector>

//==============================================================================
/** Sizing shared by the polyphonic engine and its user interface. */
namespace SynthExample
{
constexpr int voiceCount = 8;
constexpr int oscillatorCount = 2;
constexpr int maxHarmonics = 128;
constexpr int maxBlockSize = 2048;

/** Unison slots per oscillator, counting the one running the selected algorithm. */
constexpr int maxUnisonVoices = 5;

/** Harmonics the partial editor exposes, a subset of the maxHarmonics the engine renders. */
constexpr int editableHarmonics = 32;

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
/** Sums a Fourier series at one point of its period.

    yup::FourierSeries stores coefficients rather than samples, so the waveform display
    reconstructs them on demand. Only the message thread calls this.

    @param series      The coefficients to sum
    @param phase       The position in the period, normalized to 0 to 1
    @param maxHarmonic The highest harmonic to include

    @returns The value of the series at that phase.
*/
inline float evaluateFourierSeries (const yup::FourierSeries<double>& series, double phase, int maxHarmonic) noexcept
{
    const auto count = yup::jmin (maxHarmonic, series.getNumHarmonics());
    const auto theta = yup::MathConstants<double>::twoPi * phase;

    auto value = series.getDC();

    for (int harmonic = 1; harmonic <= count; ++harmonic)
    {
        const auto angle = theta * harmonic;

        value += series.getCosine (harmonic) * std::cos (angle)
               + series.getSine (harmonic) * std::sin (angle);
    }

    return static_cast<float> (value);
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
/** A delay, attack, hold, decay, sustain and release envelope with linear segments.

    yup_dsp has no envelope generator, so the example carries its own. It replaces the
    single yup::SmoothedValue the earlier revision faded notes with, which could only
    ramp between two levels and gave every patch the same shape.

    The stage is advanced one sample at a time and the note ends when the envelope
    falls idle, so a voice stays allocated for exactly as long as it is audible.

    @see SynthVoice
*/
class SynthEnvelope
{
public:
    /** Prepares the envelope and leaves it idle. */
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        reset();
    }

    /** Silences the envelope and returns it to the idle stage. */
    void reset() noexcept
    {
        stage = Stage::idle;
        level = 0.0f;
        stageSample = 0;
    }

    /** Converts the control values into per sample increments. Safe to call every block. */
    void setParameters (const SynthEnvelopeValues& values) noexcept
    {
        sustainLevel = yup::jlimit (0.0f, 1.0f, values.sustain);

        delaySamples = toSamples (values.delay);
        holdSamples = toSamples (values.hold);
        releaseSamples = toSamples (yup::jmax (0.003f, values.release));

        attackIncrement = 1.0f / static_cast<float> (toSamples (yup::jmax (0.003f, values.attack)));
        decayIncrement = (1.0f - sustainLevel) / static_cast<float> (toSamples (values.decay));
    }

    /** Starts a new note from the delay stage. */
    void noteOn() noexcept
    {
        stage = Stage::delay;
        level = 0.0f;
        stageSample = 0;
    }

    /** Begins the release from whatever level the envelope currently sits at. */
    void noteOff() noexcept
    {
        if (stage == Stage::idle)
            return;

        releaseIncrement = level / static_cast<float> (releaseSamples);
        stage = Stage::release;
        stageSample = 0;
    }

    /** Silences the envelope immediately, ending the note without a tail. */
    void noteOffImmediate() noexcept
    {
        reset();
    }

    /** Returns true while the envelope still contributes to the output. */
    bool isActive() const noexcept { return stage != Stage::idle; }

    /** Advances one sample and returns the new gain. */
    float getNextValue() noexcept
    {
        switch (stage)
        {
            case Stage::idle:
                return 0.0f;

            case Stage::delay:
                if (++stageSample >= delaySamples)
                    advanceTo (Stage::attack);

                return 0.0f;

            case Stage::attack:
                level += attackIncrement;

                if (level >= 1.0f)
                {
                    level = 1.0f;
                    advanceTo (Stage::hold);
                }

                return level;

            case Stage::hold:
                if (++stageSample >= holdSamples)
                    advanceTo (Stage::decay);

                return level;

            case Stage::decay:
                level -= decayIncrement;

                if (level <= sustainLevel || decayIncrement <= 0.0f)
                {
                    level = sustainLevel;
                    advanceTo (Stage::sustain);
                }

                return level;

            case Stage::sustain:
                level = sustainLevel;

                return level;

            case Stage::release:
                level -= releaseIncrement;

                if (level <= 0.0f || releaseIncrement <= 0.0f)
                {
                    level = 0.0f;
                    advanceTo (Stage::idle);
                }

                return level;
        }

        return level;
    }

private:
    //==============================================================================
    enum class Stage
    {
        idle,
        delay,
        attack,
        hold,
        decay,
        sustain,
        release
    };

    void advanceTo (Stage newStage) noexcept
    {
        stage = newStage;
        stageSample = 0;
    }

    int toSamples (float seconds) const noexcept
    {
        return yup::jmax (1, static_cast<int> (static_cast<double> (seconds) * sampleRate));
    }

    //==============================================================================
    Stage stage = Stage::idle;

    double sampleRate = 44100.0;
    float level = 0.0f;
    float sustainLevel = 0.7f;
    float attackIncrement = 1.0f;
    float decayIncrement = 1.0f;
    float releaseIncrement = 1.0f;

    int delaySamples = 1;
    int holdSamples = 1;
    int releaseSamples = 1;
    int stageSample = 0;
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

    The partial editor writes magnitudes continuously while the mouse is down but bumps
    harmonicGeneration at most once per user interface frame. The audio thread rebuilds
    its series only when that counter moves, which keeps a drag from forcing an inverse
    FFT per mouse event on every sounding voice.

    @see SynthOscillator, WaveformEditor
*/
struct SynthOscillatorSettings
{
    SynthOscillatorSettings()
    {
        for (auto& harmonic : harmonics)
            harmonic.store (0.0f);
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

    std::array<std::atomic<float>, SynthExample::editableHarmonics> harmonics;
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

    /** Rebuilds a prepared series from the edited magnitudes, without allocating.

        The editor works in magnitudes only and writes them as sine coefficients, the
        same convention yup::FourierSeries::setWaveform uses for its sawtooth, square
        and triangle presets.

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
            const auto magnitude = harmonics[static_cast<std::size_t> (harmonic - 1)].load() * scale;

            series.setHarmonic (harmonic, 0.0, static_cast<double> (magnitude));
        }
    }

    /** Seeds the edited magnitudes from a series, so editing starts at the visible shape. */
    void seedHarmonicsFrom (const yup::FourierSeries<double>& series) noexcept
    {
        const auto count = yup::jmin (SynthExample::editableHarmonics, series.getNumHarmonics());

        for (int harmonic = 1; harmonic <= count; ++harmonic)
            harmonics[static_cast<std::size_t> (harmonic - 1)].store (static_cast<float> (series.getMagnitude (harmonic)));

        for (int harmonic = count; harmonic < SynthExample::editableHarmonics; ++harmonic)
            harmonics[static_cast<std::size_t> (harmonic)].store (0.0f);
    }
};

//==============================================================================
/** Immutable waveform data, prepared once and read by every voice.

    Building a FourierSeries allocates, so it belongs at construction time. Once
    prepared the resources are read-only and safe to share across voices.

    @see SynthOscillator
*/
class SynthOscillatorResources
{
public:
    SynthOscillatorResources()
    {
        const yup::Waveform waveforms[] = { yup::Waveform::sine,
                                            yup::Waveform::cosine,
                                            yup::Waveform::sawtooth,
                                            yup::Waveform::square,
                                            yup::Waveform::triangle,
                                            yup::Waveform::pulse };

        for (std::size_t index = 0; index < frames.size(); ++index)
            frames[index] = yup::FourierSeries<double>::create (waveforms[index], SynthExample::maxHarmonics);
    }

    /** Returns the series of one of the Waveform presets. */
    const yup::FourierSeries<double>& getFrame (yup::Waveform waveform) const noexcept
    {
        return frames[static_cast<std::size_t> (waveform)];
    }

private:
    std::array<yup::FourierSeries<double>, 6> frames;
};

//==============================================================================
/** Turns a control snapshot into the shape yup::PrismSpectrum reads. */
inline yup::PrismSpectrum<double>::Shape toPrismShape (const SynthOscillatorValues& values) noexcept
{
    return { static_cast<double> (values.ridgeSpacing),
             static_cast<double> (values.dispersion),
             static_cast<double> (values.squeeze),
             static_cast<double> (values.squash),
             static_cast<double> (values.tilt),
             static_cast<double> (values.oddEven),
             static_cast<double> (values.formant),
             static_cast<double> (values.formantPosition),
             static_cast<double> (values.scatter) };
}

//==============================================================================
/** Shapes a source through the Prism stages and then through the sync transform.

    The audio thread and the waveform display both derive their series here, so the
    preview cannot drift from what the voices play. yup::PrismSpectrum is stateless
    and shared; the resampler keeps scratch storage, so every caller brings its own.

    @returns The series to play or draw: the shaped one, or the synced one when a
             sync mode is selected.
*/
inline yup::FourierSeries<double>& derivePrismSeries (const yup::PrismSpectrum<double>& spectrum,
                                                      yup::SyncSpectralResampler<double>& resampler,
                                                      const yup::FourierSeries<double>& source,
                                                      yup::FourierSeries<double>& shaped,
                                                      yup::FourierSeries<double>& synced,
                                                      const SynthOscillatorValues& values) noexcept
{
    spectrum.process (source, shaped, toPrismShape (values), values.color);

    if (values.syncMode == yup::SyncMode::none)
        return shaped;

    resampler.transform (shaped, static_cast<double> (values.syncRatio), values.syncMode, synced);

    return synced;
}

//==============================================================================
/** The series every voice of one oscillator slot plays, rebuilt once per block.

    A slot's spectrum does not vary per voice: the waveform, the edited partials, the
    Prism shape and the sync settings are all slot-wide. Deriving them once here rather
    than inside each voice is what makes every control modulatable - shaping 128
    harmonics is cheap, the sync transform less so, and neither should run once per
    voice per block.

    The sync transform is run for every harmonic the slot renders rather than for one
    voice's Nyquist limit, which is what lets a single derivation serve voices at
    different pitches: each voice's wavetable drops what its own pitch cannot carry.

    Voices publish nothing back; they compare getGeneration() and re-render their own
    table when it moves, which keeps yup::WavetableOscillator's crossfade doing the
    smoothing. Everything here runs on the audio thread at the top of a block, before
    any voice reads it, so no publication handshake is needed. Nothing allocates.

    @see SynthOscillator, derivePrismSeries
*/
class SynthOscillatorSlot
{
public:
    SynthOscillatorSlot()
    {
        spectrum.prepare (SynthExample::maxHarmonics);
        resampler.prepare (SynthExample::maxHarmonics);
        customSeries.resize (SynthExample::maxHarmonics);
        shapedSeries.resize (SynthExample::maxHarmonics);
        syncedSeries.resize (SynthExample::maxHarmonics);

        // The meter is only ever asked for a waveform, never played, and a frequency of
        // zero keeps every harmonic whatever rate it was prepared at.
        peakMeter.prepare (48000.0, SynthExample::maxHarmonics);
        peakMeter.setFrequency (0.0);
        peakMeter.setIncludeDC (true);
    }

    /** Rebuilds the slot's series if anything it depends on moved. Audio thread. */
    void update (const SynthOscillatorValues& values,
                 const SynthOscillatorSettings& settings,
                 const SynthOscillatorResources& resources) noexcept
    {
        const auto partialsChanged = ! hasApplied
                                  || applied.usesCustomSeries != values.usesCustomSeries
                                  || applied.harmonicGeneration != values.harmonicGeneration
                                  || applied.harmonicScale != values.harmonicScale;

        if (partialsChanged && values.usesCustomSeries)
            settings.copyHarmonicsInto (customSeries, values.harmonicScale);

        const auto& source = values.usesCustomSeries ? customSeries : resources.getFrame (values.waveform);
        const auto sourceChanged = partialsChanged || applied.waveform != values.waveform;

        const auto shapeChanged = applied.syncMode != values.syncMode
                               || applied.syncRatio != values.syncRatio
                               || applied.ridgeSpacing != values.ridgeSpacing
                               || applied.color != values.color
                               || applied.dispersion != values.dispersion
                               || applied.squeeze != values.squeeze
                               || applied.squash != values.squash
                               || applied.tilt != values.tilt
                               || applied.oddEven != values.oddEven
                               || applied.formant != values.formant
                               || applied.formantPosition != values.formantPosition
                               || applied.scatter != values.scatter;

        if (sourceChanged)
            sourcePeak = measurePeak (source);

        if (sourceChanged || shapeChanged)
        {
            auto& derived = derivePrismSeries (spectrum, resampler, source, shapedSeries, syncedSeries, values);

            matchSourcePeak (derived);
            published = &derived;
            ++generation;
        }

        applied = values;
        hasApplied = true;
    }

    /** Returns the series the voices of this slot should be playing. */
    const yup::FourierSeries<double>& getSeries() const noexcept { return *published; }

    /** Bumped whenever getSeries() changed, so a voice knows to re-render its table. */
    int getGeneration() const noexcept { return generation; }

    /** The shaper, which the waveform display reuses for its preview. */
    const yup::PrismSpectrum<double>& getSpectrum() const noexcept { return spectrum; }

private:
    /** Points the display samples the peak search walks. Twice the harmonic count
        resolves the highest harmonic; this is four times it, for a little margin. */
    static constexpr int peakResolution = 512;

    /** Returns the largest absolute value one period of a series reaches.

        The series is rendered with the same inverse FFT the voices use rather than
        summed harmonic by harmonic, which would cost a transcendental per harmonic per
        point. One transform per slot per block is nothing beside the one each sounding
        voice already pays.
    */
    double measurePeak (const yup::FourierSeries<double>& series) noexcept
    {
        peakMeter.setSeries (series);
        peakMeter.render (false);

        auto peak = 0.0;

        for (int index = 0; index < peakResolution; ++index)
        {
            const auto phase = static_cast<double> (index) / static_cast<double> (peakResolution);

            peak = yup::jmax (peak, std::abs (static_cast<double> (peakMeter.getValueAtPhase (phase))));
        }

        return peak;
    }

    /** Rescales a derived series so its waveform peaks where the source's does.

        yup::PrismSpectrum preserves the coefficient sum, which bounds a peak from well
        above - three times over for a sawtooth - and how close the waveform comes to
        that bound depends on how aligned the harmonic phases are. Dispersion, scatter
        and the sync reset all move exactly that, so without this they would swing the
        output level as they are swept rather than only recolouring it.
    */
    void matchSourcePeak (yup::FourierSeries<double>& series) noexcept
    {
        const auto derivedPeak = measurePeak (series);

        if (derivedPeak <= 1.0e-9 || sourcePeak <= 1.0e-9)
            return;

        const auto scale = sourcePeak / derivedPeak;

        for (int harmonic = 1; harmonic <= series.getNumHarmonics(); ++harmonic)
            series.setHarmonic (harmonic,
                                series.getCosine (harmonic) * scale,
                                series.getSine (harmonic) * scale);
    }

    yup::PrismSpectrum<double> spectrum;
    yup::SyncSpectralResampler<double> resampler;
    yup::WavetableOscillator<float> peakMeter;
    double sourcePeak = 0.0;
    yup::FourierSeries<double> customSeries;
    yup::FourierSeries<double> shapedSeries;
    yup::FourierSeries<double> syncedSeries;
    const yup::FourierSeries<double>* published = &shapedSeries;
    SynthOscillatorValues applied;
    int generation = 0;
    bool hasApplied = false;
};

//==============================================================================
/** One of a voice's oscillators: a wavetable playing the slot's series, plus unison.

    prepare() allocates the backends; renderBlock() is allocation-free and only
    re-renders a table when the slot's series moved, so editing a control is the only
    thing that pays for an inverse FFT.

    Unison is built from bare yup::WavetableOscillator satellites playing the same
    series as the center, detuned and panned around it.

    @see SynthOscillatorSettings, SynthOscillatorSlot
*/
class SynthOscillator
{
public:
    /** Allocates every backend and attaches the shared slot. */
    void prepare (double newSampleRate, int maxBlockSize, const SynthOscillatorSlot& sharedSlot)
    {
        const auto sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        slot = &sharedSlot;

        wavetable.prepare (sampleRate, SynthExample::maxHarmonics);

        for (auto& satellite : satellites)
            satellite.prepare (sampleRate, SynthExample::maxHarmonics);

        slotBuffer.assign (static_cast<std::size_t> (yup::jmax (1, maxBlockSize)), 0.0f);

        appliedSeriesGeneration = -1;
    }

    /** Restarts every backend, spreading the satellites so they do not stack in phase. */
    void reset (double initialPhase) noexcept
    {
        const auto phase = static_cast<float> (initialPhase);

        wavetable.setPhase (phase);

        for (std::size_t index = 0; index < satellites.size(); ++index)
        {
            const auto offset = static_cast<float> (index + 1) / static_cast<float> (satellites.size() + 1);

            satellites[index].setPhase (phase + offset - std::floor (phase + offset));
        }
    }

    /** Applies the pending changes and writes one stereo block.

        The buffers are overwritten rather than added to, so the caller does not have to
        clear them first.
    */
    void renderBlock (float* left,
                      float* right,
                      int numSamples,
                      const SynthOscillatorValues& values,
                      double frequency) noexcept
    {
        yup::FloatVectorOperations::clear (left, numSamples);
        yup::FloatVectorOperations::clear (right, numSamples);

        // The table holds one period of the synced waveform, which for mirrored sync is
        // two leader periods, so it is played at the fundamental the transform produced.
        const auto played = frequency * yup::SyncSpectralResampler<double>::getFundamentalScale (values.syncMode);

        const auto slotCount = yup::jlimit (1, SynthExample::maxUnisonVoices, values.unisonVoices);
        const auto centreIndex = (slotCount - 1) / 2;
        const auto slotGain = 1.0f / static_cast<float> (slotCount);

        applySeries();

        renderSatellite (wavetable, numSamples, detunedFrequency (played, values, centreIndex, slotCount));
        accumulateSlot (left, right, numSamples, slotOffset (centreIndex, slotCount) * values.unisonSpread, slotGain);

        for (int index = 0, satellite = 0; index < slotCount; ++index)
        {
            if (index == centreIndex)
                continue;

            renderSatellite (satellites[static_cast<std::size_t> (satellite++)],
                             numSamples,
                             detunedFrequency (played, values, index, slotCount));

            accumulateSlot (left, right, numSamples, slotOffset (index, slotCount) * values.unisonSpread, slotGain);
        }
    }

private:
    //==============================================================================
    /** Returns where a unison slot sits across the spread, from -1 to 1. */
    static float slotOffset (int slotIndex, int slotCount) noexcept
    {
        if (slotCount <= 1)
            return 0.0f;

        return 2.0f * static_cast<float> (slotIndex) / static_cast<float> (slotCount - 1) - 1.0f;
    }

    /** Combines the oscillator's own detune with the slot's share of the unison spread. */
    static double detunedFrequency (double frequency, const SynthOscillatorValues& values, int slotIndex, int slotCount) noexcept
    {
        const auto semitones = static_cast<double> (values.unisonDetune) * static_cast<double> (slotOffset (slotIndex, slotCount));

        return frequency * std::pow (2.0, semitones / 12.0);
    }

    /** Mixes the rendered slot into the stereo pair with an equal power pan. */
    void accumulateSlot (float* left, float* right, int numSamples, float pan, float gain) noexcept
    {
        const auto angle = (yup::jlimit (-1.0f, 1.0f, pan) + 1.0f) * 0.25f * yup::MathConstants<float>::pi;
        const auto leftGain = std::cos (angle) * gain;
        const auto rightGain = std::sin (angle) * gain;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto value = slotBuffer[static_cast<std::size_t> (sample)];

            left[sample] += value * leftGain;
            right[sample] += value * rightGain;
        }
    }

    /** Renders one unison slot; render() crossfades into a new table, which is what
        keeps a modulated shape from stepping. */
    void renderSatellite (yup::WavetableOscillator<float>& satellite, int numSamples, double frequency) noexcept
    {
        satellite.setFrequency (frequency);

        if (satellite.needsRender())
            satellite.render();

        satellite.processBlock (slotBuffer.data(), numSamples);
    }

    /** Hands the slot's series to every table when it moved since the last block. */
    void applySeries() noexcept
    {
        const auto generation = slot->getGeneration();

        if (generation == appliedSeriesGeneration)
            return;

        appliedSeriesGeneration = generation;

        const auto& series = slot->getSeries();

        wavetable.setSeries (series);

        for (auto& satellite : satellites)
            satellite.setSeries (series);
    }

    //==============================================================================
    yup::WavetableOscillator<float> wavetable;
    std::array<yup::WavetableOscillator<float>, SynthExample::maxUnisonVoices - 1> satellites;

    const SynthOscillatorSlot* slot = nullptr;
    std::vector<float> slotBuffer;
    int appliedSeriesGeneration = -1;
};

//==============================================================================
/** The single sound the example synthesiser plays. */
class SynthSound : public yup::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
/** A polyphonic voice made of two independently configurable oscillators. */
class SynthVoice : public yup::SynthesiserVoice
{
public:
    SynthVoice (const std::array<SynthOscillatorSettings, SynthExample::oscillatorCount>& oscillatorSettings,
                const SynthEnvelopeSettings& sharedEnvelopeSettings,
                const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& sharedSlots)
        : settings (oscillatorSettings)
        , envelopeSettings (sharedEnvelopeSettings)
        , slots (sharedSlots)
    {
    }

    /** Allocates every oscillator backend. Must run outside the audio callback. */
    void prepare (double sampleRate, int maxBlockSize)
    {
        for (std::size_t slot = 0; slot < oscillators.size(); ++slot)
            oscillators[slot].prepare (sampleRate, maxBlockSize, slots[slot]);

        for (auto& level : levels)
            level.reset (sampleRate, SynthExample::levelRampSeconds);

        envelope.prepare (sampleRate);
        playbackRate = sampleRate;
        hasPlayed = false;
        preserveNote = false;
        glideSeconds = 0.0;
        pitch.setCurrentAndTargetValue (69.0);
        bend.reset (sampleRate, SynthExample::levelRampSeconds);
        bend.setCurrentAndTargetValue (0.0);
        tailLength = yup::jlimit (2, yup::jmax (2, maxBlockSize), static_cast<int> (sampleRate * 0.006));
        tailBuffer.setSize (2, tailLength);
        tailScratch.setSize (2, tailLength);
        tailPosition = tailLength;
        clearCurrentNote();

        const auto blockSize = static_cast<std::size_t> (yup::jmax (1, maxBlockSize));

        oscLeft.assign (blockSize, 0.0f);
        oscRight.assign (blockSize, 0.0f);
        mixLeft.assign (blockSize, 0.0f);
        mixRight.assign (blockSize, 0.0f);
    }

    //==============================================================================
    bool canPlaySound (yup::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*> (sound) != nullptr;
    }

    /** Configures the next mono transition. Legato retains the envelope and phases.
        Glide is measured in seconds and interpolates pitch in semitones. */
    void setTransition (double seconds, bool legato) noexcept
    {
        glideSeconds = yup::jlimit (0.0, 2.0, seconds);
        preserveNote = legato && envelope.isActive();
    }

    void startNote (int midiNoteNumber, float velocity, yup::SynthesiserSound*, int currentPitchWheelPosition) override
    {
        const auto currentPitch = pitch.getCurrentValue();
        pitch.reset (playbackRate, glideSeconds);
        pitch.setCurrentAndTargetValue (currentPitch);
        if (glideSeconds > 0.0 && hasPlayed)
            pitch.setTargetValue (static_cast<double> (midiNoteNumber));
        else
            pitch.setCurrentAndTargetValue (static_cast<double> (midiNoteNumber));

        pitchWheelMoved (currentPitchWheelPosition);

        if (! preserveNote)
        {
            velocityGain = yup::jlimit (0.0f, 1.0f, velocity);
            for (auto& oscillator : oscillators)
                oscillator.reset (0.0);

            envelope.setParameters (envelopeSettings.read());
            envelope.noteOn();
        }

        preserveNote = false;
        hasPlayed = true;
        glideSeconds = 0.0;
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            envelope.noteOff();
            return;
        }

        if (! preserveNote)
        {
            if (envelope.isActive())
            {
                tailScratch.clear();
                renderNextBlock (tailScratch, 0, tailLength);
                for (int channel = 0; channel < 2; ++channel)
                    for (int sample = 0; sample < tailLength; ++sample)
                    {
                        const auto fade = 0.5 + 0.5 * std::cos (yup::MathConstants<double>::pi
                                                              * sample / (tailLength - 1));
                        tailBuffer.setSample (channel, sample, tailScratch.getSample (channel, sample) * static_cast<float> (fade));
                    }
                tailPosition = 0;
            }
            envelope.noteOffImmediate();
        }
        clearCurrentNote();
    }

    void pitchWheelMoved (int newPitchWheelValue) override
    {
        const auto normalized = (static_cast<double> (newPitchWheelValue) - 8192.0) / 8192.0;
        bend.setTargetValue (normalized * pitchWheelRangeSemitones);
    }

    /** Includes a recycled voice's short continuation in the activity meter. */
    bool isSounding() const noexcept { return isVoiceActive() || tailPosition < tailLength; }

    void controllerMoved (int, int) override {}

    //==============================================================================
    void renderNextBlock (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (! isSounding() || numSamples <= 0 || outputBuffer.getNumChannels() == 0)
            return;

        for (std::size_t index = 0; index < blockValues.size(); ++index)
            blockValues[index] = settings[index].read();
        envelope.setParameters (envelopeSettings.read());

        for (int offset = 0; offset < numSamples;)
        {
            const auto controlBlock = pitch.isSmoothing() || bend.isSmoothing() ? 128 : numSamples;
            const auto count = yup::jmin (numSamples - offset, static_cast<int> (mixLeft.size()), controlBlock);
            if (count <= 0)
                return;
            renderChunk (outputBuffer, startSample + offset, count);
            offset += count;
        }
    }

private:
    //==============================================================================
    void renderChunk (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) noexcept
    {
        yup::FloatVectorOperations::clear (mixLeft.data(), numSamples);
        yup::FloatVectorOperations::clear (mixRight.data(), numSamples);

        if (isVoiceActive())
        {
            const auto frequency = midiNoteToFrequency (pitch.skip (numSamples) + bend.skip (numSamples));
            for (int index = 0; index < SynthExample::oscillatorCount; ++index)
            {
                const auto slot = static_cast<std::size_t> (index);
                const auto& values = blockValues[slot];
                auto& level = levels[slot];
                const auto detuned = frequency * std::exp2 (values.octave + values.detuneSemitones / 12.0);
                oscillators[slot].renderBlock (oscLeft.data(), oscRight.data(), numSamples, values, detuned);
                level.setTargetValue (values.level);

                for (int sample = 0; sample < numSamples; ++sample)
                {
                    const auto gain = level.getNextValue();
                    mixLeft[static_cast<std::size_t> (sample)] += oscLeft[static_cast<std::size_t> (sample)] * gain;
                    mixRight[static_cast<std::size_t> (sample)] += oscRight[static_cast<std::size_t> (sample)] * gain;
                }
            }
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto gain = envelope.getNextValue() * velocityGain;
            auto left = mixLeft[static_cast<std::size_t> (sample)] * gain;
            auto right = mixRight[static_cast<std::size_t> (sample)] * gain;
            if (tailPosition < tailLength)
            {
                left += tailBuffer.getSample (0, tailPosition);
                right += tailBuffer.getSample (1, tailPosition++);
            }

            if (outputBuffer.getNumChannels() == 1)
                outputBuffer.addSample (0, startSample + sample, (left + right) * 0.5f);
            else
            {
                outputBuffer.addSample (0, startSample + sample, left);
                outputBuffer.addSample (1, startSample + sample, right);
            }
        }

        if (! envelope.isActive())
            clearCurrentNote();
    }

    static double midiNoteToFrequency (double midiNoteNumber) noexcept
    {
        return 440.0 * std::pow (2.0, (midiNoteNumber - 69) / 12.0);
    }

    //==============================================================================
    static constexpr double pitchWheelRangeSemitones = 2.0;

    const std::array<SynthOscillatorSettings, SynthExample::oscillatorCount>& settings;
    const SynthEnvelopeSettings& envelopeSettings;
    const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& slots;

    std::array<SynthOscillator, SynthExample::oscillatorCount> oscillators;
    std::array<SynthOscillatorValues, SynthExample::oscillatorCount> blockValues;
    std::array<yup::SmoothedValue<float>, SynthExample::oscillatorCount> levels;

    SynthEnvelope envelope;

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

//==============================================================================
/** Keyboard allocation and envelope behavior. */
enum class SynthPlayMode
{
    poly,   /**< Eight voices with rendered release tails when recycled. */
    mono,   /**< Last-note priority, retriggering the envelope on each note. */
    legato  /**< Overlapping notes preserve phases and envelope; glide is optional. */
};

/** Polyphonic synthesiser rendering the two oscillators of every voice.
    MIDI and rendering methods belong to the audio thread; the UI edits atomic controls. */
class HarmonicSynthEngine : public yup::Synthesiser
{
public:
    HarmonicSynthEngine()
    {
        addSound (new SynthSound());
        setMinimumRenderingSubdivisionSize (1, true);
        settings[0].color = 0.2f;
        settings[1].waveform = static_cast<int> (yup::Waveform::triangle);
        settings[1].syncMode = static_cast<int> (yup::SyncMode::hard);
        settings[1].octave = -1;
        settings[1].level = 0.3f;

        for (int index = 0; index < SynthExample::voiceCount; ++index)
        {
            auto voice = yup::ReferenceCountedObjectPtr<SynthVoice> (new SynthVoice (settings, envelopeSettings, oscillatorSlots));

            addVoice (voice);
            ownedVoices.add (voice);
        }
    }

    /** Prepares every voice. Must run outside the audio callback. */
    void prepare (double sampleRate, int maxBlockSize)
    {
        allNotesOff (0, false);
        setCurrentPlaybackSampleRate (sampleRate);
        activeVoices.store (0);

        for (int index = 0; index < ownedVoices.size(); ++index)
            ownedVoices[index]->prepare (sampleRate, maxBlockSize);
    }

    /** UI-facing performance controls, sampled at the next render boundary. */
    std::atomic<int> playMode { static_cast<int> (SynthPlayMode::poly) };
    std::atomic<float> portamento { 0.12f };

    /** Requests a release of all keys without taking the synthesiser lock on the UI thread. */
    void requestAllNotesOff() noexcept { releaseRequested.store (true); }

    /** Renders MIDI with sample-accurate note boundaries and publishes the voice meter. */
    void renderNextBlock (yup::AudioBuffer<float>& output, const yup::MidiBuffer& midi, int start, int count)
    {
        const auto requestedMode = static_cast<SynthPlayMode> (playMode.load());
        const auto release = releaseRequested.exchange (false);
        if (requestedMode != mode || release)
        {
            allNotesOff (0, true);
            mode = requestedMode;
        }
        // Every voice of a slot plays the same spectrum, so it is derived once here
        // rather than once per voice: that is what lets the Prism and sync controls be
        // modulated without paying for the shaping eight times over.
        for (std::size_t slot = 0; slot < oscillatorSlots.size(); ++slot)
            oscillatorSlots[slot].update (settings[slot].read(), settings[slot], resources);

        yup::Synthesiser::renderNextBlock (output, midi, start, count);
        int active = 0;
        for (auto* voice : ownedVoices)
            active += voice->isSounding() ? 1 : 0;
        activeVoices.store (active);
    }

    void noteOn (int channel, int note, float velocity) override
    {
        if (mode == SynthPlayMode::poly)
        {
            yup::Synthesiser::noteOn (channel, note, velocity);
            return;
        }
        auto& held = heldNotes[static_cast<std::size_t> ((channel - 1) * 128 + note)];
        held = { ++noteOrder, velocity, true };
        playMonoNote (channel, note, velocity);
    }

    void noteOff (int channel, int note, float velocity, bool tailOff) override
    {
        if (mode == SynthPlayMode::poly)
        {
            yup::Synthesiser::noteOff (channel, note, velocity, tailOff);
            return;
        }
        auto& held = heldNotes[static_cast<std::size_t> ((channel - 1) * 128 + note)];
        held.down = false;
        if (! sustain[static_cast<std::size_t> (channel - 1)] || ! tailOff)
            held.order = 0;
        selectMonoNote (tailOff);
    }

    void allNotesOff (int channel, bool tailOff) override
    {
        for (int index = 0; index < static_cast<int> (heldNotes.size()); ++index)
            if (channel <= 0 || index / 128 == channel - 1)
                heldNotes[static_cast<std::size_t> (index)] = {};
        for (int index = 0; index < 16; ++index)
            if (channel <= 0 || index == channel - 1)
                sustain[static_cast<std::size_t> (index)] = false;
        yup::Synthesiser::allNotesOff (channel, tailOff);
        if (mode != SynthPlayMode::poly)
            selectMonoNote (tailOff);
    }

    void handleController (int channel, int controller, int value) override
    {
        if (mode != SynthPlayMode::poly && controller == 64)
        {
            sustain[static_cast<std::size_t> (channel - 1)] = value >= 64;
            if (value < 64)
            {
                for (int note = 0; note < 128; ++note)
                {
                    auto& held = heldNotes[static_cast<std::size_t> ((channel - 1) * 128 + note)];
                    if (! held.down)
                        held.order = 0;
                }
                selectMonoNote (true);
            }
            return;
        }
        yup::Synthesiser::handleController (channel, controller, value);
    }

    /** Returns the settings edited by one of the user interface panels. */
    SynthOscillatorSettings& getOscillatorSettings (int oscillatorIndex) noexcept
    {
        return settings[static_cast<std::size_t> (oscillatorIndex)];
    }

    /** Returns the settings edited by the envelope panel. */
    SynthEnvelopeSettings& getEnvelopeSettings() noexcept { return envelopeSettings; }

    /** Returns the shared waveform presets, which the waveform displays also read. */
    const SynthOscillatorResources& getResources() const noexcept { return resources; }

    /** Returns the shared series derivation of one oscillator slot. */
    SynthOscillatorSlot& getOscillatorSlot (int oscillatorIndex) noexcept
    {
        return oscillatorSlots[static_cast<std::size_t> (oscillatorIndex)];
    }

    /** Returns the note of a sounding voice, or -1 when the synthesiser is silent. */
    int getCurrentlyPlayingNote() const noexcept
    {
        for (int index = 0; index < ownedVoices.size(); ++index)
            if (ownedVoices[index] != nullptr && ownedVoices[index]->isVoiceActive())
                return ownedVoices[index]->getCurrentlyPlayingNote();

        return -1;
    }

    /** Returns how many voices are currently sounding. */
    int getNumActiveVoices() const noexcept { return activeVoices.load(); }

private:
    void playMonoNote (int channel, int note, float velocity)
    {
        auto* voice = ownedVoices[0].get();
        const auto overlapping = voice->isVoiceActive() && monoKeyActive;
        voice->setTransition (overlapping ? portamento.load() : 0.0,
                              overlapping && mode == SynthPlayMode::legato);
        startVoice (voice, getSound (0).get(), channel, note, velocity);
        monoKeyActive = true;
    }

    void selectMonoNote (bool tailOff)
    {
        int latest = -1;
        for (int index = 0; index < static_cast<int> (heldNotes.size()); ++index)
            if (heldNotes[static_cast<std::size_t> (index)].order != 0
                && (latest < 0 || heldNotes[static_cast<std::size_t> (index)].order > heldNotes[static_cast<std::size_t> (latest)].order))
                latest = index;

        auto* voice = ownedVoices[0].get();
        if (latest < 0)
        {
            if (monoKeyActive)
                voice->stopNote (0.0f, tailOff);
            monoKeyActive = false;
            return;
        }
        const auto channel = latest / 128 + 1;
        const auto note = latest % 128;
        if (! voice->isVoiceActive() || voice->getCurrentlyPlayingNote() != note || ! voice->isPlayingChannel (channel))
            playMonoNote (channel, note, heldNotes[static_cast<std::size_t> (latest)].velocity);
    }

    struct HeldNote
    {
        yup::uint64 order = 0;
        float velocity = 0.0f;
        bool down = false;
    };

    std::array<HeldNote, 16 * 128> heldNotes {};
    std::array<bool, 16> sustain {};
    yup::uint64 noteOrder = 0;
    SynthPlayMode mode = SynthPlayMode::poly;
    bool monoKeyActive = false;
    std::atomic<bool> releaseRequested { false };
    std::atomic<int> activeVoices { 0 };

    SynthOscillatorResources resources;
    std::array<SynthOscillatorSlot, SynthExample::oscillatorCount> oscillatorSlots;
    std::array<SynthOscillatorSettings, SynthExample::oscillatorCount> settings;
    SynthEnvelopeSettings envelopeSettings;
    yup::ReferenceCountedArray<SynthVoice> ownedVoices;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicSynthEngine)
};

//==============================================================================
/** The palette the synthesiser panels share.

    The example draws its own chrome rather than leaning on the theme, so that the
    oscillator, envelope and display panels read as one instrument.
*/
namespace SynthTheme
{
inline constexpr yup::Color windowBackground { 0xff16191d };
inline constexpr yup::Color panelBackground { 0xff21262c };
inline constexpr yup::Color panelBorder { 0xff2e353d };
inline constexpr yup::Color displayBackground { 0xff0e1114 };
inline constexpr yup::Color accent { 0xff72ead2 };
inline constexpr yup::Color accentDim { 0xff287f78 };
inline constexpr yup::Color textPrimary { 0xffe6ebf0 };
inline constexpr yup::Color textSecondary { 0xff8b96a0 };

constexpr float panelCorner = 6.0f;
} // namespace SynthTheme

/** @internal Paints the rounded frame every panel of the instrument sits in. */
inline void paintSynthPanel (yup::Graphics& g, yup::Rectangle<float> bounds)
{
    g.setFillColor (SynthTheme::panelBackground);
    g.fillRoundedRect (bounds, SynthTheme::panelCorner);

    g.setStrokeColor (SynthTheme::panelBorder);
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), SynthTheme::panelCorner);
}

//==============================================================================
/** A rotary knob with its caption underneath. */
class KnobControl : public yup::Component
{
public:
    KnobControl (const yup::String& caption,
                 double minimum,
                 double maximum,
                 double interval,
                 double defaultValue,
                 const yup::Font& font)
        : slider (yup::Slider::RotaryVerticalDrag)
    {
        setOpaque (false); // the knob and its caption paint themselves, the row draws nothing

        slider.setRange (minimum, maximum, interval);
        slider.setDefaultValue (defaultValue);
        slider.setValue (defaultValue, yup::dontSendNotification);
        slider.setColor (yup::Slider::Style::backgroundColorId, SynthTheme::displayBackground);
        slider.setColor (yup::Slider::Style::trackColorId, SynthTheme::accent);
        slider.setColor (yup::Slider::Style::thumbColorId, SynthTheme::textPrimary);
        slider.setColor (yup::Slider::Style::thumbOverColorId, SynthTheme::accent);
        slider.setColor (yup::Slider::Style::thumbDownColorId, SynthTheme::accent);
        slider.onValueChanged = [this] (double value)
        {
            if (onChange != nullptr)
                onChange (value);
        };
        addAndMakeVisible (slider);

        label.setText (caption, yup::dontSendNotification);
        label.setFont (font);
        label.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        addAndMakeVisible (label);
    }

    /** Called with the new knob value. */
    std::function<void (double)> onChange;

    yup::Slider& getSlider() noexcept { return slider; }

    void resized() override
    {
        auto bounds = getLocalBounds();

        label.setBounds (bounds.removeFromBottom (captionHeight));

        const auto size = yup::jmin (bounds.getWidth(), bounds.getHeight());

        slider.setBounds (bounds.withSizeKeepingCenter (size, size));
    }

private:
    static constexpr float captionHeight = 13.0f;

    yup::Slider slider;
    yup::Label label;
};

//==============================================================================
/** A combo box with its caption above it. */
class ChoiceControl : public yup::Component
{
public:
    ChoiceControl (const yup::String& caption, const yup::StringArray& items, const yup::Font& font)
    {
        setOpaque (false); // the caption and combo box paint themselves, the row draws nothing

        label.setText (caption, yup::dontSendNotification);
        label.setFont (font);
        label.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        addAndMakeVisible (label);

        comboBox.addItemList (items, 1);
        comboBox.setTextWhenNothingSelected ("-");
        comboBox.setColor (yup::ComboBox::Style::backgroundColorId, SynthTheme::displayBackground);
        comboBox.setColor (yup::ComboBox::Style::textColorId, SynthTheme::textPrimary);
        comboBox.setColor (yup::ComboBox::Style::borderColorId, SynthTheme::panelBorder);
        comboBox.setColor (yup::ComboBox::Style::arrowColorId, SynthTheme::accent);
        comboBox.onSelectedItemChanged = [this]
        {
            if (onChange != nullptr)
                onChange (comboBox.getSelectedId());
        };
        addAndMakeVisible (comboBox);
    }

    /** Called with the 1-based identifier of the newly selected item. */
    std::function<void (int)> onChange;

    yup::ComboBox& getComboBox() noexcept { return comboBox; }

    void resized() override
    {
        auto bounds = getLocalBounds();

        label.setBounds (bounds.removeFromTop (captionHeight));
        comboBox.setBounds (bounds);
    }

private:
    static constexpr float captionHeight = 13.0f;

    yup::Label label;
    yup::ComboBox comboBox;
};

//==============================================================================
/** The waveform of one oscillator, either drawn or edited a partial at a time.

    In drawing mode the component reconstructs the series and shows one period of it.
    In editing mode it shows the magnitude of each harmonic as a bar that can be
    dragged, which is what actually defines the waveform the oscillator renders.

    Dragging writes straight into the settings, but the generation counter the audio
    thread watches is only bumped by commitPendingEdits(), once per user interface
    frame. Without that, one drag would queue an inverse FFT per mouse event on every
    sounding voice.

    @see SynthOscillatorSettings
*/
class WaveformEditor : public yup::Component
{
public:
    WaveformEditor (SynthOscillatorSettings& settingsToEdit,
                    const SynthOscillatorResources& sharedResources,
                    const SynthOscillatorSlot& sharedSlot)
        : settings (settingsToEdit)
        , resources (sharedResources)
        , slot (sharedSlot)
    {
        resampler.prepare (SynthExample::maxHarmonics);
        displaySeries.resize (SynthExample::maxHarmonics);
        shapedSeries.resize (SynthExample::maxHarmonics);
        syncedSeries.resize (SynthExample::maxHarmonics);
        displaySamples.assign (displayResolution, 0.0f);

        refresh();
    }

    /** Called whenever a drag changed the partials, so the panel can follow along. */
    std::function<void()> onPartialsChanged;

    /** Switches between drawing the waveform and editing its partials. */
    void setEditingPartials (bool shouldEdit)
    {
        editingPartials = shouldEdit;
        repaint();
    }

    bool isEditingPartials() const noexcept { return editingPartials; }

    /** Rereads the series from the settings, following a preset or randomize change. */
    void refresh()
    {
        const auto usesCustomSeries = settings.usesCustomSeries.load();

        if (usesCustomSeries)
            settings.copyHarmonicsInto (displaySeries, 1.0f);
        else
            displaySeries.copyFrom (resources.getFrame (static_cast<yup::Waveform> (settings.waveform.load())));

        reconstruct (displaySeries);

        // The reconstruction is measured here, on the message thread, so the audio thread
        // never has to work out how loud an edited spectrum turned out to be. This is the
        // source's peak, which is a different quantity from the drawn waveform's below.
        if (usesCustomSeries)
        {
            const auto sourcePeak = measurePeak();

            if (sourcePeak > 1.0e-6f)
                settings.harmonicScale.store (1.0f / sourcePeak);
        }

        // Shaping and sync have no inverse FFT behind them, so the preview follows every
        // control live, derived exactly as the voices derive theirs once per block.
        reconstruct (derivePrismSeries (slot.getSpectrum(), resampler, displaySeries, shapedSeries, syncedSeries, settings.read()));

        // Normalize whatever is actually drawn. The shaper preserves the coefficient sum
        // rather than the peak, and a sum bounds a peak from well above - three times over
        // for a sawtooth - so scaling the derived waveform by the source's peak would draw
        // it clean outside the display.
        const auto peak = measurePeak();

        if (peak > 1.0e-6f)
        {
            const auto scale = 1.0f / peak;

            for (auto& sample : displaySamples)
                sample *= scale;
        }

        repaint();
    }

    /** Sums a series into the display buffer at the display's resolution. */
    void reconstruct (const yup::FourierSeries<double>& series) noexcept
    {
        for (int index = 0; index < displayResolution; ++index)
        {
            const auto phase = static_cast<double> (index) / static_cast<double> (displayResolution - 1);

            displaySamples[static_cast<std::size_t> (index)] =
                evaluateFourierSeries (series, phase, SynthExample::displayHarmonics);
        }
    }

    /** Returns the largest magnitude currently in the display buffer. */
    float measurePeak() const noexcept
    {
        auto peak = 0.0f;

        for (auto sample : displaySamples)
            peak = yup::jmax (peak, std::abs (sample));

        return peak;
    }

    /** Publishes a pending drag to the audio thread, coalescing a frame's worth of edits. */
    void commitPendingEdits()
    {
        if (! pendingEdit)
            return;

        pendingEdit = false;
        settings.harmonicGeneration.fetch_add (1);
    }

    /** Drops the edited partials and returns to the selected waveform preset. */
    void revertToPreset()
    {
        settings.usesCustomSeries.store (false);
        pendingEdit = true;

        refresh();

        if (onPartialsChanged != nullptr)
            onPartialsChanged();
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        g.setFillColor (SynthTheme::displayBackground);
        g.fillRoundedRect (bounds, 4.0f);

        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

        if (editingPartials)
            paintPartials (g, bounds.reduced (contentInset));
        else
            paintWaveform (g, bounds.reduced (contentInset));
    }

    void mouseDown (const yup::MouseEvent& event) override { applyEdit (event); }

    void mouseDrag (const yup::MouseEvent& event) override { applyEdit (event); }

private:
    //==============================================================================
    /** Draws one period of the reconstructed series. */
    void paintWaveform (yup::Graphics& g, yup::Rectangle<float> bounds)
    {
        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeLine (bounds.getX(), bounds.getCenterY(), bounds.getRight(), bounds.getCenterY());

        path.clear();
        path.reserveSpace (displayResolution);

        for (int index = 0; index < displayResolution; ++index)
        {
            const auto x = bounds.getX() + bounds.getWidth() * static_cast<float> (index)
                                             / static_cast<float> (displayResolution - 1);

            const auto y = bounds.getCenterY() - displaySamples[static_cast<std::size_t> (index)] * bounds.getHeight() * 0.45f;

            if (index == 0)
                path.moveTo (x, y);
            else
                path.lineTo (x, y);
        }

        g.setStrokeColor (SynthTheme::accent.withAlpha (0.35f));
        g.setStrokeWidth (4.0f);
        g.setFeather (6.0f);
        g.strokePath (path);

        g.setFeather (0.0f);
        g.setStrokeColor (SynthTheme::accent);
        g.setStrokeWidth (1.5f);
        g.strokePath (path);
    }

    /** Draws the editable magnitude of every harmonic. */
    void paintPartials (yup::Graphics& g, yup::Rectangle<float> bounds)
    {
        const auto barWidth = bounds.getWidth() / static_cast<float> (SynthExample::editableHarmonics);

        for (int index = 0; index < SynthExample::editableHarmonics; ++index)
        {
            const auto magnitude = yup::jlimit (0.0f, 1.0f, static_cast<float> (displaySeries.getMagnitude (index + 1)));
            const auto height = yup::jmax (1.0f, magnitude * bounds.getHeight());
            const auto x = bounds.getX() + barWidth * static_cast<float> (index);

            const yup::Rectangle<float> bar { x + barGap, bounds.getBottom() - height, yup::jmax (1.0f, barWidth - barGap * 2.0f), height };

            g.setFillColor (magnitude > 0.0f ? SynthTheme::accent : SynthTheme::panelBorder);
            g.fillRect (bar);
        }
    }

    //==============================================================================
    /** Turns a mouse position into the magnitude of one harmonic. */
    void applyEdit (const yup::MouseEvent& event)
    {
        if (! editingPartials)
            return;

        const auto bounds = getLocalBounds().reduced (contentInset);

        if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f)
            return;

        // Editing a preset copies its partials in first, so the drag starts from the
        // shape that is on screen instead of from silence.
        if (! settings.usesCustomSeries.load())
        {
            settings.seedHarmonicsFrom (displaySeries);
            settings.usesCustomSeries.store (true);
        }

        const auto position = event.getPosition();
        const auto barWidth = bounds.getWidth() / static_cast<float> (SynthExample::editableHarmonics);
        const auto index = yup::jlimit (0,
                                        SynthExample::editableHarmonics - 1,
                                        static_cast<int> ((position.getX() - bounds.getX()) / barWidth));

        const auto magnitude = yup::jlimit (0.0f, 1.0f, (bounds.getBottom() - position.getY()) / bounds.getHeight());

        settings.harmonics[static_cast<std::size_t> (index)].store (magnitude);
        pendingEdit = true;

        refresh();

        if (onPartialsChanged != nullptr)
            onPartialsChanged();
    }

    //==============================================================================
    static constexpr int displayResolution = 256;
    static constexpr float contentInset = 6.0f;
    static constexpr float barGap = 1.0f;

    SynthOscillatorSettings& settings;
    const SynthOscillatorResources& resources;
    const SynthOscillatorSlot& slot;

    yup::SyncSpectralResampler<double> resampler;
    yup::FourierSeries<double> displaySeries;
    yup::FourierSeries<double> shapedSeries;
    yup::FourierSeries<double> syncedSeries;
    std::vector<float> displaySamples;
    yup::Path path;

    bool editingPartials = false;
    bool pendingEdit = false;
};

//==============================================================================
/** Draws the most recent block of rendered audio as a waveform. */
class Oscilloscope : public yup::Component
{
public:
    Oscilloscope()
        : Component ("Oscilloscope")
    {
    }

    /** Copies the samples to display. Called from the message thread. */
    void setRenderData (const std::vector<float>& data)
    {
        renderData = data;
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        g.setFillColor (SynthTheme::displayBackground);
        g.fillRoundedRect (bounds, 4.0f);

        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);
        g.strokeLine (bounds.getX(), bounds.getCenterY(), bounds.getRight(), bounds.getCenterY());

        if (renderData.empty())
            return;

        const auto pointCount = yup::jmin (512, static_cast<int> (renderData.size()));
        const auto xSize = bounds.getWidth() / static_cast<float> (yup::jmax (1, pointCount - 1));

        path.clear();
        path.reserveSpace (pointCount);
        path.moveTo (bounds.getX(), bounds.getCenterY() - renderData[0] * bounds.getHeight() * 0.45f);

        for (int i = 1; i < pointCount; ++i)
        {
            const auto sample = static_cast<std::size_t> (i) * (renderData.size() - 1) / static_cast<std::size_t> (pointCount - 1);
            path.lineTo (bounds.getX() + static_cast<float> (i) * xSize,
                         bounds.getCenterY() - renderData[sample] * bounds.getHeight() * 0.45f);
        }

        filledPath = path.createStrokePolygon (4.0f);

        g.setFillColor (SynthTheme::accent.withAlpha (0.5f));
        g.setFeather (8.0f);
        g.fillPath (filledPath);

        g.setFeather (4.0f);
        g.fillPath (filledPath);

        g.setFeather (0.0f);
        g.setStrokeColor (SynthTheme::accent);
        g.setStrokeWidth (1.5f);
        g.strokePath (path);
    }

private:
    std::vector<float> renderData;
    yup::Path path;
    yup::Path filledPath;
};

//==============================================================================
/** @internal Spreads controls evenly across a row. */
inline void layoutControlsInRow (yup::Rectangle<float> area, const std::vector<yup::Component*>& controls)
{
    if (controls.empty())
        return;

    const auto width = area.getWidth() / static_cast<float> (controls.size());

    for (auto* control : controls)
        control->setBounds (area.removeFromLeft (width).reduced (3.0f, 0.0f));
}

//==============================================================================
/** Draws the shape the amplitude envelope traces, with a point at every breakpoint. */
class EnvelopeDisplay : public yup::Component
{
public:
    /** Updates the drawn shape. Called from the message thread. */
    void setValues (const SynthEnvelopeValues& newValues)
    {
        values = newValues;
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();

        g.setFillColor (SynthTheme::displayBackground);
        g.fillRoundedRect (bounds, 4.0f);

        g.setStrokeColor (SynthTheme::panelBorder);
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

        const auto area = bounds.reduced (8.0f);

        if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
            return;

        // The sustain stage has no duration of its own, so it is given a fixed share of
        // the width and the timed stages share what is left.
        const auto totalSeconds = yup::jmax (1.0e-4f, values.delay + values.attack + values.hold + values.decay + values.release);
        const auto timedWidth = area.getWidth() * (1.0f - sustainShare);
        const auto secondsToPixels = timedWidth / totalSeconds;

        const auto levelToY = [area] (float level)
        {
            return area.getBottom() - yup::jlimit (0.0f, 1.0f, level) * area.getHeight();
        };

        constexpr int numPoints = 7;

        const yup::Point<float> points[numPoints] = {
            { area.getX(), levelToY (0.0f) },
            { area.getX() + values.delay * secondsToPixels, levelToY (0.0f) },
            { area.getX() + (values.delay + values.attack) * secondsToPixels, levelToY (1.0f) },
            { area.getX() + (values.delay + values.attack + values.hold) * secondsToPixels, levelToY (1.0f) },
            { area.getX() + (values.delay + values.attack + values.hold + values.decay) * secondsToPixels, levelToY (values.sustain) },
            { area.getX() + (values.delay + values.attack + values.hold + values.decay) * secondsToPixels + area.getWidth() * sustainShare, levelToY (values.sustain) },
            { area.getRight(), levelToY (0.0f) }
        };

        path.clear();
        path.reserveSpace (numPoints + 2);
        path.moveTo (points[0]);

        for (int index = 1; index < numPoints; ++index)
            path.lineTo (points[index]);

        g.setStrokeColor (SynthTheme::accent);
        g.setStrokeWidth (1.5f);
        g.strokePath (path);

        for (int index = 1; index < numPoints - 1; ++index)
        {
            g.setFillColor (SynthTheme::accent);
            g.fillEllipse (yup::Rectangle<float> (points[index].getX() - pointRadius,
                                                  points[index].getY() - pointRadius,
                                                  pointRadius * 2.0f,
                                                  pointRadius * 2.0f));
        }
    }

private:
    static constexpr float sustainShare = 0.22f;
    static constexpr float pointRadius = 3.0f;

    SynthEnvelopeValues values;
    yup::Path path;
};

//==============================================================================
/** The editing surface of the amplitude envelope.

    @see SynthEnvelopeSettings
*/
class SynthEnvelopePanel : public yup::Component
{
public:
    SynthEnvelopePanel (SynthEnvelopeSettings& settingsToEdit, const yup::Font& font)
        : settings (settingsToEdit)
        , delayKnob ("DELAY", 0.0, 2.0, 0.001, 0.0, font)
        , attackKnob ("ATTACK", 0.001, 4.0, 0.001, 0.005, font)
        , holdKnob ("HOLD", 0.0, 2.0, 0.001, 0.0, font)
        , decayKnob ("DECAY", 0.001, 4.0, 0.001, 0.35, font)
        , sustainKnob ("SUSTAIN", 0.0, 1.0, 0.001, 0.7, font)
        , releaseKnob ("RELEASE", 0.001, 8.0, 0.001, 0.35, font)
    {
        titleLabel.setText ("ENVELOPE", yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (12.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        addAndMakeVisible (display);

        for (auto* knob : { &delayKnob, &attackKnob, &holdKnob, &decayKnob, &sustainKnob, &releaseKnob })
            addAndMakeVisible (*knob);

        delayKnob.onChange = [this] (double value) { settings.delay = static_cast<float> (value); refreshDisplay(); };
        attackKnob.onChange = [this] (double value) { settings.attack = static_cast<float> (value); refreshDisplay(); };
        holdKnob.onChange = [this] (double value) { settings.hold = static_cast<float> (value); refreshDisplay(); };
        decayKnob.onChange = [this] (double value) { settings.decay = static_cast<float> (value); refreshDisplay(); };
        sustainKnob.onChange = [this] (double value) { settings.sustain = static_cast<float> (value); refreshDisplay(); };
        releaseKnob.onChange = [this] (double value) { settings.release = static_cast<float> (value); refreshDisplay(); };

        refresh();
    }

    /** Reads the settings back into the knobs and the drawn shape. */
    void refresh()
    {
        delayKnob.getSlider().setValue (settings.delay.load(), yup::dontSendNotification);
        attackKnob.getSlider().setValue (settings.attack.load(), yup::dontSendNotification);
        holdKnob.getSlider().setValue (settings.hold.load(), yup::dontSendNotification);
        decayKnob.getSlider().setValue (settings.decay.load(), yup::dontSendNotification);
        sustainKnob.getSlider().setValue (settings.sustain.load(), yup::dontSendNotification);
        releaseKnob.getSlider().setValue (settings.release.load(), yup::dontSendNotification);

        refreshDisplay();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (panelInset);

        titleLabel.setBounds (bounds.removeFromTop (headerHeight));
        bounds.removeFromTop (spacing);

        auto knobArea = bounds.removeFromBottom (knobRowHeight);
        bounds.removeFromBottom (spacing);

        display.setBounds (bounds);

        layoutControlsInRow (knobArea, { &delayKnob, &attackKnob, &holdKnob, &decayKnob, &sustainKnob, &releaseKnob });
    }

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 16.0f;
    static constexpr float knobRowHeight = 58.0f;
    static constexpr float spacing = 6.0f;

    void refreshDisplay() { display.setValues (settings.read()); }

    SynthEnvelopeSettings& settings;

    yup::Label titleLabel;
    EnvelopeDisplay display;

    KnobControl delayKnob;
    KnobControl attackKnob;
    KnobControl holdKnob;
    KnobControl decayKnob;
    KnobControl sustainKnob;
    KnobControl releaseKnob;
};

//==============================================================================
/** The editing surface of one oscillator, writing straight into the voice settings.

    Every widget is wired to a single atomic setting, and refresh() copies the
    settings back into the widgets for changes coming from somewhere else, such as
    the randomize button.

    @see SynthOscillatorSettings, WaveformEditor
*/
class SynthOscillatorPanel : public yup::Component
{
public:
    SynthOscillatorPanel (const yup::String& panelTitle,
                          SynthOscillatorSettings& settingsToEdit,
                          const SynthOscillatorResources& resources,
                          const SynthOscillatorSlot& sharedSlot,
                          const yup::Font& font)
        : settings (settingsToEdit)
        , editor (settingsToEdit, resources, sharedSlot)
        , waveformChoice ("WAVEFORM", getSynthWaveformNames(), font)
        , syncModeChoice ("SYNC", getSynthSyncModeNames(), font)
        , levelKnob ("LEVEL", 0.0, 1.0, 0.001, 0.5, font)
        , octaveKnob ("OCTAVE", -3.0, 3.0, 1.0, 0.0, font)
        , detuneKnob ("CENTS", -100.0, 100.0, 1.0, 0.0, font)
        , ridgesKnob ("RIDGES", 0.25, 8.0, 0.01, 1.5, font)
        , colorKnob ("COLOR", 0.0, 1.0, 0.001, 0.0, font)
        , dispersionKnob ("DISPERSION", 0.01, 0.99, 0.001, 0.5, font)
        , squeezeKnob ("SQUEEZE", 0.0, 0.5, 0.001, 0.0, font)
        , squashKnob ("SQUASH", 0.1, 4.0, 0.01, 1.0, font)
        , tiltKnob ("TILT", -4.0, 4.0, 0.01, 0.0, font)
        , oddEvenKnob ("ODD/EVEN", 0.0, 1.0, 0.001, 0.5, font)
        , formantKnob ("FORMANT", -4.0, 4.0, 0.01, 0.0, font)
        , formantPositionKnob ("F.POS", 0.0, 7.0, 0.01, 2.0, font)
        , scatterKnob ("SCATTER", 0.0, 1.0, 0.001, 0.0, font)
        , syncRatioKnob ("SYNC RATIO", 1.0, 8.0, 0.01, 1.5, font)
        , unisonKnob ("UNISON", 1.0, static_cast<double> (SynthExample::maxUnisonVoices), 1.0, 1.0, font)
        , unisonDetuneKnob ("U.DETUNE", 0.0, 1.0, 0.001, 0.2, font)
        , spreadKnob ("SPREAD", 0.0, 1.0, 0.001, 0.6, font)
    {
        titleLabel.setText (panelTitle, yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (12.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        partialsButton.setButtonText ("PARTIALS");
        partialsButton.setColor (yup::ToggleButton::Style::backgroundColorId, SynthTheme::displayBackground);
        partialsButton.setColor (yup::ToggleButton::Style::backgroundToggledColorId, SynthTheme::accentDim);
        partialsButton.setColor (yup::ToggleButton::Style::textColorId, SynthTheme::textSecondary);
        partialsButton.setColor (yup::ToggleButton::Style::textToggledColorId, SynthTheme::textPrimary);
        partialsButton.setColor (yup::ToggleButton::Style::borderColorId, SynthTheme::panelBorder);
        partialsButton.setColor (yup::ToggleButton::Style::borderToggledColorId, SynthTheme::accent);
        partialsButton.onClick = [this] { editor.setEditingPartials (partialsButton.getToggleState()); };
        addAndMakeVisible (partialsButton);

        resetButton.setColor (yup::TextButton::Style::backgroundColorId, SynthTheme::displayBackground);
        resetButton.setColor (yup::TextButton::Style::textColorId, SynthTheme::textSecondary);
        resetButton.setColor (yup::TextButton::Style::outlineColorId, SynthTheme::panelBorder);
        resetButton.onClick = [this] { editor.revertToPreset(); };
        addAndMakeVisible (resetButton);

        addAndMakeVisible (editor);

        for (auto* choice : { &waveformChoice, &syncModeChoice })
            addAndMakeVisible (*choice);

        for (auto* knob : { &levelKnob, &octaveKnob, &detuneKnob, &ridgesKnob, &colorKnob, &dispersionKnob,
                            &squeezeKnob, &squashKnob, &tiltKnob, &oddEvenKnob, &formantKnob,
                            &formantPositionKnob, &scatterKnob, &syncRatioKnob,
                            &unisonKnob, &unisonDetuneKnob, &spreadKnob })
            addAndMakeVisible (*knob);

        // Picking a preset drops any edited partials, otherwise the oscillator would keep
        // playing the edited shape while the combo box claims something else.
        waveformChoice.onChange = [this] (int id)
        {
            settings.waveform = id - 1;
            editor.revertToPreset();
        };

        syncModeChoice.onChange = [this] (int id)
        {
            settings.syncMode = id - 1;
            updateSyncAvailability();
            editor.refresh();
        };

        levelKnob.onChange = [this] (double value) { settings.level = static_cast<float> (value); };
        octaveKnob.onChange = [this] (double value) { settings.octave = static_cast<int> (value); };
        detuneKnob.onChange = [this] (double value) { settings.detuneSemitones = static_cast<float> (value * 0.01); };
        ridgesKnob.onChange = [this] (double value) { settings.ridgeSpacing = static_cast<float> (value); editor.refresh(); };
        colorKnob.onChange = [this] (double value) { settings.color = static_cast<float> (value); editor.refresh(); };
        dispersionKnob.onChange = [this] (double value) { settings.dispersion = static_cast<float> (value); editor.refresh(); };
        squeezeKnob.onChange = [this] (double value) { settings.squeeze = static_cast<float> (value); editor.refresh(); };
        squashKnob.onChange = [this] (double value) { settings.squash = static_cast<float> (value); editor.refresh(); };
        tiltKnob.onChange = [this] (double value) { settings.tilt = static_cast<float> (value); editor.refresh(); };
        oddEvenKnob.onChange = [this] (double value) { settings.oddEven = static_cast<float> (value); editor.refresh(); };
        formantKnob.onChange = [this] (double value) { settings.formant = static_cast<float> (value); editor.refresh(); };
        formantPositionKnob.onChange = [this] (double value) { settings.formantPosition = static_cast<float> (value); editor.refresh(); };
        scatterKnob.onChange = [this] (double value) { settings.scatter = static_cast<float> (value); editor.refresh(); };
        syncRatioKnob.onChange = [this] (double value) { settings.syncRatio = static_cast<float> (value); editor.refresh(); };
        unisonKnob.onChange = [this] (double value) { settings.unisonVoices = static_cast<int> (value); };
        unisonDetuneKnob.onChange = [this] (double value) { settings.unisonDetune = static_cast<float> (value); };
        spreadKnob.onChange = [this] (double value) { settings.unisonSpread = static_cast<float> (value); };

        refresh();
    }

    /** Reads the settings back into the widgets. */
    void refresh()
    {
        waveformChoice.getComboBox().setSelectedId (settings.waveform.load() + 1, yup::dontSendNotification);
        syncModeChoice.getComboBox().setSelectedId (settings.syncMode.load() + 1, yup::dontSendNotification);

        levelKnob.getSlider().setValue (settings.level.load(), yup::dontSendNotification);
        octaveKnob.getSlider().setValue (settings.octave.load(), yup::dontSendNotification);
        detuneKnob.getSlider().setValue (settings.detuneSemitones.load() * 100.0, yup::dontSendNotification);
        ridgesKnob.getSlider().setValue (settings.ridgeSpacing.load(), yup::dontSendNotification);
        colorKnob.getSlider().setValue (settings.color.load(), yup::dontSendNotification);
        dispersionKnob.getSlider().setValue (settings.dispersion.load(), yup::dontSendNotification);
        squeezeKnob.getSlider().setValue (settings.squeeze.load(), yup::dontSendNotification);
        squashKnob.getSlider().setValue (settings.squash.load(), yup::dontSendNotification);
        tiltKnob.getSlider().setValue (settings.tilt.load(), yup::dontSendNotification);
        oddEvenKnob.getSlider().setValue (settings.oddEven.load(), yup::dontSendNotification);
        formantKnob.getSlider().setValue (settings.formant.load(), yup::dontSendNotification);
        formantPositionKnob.getSlider().setValue (settings.formantPosition.load(), yup::dontSendNotification);
        scatterKnob.getSlider().setValue (settings.scatter.load(), yup::dontSendNotification);
        syncRatioKnob.getSlider().setValue (settings.syncRatio.load(), yup::dontSendNotification);
        unisonKnob.getSlider().setValue (settings.unisonVoices.load(), yup::dontSendNotification);
        unisonDetuneKnob.getSlider().setValue (settings.unisonDetune.load(), yup::dontSendNotification);
        spreadKnob.getSlider().setValue (settings.unisonSpread.load(), yup::dontSendNotification);

        updateSyncAvailability();

        editor.refresh();
    }

    /** Publishes a frame's worth of partial edits to the audio thread. */
    void commitPendingEdits() { editor.commitPendingEdits(); }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (panelInset);

        auto header = bounds.removeFromTop (headerHeight);
        resetButton.setBounds (header.removeFromRight (buttonWidth));
        header.removeFromRight (spacing);
        partialsButton.setBounds (header.removeFromRight (buttonWidth));
        titleLabel.setBounds (header);

        bounds.removeFromTop (spacing);

        auto knobArea = bounds.removeFromBottom (knobRowHeight * 3.0f + spacing * 2.0f);
        bounds.removeFromBottom (spacing);

        auto choiceArea = bounds.removeFromBottom (choiceRowHeight);
        bounds.removeFromBottom (spacing);

        editor.setBounds (bounds);

        layoutControlsInRow (choiceArea, { &waveformChoice, &syncModeChoice });

        layoutControlsInRow (knobArea.removeFromTop (knobRowHeight),
                             { &levelKnob, &octaveKnob, &detuneKnob, &ridgesKnob, &colorKnob, &dispersionKnob });

        knobArea.removeFromTop (spacing);

        layoutControlsInRow (knobArea.removeFromTop (knobRowHeight),
                             { &squeezeKnob, &squashKnob, &tiltKnob, &oddEvenKnob, &formantKnob, &formantPositionKnob });

        knobArea.removeFromTop (spacing);

        layoutControlsInRow (knobArea,
                             { &scatterKnob, &syncRatioKnob, &unisonKnob, &unisonDetuneKnob, &spreadKnob });
    }

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    //==============================================================================
    /** Greys out the ratio knob while no sync mode reads it. */
    void updateSyncAvailability()
    {
        syncRatioKnob.setEnabled (static_cast<yup::SyncMode> (settings.syncMode.load()) != yup::SyncMode::none);
    }

    //==============================================================================
    static constexpr float panelInset = 8.0f;
    static constexpr float headerHeight = 18.0f;
    static constexpr float choiceRowHeight = 36.0f;
    static constexpr float knobRowHeight = 58.0f;
    static constexpr float buttonWidth = 68.0f;
    static constexpr float spacing = 6.0f;

    SynthOscillatorSettings& settings;

    yup::Label titleLabel;
    yup::ToggleButton partialsButton;
    yup::TextButton resetButton { "RESET" };
    WaveformEditor editor;

    ChoiceControl waveformChoice;
    ChoiceControl syncModeChoice;

    KnobControl levelKnob;
    KnobControl octaveKnob;
    KnobControl detuneKnob;
    KnobControl ridgesKnob;
    KnobControl colorKnob;
    KnobControl dispersionKnob;
    KnobControl squeezeKnob;
    KnobControl squashKnob;
    KnobControl tiltKnob;
    KnobControl oddEvenKnob;
    KnobControl formantKnob;
    KnobControl formantPositionKnob;
    KnobControl scatterKnob;
    KnobControl syncRatioKnob;
    KnobControl unisonKnob;
    KnobControl unisonDetuneKnob;
    KnobControl spreadKnob;
};

//==============================================================================
class AudioExample
    : public yup::Component
    , public yup::AudioIODeviceCallback
{
public:
    AudioExample()
        : Component ("AudioExample")
        , keyboardComponent (keyboardState, yup::MidiKeyboardComponent::horizontalKeyboard)
    {
        audioDeviceError = deviceManager.initialiseWithDefaultDevices (0, 2);

        // The keyboard state is pumped into the synth by processNextMidiBuffer(), so no note
        // listener is registered here: listening as well would trigger every note twice.
        keyboardComponent.setAvailableRange (36, 84); // C2 to C6
        keyboardComponent.setLowestVisibleKey (48);   // Start from C3
        keyboardComponent.setMidiChannel (1);
        keyboardComponent.setVelocity (0.7f);
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::whiteKeyColorId, yup::Color (0xffd7dde3));
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::whiteKeyPressedColorId, SynthTheme::accent);
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::blackKeyColorId, yup::Color (0xff191d21));
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::blackKeyPressedColorId, SynthTheme::accentDim);
        keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::keyOutlineColorId, SynthTheme::panelBorder);
        addAndMakeVisible (keyboardComponent);
        keyboardComponent.setVisible (false);

        const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        titleLabel.setText ("P R I S M   /   SPECTRAL SYNTH", yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (17.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        subtitleLabel.setText ("Sculpt harmonics. Scatter phases. Play the spectrum.", yup::dontSendNotification);
        subtitleLabel.setFont (font.withHeight (11.0f));
        subtitleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        addAndMakeVisible (subtitleLabel);

        loadLabel.setFont (font.withHeight (11.0f));
        loadLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        addAndMakeVisible (loadLabel);

        voiceLabel.setText ("", yup::dontSendNotification);
        voiceLabel.setFont (font.withHeight (11.0f));
        voiceLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::accent);
        addAndMakeVisible (voiceLabel);

        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            auto panel = std::make_unique<SynthOscillatorPanel> (
                yup::String ("OSC ") + yup::String (index + 1),
                synth.getOscillatorSettings (index),
                synth.getResources(),
                synth.getOscillatorSlot (index),
                font.withHeight (10.0f));

            addAndMakeVisible (*panel);
            oscillatorPanels[static_cast<std::size_t> (index)] = std::move (panel);
        }

        envelopePanel = std::make_unique<SynthEnvelopePanel> (synth.getEnvelopeSettings(), font.withHeight (10.0f));
        addAndMakeVisible (*envelopePanel);

        randomizeButton.setColor (yup::TextButton::Style::backgroundColorId, SynthTheme::panelBackground);
        randomizeButton.setColor (yup::TextButton::Style::textColorId, SynthTheme::textPrimary);
        randomizeButton.setColor (yup::TextButton::Style::outlineColorId, SynthTheme::panelBorder);
        randomizeButton.onClick = [this] { randomizeOscillators(); };
        addAndMakeVisible (randomizeButton);

        clearButton.setColor (yup::TextButton::Style::backgroundColorId, SynthTheme::panelBackground);
        clearButton.setColor (yup::TextButton::Style::textColorId, SynthTheme::textPrimary);
        clearButton.setColor (yup::TextButton::Style::outlineColorId, SynthTheme::panelBorder);
        clearButton.onClick = [this]
        {
            keyboardState.allNotesOff (0); // Turn off all notes on all channels
            synth.requestAllNotesOff();
        };
        addAndMakeVisible (clearButton);

        volumeKnob = std::make_unique<KnobControl> ("VOLUME", 0.0, 1.0, 0.001, 0.5, font.withHeight (10.0f));
        volumeKnob->onChange = [this] (double value) { masterVolume = static_cast<float> (value); };
        addAndMakeVisible (*volumeKnob);

        modeChoice = std::make_unique<ChoiceControl> ("VOICE MODE", yup::StringArray { "Poly / 8 voices", "Mono / retrigger", "Legato / glide" }, font.withHeight (10.0f));
        modeChoice->getComboBox().setSelectedId (1, yup::dontSendNotification);
        modeChoice->onChange = [this] (int id)
        {
            synth.playMode = id - 1;
            glideKnob->setEnabled (id != 1);
        };
        addAndMakeVisible (*modeChoice);
        glideKnob = std::make_unique<KnobControl> ("GLIDE / ms", 0.0, 2000.0, 1.0, 120.0, font.withHeight (10.0f));
        glideKnob->onChange = [this] (double value) { synth.portamento = static_cast<float> (value * 0.001); };
        glideKnob->setEnabled (false);
        addAndMakeVisible (*glideKnob);
        midiDevices = yup::MidiInput::getAvailableDevices();
        yup::StringArray midiNames { "No MIDI input" };
        for (const auto& device : midiDevices)
            midiNames.add (device.name);
        midiChoice = std::make_unique<ChoiceControl> ("MIDI INPUT", midiNames, font.withHeight (10.0f));
        midiChoice->getComboBox().setSelectedId (midiDevices.isEmpty() ? 1 : 2, yup::dontSendNotification);
        midiChoice->onChange = [this] (int)
        {
            closeMidiInput();
            synth.requestAllNotesOff();
            if (isVisible())
                openMidiInput();
        };
        addAndMakeVisible (*midiChoice);
        renderData.resize (SynthExample::maxBlockSize);
        addAndMakeVisible (oscilloscope);
    }

    ~AudioExample() override
    {
        closeMidiInput();

        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (outerInset);

        auto header = bounds.removeFromTop (headerHeight);

        volumeKnob->setBounds (header.removeFromRight (64.0f));
        header.removeFromRight (spacing);
        clearButton.setBounds (header.removeFromRight (110.0f).reduced (0.0f, 14.0f));
        header.removeFromRight (spacing);
        randomizeButton.setBounds (header.removeFromRight (110.0f).reduced (0.0f, 14.0f));
        header.removeFromRight (spacing * 2.0f);
        voiceLabel.setBounds (header.removeFromRight (110.0f));

        titleLabel.setBounds (header.removeFromTop (header.getHeight() * 0.5f));
        subtitleLabel.setBounds (header);

        bounds.removeFromTop (spacing);

        keyboardComponent.setBounds (bounds.removeFromBottom (proportionOfHeight (0.19f)));

        bounds.removeFromBottom (spacing);

        auto performance = bounds.removeFromBottom (58.0f);
        modeChoice->setBounds (performance.removeFromLeft (190.0f).reduced (4.0f, 9.0f));
        glideKnob->setBounds (performance.removeFromLeft (78.0f));
        performance.removeFromLeft (spacing);
        midiChoice->setBounds (performance.removeFromLeft (210.0f).reduced (4.0f, 9.0f));
        performance.removeFromLeft (spacing);
        loadLabel.setBounds (performance);
        bounds.removeFromBottom (spacing);

        auto modulation = bounds.removeFromBottom (yup::jmin (150.0f, bounds.getHeight() * 0.32f));
        envelopePanel->setBounds (modulation.removeFromLeft (modulation.getWidth() * 0.62f));
        modulation.removeFromLeft (spacing);
        oscilloscope.setBounds (modulation);
        bounds.removeFromBottom (spacing);

        const auto panelWidth = (bounds.getWidth() - spacing) / static_cast<float> (SynthExample::oscillatorCount);
        for (auto& panel : oscillatorPanels)
        {
            panel->setBounds (bounds.removeFromLeft (panelWidth));
            bounds.removeFromLeft (spacing);
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (SynthTheme::windowBackground);
        g.fillAll();
    }

    void mouseDown (const yup::MouseEvent&) override
    {
        takeKeyboardFocus();
    }

    void refreshDisplay (double) override
    {
        if (scopeReady.load (std::memory_order_acquire))
        {
            renderData.assign (scopeSamples.begin(), scopeSamples.begin() + scopeCount);
            scopeReady.store (false, std::memory_order_release);
            oscilloscope.setRenderData (renderData);
        }

        if (oscilloscope.isVisible())
            oscilloscope.repaint();

        // One generation bump per frame, however many mouse events the drag produced.
        for (auto& panel : oscillatorPanels)
            if (panel != nullptr)
                panel->commitPendingEdits();

        const auto activeVoices = synth.getNumActiveVoices();

        const auto status = audioDeviceError.isNotEmpty() ? audioDeviceError
                          : midiInputError.isNotEmpty() ? midiInputError
                          : yup::String (loadMeasurer.getLoadAsPercentage(), 1) + "% AUDIO / "
                                + yup::String (loadMeasurer.getXRunCount()) + " OVERRUNS / "
                                + yup::String (receivedNoteOns.load()) + " NOTES IN";
        loadLabel.setText (status, yup::dontSendNotification);
        voiceLabel.setText (yup::String (activeVoices) + " / 8 VOICES",
                            yup::dontSendNotification);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        const auto maxBlockSize = yup::jmax (device->getDefaultBufferSize(), SynthExample::maxBlockSize);

        synth.prepare (device->getCurrentSampleRate(), maxBlockSize);

        renderBuffer.setSize (2, maxBlockSize, false, true, true);
        loadMeasurer.reset (device->getCurrentSampleRate(), device->getDefaultBufferSize());
        midiBuffer.ensureSize (16384);
        outputGain.reset (device->getCurrentSampleRate(), 0.02);
        outputGain.setCurrentAndTargetValue (masterVolume.load());

        midiCollector.reset (device->getCurrentSampleRate());
        midiCollector.ensureStorageAllocated (midiQueueBytes);
    }

    void audioDeviceStopped() override
    {
    }

    void audioDeviceIOCallbackWithContext (const float* const*,
                                           int,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const yup::AudioIODeviceCallbackContext&) override
    {
        if (numSamples <= 0)
            return;
        const yup::ScopedNoDenormals noDenormals;
        const yup::AudioProcessLoadMeasurer::ScopedTimer renderTimer (loadMeasurer, numSamples);
        if (numSamples <= 0 || numSamples > renderBuffer.getNumSamples())
        {
            for (int channel = 0; channel < numOutputChannels; ++channel)
                if (outputChannelData[channel] != nullptr)
                    yup::FloatVectorOperations::clear (outputChannelData[channel], numSamples);

            return;
        }

        for (int channel = 0; channel < renderBuffer.getNumChannels(); ++channel)
            yup::FloatVectorOperations::clear (renderBuffer.getWritePointer (channel), numSamples);

        midiBuffer.clear();

        // processNextMidiBuffer() reads whatever is already in the buffer before injecting
        // the on-screen keyboard's own events, so collecting the hardware input first is
        // what lights up the drawn keys as well as playing the notes.
        midiCollector.removeNextBlockOfMessages (midiBuffer, numSamples);
        keyboardState.processNextMidiBuffer (midiBuffer, 0, numSamples, true);
        for (const auto metadata : midiBuffer)
            if (metadata.getMessage().isNoteOn())
                receivedNoteOns.fetch_add (1, std::memory_order_relaxed);
        synth.renderNextBlock (renderBuffer, midiBuffer, 0, numSamples);

        outputGain.setTargetValue (masterVolume.load());
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto gain = outputGain.getNextValue();
            for (int channel = 0; channel < numOutputChannels; ++channel)
            {
                const auto sourceChannel = yup::jmin (channel, renderBuffer.getNumChannels() - 1);
                if (outputChannelData[channel] != nullptr)
                    outputChannelData[channel][sample] = renderBuffer.getSample (sourceChannel, sample) * gain;
            }
            renderBuffer.setSample (0, sample, renderBuffer.getSample (0, sample) * gain);
        }

        if (! scopeReady.load (std::memory_order_acquire))
        {
            scopeCount = yup::jmin (numSamples, SynthExample::maxBlockSize);
            std::copy_n (renderBuffer.getReadPointer (0), scopeCount, scopeSamples.begin());
            scopeReady.store (true, std::memory_order_release);
        }
    }

    void visibilityChanged() override
    {
        if (! isVisible())
        {
            closeMidiInput();
            deviceManager.removeAudioCallback (this);
        }
        else
        {
            deviceManager.addAudioCallback (this);
            openMidiInput();
        }
    }

private:
    //==============================================================================
    /** Opens the selected hardware input and reports device-open failures in the UI. */
    void openMidiInput()
    {
        if (midiInputIdentifier.isNotEmpty() || midiChoice == nullptr)
            return;

        midiInputError.clear();
        const auto index = midiChoice->getComboBox().getSelectedId() - 2;
        if (! yup::isPositiveAndBelow (index, midiDevices.size()))
            return;

        const auto identifier = midiDevices[index].identifier;
        deviceManager.setMidiInputDeviceEnabled (identifier, true);
        if (! deviceManager.isMidiInputDeviceEnabled (identifier))
        {
            midiInputError = "Cannot open MIDI input: " + midiDevices[index].name;
            return;
        }
        midiInputIdentifier = identifier;
        deviceManager.addMidiInputDeviceCallback (midiInputIdentifier, &midiCollector);
    }

    /** Releases the input again, so a hidden demo does not hold the device open. */
    void closeMidiInput()
    {
        if (midiInputIdentifier.isEmpty())
            return;

        deviceManager.removeMidiInputDeviceCallback (midiInputIdentifier, &midiCollector);
        deviceManager.setMidiInputDeviceEnabled (midiInputIdentifier, false);

        midiInputIdentifier.clear();
    }

    //==============================================================================
    void randomizeOscillators()
    {
        auto& random = yup::Random::getSystemRandom();

        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            auto& settings = synth.getOscillatorSettings (index);

            settings.waveform = random.nextInt (6);
            settings.syncMode = random.nextInt (4);
            settings.syncRatio = 1.0f + random.nextFloat() * 3.0f;
            settings.level = 0.2f + random.nextFloat() * 0.8f;
            settings.octave = random.nextInt (3) - 1;
            settings.detuneSemitones = (random.nextFloat() - 0.5f) * 0.3f;
            settings.ridgeSpacing = 0.5f + random.nextFloat() * 3.0f;
            settings.color = random.nextFloat();
            settings.dispersion = 0.1f + random.nextFloat() * 0.8f;
            settings.squeeze = random.nextBool() ? 0.0f : random.nextFloat() * 0.5f;
            settings.squash = 0.4f + random.nextFloat() * 1.6f;
            settings.tilt = (random.nextFloat() - 0.5f) * 3.0f;
            settings.oddEven = 0.2f + random.nextFloat() * 0.6f;
            settings.formant = (random.nextFloat() - 0.4f) * 5.0f;
            settings.formantPosition = random.nextFloat() * 6.0f;
            settings.scatter = random.nextBool() ? 0.0f : random.nextFloat() * 0.6f;
            settings.unisonVoices = 1 + random.nextInt (SynthExample::maxUnisonVoices);
            settings.unisonDetune = random.nextFloat() * 0.5f;
            settings.unisonSpread = random.nextFloat();

            // A randomized waveform is only audible once the edited partials are dropped.
            settings.usesCustomSeries = false;
            settings.harmonicGeneration.fetch_add (1);

            if (auto& panel = oscillatorPanels[static_cast<std::size_t> (index)]; panel != nullptr)
                panel->refresh();
        }
    }

    //==============================================================================
    static constexpr std::size_t midiQueueBytes = 2048;

    static constexpr float outerInset = 10.0f;
    static constexpr float headerHeight = 44.0f;
    static constexpr float spacing = 8.0f;

    //==============================================================================
    yup::AudioDeviceManager deviceManager;
    HarmonicSynthEngine synth;

    // MIDI keyboard components
    yup::MidiKeyboardState keyboardState;
    yup::MidiKeyboardComponent keyboardComponent;
    yup::MidiMessageCollector midiCollector;
    yup::String midiInputIdentifier;
    yup::String audioDeviceError;
    yup::String midiInputError;
    yup::Array<yup::MidiDeviceInfo> midiDevices;
    std::atomic<int> receivedNoteOns { 0 };

    yup::AudioBuffer<float> renderBuffer;
    yup::MidiBuffer midiBuffer;
    std::vector<float> renderData;
    std::array<float, SynthExample::maxBlockSize> scopeSamples {};
    std::atomic<bool> scopeReady { false };
    int scopeCount = 0;
    yup::SmoothedValue<float> outputGain;
    yup::AudioProcessLoadMeasurer loadMeasurer;

    // UI Components
    yup::Label titleLabel;
    yup::Label subtitleLabel;
    yup::Label voiceLabel;
    yup::Label loadLabel;

    std::array<std::unique_ptr<SynthOscillatorPanel>, SynthExample::oscillatorCount> oscillatorPanels;
    std::unique_ptr<SynthEnvelopePanel> envelopePanel;

    yup::TextButton randomizeButton { "RANDOMIZE" };
    yup::TextButton clearButton { "ALL NOTES OFF" };
    std::unique_ptr<KnobControl> volumeKnob;
    std::unique_ptr<ChoiceControl> modeChoice;
    std::unique_ptr<ChoiceControl> midiChoice;
    std::unique_ptr<KnobControl> glideKnob;
    Oscilloscope oscilloscope;

    std::atomic<float> masterVolume { 0.5f };
};
