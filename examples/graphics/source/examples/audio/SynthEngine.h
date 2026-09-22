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

#include "SynthSettings.h"

#include <array>
#include <atomic>
#include <cmath>
#include <vector>

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

    /** Returns the gain the envelope last produced, for use as a modulation source. */
    float getLevel() const noexcept { return level; }

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
/** Derives the series one oscillator plays: partials or preset, Prism shaping, sync.

    Shared by the engine's slot, by a voice that needs its own spectrum while an
    envelope modulates it, and by the waveform preview. prepare() allocates; update()
    does not.

    The derived series is rescaled so its waveform peaks where the source's does.
    yup::PrismSpectrum preserves the coefficient sum, which bounds a peak from well
    above - three times over for a sawtooth - and how close the waveform comes to
    that bound depends on how aligned the harmonic phases are. Dispersion, scatter and
    the sync reset all move exactly that, so without the rescale they would swing the
    output level as they are swept rather than only recolouring it.

    @see SynthOscillatorSlot, SynthOscillator, WaveformEditor
*/
class SynthSpectrumDerivation
{
public:
    /** Allocates the shaper, the resampler, the series and the peak meter. */
    void prepare()
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

        hasApplied = false;
    }

    /** Rebuilds the series if anything it depends on moved. Returns true when it did. */
    bool update (const SynthOscillatorValues& values,
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

        applied = values;
        hasApplied = true;

        if (sourceChanged)
            sourcePeak = measurePeak (source);

        if (! (sourceChanged || shapeChanged))
            return false;

        spectrum.process (source, shapedSeries, toPrismShape (values), values.color);

        if (values.syncMode == yup::SyncMode::none)
        {
            derived = &shapedSeries;
        }
        else
        {
            resampler.transform (shapedSeries, static_cast<double> (values.syncRatio), values.syncMode, syncedSeries);
            derived = &syncedSeries;
        }

        matchSourcePeak (*derived);
        return true;
    }

    /** Returns the series the last update() derived. */
    const yup::FourierSeries<double>& getSeries() const noexcept { return *derived; }

    /** Forgets what was applied so the next update() rebuilds unconditionally. */
    void invalidate() noexcept { hasApplied = false; }

private:
    /** Points the display samples the peak search walks. Twice the harmonic count
        resolves the highest harmonic; this is four times it, for a little margin. */
    static constexpr int peakResolution = 512;

    /** Returns the largest absolute value one period of a series reaches.

        The series is rendered with the same inverse FFT the voices use rather than
        summed harmonic by harmonic, which would cost a transcendental per harmonic per
        point.
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
    yup::FourierSeries<double>* derived = &shapedSeries;
    SynthOscillatorValues applied;
    bool hasApplied = false;
};

//==============================================================================
/** The series every voice of one oscillator slot plays, rebuilt once per block.

    A slot's spectrum does not vary per voice unless an envelope is routed into it,
    so it is derived once here from the LFO-modulated values rather than inside each
    voice. Voices publish nothing back; they compare getGeneration() and re-render
    their own table when it moves, which keeps yup::WavetableOscillator's crossfade
    doing the smoothing. Runs on the audio thread before any voice reads it.

    @see SynthSpectrumDerivation, SynthOscillator
*/
class SynthOscillatorSlot
{
public:
    SynthOscillatorSlot() { derivation.prepare(); }

    /** Rebuilds the slot's series if anything it depends on moved. Audio thread. */
    void update (const SynthOscillatorValues& values,
                 const SynthOscillatorSettings& settings,
                 const SynthOscillatorResources& resources) noexcept
    {
        if (derivation.update (values, settings, resources))
            ++generation;
    }

    /** Returns the series the voices of this slot should be playing. */
    const yup::FourierSeries<double>& getSeries() const noexcept { return derivation.getSeries(); }

    /** Bumped whenever getSeries() changed, so a voice knows to re-render its table. */
    int getGeneration() const noexcept { return generation; }

private:
    SynthSpectrumDerivation derivation;
    int generation = 0;
};

//==============================================================================
/** One of a voice's oscillators: a wavetable playing a derived series, plus unison.

    prepare() allocates the backends; renderBlock() is allocation-free and only
    re-renders a table when its series moved. The series normally comes from the
    shared slot; while an envelope is routed into this oscillator's spectrum it is
    derived here instead, from the voice's own values.

    Unison is built from bare yup::WavetableOscillator satellites playing the same
    series as the center, detuned and panned around it.

    @see SynthOscillatorSettings, SynthOscillatorSlot, SynthSpectrumDerivation
*/
class SynthOscillator
{
public:
    /** Allocates every backend and attaches the shared slot, settings and resources. */
    void prepare (double newSampleRate,
                  int maxBlockSize,
                  const SynthOscillatorSlot& sharedSlot,
                  const SynthOscillatorSettings& oscillatorSettings,
                  const SynthOscillatorResources& oscillatorResources)
    {
        const auto sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        slot = &sharedSlot;
        settings = &oscillatorSettings;
        resources = &oscillatorResources;

        wavetable.prepare (sampleRate, SynthExample::maxHarmonics);

        for (auto& satellite : satellites)
            satellite.prepare (sampleRate, SynthExample::maxHarmonics);

        localDerivation.prepare();
        slotBuffer.assign (static_cast<std::size_t> (yup::jmax (1, maxBlockSize)), 0.0f);

        appliedSeriesGeneration = -1;
        usingLocalSeries = false;
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

        @param deriveLocally  True while an envelope modulates this oscillator's spectrum,
                              in which case the series is derived here from these values
                              rather than taken from the slot.
    */
    void renderBlock (float* left,
                      float* right,
                      int numSamples,
                      const SynthOscillatorValues& values,
                      double frequency,
                      bool deriveLocally) noexcept
    {
        yup::FloatVectorOperations::clear (left, numSamples);
        yup::FloatVectorOperations::clear (right, numSamples);

        // The table holds one period of the synced waveform, which for mirrored sync is
        // two leader periods, so it is played at the fundamental the transform produced.
        const auto played = frequency * yup::SyncSpectralResampler<double>::getFundamentalScale (values.syncMode);

        const auto slotCount = yup::jlimit (1, SynthExample::maxUnisonVoices, values.unisonVoices);
        const auto centreIndex = (slotCount - 1) / 2;
        const auto slotGain = 1.0f / static_cast<float> (slotCount);

        applySeries (values, deriveLocally);

        renderTable (wavetable, numSamples, detunedFrequency (played, values, centreIndex, slotCount));
        accumulateSlot (left, right, numSamples, slotOffset (centreIndex, slotCount) * values.unisonSpread, slotGain);

        for (int index = 0, satellite = 0; index < slotCount; ++index)
        {
            if (index == centreIndex)
                continue;

            renderTable (satellites[static_cast<std::size_t> (satellite++)],
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

    /** Renders one table into the slot buffer; render() crossfades into a new table,
        which is what keeps a modulated shape from stepping. */
    void renderTable (yup::WavetableOscillator<float>& table, int numSamples, double frequency) noexcept
    {
        table.setFrequency (frequency);

        if (table.needsRender())
            table.render();

        table.processBlock (slotBuffer.data(), numSamples);
    }

    /** Hands the right series to every table when it moved since the last block. */
    void applySeries (const SynthOscillatorValues& values, bool deriveLocally) noexcept
    {
        if (deriveLocally)
        {
            if (! usingLocalSeries)
            {
                localDerivation.invalidate();
                usingLocalSeries = true;
            }

            if (localDerivation.update (values, *settings, *resources))
                setSeries (localDerivation.getSeries());

            return;
        }

        const auto generation = slot->getGeneration();

        if (usingLocalSeries || generation != appliedSeriesGeneration)
        {
            usingLocalSeries = false;
            appliedSeriesGeneration = generation;
            setSeries (slot->getSeries());
        }
    }

    void setSeries (const yup::FourierSeries<double>& series) noexcept
    {
        wavetable.setSeries (series);

        for (auto& satellite : satellites)
            satellite.setSeries (series);
    }

    //==============================================================================
    yup::WavetableOscillator<float> wavetable;
    std::array<yup::WavetableOscillator<float>, SynthExample::maxUnisonVoices - 1> satellites;
    SynthSpectrumDerivation localDerivation;

    const SynthOscillatorSlot* slot = nullptr;
    const SynthOscillatorSettings* settings = nullptr;
    const SynthOscillatorResources* resources = nullptr;
    std::vector<float> slotBuffer;
    int appliedSeriesGeneration = -1;
    bool usingLocalSeries = false;
};

//==============================================================================
/** The per-voice filter: a drive stage into one VAStateVariableFilter per channel.

    Cutoff, resonance and drive are smoothed across a control chunk so modulation
    cannot zipper. Off skips everything, leaving the filter state where it was.

    @see SynthFilterSettings
*/
class SynthFilterStage
{
public:
    /** Prepares both channels and the parameter smoothers. */
    void prepare (double sampleRate)
    {
        for (auto& filter : filters)
            filter.prepare (sampleRate, SynthExample::maxBlockSize);

        cutoff.reset (sampleRate, 0.005);
        resonance.reset (sampleRate, 0.005);
        drive.reset (sampleRate, 0.005);

        cutoff.setCurrentAndTargetValue (8000.0f);
        resonance.setCurrentAndTargetValue (0.2f);
        drive.setCurrentAndTargetValue (0.0f);
    }

    /** Clears the filter memory, for a fresh note. */
    void reset() noexcept
    {
        for (auto& filter : filters)
            filter.reset();
    }

    /** Filters a stereo chunk in place at the note's keytracked cutoff. */
    void process (float* left, float* right, int numSamples, const SynthFilterValues& values, double midiNote) noexcept
    {
        if (values.type == SynthFilterType::off)
            return;

        const auto tracked = values.cutoff * std::exp2 (values.keytrack * static_cast<float> (midiNote - 60.0) / 12.0f);

        cutoff.setTargetValue (yup::jlimit (20.0f, 20000.0f, tracked));
        resonance.setTargetValue (values.resonance);
        drive.setTargetValue (values.drive);

        const auto mode = toFilterMode (values.type);

        for (auto& filter : filters)
        {
            filter.setMode (mode);
            filter.setShelfGain (12.0f);
        }

        float* channels[] = { left, right };

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto frequency = cutoff.getNextValue();
            const auto q = yup::VAStateVariableFilter<float>::resonanceToQ (resonance.getNextValue());
            const auto amount = drive.getNextValue();
            const auto gain = 1.0f + 9.0f * amount;
            const auto makeup = amount > 0.0f ? 1.0f / std::tanh (gain) : 1.0f;

            for (std::size_t channel = 0; channel < filters.size(); ++channel)
            {
                auto& filter = filters[channel];
                filter.setCutoffFrequency (frequency);
                filter.setQ (q);

                auto x = channels[channel][sample];

                if (amount > 0.0f)
                    x = std::tanh (x * gain) * makeup;

                channels[channel][sample] = filter.processSample (x);
            }
        }
    }

private:
    std::array<yup::VAStateVariableFilter<float>, 2> filters;
    yup::SmoothedValue<float, yup::ValueSmoothingTypes::Multiplicative> cutoff;
    yup::SmoothedValue<float> resonance;
    yup::SmoothedValue<float> drive;
};

//==============================================================================
/** What the engine derived for this block, read by every voice while it renders.

    Written at the top of HarmonicSynthEngine::renderNextBlock, before any voice runs,
    on the same thread, so no publication handshake is needed.
*/
struct SynthBlockContext
{
    SynthPatchValues patch;                                                     /**< Settings with the LFO routings applied. */
    SynthModulationValues modulation;                                           /**< The routes, for the voice's envelope layer. */
    std::array<SynthEnvelopeValues, SynthExample::envelopeCount> envelopes;
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
/** A polyphonic voice: two oscillators into a filter, shaped by two envelopes. */
class SynthVoice : public yup::SynthesiserVoice
{
public:
    SynthVoice (const std::array<SynthOscillatorSettings, SynthExample::oscillatorCount>& oscillatorSettings,
                const SynthOscillatorResources& oscillatorResources,
                const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& sharedSlots,
                const SynthBlockContext& blockContext)
        : settings (oscillatorSettings)
        , resources (oscillatorResources)
        , slots (sharedSlots)
        , context (blockContext)
    {
    }

    /** Allocates every oscillator backend. Must run outside the audio callback. */
    void prepare (double sampleRate, int maxBlockSize)
    {
        for (std::size_t slot = 0; slot < oscillators.size(); ++slot)
            oscillators[slot].prepare (sampleRate, maxBlockSize, slots[slot], settings[slot], resources);

        for (auto& level : levels)
            level.reset (sampleRate, SynthExample::levelRampSeconds);

        filter.prepare (sampleRate);
        envelope.prepare (sampleRate);
        modulationEnvelope.prepare (sampleRate);
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

            envelope.setParameters (context.envelopes[0]);
            envelope.noteOn();
            modulationEnvelope.setParameters (context.envelopes[1]);
            modulationEnvelope.noteOn();
            filter.reset();
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
            modulationEnvelope.noteOff();
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
            modulationEnvelope.noteOffImmediate();
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

        envelope.setParameters (context.envelopes[0]);
        modulationEnvelope.setParameters (context.envelopes[1]);

        for (int offset = 0; offset < numSamples;)
        {
            const auto count = yup::jmin (numSamples - offset, static_cast<int> (mixLeft.size()), SynthExample::controlChunk);
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
            // The engine applied the LFO routings for this block; the envelopes are per
            // voice, so their routings are applied here on top, once per control chunk.
            auto patch = context.patch;
            const std::array<float, 4> sources { envelope.getLevel(), modulationEnvelope.getLevel(), 0.0f, 0.0f };
            applyModulation (patch, context.modulation, sources);

            const auto note = pitch.skip (numSamples) + bend.skip (numSamples);
            const auto frequency = midiNoteToFrequency (note);
            for (int index = 0; index < SynthExample::oscillatorCount; ++index)
            {
                const auto slot = static_cast<std::size_t> (index);
                const auto& values = patch.oscillators[slot];
                auto& level = levels[slot];
                const auto detuned = frequency * std::exp2 (values.octave + values.detuneSemitones / 12.0);
                oscillators[slot].renderBlock (oscLeft.data(), oscRight.data(), numSamples, values, detuned,
                                               context.modulation.hasVoiceSpectrumRoute (index));
                level.setTargetValue (values.level);

                for (int sample = 0; sample < numSamples; ++sample)
                {
                    const auto gain = level.getNextValue();
                    mixLeft[static_cast<std::size_t> (sample)] += oscLeft[static_cast<std::size_t> (sample)] * gain;
                    mixRight[static_cast<std::size_t> (sample)] += oscRight[static_cast<std::size_t> (sample)] * gain;
                }
            }

            filter.process (mixLeft.data(), mixRight.data(), numSamples, patch.filter, note);
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto gain = envelope.getNextValue() * velocityGain;
            modulationEnvelope.getNextValue();
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
    const SynthOscillatorResources& resources;
    const std::array<SynthOscillatorSlot, SynthExample::oscillatorCount>& slots;
    const SynthBlockContext& context;

    std::array<SynthOscillator, SynthExample::oscillatorCount> oscillators;
    std::array<yup::SmoothedValue<float>, SynthExample::oscillatorCount> levels;

    SynthEnvelope envelope;
    SynthEnvelope modulationEnvelope;
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
        envelopeSettings[1].attack = 0.2f;
        envelopeSettings[1].decay = 0.5f;
        envelopeSettings[1].sustain = 0.3f;
        envelopeSettings[1].release = 0.5f;

        for (int index = 0; index < SynthExample::voiceCount; ++index)
        {
            auto voice = yup::ReferenceCountedObjectPtr<SynthVoice> (new SynthVoice (settings, resources, oscillatorSlots, context));

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

        for (auto& lfo : lfos)
            lfo.prepare (sampleRate);

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
        // The LFOs are global, so their routings are applied once here and every voice
        // of a slot plays the same spectrum; the envelope routings are per voice and are
        // applied by each voice on top of this.
        std::array<float, 4> sources { 0.0f, 0.0f, 0.0f, 0.0f };

        for (std::size_t index = 0; index < lfos.size(); ++index)
        {
            const auto values = lfoSettings[index].read();
            auto& lfo = lfos[index];

            lfo.setShape (values.shape);
            lfo.setFrequency (values.rate);
            lfo.setPhaseOffset (values.phase);

            sources[2 + index] = lfo.getValue();
            lfoPhases[index].store (lfo.getPhase());
            lfo.skip (count);
        }

        for (std::size_t slot = 0; slot < oscillatorSlots.size(); ++slot)
            context.patch.oscillators[slot] = settings[slot].read();

        context.patch.filter = filterSettings.read();
        context.modulation = modulationSettings.read();

        for (std::size_t index = 0; index < envelopeSettings.size(); ++index)
            context.envelopes[index] = envelopeSettings[index].read();

        applyModulation (context.patch, context.modulation, sources);

        for (std::size_t slot = 0; slot < oscillatorSlots.size(); ++slot)
            oscillatorSlots[slot].update (context.patch.oscillators[slot], settings[slot], resources);

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

    /** Returns the settings edited by one of the envelope panels. */
    SynthEnvelopeSettings& getEnvelopeSettings (int envelopeIndex) noexcept
    {
        return envelopeSettings[static_cast<std::size_t> (envelopeIndex)];
    }

    /** Returns the settings edited by the filter panel. */
    SynthFilterSettings& getFilterSettings() noexcept { return filterSettings; }

    /** Returns the settings edited by one of the LFO panels. */
    SynthLFOSettings& getLFOSettings (int lfoIndex) noexcept
    {
        return lfoSettings[static_cast<std::size_t> (lfoIndex)];
    }

    /** Returns the routings edited by the modulation page. */
    SynthModulationSettings& getModulationSettings() noexcept { return modulationSettings; }

    /** Returns the phase of an LFO at the top of the last block, for its display. */
    float getLFOPhase (int lfoIndex) const noexcept
    {
        return lfoPhases[static_cast<std::size_t> (lfoIndex)].load();
    }

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
    std::array<SynthEnvelopeSettings, SynthExample::envelopeCount> envelopeSettings;
    SynthFilterSettings filterSettings;
    std::array<SynthLFOSettings, SynthExample::lfoCount> lfoSettings;
    SynthModulationSettings modulationSettings;
    std::array<yup::LFO<float>, SynthExample::lfoCount> lfos;
    std::array<std::atomic<float>, SynthExample::lfoCount> lfoPhases {};
    SynthBlockContext context;
    yup::ReferenceCountedArray<SynthVoice> ownedVoices;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicSynthEngine)
};
