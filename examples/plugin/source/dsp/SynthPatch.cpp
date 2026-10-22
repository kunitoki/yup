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
constexpr int stateVersion = 2;

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

struct PresetRoute
{
    SynthModulationSource source;
    const char* destination;
    float depth;
};

struct PresetEnvelope
{
    int index;
    float attack;
    float decay;
    float sustain;
    float release;
    float curve;
};

struct FactoryPreset
{
    const char* name;
    std::vector<PresetValue> values;
    std::vector<PresetRoute> routes;
    std::vector<PresetEnvelope> envelopes;
    std::vector<float> osc1Partials; /**< Sine magnitudes from the fundamental up; empty plays the waveform. */
};

constexpr float sine = static_cast<float> (yup::Waveform::sine);
constexpr float sawtooth = static_cast<float> (yup::Waveform::sawtooth);
constexpr float square = static_cast<float> (yup::Waveform::square);

const std::vector<FactoryPreset>& getFactoryPresets()
{
    using Source = SynthModulationSource;

    static const std::vector<FactoryPreset> presets {
        { "Init", {}, {}, {}, {} },

        { "FM Bell",
          { { "osc1.waveform", sine }, { "osc1.level", 0.7f }, { "osc1.color", 0.0f },
            { "osc2.ratio", 3.5f }, { "fm.12", 0.6f },
            { "flt1.type", static_cast<float> (SynthFilterType::off) } },
          { { Source::env2, "fm.12", 0.3f } },
          { { 0, 0.002f, 2.5f, 0.0f, 2.5f, -0.7f }, { 1, 0.001f, 1.2f, 0.0f, 1.2f, -0.6f } },
          {} },

        { "FM Bass",
          { { "global.mode", static_cast<float> (SynthPlayMode::mono) }, { "global.glide", 0.0f },
            { "osc1.waveform", sine }, { "osc1.level", 0.8f }, { "osc1.color", 0.0f }, { "osc1.ratio", 0.5f },
            { "osc2.ratio", 0.5f }, { "fm.12", 0.5f }, { "fm.22", 0.7f },
            { "flt1.type", static_cast<float> (SynthFilterType::lowpass24) }, { "flt1.cutoff", 1800.0f }, { "flt1.resonance", 0.25f },
            { "flt1.drive", 0.3f } },
          { { Source::env2, "fm.12", 0.35f } },
          { { 0, 0.002f, 0.4f, 0.6f, 0.12f, -0.5f }, { 1, 0.001f, 0.25f, 0.0f, 0.1f, -0.6f } },
          {} },

        { "Spectral Pad",
          { { "global.unisonVoices", 4.0f }, { "global.unisonDetune", 0.25f }, { "global.unisonSpread", 0.8f },
            { "osc1.ridges", 2.5f }, { "osc1.color", 0.6f }, { "osc1.dispersion", 0.3f }, { "osc1.scatter", 0.2f },
            { "osc2.waveform", sawtooth }, { "osc2.ratio", 0.5f }, { "osc2.level", 0.25f }, { "osc2.ridges", 3.0f },
            { "flt1.cutoff", 3500.0f }, { "flt1.resonance", 0.3f }, { "lfo1.rate", 0.3f } },
          { { Source::lfo1, "osc1.color", 0.3f }, { Source::env2, "flt1.cutoff", 0.2f } },
          { { 0, 0.8f, 1.2f, 0.8f, 2.0f, -0.4f } },
          {} },

        { "Drawn Organ",
          { { "flt1.type", static_cast<float> (SynthFilterType::off) }, { "lfo1.rate", 6.0f } },
          { { Source::lfo1, "osc1.fine", 0.02f } },
          { { 0, 0.005f, 0.1f, 1.0f, 0.08f, 0.0f } },
          { 0.8f, 0.6f, 0.5f, 0.4f, 0.0f, 0.3f, 0.0f, 0.25f } },

        { "Pluck Square",
          { { "osc1.waveform", square }, { "osc1.squeeze", 0.1f },
            { "flt1.type", static_cast<float> (SynthFilterType::moogLadder) }, { "flt1.cutoff", 400.0f },
            { "flt1.resonance", 0.35f }, { "flt1.keytrack", 0.3f } },
          { { Source::env2, "flt1.cutoff", 0.45f } },
          { { 0, 0.002f, 0.35f, 0.2f, 0.15f, -0.5f }, { 1, 0.001f, 0.25f, 0.0f, 0.15f, -0.6f } },
          {} }
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

    void addChild (const yup::DataTree& child)
    {
        transaction.addChild (child);
    }

private:
    yup::DataTree::Transaction transaction;
};

/** Builds a "Routes" tree like SynthRoutes writes, from sources and destination IDs. */
yup::DataTree createRoutesTree (const std::vector<std::pair<SynthModulationSource, yup::String>>& routes)
{
    yup::DataTree tree (SynthRoutes::treeType);
    auto transaction = tree.beginTransaction();
    const auto sourceNames = getSynthModulationSourceNames();

    for (std::size_t slot = 0; slot < routes.size(); ++slot)
    {
        yup::DataTree child ("Route");

        {
            auto childTransaction = child.beginTransaction();
            childTransaction.setProperty ("slot", static_cast<int> (slot));
            childTransaction.setProperty ("source", sourceNames[static_cast<int> (routes[slot].first)]);
            childTransaction.setProperty ("destination", routes[slot].second);
        }

        transaction.addChild (child);
    }

    return tree;
}
} // namespace

//==============================================================================
yup::DataTree createState (yup::Span<const yup::AudioParameter::Ptr> parameters, const State& state, int presetIndex)
{
    auto tree = createEmptyState (presetIndex);
    StateBuilder builder (tree);

    for (const auto& parameter : parameters)
        builder.keep (parameter);

    for (std::size_t index = 0; index < state.partials.size(); ++index)
    {
        yup::DataTree child (getPartialsType (index));
        state.partials[index].writeTo (child);
        builder.addChild (child);
    }

    builder.addChild (state.curves.createTree());
    builder.addChild (state.routes.createTree());

    return tree;
}

yup::Result loadState (const yup::DataTree& tree, yup::Span<const yup::AudioParameter::Ptr> parameters, const State& state)
{
    if (! tree.isValid() || tree.getType() != stateType)
        return yup::Result::fail ("Not a Prism patch");

    const auto version = static_cast<int> (tree.getProperty (versionProperty, 0));

    if (version > stateVersion)
        return yup::Result::fail ("The patch was saved by a newer version of Prism");

    if (version < stateVersion)
        return yup::Result::fail ("The patch was saved by the two oscillator Prism, which this version cannot load");

    for (const auto& parameter : parameters)
    {
        const yup::Identifier id (parameter->getID());

        parameter->setValue (tree.hasProperty (id) ? static_cast<float> (static_cast<double> (tree.getProperty (id)))
                                                   : parameter->getDefaultValue());
    }

    for (std::size_t index = 0; index < state.partials.size(); ++index)
        state.partials[index].readFrom (tree.getChildWithName (getPartialsType (index)));

    state.curves.readFrom (tree.getChildWithName (SynthCurveState::treeType));
    state.routes.readFrom (tree.getChildWithName (SynthRoutes::treeType),
                           [parameters] (const yup::String& id) { return findRouteDestination (parameters, id); });

    for (auto& oscillator : state.partials)
        oscillator.bumpGeneration();

    return yup::Result::ok();
}

int getPresetIndex (const yup::DataTree& state)
{
    return static_cast<int> (state.getProperty (presetProperty, -1));
}

int findRouteDestination (yup::Span<const yup::AudioParameter::Ptr> parameters, const yup::String& parameterID)
{
    for (const auto& parameter : parameters)
    {
        if (parameter->getID() != parameterID)
            continue;

        // Volume and glide act outside the voices, and route depths are read before any
        // route applies, so modulating them would do nothing.
        if (parameter->isStepped() || ! parameter->isAutomatable()
            || parameterID == "global.volume" || parameterID == "global.glide" || parameterID.startsWith ("route."))
            return -1;

        return parameter->getIndexInContainer();
    }

    return -1;
}

yup::DataTree createRandomState (const SynthParameters& parameters, yup::Random& random)
{
    auto state = createEmptyState (-1);
    StateBuilder builder (state);

    for (const auto& parameter : { parameters.volume, parameters.playMode, parameters.glide, parameters.quality })
        builder.keep (parameter);

    builder.set (parameters.unisonVoices, static_cast<float> (random.nextInt (3) == 0 ? 1 + random.nextInt (SynthExample::maxUnisonVoices) : 1));
    builder.set (parameters.unisonDetune, random.nextFloat() * 0.5f);
    builder.set (parameters.unisonSpread, random.nextFloat());
    builder.set (parameters.filterRouting, static_cast<float> (random.nextInt (2)));

    // Musical FM lives on small integer ratios and moderate depths, so most of the draws do.
    const float ratios[] = { 0.5f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 4.0f, 5.0f, 7.0f };

    for (std::size_t index = 0; index < parameters.oscillators.size(); ++index)
    {
        const auto& oscillator = parameters.oscillators[index];
        const auto carrier = index == 0 || random.nextInt (3) == 0;

        builder.set (oscillator.waveform, static_cast<float> (random.nextInt (3) == 0 ? random.nextInt (6) : 0));
        builder.set (oscillator.syncMode, static_cast<float> (random.nextInt (4) == 0 ? random.nextInt (4) : 0));
        builder.set (oscillator.syncRatio, 1.0f + random.nextFloat() * 3.0f);
        builder.set (oscillator.ratio, ratios[random.nextInt (static_cast<int> (std::size (ratios)))]);
        builder.set (oscillator.fine, random.nextInt (4) == 0 ? (random.nextFloat() - 0.5f) * 20.0f : 0.0f);
        builder.set (oscillator.level, carrier ? 0.4f + random.nextFloat() * 0.5f : 0.0f);
        builder.set (oscillator.pan, (random.nextFloat() - 0.5f) * 0.6f);
        builder.set (oscillator.toFilter1, random.nextInt (4) == 0 ? 0.0f : 1.0f);
        builder.set (oscillator.toFilter2, random.nextBool() ? random.nextFloat() : 0.0f);
        builder.set (oscillator.direct, random.nextInt (4) == 0 ? random.nextFloat() * 0.5f : 0.0f);
        builder.set (oscillator.ridgeSpacing, 0.5f + random.nextFloat() * 3.0f);
        builder.set (oscillator.color, random.nextFloat() * 0.6f);
        builder.set (oscillator.dispersion, 0.1f + random.nextFloat() * 0.8f);
        builder.set (oscillator.tilt, (random.nextFloat() - 0.5f) * 2.0f);
        builder.set (oscillator.oddEven, 0.3f + random.nextFloat() * 0.4f);
        builder.set (oscillator.scatter, random.nextBool() ? 0.0f : random.nextFloat() * 0.4f);
    }

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
        {
            const auto& parameter = parameters.fm.get (carrier, modulator);

            if (carrier == modulator)
                builder.set (parameter, random.nextInt (5) == 0 ? random.nextFloat() : 0.0f);
            else
                builder.set (parameter, random.nextInt (4) == 0 ? random.nextFloat() * random.nextFloat() * 2.0f : 0.0f);
        }
    }

    for (std::size_t index = 0; index < parameters.filters.size(); ++index)
    {
        const auto& filter = parameters.filters[index];
        builder.set (filter.type, static_cast<float> (index == 0 || random.nextBool() ? 1 + random.nextInt (14) : 0));
        builder.set (filter.cutoff, 200.0f * std::exp2 (random.nextFloat() * 6.0f));
        builder.set (filter.resonance, random.nextFloat() * 0.7f);
        builder.set (filter.drive, random.nextBool() ? 0.0f : random.nextFloat() * 0.5f);
        builder.set (filter.keytrack, random.nextBool() ? 0.0f : 1.0f);
    }

    for (const auto& lfo : parameters.lfos)
    {
        builder.set (lfo.shape, static_cast<float> (random.nextInt (7)));
        builder.set (lfo.rate, 0.1f * std::exp2 (random.nextFloat() * 6.0f));
        builder.set (lfo.phase, random.nextBool() ? 0.0f : random.nextFloat());
    }

    // A few live routes, never to the operator levels, so a random patch cannot fall silent.
    const char* destinations[] = { "osc1.color", "osc1.tilt", "osc1.fine", "osc2.ratio", "fm.12", "fm.21", "fm.13",
                                   "flt1.cutoff", "flt1.resonance", "flt2.cutoff", "osc1.pan", "osc1.formantPosition" };
    std::vector<std::pair<SynthModulationSource, yup::String>> routes;
    const auto liveRoutes = 1 + random.nextInt (4);

    for (int index = 0; index < liveRoutes; ++index)
    {
        const auto source = static_cast<SynthModulationSource> (random.nextInt (static_cast<int> (SynthModulationSource::lfo4) + 1));
        routes.emplace_back (source, destinations[random.nextInt (static_cast<int> (std::size (destinations)))]);
        builder.set (parameters.modulation.depths[static_cast<std::size_t> (index)], (random.nextFloat() - 0.5f) * 0.6f);
    }

    builder.addChild (createRoutesTree (routes));

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
    auto transaction = state.beginTransaction();

    for (const auto& value : preset.values)
        transaction.setProperty (value.id, value.value);

    std::vector<std::pair<SynthModulationSource, yup::String>> routes;

    for (std::size_t slot = 0; slot < preset.routes.size(); ++slot)
    {
        routes.emplace_back (preset.routes[slot].source, preset.routes[slot].destination);
        transaction.setProperty (yup::Identifier ("route." + yup::String (static_cast<int> (slot) + 1) + ".depth"), preset.routes[slot].depth);
    }

    transaction.addChild (createRoutesTree (routes));

    if (! preset.envelopes.empty())
    {
        SynthCurveState curves;

        for (const auto& envelope : preset.envelopes)
            curves.setEnvelope (envelope.index, SynthCurveState::makeAdsr (envelope.attack, envelope.decay, envelope.sustain, envelope.release, envelope.curve));

        transaction.addChild (curves.createTree());
    }

    if (! preset.osc1Partials.empty())
    {
        SynthPartials seed;

        for (std::size_t harmonic = 0; harmonic < preset.osc1Partials.size(); ++harmonic)
            seed.setHarmonicMagnitude (static_cast<int> (harmonic), preset.osc1Partials[harmonic]);

        seed.setUsesCustomSeries (true);

        yup::DataTree child (getPartialsType (0));
        seed.writeTo (child);
        transaction.addChild (child);
    }

    return state;
}
} // namespace SynthPatch
