/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
constexpr double envelopeRampSeconds = 0.02;
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
    additive,  /**< Exact additive synthesis of a Fourier series   */
    wavetable, /**< The same series rendered once, then played back */
    sync,      /**< Alias-free spectral oscillator synchronization */
    morphing,  /**< Blends two synchronized endpoint spectra       */
    modulated  /**< Oversampled morph, FM, PM and phase distortion */
};

/** @internal Item names for SynthOscillatorType, index aligned with the enumeration. */
inline yup::StringArray getSynthOscillatorTypeNames()
{
    return { "Additive", "Wavetable", "Sync", "Morphing", "Modulated" };
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
/** A plain snapshot of one oscillator's controls.

    The audio thread takes one snapshot per block and compares it with the values it
    applied last time, so a control that did not move never costs a spectral
    transform or a table render.
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
};

/** The same controls, edited from the message thread while the audio thread reads them. */
struct SynthOscillatorSettings
{
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
                 fmRatio.load() };
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
    only the controls that actually moved, so editing a slider is the only thing that
    pays for a spectral transform or a table render.

    @see SynthOscillatorSettings, SynthOscillatorResources
*/
class SynthOscillator
{
public:
    /** Allocates every backend and attaches the shared waveform resources. */
    void prepare (double newSampleRate, int maxBlockSize, const SynthOscillatorResources& oscillatorResources)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        resources = &oscillatorResources;

        additive.prepare (sampleRate, SynthExample::maxHarmonics);
        wavetable.prepare (sampleRate, SynthExample::maxHarmonics);
        sync.prepare (sampleRate, SynthExample::maxHarmonics);
        morphing.prepare (sampleRate, SynthExample::maxHarmonics);
        modulated.prepare (sampleRate, maxBlockSize, resources->getBank());

        applied = {};
        hasAppliedValues = false;
    }

    /** Restarts every backend from a common phase. */
    void reset (double initialPhase) noexcept
    {
        const auto phase = static_cast<float> (initialPhase);

        additive.setPhase (phase);
        wavetable.setPhase (phase);
        sync.setPhase (phase);
        morphing.setPhase (phase);
        modulated.reset (initialPhase);

        modulatorPhase = 0.0;
    }

    /** Applies the pending changes and writes one block of the selected algorithm. */
    void renderBlock (float* output, int numSamples, const SynthOscillatorValues& values, double frequency) noexcept
    {
        applyParameters (values, frequency);

        switch (values.type)
        {
            case SynthOscillatorType::additive:
                additive.processBlock (output, numSamples);
                break;

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

private:
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

        modulated.processModulatedBlock (output, numSamples, [this, &parameters, modulatorIncrement, modulatorDepth] (int)
        {
            parameters.linearFM = modulatorDepth * std::sin (yup::MathConstants<double>::twoPi * modulatorPhase);

            modulatorPhase += modulatorIncrement;
            modulatorPhase -= std::floor (modulatorPhase);

            return parameters;
        });
    }

    //==============================================================================
    /** Pushes only the controls whose value changed since the last block. */
    void applyParameters (const SynthOscillatorValues& values, double frequency) noexcept
    {
        const auto typeChanged = ! hasAppliedValues || applied.type != values.type;
        const auto waveformChanged = typeChanged || applied.waveform != values.waveform;
        const auto shapeChanged = typeChanged || applied.shape != values.shape;
        const auto syncModeChanged = typeChanged || applied.syncMode != values.syncMode;
        const auto ratioChanged = typeChanged || applied.followerRatio != values.followerRatio;

        switch (values.type)
        {
            case SynthOscillatorType::additive:
                if (waveformChanged)
                    additive.setWaveform (values.waveform);
                break;

            case SynthOscillatorType::wavetable:
                if (waveformChanged)
                    wavetable.setWaveform (values.waveform);
                break;

            case SynthOscillatorType::sync:
                if (waveformChanged)
                    sync.setWaveform (values.waveform);
                if (syncModeChanged)
                    sync.setSyncMode (values.syncMode);
                if (ratioChanged)
                    sync.setFollowerRatio (values.followerRatio);
                break;

            case SynthOscillatorType::morphing:
                if (waveformChanged || shapeChanged)
                    morphing.setSeries (resources->getFrame (values.waveform), resources->getFrame (values.shape));
                if (syncModeChanged)
                    morphing.setSyncMode (values.syncMode);
                if (ratioChanged)
                    morphing.setFollowerRatio (values.followerRatio);
                break;

            case SynthOscillatorType::modulated:
                break;
        }

        additive.setFrequency (frequency);
        wavetable.setFrequency (frequency);
        sync.setFrequency (frequency);
        morphing.setFrequency (frequency);

        applied = values;
        hasAppliedValues = true;
    }

    //==============================================================================
    yup::AdditiveOscillator<float> additive;
    yup::WavetableOscillator<float> wavetable;
    yup::SyncOscillator<float> sync;
    yup::MorphingOscillator<float> morphing;
    yup::ModulatedOscillator<float> modulated;

    const SynthOscillatorResources* resources = nullptr;
    SynthOscillatorValues applied;
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
                const SynthOscillatorResources& oscillatorResources)
        : settings (oscillatorSettings)
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

        envelope.reset (sampleRate, SynthExample::envelopeRampSeconds);

        const auto blockSize = static_cast<std::size_t> (yup::jmax (1, maxBlockSize));

        scratch.assign (blockSize, 0.0f);
        mix.assign (blockSize, 0.0f);
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
        releaseRequested = false;

        pitchWheelMoved (currentPitchWheelPosition);

        for (auto& oscillator : oscillators)
            oscillator.reset (0.0);

        envelope.reset (getSampleRate(), SynthExample::envelopeRampSeconds);
        envelope.setTargetValue (1.0f);
    }

    void stopNote (float, bool allowTailOff) override
    {
        envelope.setTargetValue (0.0f);

        if (! allowTailOff)
        {
            releaseRequested = false;
            clearCurrentNote();
            return;
        }

        releaseRequested = true;
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

        jassert (numSamples <= static_cast<int> (scratch.size()));

        const auto frequency = noteFrequency * pitchWheelRatio;
        const auto numChannelsToWrite = yup::jmin (outputBuffer.getNumChannels(), 2);

        float* channels[2] = {};

        for (int channel = 0; channel < numChannelsToWrite; ++channel)
            channels[channel] = outputBuffer.getWritePointer (channel, startSample);

        yup::FloatVectorOperations::clear (mix.data(), numSamples);

        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            const auto values = settings[static_cast<std::size_t> (index)].read();
            auto& level = levels[static_cast<std::size_t> (index)];

            yup::FloatVectorOperations::clear (scratch.data(), numSamples);
            oscillators[static_cast<std::size_t> (index)].renderBlock (scratch.data(), numSamples, values, frequency);

            level.setTargetValue (values.level);

            for (int sample = 0; sample < numSamples; ++sample)
                mix[static_cast<std::size_t> (sample)] += scratch[static_cast<std::size_t> (sample)] * level.getNextValue();
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto value = mix[static_cast<std::size_t> (sample)] * velocityGain * envelope.getNextValue();

            for (int channel = 0; channel < numChannelsToWrite; ++channel)
                channels[channel][sample] += value;
        }

        if (releaseRequested && envelope.getCurrentValue() <= 0.0f)
        {
            releaseRequested = false;
            clearCurrentNote();
        }
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
    const SynthOscillatorResources& resources;

    std::array<SynthOscillator, SynthExample::oscillatorCount> oscillators;
    std::array<yup::SmoothedValue<float>, SynthExample::oscillatorCount> levels;

    yup::SmoothedValue<float> envelope;
    std::vector<float> scratch;
    std::vector<float> mix;

    double noteFrequency = 440.0;
    double pitchWheelRatio = 1.0;
    float velocityGain = 1.0f;
    bool releaseRequested = false;
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
            auto voice = yup::ReferenceCountedObjectPtr<SynthVoice> (new SynthVoice (settings, resources));

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

    /** Returns the note of a sounding voice, or -1 when the synthesiser is silent. */
    int getCurrentlyPlayingNote() const noexcept
    {
        for (int index = 0; index < ownedVoices.size(); ++index)
            if (ownedVoices[index] != nullptr && ownedVoices[index]->isVoiceActive())
                return ownedVoices[index]->getCurrentlyPlayingNote();

        return -1;
    }

private:
    SynthOscillatorResources resources;
    std::array<SynthOscillatorSettings, SynthExample::oscillatorCount> settings;
    yup::ReferenceCountedArray<SynthVoice> ownedVoices;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicSynthEngine)
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
        g.setFillColor (yup::Color (0xff101010));
        g.fillAll();

        if (renderData.empty())
            return;

        const auto lineColor = yup::Color (0xff4b4bff);
        const auto xSize = getWidth() / static_cast<float> (renderData.size());

        path.clear();
        path.reserveSpace (static_cast<int> (renderData.size()));
        path.moveTo (0.0f, (renderData[0] + 1.0f) * 0.5f * getHeight());

        for (std::size_t i = 1; i < renderData.size(); ++i)
            path.lineTo (static_cast<float> (i) * xSize, (renderData[i] + 1.0f) * 0.5f * getHeight());

        filledPath = path.createStrokePolygon (4.0f);

        g.setFillColor (lineColor);
        g.setFeather (8.0f);
        g.fillPath (filledPath);

        g.setFillColor (lineColor.brighter (0.2f));
        g.setFeather (4.0f);
        g.fillPath (filledPath);

        g.setStrokeColor (lineColor.withAlpha (0.8f));
        g.setStrokeWidth (2.0f);
        g.strokePath (path);

        g.setStrokeColor (lineColor.brighter (0.3f));
        g.setStrokeWidth (1.0f);
        g.strokePath (path);

        g.setStrokeColor (yup::Colors::white.withAlpha (0.9f));
        g.setStrokeWidth (0.5f);
        g.strokePath (path);
    }

private:
    std::vector<float> renderData;
    yup::Path path;
    yup::Path filledPath;
};

//==============================================================================
/** A caption and a combo box laid out as a single row. */
class LabeledComboBox : public yup::Component
{
public:
    LabeledComboBox (const yup::String& caption, const yup::StringArray& items, const yup::Font& font)
    {
        setOpaque (false); // the row draws nothing itself, only its caption and combo box do

        label.setText (caption, yup::dontSendNotification);
        label.setFont (font);
        label.setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);
        addAndMakeVisible (label);

        comboBox.addItemList (items, 1);
        comboBox.setTextWhenNothingSelected ("-");
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
        label.setBounds (bounds.removeFromLeft (captionWidth()));
        comboBox.setBounds (bounds);
    }

private:
    int captionWidth() const noexcept { return yup::jmax (28, (int) getWidth() / 3); }

    yup::Label label;
    yup::ComboBox comboBox;
};

//==============================================================================
/** A caption and a horizontal slider laid out as a single row. */
class LabeledSlider : public yup::Component
{
public:
    LabeledSlider (const yup::String& caption,
                   double minimum,
                   double maximum,
                   double interval,
                   double defaultValue,
                   const yup::Font& font)
    {
        setOpaque (false); // the row draws nothing itself, only its caption and slider do

        label.setText (caption, yup::dontSendNotification);
        label.setFont (font);
        label.setColor (yup::Label::Style::textFillColorId, yup::Colors::lightgray);
        addAndMakeVisible (label);

        slider.setRange (minimum, maximum, interval);
        slider.setDefaultValue (defaultValue);
        slider.setValue (defaultValue, yup::dontSendNotification);
        slider.onValueChanged = [this] (double value)
        {
            if (onChange != nullptr)
                onChange (value);
        };
        addAndMakeVisible (slider);
    }

    /** Called with the new slider value. */
    std::function<void (double)> onChange;

    yup::Slider& getSlider() noexcept { return slider; }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds (bounds.removeFromLeft (captionWidth()));
        slider.setBounds (bounds);
    }

private:
    int captionWidth() const noexcept { return yup::jmax (28, (int) getWidth() / 3); }

    yup::Label label;
    yup::Slider slider { yup::Slider::LinearHorizontal };
};

//==============================================================================
/** The editing surface of one oscillator, writing straight into the voice settings.

    Every widget is wired to a single atomic setting, and refresh() copies the
    settings back into the widgets for changes coming from somewhere else, such as
    the randomize button.

    @see SynthOscillatorSettings
*/
class SynthOscillatorPanel : public yup::Component
{
public:
    SynthOscillatorPanel (const yup::String& panelTitle, SynthOscillatorSettings& settingsToEdit, const yup::Font& font)
        : settings (settingsToEdit)
        , algorithmRow ("Algorithm", getSynthOscillatorTypeNames(), font)
        , waveformRow ("Waveform", getSynthWaveformNames(), font)
        , shapeRow ("Shape B", getSynthWaveformNames(), font)
        , syncModeRow ("Sync Mode", getSynthSyncModeNames(), font)
        , levelRow ("Level", 0.0, 1.0, 0.001, 0.5, font)
        , detuneRow ("Detune", -24.0, 24.0, 0.1, 0.0, font)
        , ratioRow ("Sync Ratio", 0.25, 8.0, 0.01, 1.5, font)
        , morphRow ("Morph", 0.0, 1.0, 0.001, 0.0, font)
        , distortionRow ("Distortion", 0.01, 0.99, 0.001, 0.5, font)
        , fmAmountRow ("FM Amount", 0.0, 4.0, 0.001, 0.0, font)
        , fmRatioRow ("FM Ratio", 0.25, 8.0, 0.01, 2.0, font)
    {
        titleLabel.setText (panelTitle, yup::dontSendNotification);
        titleLabel.setFont (font.withHeight (12.0f));
        titleLabel.setColor (yup::Label::Style::textFillColorId, yup::Colors::white);
        addAndMakeVisible (titleLabel);

        const auto addRow = [this] (yup::Component& row)
        {
            addAndMakeVisible (row);
            rows.push_back (&row);
        };

        addRow (algorithmRow);
        addRow (waveformRow);
        addRow (shapeRow);
        addRow (syncModeRow);
        addRow (levelRow);
        addRow (detuneRow);
        addRow (ratioRow);
        addRow (morphRow);
        addRow (distortionRow);
        addRow (fmAmountRow);
        addRow (fmRatioRow);

        algorithmRow.onChange = [this] (int id) { settings.type = id - 1; };
        waveformRow.onChange = [this] (int id) { settings.waveform = id - 1; };
        shapeRow.onChange = [this] (int id) { settings.shape = id - 1; };
        syncModeRow.onChange = [this] (int id) { settings.syncMode = id - 1; };
        levelRow.onChange = [this] (double value) { settings.level = static_cast<float> (value); };
        detuneRow.onChange = [this] (double value) { settings.detuneSemitones = static_cast<float> (value); };
        ratioRow.onChange = [this] (double value) { settings.followerRatio = static_cast<float> (value); };
        morphRow.onChange = [this] (double value) { settings.morph = static_cast<float> (value); };
        distortionRow.onChange = [this] (double value) { settings.phaseDistortion = static_cast<float> (value); };
        fmAmountRow.onChange = [this] (double value) { settings.fmAmount = static_cast<float> (value); };
        fmRatioRow.onChange = [this] (double value) { settings.fmRatio = static_cast<float> (value); };

        refresh();
    }

    /** Reads the settings back into the widgets. */
    void refresh()
    {
        algorithmRow.getComboBox().setSelectedId (settings.type.load() + 1, yup::dontSendNotification);
        waveformRow.getComboBox().setSelectedId (settings.waveform.load() + 1, yup::dontSendNotification);
        shapeRow.getComboBox().setSelectedId (settings.shape.load() + 1, yup::dontSendNotification);
        syncModeRow.getComboBox().setSelectedId (settings.syncMode.load() + 1, yup::dontSendNotification);

        levelRow.getSlider().setValue (settings.level.load(), yup::dontSendNotification);
        detuneRow.getSlider().setValue (settings.detuneSemitones.load(), yup::dontSendNotification);
        ratioRow.getSlider().setValue (settings.followerRatio.load(), yup::dontSendNotification);
        morphRow.getSlider().setValue (settings.morph.load(), yup::dontSendNotification);
        distortionRow.getSlider().setValue (settings.phaseDistortion.load(), yup::dontSendNotification);
        fmAmountRow.getSlider().setValue (settings.fmAmount.load(), yup::dontSendNotification);
        fmRatioRow.getSlider().setValue (settings.fmRatio.load(), yup::dontSendNotification);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (4);
        titleLabel.setBounds (bounds.removeFromTop (titleHeight()));

        if (rows.empty())
            return;

        const auto rowHeight = bounds.getHeight() / static_cast<int> (rows.size());

        for (auto* row : rows)
            row->setBounds (bounds.removeFromTop (rowHeight));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray).darker (0.4f));
        g.fillAll();
    }

private:
    int titleHeight() const noexcept { return 18; }

    SynthOscillatorSettings& settings;

    yup::Label titleLabel;
    std::vector<yup::Component*> rows;

    LabeledComboBox algorithmRow;
    LabeledComboBox waveformRow;
    LabeledComboBox shapeRow;
    LabeledComboBox syncModeRow;
    LabeledSlider levelRow;
    LabeledSlider detuneRow;
    LabeledSlider ratioRow;
    LabeledSlider morphRow;
    LabeledSlider distortionRow;
    LabeledSlider fmAmountRow;
    LabeledSlider fmRatioRow;
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
        addAndMakeVisible (keyboardComponent);

        titleLabel = std::make_unique<yup::Label> ("Title");
        titleLabel->setText ("YUP Polyphonic Oscillator Synthesizer");
        titleLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::white);
        addAndMakeVisible (*titleLabel);

        subtitleLabel = std::make_unique<yup::Label> ("Subtitle");
        subtitleLabel->setText ("Eight voices, two oscillators each - play the keyboard and shape both oscillators of the sound");
        subtitleLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::white);
        addAndMakeVisible (*subtitleLabel);

        noteIndicatorLabel = std::make_unique<yup::Label> ("NoteIndicator");
        noteIndicatorLabel->setText ("");
        noteIndicatorLabel->setColor (yup::Label::Style::textFillColorId, yup::Colors::black);
        noteIndicatorLabel->setColor (yup::Label::Style::backgroundColorId, yup::Colors::yellow.withAlpha (0.8f));
        addChildComponent (*noteIndicatorLabel);

        const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();

        for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        {
            oscillatorPanels[static_cast<std::size_t> (index)] = std::make_unique<SynthOscillatorPanel> (
                yup::String ("Oscillator ") + yup::String (index + 1),
                synth.getOscillatorSettings (index),
                font.withHeight (11.0f));

            addAndMakeVisible (*oscillatorPanels[static_cast<std::size_t> (index)]);
        }

        randomizeButton = std::make_unique<yup::TextButton> ("Randomize");
        randomizeButton->onClick = [this] { randomizeOscillators(); };
        addAndMakeVisible (*randomizeButton);

        clearButton = std::make_unique<yup::TextButton> ("All Notes Off");
        clearButton->onClick = [this]
        {
            keyboardState.allNotesOff (0); // Turn off all notes on all channels
            synth.allNotesOff (0, true);
        };
        addAndMakeVisible (*clearButton);

        volumeSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal, "Volume");
        volumeSlider->setRange (0.0, 1.0, 0.001);
        volumeSlider->setDefaultValue (0.5);
        volumeSlider->onValueChanged = [this] (double value)
        {
            masterVolume = static_cast<float> (value);
        };
        volumeSlider->setValue (0.5, yup::dontSendNotification);
        addAndMakeVisible (*volumeSlider);

        addAndMakeVisible (oscilloscope);
    }

    ~AudioExample() override
    {
        deviceManager.removeAudioCallback (this);
        deviceManager.closeAudioDevice();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        titleLabel->setBounds (bounds.removeFromTop (proportionOfHeight (0.05f)));
        subtitleLabel->setBounds (bounds.removeFromTop (proportionOfHeight (0.03f)));

        keyboardComponent.setBounds (bounds.removeFromBottom (proportionOfHeight (0.20f))
                                           .reduced (proportionOfWidth (0.02f), proportionOfHeight (0.01f)));

        oscilloscope.setBounds (bounds.removeFromBottom (proportionOfHeight (0.20f))
                                      .reduced (proportionOfWidth (0.01f), proportionOfHeight (0.01f)));

        auto buttonArea = bounds.removeFromBottom (proportionOfHeight (0.08f));

        const auto buttonWidth = buttonArea.getWidth() / 3;
        const auto buttonInsetX = proportionOfWidth (0.01f);
        const auto buttonInsetY = proportionOfHeight (0.01f);

        randomizeButton->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (buttonInsetX, buttonInsetY));
        clearButton->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (buttonInsetX, buttonInsetY));
        volumeSlider->setBounds (buttonArea.removeFromLeft (buttonWidth).reduced (buttonInsetX, buttonInsetY));

        const auto panelWidth = bounds.getWidth() / SynthExample::oscillatorCount;

        for (auto& panel : oscillatorPanels)
            if (panel != nullptr)
                panel->setBounds (bounds.removeFromLeft (panelWidth).reduced (6));

        noteIndicatorLabel->setBounds (yup::Rectangle<int> (10, getHeight() - 40, 200, 30));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
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

        const auto playingNote = synth.getCurrentlyPlayingNote();

        if (playingNote >= 0)
        {
            noteIndicatorLabel->setText (yup::String ("Playing Note: ") + yup::String (playingNote),
                                         yup::dontSendNotification);
            noteIndicatorLabel->setVisible (true);
        }
        else
        {
            noteIndicatorLabel->setVisible (false);
        }
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

            settings.type = random.nextInt (5);
            settings.waveform = random.nextInt (6);
            settings.shape = random.nextInt (6);
            settings.syncMode = random.nextInt (4);
            settings.level = 0.2f + random.nextFloat() * 0.8f;
            settings.detuneSemitones = random.nextFloat() * 24.0f - 12.0f;
            settings.followerRatio = 0.5f + random.nextFloat() * 3.0f;
            settings.morph = random.nextFloat();
            settings.phaseDistortion = 0.1f + random.nextFloat() * 0.8f;
            settings.fmAmount = random.nextFloat() * 2.0f;
            settings.fmRatio = 0.5f + random.nextFloat() * 3.0f;

            if (auto& panel = oscillatorPanels[static_cast<std::size_t> (index)]; panel != nullptr)
                panel->refresh();
        }
    }

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
    std::unique_ptr<yup::Label> titleLabel;
    std::unique_ptr<yup::Label> subtitleLabel;
    std::unique_ptr<yup::Label> noteIndicatorLabel;

    std::array<std::unique_ptr<SynthOscillatorPanel>, SynthExample::oscillatorCount> oscillatorPanels;

    std::unique_ptr<yup::TextButton> randomizeButton;
    std::unique_ptr<yup::TextButton> clearButton;
    std::unique_ptr<yup::Slider> volumeSlider;
    Oscilloscope oscilloscope;

    std::atomic<float> masterVolume { 0.5f };
};
