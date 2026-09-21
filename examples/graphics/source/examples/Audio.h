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

//==============================================================================
/** The synthesis algorithm a voice oscillator renders with.

    Every value maps onto one of the bandlimited yup_dsp oscillators, which differ
    in how they derive their spectrum and in how they can be modulated.

    @see SynthOscillator
*/
enum class SynthOscillatorType
{
    wavetable, /**< The same series rendered once, then played back */
    sync,      /**< Alias-free spectral oscillator synchronization */
    morphing,  /**< Blends two synchronized endpoint spectra       */
    modulated  /**< Oversampled morph, FM, PM and phase distortion */
};

/** @internal Item names for SynthOscillatorType, index aligned with the enumeration. */
inline yup::StringArray getSynthOscillatorTypeNames()
{
    return { "Wavetable", "Sync", "Morphing", "Modulated" };
}

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
        releaseSamples = toSamples (values.release);

        attackIncrement = 1.0f / static_cast<float> (toSamples (values.attack));
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
    SynthOscillatorType type = SynthOscillatorType::wavetable;
    yup::Waveform waveform = yup::Waveform::sawtooth;
    yup::Waveform shape = yup::Waveform::square;
    yup::SyncMode syncMode = yup::SyncMode::hard;
    float level = 0.5f;
    float detuneSemitones = 0.0f;
    float followerRatio = 1.5f;
    float morph = 0.0f;
    float phaseDistortion = 0.5f;
    float fmAmount = 0.0f;
    float fmRatio = 2.0f;
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

    std::atomic<int> type { static_cast<int> (SynthOscillatorType::wavetable) };
    std::atomic<int> waveform { static_cast<int> (yup::Waveform::sawtooth) };
    std::atomic<int> shape { static_cast<int> (yup::Waveform::square) };
    std::atomic<int> syncMode { static_cast<int> (yup::SyncMode::hard) };
    std::atomic<float> level { 0.5f };
    std::atomic<float> detuneSemitones { 0.0f };
    std::atomic<float> followerRatio { 1.5f };
    std::atomic<float> morph { 0.0f };
    std::atomic<float> phaseDistortion { 0.5f };
    std::atomic<float> fmAmount { 0.0f };
    std::atomic<float> fmRatio { 2.0f };
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
        return { static_cast<SynthOscillatorType> (type.load()),
                 static_cast<yup::Waveform> (waveform.load()),
                 static_cast<yup::Waveform> (shape.load()),
                 static_cast<yup::SyncMode> (syncMode.load()),
                 level.load(),
                 detuneSemitones.load(),
                 followerRatio.load(),
                 morph.load(),
                 phaseDistortion.load(),
                 fmAmount.load(),
                 fmRatio.load(),
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

    Building a FourierSeries allocates, and rendering a WaveformBank runs one inverse
    FFT per frame and bandwidth level, so both belong at construction time. Once
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

        bank.prepare ({ frames.data(), frames.size() });
    }

    /** Returns the series of one of the Waveform presets. */
    const yup::FourierSeries<double>& getFrame (yup::Waveform waveform) const noexcept
    {
        return frames[static_cast<std::size_t> (waveform)];
    }

    /** Returns the bank of frames the modulated oscillator morphs across. */
    const yup::WaveformBank<float>& getBank() const noexcept { return bank; }

private:
    std::array<yup::FourierSeries<double>, 6> frames;
    yup::WaveformBank<float> bank;
};

//==============================================================================
/** One of a voice's oscillators, owning every algorithm it can switch between.

    prepare() allocates all the backends; renderBlock() is allocation-free and pushes
    only the controls that actually moved, so editing a knob is the only thing that
    pays for a spectral transform or a table render.

    Unison is built from bare yup::WavetableOscillator satellites rather than from
    further copies of this class. A SynthOscillator owns four wavetable oscillators
    once the sync and morphing backends are counted, each with its own FFT and tables,
    so replicating it per unison slot would cost several times the memory and startup
    work that one satellite does. The satellites play the same series as the selected
    algorithm, detuned and panned around it, which is exact for the wavetable algorithm
    and is why unison is offered there alone.

    @see SynthOscillatorSettings, SynthOscillatorResources
*/
class SynthOscillator
{
public:
    /** Returns true if the algorithm can be widened with unison satellites. */
    static bool supportsUnison (SynthOscillatorType type) noexcept
    {
        return type == SynthOscillatorType::wavetable;
    }

    /** Allocates every backend and attaches the shared waveform resources. */
    void prepare (double newSampleRate, int maxBlockSize, const SynthOscillatorResources& oscillatorResources)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        resources = &oscillatorResources;

        wavetable.prepare (sampleRate, SynthExample::maxHarmonics);
        sync.prepare (sampleRate, SynthExample::maxHarmonics);
        morphing.prepare (sampleRate, SynthExample::maxHarmonics);
        modulated.prepare (sampleRate, maxBlockSize, resources->getBank());

        for (auto& satellite : satellites)
            satellite.prepare (sampleRate, SynthExample::maxHarmonics);

        customSeries.resize (SynthExample::maxHarmonics);
        slotBuffer.assign (static_cast<std::size_t> (yup::jmax (1, maxBlockSize)), 0.0f);

        applied = {};
        hasAppliedValues = false;
    }

    /** Restarts every backend, spreading the satellites so they do not stack in phase. */
    void reset (double initialPhase) noexcept
    {
        const auto phase = static_cast<float> (initialPhase);

        wavetable.setPhase (phase);
        sync.setPhase (phase);
        morphing.setPhase (phase);
        modulated.reset (initialPhase);

        for (std::size_t index = 0; index < satellites.size(); ++index)
        {
            const auto offset = static_cast<float> (index + 1) / static_cast<float> (satellites.size() + 1);

            satellites[index].setPhase (phase + offset - std::floor (phase + offset));
        }

        modulatorPhase = 0.0;
    }

    /** Applies the pending changes and writes one stereo block of the selected algorithm.

        The buffers are overwritten rather than added to, so the caller does not have to
        clear them first.
    */
    void renderBlock (float* left,
                      float* right,
                      int numSamples,
                      const SynthOscillatorValues& values,
                      const SynthOscillatorSettings& settings,
                      double frequency) noexcept
    {
        yup::FloatVectorOperations::clear (left, numSamples);
        yup::FloatVectorOperations::clear (right, numSamples);

        const auto slotCount = supportsUnison (values.type)
                                 ? yup::jlimit (1, SynthExample::maxUnisonVoices, values.unisonVoices)
                                 : 1;

        const auto centreIndex = (slotCount - 1) / 2;
        const auto slotGain = 1.0f / std::sqrt (static_cast<float> (slotCount));

        applyParameters (values, settings, detunedFrequency (frequency, values, centreIndex, slotCount));

        renderAlgorithm (slotBuffer.data(), numSamples, values, frequency);
        accumulateSlot (left, right, numSamples, slotOffset (centreIndex, slotCount) * values.unisonSpread, slotGain);

        for (int index = 0, satellite = 0; index < slotCount; ++index)
        {
            if (index == centreIndex)
                continue;

            renderSatellite (satellites[static_cast<std::size_t> (satellite++)],
                             numSamples,
                             detunedFrequency (frequency, values, index, slotCount));

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

    /** Renders one unison satellite, which always plays the wavetable algorithm. */
    void renderSatellite (yup::WavetableOscillator<float>& satellite, int numSamples, double frequency) noexcept
    {
        satellite.setFrequency (frequency);

        if (satellite.needsRender())
            satellite.render();

        satellite.processBlock (slotBuffer.data(), numSamples);
    }

    /** Writes the block of whichever algorithm is selected. */
    void renderAlgorithm (float* output, int numSamples, const SynthOscillatorValues& values, double frequency) noexcept
    {
        switch (values.type)
        {
            case SynthOscillatorType::wavetable:
                if (wavetable.needsRender())
                    wavetable.render();

                wavetable.processBlock (output, numSamples);
                break;

            case SynthOscillatorType::sync:
                sync.update();
                sync.processBlock (output, numSamples);
                break;

            case SynthOscillatorType::morphing:
                morphing.update();
                morphing.processBlock (output, numSamples, static_cast<double> (values.morph));
                break;

            case SynthOscillatorType::modulated:
                renderModulatedBlock (output, numSamples, values, frequency);
                break;
        }
    }

    //==============================================================================
    /** Renders the oversampled modulation path, driving its FM from an internal sine. */
    void renderModulatedBlock (float* output, int numSamples, const SynthOscillatorValues& values, double frequency) noexcept
    {
        yup::ModulatedOscillator<float>::Parameters parameters;
        parameters.frequency = frequency;
        parameters.morph = static_cast<double> (values.morph);
        parameters.phaseDistortion = static_cast<double> (values.phaseDistortion);
        parameters.syncFrequency = values.syncMode == yup::SyncMode::none
                                     ? 0.0
                                     : frequency * static_cast<double> (values.followerRatio);

        const auto internalSampleRate = modulated.getInternalSampleRate();
        const auto modulatorIncrement = static_cast<double> (values.fmRatio) * frequency / internalSampleRate;
        const auto modulatorDepth = static_cast<double> (values.fmAmount) * frequency;

        // Unlike the other backends this one can decline to write anything, which would
        // otherwise leave the previous unison slot's samples in the shared buffer.
        const auto rendered = modulated.processModulatedBlock (output, numSamples, [this, &parameters, modulatorIncrement, modulatorDepth] (int)
        {
            parameters.linearFM = modulatorDepth * std::sin (yup::MathConstants<double>::twoPi * modulatorPhase);

            modulatorPhase += modulatorIncrement;
            modulatorPhase -= std::floor (modulatorPhase);

            return parameters;
        });

        if (! rendered)
            yup::FloatVectorOperations::clear (output, numSamples);
    }

    //==============================================================================
    /** Pushes only the controls whose value changed since the last block. */
    void applyParameters (const SynthOscillatorValues& values, const SynthOscillatorSettings& settings, double frequency) noexcept
    {
        const auto typeChanged = ! hasAppliedValues || applied.type != values.type;
        const auto waveformChanged = typeChanged || applied.waveform != values.waveform;
        const auto shapeChanged = typeChanged || applied.shape != values.shape;
        const auto syncModeChanged = typeChanged || applied.syncMode != values.syncMode;
        const auto ratioChanged = typeChanged || applied.followerRatio != values.followerRatio;

        const auto partialsChanged = ! hasAppliedValues
                                  || applied.usesCustomSeries != values.usesCustomSeries
                                  || applied.harmonicGeneration != values.harmonicGeneration;

        if (partialsChanged && values.usesCustomSeries)
            settings.copyHarmonicsInto (customSeries, values.harmonicScale);

        const auto seriesChanged = waveformChanged || partialsChanged;
        const auto& series = values.usesCustomSeries ? customSeries : resources->getFrame (values.waveform);

        if (seriesChanged)
            for (auto& satellite : satellites)
                satellite.setSeries (series);

        switch (values.type)
        {
            case SynthOscillatorType::wavetable:
                if (seriesChanged)
                    wavetable.setSeries (series);
                break;

            case SynthOscillatorType::sync:
                if (seriesChanged)
                    sync.setFollowerSeries (series);
                if (syncModeChanged)
                    sync.setSyncMode (values.syncMode);
                if (ratioChanged)
                    sync.setFollowerRatio (values.followerRatio);
                break;

            case SynthOscillatorType::morphing:
                if (seriesChanged || shapeChanged)
                    morphing.setSeries (series, resources->getFrame (values.shape));
                if (syncModeChanged)
                    morphing.setSyncMode (values.syncMode);
                if (ratioChanged)
                    morphing.setFollowerRatio (values.followerRatio);
                break;

            case SynthOscillatorType::modulated:
                break;
        }

        wavetable.setFrequency (frequency);
        sync.setFrequency (frequency);
        morphing.setFrequency (frequency);

        applied = values;
        hasAppliedValues = true;
    }

    //==============================================================================
    yup::WavetableOscillator<float> wavetable;
    yup::SyncOscillator<float> sync;
    yup::MorphingOscillator<float> morphing;
    yup::ModulatedOscillator<float> modulated;

    std::array<yup::WavetableOscillator<float>, SynthExample::maxUnisonVoices - 1> satellites;

    const SynthOscillatorResources* resources = nullptr;
    SynthOscillatorValues applied;
    yup::FourierSeries<double> customSeries;
    std::vector<float> slotBuffer;
    double sampleRate = 44100.0;
    double modulatorPhase = 0.0;
    bool hasAppliedValues = false;
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
                const SynthOscillatorResources& oscillatorResources)
        : settings (oscillatorSettings)
        , envelopeSettings (sharedEnvelopeSettings)
        , resources (oscillatorResources)
    {
    }

    /** Allocates every oscillator backend. Must run outside the audio callback. */
    void prepare (double sampleRate, int maxBlockSize)
    {
        for (auto& oscillator : oscillators)
            oscillator.prepare (sampleRate, maxBlockSize, resources);

        for (auto& level : levels)
            level.reset (sampleRate, SynthExample::levelRampSeconds);

        envelope.prepare (sampleRate);

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

    void startNote (int midiNoteNumber, float velocity, yup::SynthesiserSound*, int currentPitchWheelPosition) override
    {
        noteFrequency = midiNoteToFrequency (midiNoteNumber);
        velocityGain = yup::jmax (0.05f, velocity);

        pitchWheelMoved (currentPitchWheelPosition);

        for (auto& oscillator : oscillators)
            oscillator.reset (0.0);

        envelope.setParameters (envelopeSettings.read());
        envelope.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            envelope.noteOff();
            return;
        }

        envelope.noteOffImmediate();
        clearCurrentNote();
    }

    void pitchWheelMoved (int newPitchWheelValue) override
    {
        const auto normalized = (static_cast<double> (newPitchWheelValue) - 8192.0) / 8192.0;
        pitchWheelRatio = std::pow (2.0, normalized * pitchWheelRangeSemitones / 12.0);
    }

    void controllerMoved (int, int) override {}

    //==============================================================================
    void renderNextBlock (yup::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (! isVoiceActive() || numSamples <= 0)
            return;

        jassert (numSamples <= static_cast<int> (mixLeft.size()));

        const auto frequency = noteFrequency * pitchWheelRatio;
        const auto numChannelsToWrite = yup::jmin (outputBuffer.getNumChannels(), 2);

        float* channels[2] = {};

        for (int channel = 0; channel < numChannelsToWrite; ++channel)
            channels[channel] = outputBuffer.getWritePointer (channel, startSample);

        yup::FloatVectorOperations::clear (mixLeft.data(), numSamples);
        yup::FloatVectorOperations::clear (mixRight.data(), numSamples);

        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            const auto& oscillatorSettings = settings[static_cast<std::size_t> (index)];
            const auto values = oscillatorSettings.read();
            auto& level = levels[static_cast<std::size_t> (index)];

            // The oscillator's own detune is what makes the two of them beat against each
            // other, so it has to reach the frequency the backends are driven with.
            const auto detuned = frequency * std::pow (2.0, static_cast<double> (values.detuneSemitones) / 12.0);

            oscillators[static_cast<std::size_t> (index)]
                .renderBlock (oscLeft.data(), oscRight.data(), numSamples, values, oscillatorSettings, detuned);

            level.setTargetValue (values.level);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                const auto gain = level.getNextValue();

                mixLeft[static_cast<std::size_t> (sample)] += oscLeft[static_cast<std::size_t> (sample)] * gain;
                mixRight[static_cast<std::size_t> (sample)] += oscRight[static_cast<std::size_t> (sample)] * gain;
            }
        }

        envelope.setParameters (envelopeSettings.read());

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto gain = envelope.getNextValue() * velocityGain;
            const auto left = mixLeft[static_cast<std::size_t> (sample)] * gain;
            const auto right = mixRight[static_cast<std::size_t> (sample)] * gain;

            if (numChannelsToWrite == 1)
            {
                channels[0][sample] += (left + right) * 0.5f;
            }
            else
            {
                channels[0][sample] += left;
                channels[1][sample] += right;
            }
        }

        if (! envelope.isActive())
            clearCurrentNote();
    }

private:
    //==============================================================================
    static double midiNoteToFrequency (int midiNoteNumber) noexcept
    {
        return 440.0 * std::pow (2.0, (midiNoteNumber - 69) / 12.0);
    }

    //==============================================================================
    static constexpr double pitchWheelRangeSemitones = 2.0;

    const std::array<SynthOscillatorSettings, SynthExample::oscillatorCount>& settings;
    const SynthEnvelopeSettings& envelopeSettings;
    const SynthOscillatorResources& resources;

    std::array<SynthOscillator, SynthExample::oscillatorCount> oscillators;
    std::array<yup::SmoothedValue<float>, SynthExample::oscillatorCount> levels;

    SynthEnvelope envelope;

    std::vector<float> oscLeft;
    std::vector<float> oscRight;
    std::vector<float> mixLeft;
    std::vector<float> mixRight;

    double noteFrequency = 440.0;
    double pitchWheelRatio = 1.0;
    float velocityGain = 1.0f;
};

//==============================================================================
/** Polyphonic synthesiser rendering the two oscillators of every voice. */
class HarmonicSynthEngine : public yup::Synthesiser
{
public:
    HarmonicSynthEngine()
    {
        addSound (new SynthSound());

        for (int index = 0; index < SynthExample::voiceCount; ++index)
        {
            auto voice = yup::ReferenceCountedObjectPtr<SynthVoice> (new SynthVoice (settings, envelopeSettings, resources));

            addVoice (voice);
            ownedVoices.add (voice);
        }
    }

    /** Prepares every voice, including the oscillators of the modulated algorithm. */
    void prepare (double sampleRate, int maxBlockSize)
    {
        setCurrentPlaybackSampleRate (sampleRate);

        for (int index = 0; index < ownedVoices.size(); ++index)
            ownedVoices[index]->prepare (sampleRate, maxBlockSize);
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

    /** Returns the note of a sounding voice, or -1 when the synthesiser is silent. */
    int getCurrentlyPlayingNote() const noexcept
    {
        for (int index = 0; index < ownedVoices.size(); ++index)
            if (ownedVoices[index] != nullptr && ownedVoices[index]->isVoiceActive())
                return ownedVoices[index]->getCurrentlyPlayingNote();

        return -1;
    }

    /** Returns how many voices are currently sounding. */
    int getNumActiveVoices() const noexcept
    {
        int count = 0;

        for (int index = 0; index < ownedVoices.size(); ++index)
            if (ownedVoices[index] != nullptr && ownedVoices[index]->isVoiceActive())
                ++count;

        return count;
    }

private:
    SynthOscillatorResources resources;
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
inline constexpr yup::Color accent { 0xff4dc3ff };
inline constexpr yup::Color accentDim { 0xff2b6f8f };
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
    WaveformEditor (SynthOscillatorSettings& settingsToEdit, const SynthOscillatorResources& sharedResources)
        : settings (settingsToEdit)
        , resources (sharedResources)
    {
        displaySeries.resize (SynthExample::maxHarmonics);
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

        for (int index = 0; index < displayResolution; ++index)
        {
            const auto phase = static_cast<double> (index) / static_cast<double> (displayResolution - 1);

            displaySamples[static_cast<std::size_t> (index)] =
                evaluateFourierSeries (displaySeries, phase, SynthExample::displayHarmonics);
        }

        auto peak = 0.0f;

        for (auto sample : displaySamples)
            peak = yup::jmax (peak, std::abs (sample));

        if (peak <= 1.0e-6f)
        {
            repaint();
            return;
        }

        // The reconstruction is measured here, on the message thread, so the audio thread
        // never has to work out how loud an edited spectrum turned out to be.
        if (usesCustomSeries)
            settings.harmonicScale.store (1.0f / peak);

        const auto scale = 1.0f / peak;

        for (auto& sample : displaySamples)
            sample *= scale;

        repaint();
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
            const auto magnitude = yup::jlimit (0.0f, 1.0f, settings.harmonics[static_cast<std::size_t> (index)].load());
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

    yup::FourierSeries<double> displaySeries;
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

        const auto xSize = bounds.getWidth() / static_cast<float> (renderData.size());

        path.clear();
        path.reserveSpace (static_cast<int> (renderData.size()));
        path.moveTo (bounds.getX(), bounds.getCenterY() - renderData[0] * bounds.getHeight() * 0.45f);

        for (std::size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (bounds.getX() + static_cast<float> (i) * xSize,
                         bounds.getCenterY() - renderData[i] * bounds.getHeight() * 0.45f);

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
                          const yup::Font& font)
        : settings (settingsToEdit)
        , editor (settingsToEdit, resources)
        , algorithmChoice ("ALGORITHM", getSynthOscillatorTypeNames(), font)
        , waveformChoice ("WAVEFORM", getSynthWaveformNames(), font)
        , shapeChoice ("SHAPE B", getSynthWaveformNames(), font)
        , syncModeChoice ("SYNC", getSynthSyncModeNames(), font)
        , levelKnob ("LEVEL", 0.0, 1.0, 0.001, 0.5, font)
        , detuneKnob ("DETUNE", -24.0, 24.0, 0.01, 0.0, font)
        , ratioKnob ("RATIO", 0.25, 8.0, 0.01, 1.5, font)
        , morphKnob ("MORPH", 0.0, 1.0, 0.001, 0.0, font)
        , distortionKnob ("DIST", 0.01, 0.99, 0.001, 0.5, font)
        , fmAmountKnob ("FM AMT", 0.0, 4.0, 0.001, 0.0, font)
        , fmRatioKnob ("FM RATIO", 0.25, 8.0, 0.01, 2.0, font)
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

        for (auto* choice : { &algorithmChoice, &waveformChoice, &shapeChoice, &syncModeChoice })
            addAndMakeVisible (*choice);

        for (auto* knob : { &levelKnob, &detuneKnob, &ratioKnob, &morphKnob, &distortionKnob,
                            &fmAmountKnob, &fmRatioKnob, &unisonKnob, &unisonDetuneKnob, &spreadKnob })
            addAndMakeVisible (*knob);

        algorithmChoice.onChange = [this] (int id)
        {
            settings.type = id - 1;
            updateUnisonAvailability();
        };

        // Picking a preset drops any edited partials, otherwise the oscillator would keep
        // playing the edited shape while the combo box claims something else.
        waveformChoice.onChange = [this] (int id)
        {
            settings.waveform = id - 1;
            editor.revertToPreset();
        };

        shapeChoice.onChange = [this] (int id) { settings.shape = id - 1; };
        syncModeChoice.onChange = [this] (int id) { settings.syncMode = id - 1; };
        levelKnob.onChange = [this] (double value) { settings.level = static_cast<float> (value); };
        detuneKnob.onChange = [this] (double value) { settings.detuneSemitones = static_cast<float> (value); };
        ratioKnob.onChange = [this] (double value) { settings.followerRatio = static_cast<float> (value); };
        morphKnob.onChange = [this] (double value) { settings.morph = static_cast<float> (value); };
        distortionKnob.onChange = [this] (double value) { settings.phaseDistortion = static_cast<float> (value); };
        fmAmountKnob.onChange = [this] (double value) { settings.fmAmount = static_cast<float> (value); };
        fmRatioKnob.onChange = [this] (double value) { settings.fmRatio = static_cast<float> (value); };
        unisonKnob.onChange = [this] (double value) { settings.unisonVoices = static_cast<int> (value); };
        unisonDetuneKnob.onChange = [this] (double value) { settings.unisonDetune = static_cast<float> (value); };
        spreadKnob.onChange = [this] (double value) { settings.unisonSpread = static_cast<float> (value); };

        refresh();
    }

    /** Reads the settings back into the widgets. */
    void refresh()
    {
        algorithmChoice.getComboBox().setSelectedId (settings.type.load() + 1, yup::dontSendNotification);
        waveformChoice.getComboBox().setSelectedId (settings.waveform.load() + 1, yup::dontSendNotification);
        shapeChoice.getComboBox().setSelectedId (settings.shape.load() + 1, yup::dontSendNotification);
        syncModeChoice.getComboBox().setSelectedId (settings.syncMode.load() + 1, yup::dontSendNotification);

        levelKnob.getSlider().setValue (settings.level.load(), yup::dontSendNotification);
        detuneKnob.getSlider().setValue (settings.detuneSemitones.load(), yup::dontSendNotification);
        ratioKnob.getSlider().setValue (settings.followerRatio.load(), yup::dontSendNotification);
        morphKnob.getSlider().setValue (settings.morph.load(), yup::dontSendNotification);
        distortionKnob.getSlider().setValue (settings.phaseDistortion.load(), yup::dontSendNotification);
        fmAmountKnob.getSlider().setValue (settings.fmAmount.load(), yup::dontSendNotification);
        fmRatioKnob.getSlider().setValue (settings.fmRatio.load(), yup::dontSendNotification);
        unisonKnob.getSlider().setValue (settings.unisonVoices.load(), yup::dontSendNotification);
        unisonDetuneKnob.getSlider().setValue (settings.unisonDetune.load(), yup::dontSendNotification);
        spreadKnob.getSlider().setValue (settings.unisonSpread.load(), yup::dontSendNotification);

        updateUnisonAvailability();

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

        auto knobArea = bounds.removeFromBottom (knobRowHeight * 2.0f + spacing);
        bounds.removeFromBottom (spacing);

        auto choiceArea = bounds.removeFromBottom (choiceRowHeight);
        bounds.removeFromBottom (spacing);

        editor.setBounds (bounds);

        layoutControlsInRow (choiceArea, { &algorithmChoice, &waveformChoice, &shapeChoice, &syncModeChoice });

        layoutControlsInRow (knobArea.removeFromTop (knobRowHeight),
                             { &levelKnob, &detuneKnob, &ratioKnob, &morphKnob, &distortionKnob });

        knobArea.removeFromTop (spacing);

        layoutControlsInRow (knobArea,
                             { &fmAmountKnob, &fmRatioKnob, &unisonKnob, &unisonDetuneKnob, &spreadKnob });
    }

    void paint (yup::Graphics& g) override
    {
        paintSynthPanel (g, getLocalBounds());
    }

private:
    //==============================================================================
    /** Greys out the unison knobs for the algorithms the satellites cannot reproduce. */
    void updateUnisonAvailability()
    {
        const auto supported = SynthOscillator::supportsUnison (static_cast<SynthOscillatorType> (settings.type.load()));

        unisonKnob.setEnabled (supported);
        unisonDetuneKnob.setEnabled (supported);
        spreadKnob.setEnabled (supported);
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

    ChoiceControl algorithmChoice;
    ChoiceControl waveformChoice;
    ChoiceControl shapeChoice;
    ChoiceControl syncModeChoice;

    KnobControl levelKnob;
    KnobControl detuneKnob;
    KnobControl ratioKnob;
    KnobControl morphKnob;
    KnobControl distortionKnob;
    KnobControl fmAmountKnob;
    KnobControl fmRatioKnob;
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
        deviceManager.initialiseWithDefaultDevices (0, 2);

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

        const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        titleLabel.setText ("YUP POLYPHONIC SYNTHESIZER", yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (17.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary);
        addAndMakeVisible (titleLabel);

        subtitleLabel.setText ("Two unison oscillators per voice - draw the waveform or edit its partials", yup::dontSendNotification);
        subtitleLabel.setFont (font.withHeight (11.0f));
        subtitleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary);
        addAndMakeVisible (subtitleLabel);

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
            synth.allNotesOff (0, true);
        };
        addAndMakeVisible (clearButton);

        volumeKnob = std::make_unique<KnobControl> ("VOLUME", 0.0, 1.0, 0.001, 0.5, font.withHeight (10.0f));
        volumeKnob->onChange = [this] (double value) { masterVolume = static_cast<float> (value); };
        addAndMakeVisible (*volumeKnob);

        addAndMakeVisible (oscilloscope);
    }

    ~AudioExample() override
    {
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

        auto rightColumn = bounds.removeFromRight (bounds.getWidth() * 0.42f);
        bounds.removeFromRight (spacing);

        envelopePanel->setBounds (rightColumn.removeFromTop (rightColumn.getHeight() * 0.5f));
        rightColumn.removeFromTop (spacing);
        oscilloscope.setBounds (rightColumn);

        const auto panelHeight = (bounds.getHeight() - spacing) / static_cast<float> (SynthExample::oscillatorCount);

        for (auto& panel : oscillatorPanels)
        {
            if (panel == nullptr)
                continue;

            panel->setBounds (bounds.removeFromTop (panelHeight));
            bounds.removeFromTop (spacing);
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
        {
            const yup::CriticalSection::ScopedLockType sl (renderMutex);
            oscilloscope.setRenderData (renderData);
        }

        if (oscilloscope.isVisible())
            oscilloscope.repaint();

        // One generation bump per frame, however many mouse events the drag produced.
        for (auto& panel : oscillatorPanels)
            if (panel != nullptr)
                panel->commitPendingEdits();

        const auto activeVoices = synth.getNumActiveVoices();

        voiceLabel.setText (activeVoices > 0 ? yup::String (activeVoices) + " VOICES" : yup::String(),
                            yup::dontSendNotification);
    }

    void audioDeviceAboutToStart (yup::AudioIODevice* device) override
    {
        const auto maxBlockSize = yup::jmax (device->getDefaultBufferSize(), SynthExample::maxBlockSize);

        synth.prepare (device->getCurrentSampleRate(), maxBlockSize);

        renderBuffer.setSize (2, maxBlockSize, false, true, true);
        inputData.assign (static_cast<std::size_t> (maxBlockSize), 0.0f);
        renderData.assign (static_cast<std::size_t> (maxBlockSize), 0.0f);
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
        if (numSamples > renderBuffer.getNumSamples())
        {
            for (int channel = 0; channel < numOutputChannels; ++channel)
                yup::FloatVectorOperations::clear (outputChannelData[channel], numSamples);

            return;
        }

        for (int channel = 0; channel < renderBuffer.getNumChannels(); ++channel)
            yup::FloatVectorOperations::clear (renderBuffer.getWritePointer (channel), numSamples);

        midiBuffer.clear();
        keyboardState.processNextMidiBuffer (midiBuffer, 0, numSamples, true);
        synth.renderNextBlock (renderBuffer, midiBuffer, 0, numSamples);

        const auto gain = masterVolume.load();
        const auto* display = renderBuffer.getReadPointer (0);

        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            const auto* source = renderBuffer.getReadPointer (yup::jmin (channel, renderBuffer.getNumChannels() - 1));
            auto* destination = outputChannelData[channel];

            for (int sample = 0; sample < numSamples; ++sample)
                destination[sample] = std::tanh (source[sample] * gain);
        }

        {
            const yup::CriticalSection::ScopedLockType sl (renderMutex);

            for (int sample = 0; sample < numSamples; ++sample)
                inputData[static_cast<std::size_t> (sample)] = std::tanh (display[sample] * gain);

            std::swap (inputData, renderData);
        }
    }

    void visibilityChanged() override
    {
        if (! isVisible())
            deviceManager.removeAudioCallback (this);
        else
            deviceManager.addAudioCallback (this);
    }

private:
    void randomizeOscillators()
    {
        auto& random = yup::Random::getSystemRandom();

        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            auto& settings = synth.getOscillatorSettings (index);

            settings.type = random.nextInt (4);
            settings.waveform = random.nextInt (6);
            settings.shape = random.nextInt (6);
            settings.syncMode = random.nextInt (4);
            settings.level = 0.2f + random.nextFloat() * 0.8f;
            // The knob still reaches two octaves for deliberate stacking, but randomizing
            // that far apart just sounds out of tune, so this stays within a beating range.
            settings.detuneSemitones = random.nextFloat() - 0.5f;
            settings.followerRatio = 0.5f + random.nextFloat() * 3.0f;
            settings.morph = random.nextFloat();
            settings.phaseDistortion = 0.1f + random.nextFloat() * 0.8f;
            settings.fmAmount = random.nextFloat() * 2.0f;
            settings.fmRatio = 0.5f + random.nextFloat() * 3.0f;
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
    static constexpr float outerInset = 10.0f;
    static constexpr float headerHeight = 44.0f;
    static constexpr float spacing = 8.0f;

    //==============================================================================
    yup::AudioDeviceManager deviceManager;
    HarmonicSynthEngine synth;

    // MIDI keyboard components
    yup::MidiKeyboardState keyboardState;
    yup::MidiKeyboardComponent keyboardComponent;

    yup::AudioBuffer<float> renderBuffer;
    yup::MidiBuffer midiBuffer;
    std::vector<float> renderData;
    std::vector<float> inputData;
    yup::CriticalSection renderMutex;

    // UI Components
    yup::Label titleLabel;
    yup::Label subtitleLabel;
    yup::Label voiceLabel;

    std::array<std::unique_ptr<SynthOscillatorPanel>, SynthExample::oscillatorCount> oscillatorPanels;
    std::unique_ptr<SynthEnvelopePanel> envelopePanel;

    yup::TextButton randomizeButton { "RANDOMIZE" };
    yup::TextButton clearButton { "ALL NOTES OFF" };
    std::unique_ptr<KnobControl> volumeKnob;
    Oscilloscope oscilloscope;

    std::atomic<float> masterVolume { 0.5f };
};
