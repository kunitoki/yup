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

#include "SynthPatch.h"

#include <vector>

namespace SynthPatch
{
namespace
{
const yup::Identifier stateType ("PrismState");
const yup::Identifier versionProperty ("version");
const yup::Identifier presetProperty ("preset");
constexpr int stateVersion = 1;

yup::Identifier getPartialsType (std::size_t oscillator)
{
    return yup::Identifier ("Partials" + yup::String (static_cast<int> (oscillator) + 1));
}

yup::DataTree createEmptyState (int presetIndex)
{
    yup::DataTree state (stateType);

    auto transaction = state.beginTransaction();
    transaction.setProperty (versionProperty, stateVersion);
    transaction.setProperty (presetProperty, presetIndex);

    return state;
}

//==============================================================================
struct PresetValue
{
    const char* id;
    float value;
};

struct FactoryPreset
{
    const char* name;
    std::vector<PresetValue> values;
    std::vector<float> osc1Partials; /**< Sine magnitudes from the fundamental up; empty plays the waveform. */
};

constexpr float toValue (SynthModulationSource source) noexcept { return static_cast<float> (source); }
constexpr float toValue (SynthModulationDestination destination) noexcept { return static_cast<float> (destination); }

const std::vector<FactoryPreset>& getFactoryPresets()
{
    using Source = SynthModulationSource;
    using Destination = SynthModulationDestination;

    static const std::vector<FactoryPreset> presets {
        { "Init", {}, {} },

        { "Glass Pad",
          { { "osc1.waveform", 0.0f }, { "osc1.ridges", 2.5f }, { "osc1.color", 0.6f }, { "osc1.dispersion", 0.3f },
            { "osc1.scatter", 0.2f }, { "osc1.unison", 3.0f }, { "osc1.unisonDetune", 0.25f }, { "osc1.unisonSpread", 0.8f },
            { "osc2.waveform", 0.0f }, { "osc2.syncMode", 0.0f }, { "osc2.octave", 1.0f }, { "osc2.level", 0.25f },
            { "osc2.ridges", 3.0f }, { "osc2.color", 0.4f },
            { "filter.cutoff", 3500.0f }, { "filter.resonance", 0.3f },
            { "env1.attack", 0.8f }, { "env1.decay", 1.2f }, { "env1.sustain", 0.8f }, { "env1.release", 2.0f },
            { "lfo1.rate", 0.3f },
            { "mod1.source", toValue (Source::lfo1) }, { "mod1.destination", toValue (Destination::osc1Color) }, { "mod1.depth", 0.3f },
            { "mod2.source", toValue (Source::env2) }, { "mod2.destination", toValue (Destination::filterCutoff) }, { "mod2.depth", 0.2f } },
          {} },

        { "Sync Lead",
          { { "voice.mode", static_cast<float> (SynthPlayMode::legato) }, { "voice.glide", 80.0f },
            { "osc1.syncMode", 1.0f }, { "osc1.syncRatio", 2.0f }, { "osc1.level", 0.6f },
            { "osc2.waveform", 3.0f }, { "osc2.octave", 0.0f }, { "osc2.syncMode", 0.0f }, { "osc2.detune", 7.0f },
            { "filter.cutoff", 2500.0f }, { "filter.resonance", 0.45f }, { "filter.drive", 0.3f }, { "filter.keytrack", 0.5f },
            { "env1.attack", 0.003f }, { "env1.decay", 0.4f }, { "env1.sustain", 0.6f }, { "env1.release", 0.25f },
            { "env2.attack", 0.001f }, { "env2.decay", 0.6f }, { "env2.sustain", 0.0f }, { "env2.release", 0.3f },
            { "lfo1.rate", 5.5f }, { "lfo1.retrigger", 0.0f },
            { "mod1.source", toValue (Source::env2) }, { "mod1.destination", toValue (Destination::osc1SyncRatio) }, { "mod1.depth", 0.4f },
            { "mod2.source", toValue (Source::modWheel) }, { "mod2.destination", toValue (Destination::filterCutoff) }, { "mod2.depth", 0.4f },
            { "mod3.source", toValue (Source::lfo1) }, { "mod3.destination", toValue (Destination::osc1Cents) }, { "mod3.depth", 0.05f } },
          {} },

        { "Formant Choir",
          { { "osc1.formant", 2.0f }, { "osc1.formantPosition", 3.0f }, { "osc1.oddEven", 0.4f }, { "osc1.tilt", -1.0f },
            { "osc1.unison", 5.0f }, { "osc1.unisonDetune", 0.35f }, { "osc1.unisonSpread", 1.0f },
            { "osc2.waveform", 2.0f }, { "osc2.syncMode", 0.0f }, { "osc2.octave", 0.0f }, { "osc2.level", 0.15f },
            { "osc2.formant", 1.5f }, { "osc2.formantPosition", 4.0f },
            { "filter.cutoff", 5000.0f },
            { "env1.attack", 0.4f }, { "env1.decay", 1.0f }, { "env1.sustain", 0.85f }, { "env1.release", 1.5f },
            { "lfo1.rate", 0.25f }, { "lfo2.shape", 1.0f }, { "lfo2.rate", 0.15f },
            { "mod1.source", toValue (Source::lfo1) }, { "mod1.destination", toValue (Destination::osc1FormantPosition) }, { "mod1.depth", 0.1f },
            { "mod2.source", toValue (Source::lfo2) }, { "mod2.destination", toValue (Destination::osc2FormantPosition) }, { "mod2.depth", 0.1f } },
          {} },

        { "Pluck Bass",
          { { "voice.mode", static_cast<float> (SynthPlayMode::mono) }, { "voice.glide", 0.0f },
            { "osc1.waveform", 3.0f }, { "osc1.octave", -1.0f }, { "osc1.level", 0.6f }, { "osc1.squeeze", 0.1f },
            { "osc2.waveform", 2.0f }, { "osc2.syncMode", 0.0f }, { "osc2.octave", -1.0f }, { "osc2.level", 0.4f }, { "osc2.detune", -8.0f },
            { "filter.cutoff", 400.0f }, { "filter.resonance", 0.35f }, { "filter.drive", 0.4f }, { "filter.keytrack", 0.3f },
            { "env1.attack", 0.002f }, { "env1.decay", 0.35f }, { "env1.sustain", 0.2f }, { "env1.release", 0.15f },
            { "env2.attack", 0.001f }, { "env2.decay", 0.25f }, { "env2.sustain", 0.0f }, { "env2.release", 0.15f },
            { "mod1.source", toValue (Source::env2) }, { "mod1.destination", toValue (Destination::filterCutoff) }, { "mod1.depth", 0.45f } },
          {} },

        { "Drawn Organ",
          { { "osc2.level", 0.0f },
            { "filter.type", static_cast<float> (SynthFilterType::off) },
            { "env1.attack", 0.005f }, { "env1.decay", 0.1f }, { "env1.sustain", 1.0f }, { "env1.release", 0.08f },
            { "lfo1.rate", 6.0f },
            { "mod1.source", toValue (Source::lfo1) }, { "mod1.destination", toValue (Destination::osc1Cents) }, { "mod1.depth", 0.02f } },
          { 0.8f, 0.6f, 0.5f, 0.4f, 0.0f, 0.3f, 0.0f, 0.25f } }
    };

    return presets;
}

//==============================================================================
/** Collects parameter values into a patch, by parameter ID. */
class StateBuilder
{
public:
    explicit StateBuilder (yup::DataTree& stateToFill)
        : transaction (stateToFill.beginTransaction())
    {
    }

    void set (const yup::AudioParameter::Ptr& parameter, float value)
    {
        transaction.setProperty (parameter->getID(), value);
    }

    void keep (const yup::AudioParameter::Ptr& parameter)
    {
        set (parameter, parameter->getValue());
    }

private:
    yup::DataTree::Transaction transaction;
};
} // namespace

//==============================================================================
yup::DataTree createState (yup::Span<const yup::AudioParameter::Ptr> parameters, const Partials& partials, int presetIndex)
{
    auto state = createEmptyState (presetIndex);

    {
        StateBuilder builder (state);

        for (const auto& parameter : parameters)
            builder.keep (parameter);
    }

    auto transaction = state.beginTransaction();

    for (std::size_t index = 0; index < partials.size(); ++index)
    {
        yup::DataTree child (getPartialsType (index));
        partials[index].writeTo (child);
        transaction.addChild (child);
    }

    return state;
}

yup::Result loadState (const yup::DataTree& state, yup::Span<const yup::AudioParameter::Ptr> parameters, Partials& partials)
{
    if (! state.isValid() || state.getType() != stateType)
        return yup::Result::fail ("Not a Prism patch");

    if (static_cast<int> (state.getProperty (versionProperty, 0)) > stateVersion)
        return yup::Result::fail ("The patch was saved by a newer version of Prism");

    for (const auto& parameter : parameters)
    {
        const yup::Identifier id (parameter->getID());

        parameter->setValue (state.hasProperty (id) ? static_cast<float> (static_cast<double> (state.getProperty (id)))
                                                    : parameter->getDefaultValue());
    }

    for (std::size_t index = 0; index < partials.size(); ++index)
        partials[index].readFrom (state.getChildWithName (getPartialsType (index)));

    for (auto& oscillator : partials)
        oscillator.bumpGeneration();

    return yup::Result::ok();
}

int getPresetIndex (const yup::DataTree& state)
{
    return static_cast<int> (state.getProperty (presetProperty, -1));
}

yup::DataTree createRandomState (const SynthParameters& parameters, yup::Random& random)
{
    auto state = createEmptyState (-1);
    StateBuilder builder (state);

    builder.keep (parameters.volume);
    builder.keep (parameters.playMode);
    builder.keep (parameters.glide);

    for (const auto& oscillator : parameters.oscillators)
    {
        builder.set (oscillator.waveform, static_cast<float> (random.nextInt (6)));
        builder.set (oscillator.syncMode, static_cast<float> (random.nextInt (4)));
        builder.set (oscillator.syncRatio, 1.0f + random.nextFloat() * 3.0f);
        builder.set (oscillator.level, 0.2f + random.nextFloat() * 0.8f);
        builder.set (oscillator.octave, static_cast<float> (random.nextInt (3) - 1));
        builder.set (oscillator.detune, (random.nextFloat() - 0.5f) * 30.0f);
        builder.set (oscillator.ridgeSpacing, 0.5f + random.nextFloat() * 3.0f);
        builder.set (oscillator.color, random.nextFloat());
        builder.set (oscillator.dispersion, 0.1f + random.nextFloat() * 0.8f);
        builder.set (oscillator.squeeze, random.nextBool() ? 0.0f : random.nextFloat() * 0.5f);
        builder.set (oscillator.squash, 0.4f + random.nextFloat() * 1.6f);
        builder.set (oscillator.tilt, (random.nextFloat() - 0.5f) * 3.0f);
        builder.set (oscillator.oddEven, 0.2f + random.nextFloat() * 0.6f);
        builder.set (oscillator.formant, (random.nextFloat() - 0.4f) * 5.0f);
        builder.set (oscillator.formantPosition, random.nextFloat() * 6.0f);
        builder.set (oscillator.scatter, random.nextBool() ? 0.0f : random.nextFloat() * 0.6f);
        builder.set (oscillator.unisonVoices, static_cast<float> (1 + random.nextInt (SynthExample::maxUnisonVoices)));
        builder.set (oscillator.unisonDetune, random.nextFloat() * 0.5f);
        builder.set (oscillator.unisonSpread, random.nextFloat());
    }

    const auto& filter = parameters.filter;
    builder.set (filter.type, static_cast<float> (1 + random.nextInt (6)));
    builder.set (filter.cutoff, 200.0f * std::exp2 (random.nextFloat() * 6.0f));
    builder.set (filter.resonance, random.nextFloat() * 0.8f);
    builder.set (filter.drive, random.nextBool() ? 0.0f : random.nextFloat() * 0.6f);
    builder.set (filter.keytrack, random.nextBool() ? 0.0f : 1.0f);

    // The amplitude envelope keeps a short attack more often than not, so the patch
    // still speaks when played; the modulation envelope is free to be slow.
    for (std::size_t index = 0; index < parameters.envelopes.size(); ++index)
    {
        const auto& envelope = parameters.envelopes[index];
        const auto slow = index > 0 || random.nextInt (4) == 0;

        builder.set (envelope.delay, random.nextInt (4) == 0 ? random.nextFloat() * 0.3f : 0.0f);
        builder.set (envelope.attack, 0.003f + random.nextFloat() * (slow ? 1.5f : 0.15f));
        builder.set (envelope.hold, random.nextBool() ? 0.0f : random.nextFloat() * 0.3f);
        builder.set (envelope.decay, 0.05f + random.nextFloat() * 1.5f);
        builder.set (envelope.sustain, random.nextFloat());
        builder.set (envelope.release, 0.05f + random.nextFloat() * 1.5f);
    }

    for (const auto& lfo : parameters.lfos)
    {
        builder.set (lfo.shape, static_cast<float> (random.nextInt (5)));
        builder.set (lfo.rate, 0.1f * std::exp2 (random.nextFloat() * 6.0f));
        builder.set (lfo.phase, random.nextBool() ? 0.0f : random.nextFloat());
        builder.set (lfo.retrigger, random.nextBool() ? 1.0f : 0.0f);
    }

    // A few live routes, never to the oscillator levels, so a random patch cannot
    // fall silent; the remaining slots load their unrouted defaults.
    const auto liveRoutes = 1 + random.nextInt (4);

    for (int index = 0; index < liveRoutes; ++index)
    {
        const auto& slot = parameters.modulation.slots[static_cast<std::size_t> (index)];
        auto destination = SynthModulationDestination::none;

        do
        {
            destination = static_cast<SynthModulationDestination> (1 + random.nextInt (static_cast<int> (SynthModulationDestination::count) - 1));
        } while (destination == SynthModulationDestination::osc1Level || destination == SynthModulationDestination::osc2Level);

        builder.set (slot.source, static_cast<float> (random.nextInt (4)));
        builder.set (slot.destination, static_cast<float> (destination));
        builder.set (slot.depth, (random.nextFloat() - 0.5f) * (random.nextBool() ? 2.0f : 1.0f));
    }

    return state;
}

//==============================================================================
int getNumFactoryPresets()
{
    return static_cast<int> (getFactoryPresets().size());
}

yup::String getFactoryPresetName (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    return getFactoryPresets()[static_cast<std::size_t> (index)].name;
}

yup::DataTree createFactoryState (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    const auto& preset = getFactoryPresets()[static_cast<std::size_t> (index)];
    auto state = createEmptyState (index);

    {
        auto transaction = state.beginTransaction();

        for (const auto& value : preset.values)
            transaction.setProperty (value.id, value.value);
    }

    if (! preset.osc1Partials.empty())
    {
        SynthPartials seed;

        for (std::size_t harmonic = 0; harmonic < preset.osc1Partials.size(); ++harmonic)
            seed.setHarmonicMagnitude (static_cast<int> (harmonic), preset.osc1Partials[harmonic]);

        seed.setUsesCustomSeries (true);

        yup::DataTree child (getPartialsType (0));
        seed.writeTo (child);

        auto transaction = state.beginTransaction();
        transaction.addChild (child);
    }

    return state;
}
} // namespace SynthPatch
