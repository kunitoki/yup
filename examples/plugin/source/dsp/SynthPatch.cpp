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
#include "SynthConstants.h"
#include "../browser/PresetBank.h"

#include <FactoryBank/FactoryBank.h>

#include <algorithm>
#include <numeric>
#include <vector>

namespace SynthPatch
{
namespace
{
const yup::Identifier stateType ("RetraceState");
const yup::Identifier versionProperty ("version");
const yup::Identifier presetProperty ("preset");
const yup::Identifier nameProperty ("name");
const yup::Identifier bankProperty ("bank");
const yup::Identifier authorProperty ("author");
const yup::Identifier tagsProperty ("tags");
const yup::Identifier categoryProperty ("category");
const yup::Identifier descriptionProperty ("description");
const yup::Identifier notesProperty ("notes");
const yup::Identifier presetIdProperty ("presetId");
constexpr int stateVersion = 3;

yup::Identifier getMacroNameProperty (std::size_t macro)
{
    return yup::Identifier ("macro" + yup::String (static_cast<int> (macro) + 1) + "Name");
}

yup::Identifier getPartialsType (std::size_t oscillator)
{
    return yup::Identifier ("Partials" + yup::String (static_cast<int> (oscillator) + 1));
}

yup::DataTree createEmptyState (int presetIndex, const SynthPatch::PresetInfo& info)
{
    yup::DataTree state (stateType);

    {
        auto transaction = state.beginTransaction();
        transaction.setProperty (versionProperty, stateVersion);
        transaction.setProperty (presetProperty, presetIndex);
    }

    SynthPatch::writeInfo (state, info);
    return state;
}

//==============================================================================
constexpr float sine = static_cast<float> (yup::Waveform::sine);
constexpr float sawtooth = static_cast<float> (yup::Waveform::sawtooth);
constexpr float square = static_cast<float> (yup::Waveform::square);
constexpr float triangle = static_cast<float> (yup::Waveform::triangle);
constexpr float pulse = static_cast<float> (yup::Waveform::pulse);

//==============================================================================
/** A factory preset: Init, built in code, or one read from the embedded factory bank. */
struct FactoryPreset
{
    PresetInfo info;
    yup::String folder; /**< The folder inside the bank, "Pads" or "Pads/Warm". */
    yup::String xml; /**< Empty for Init. */
};

PresetInfo getInitInfo()
{
    return { "Init", "Factory", "kunitoki", "init", "Init", "A single sine, the starting point.", {}, "ed6a924c-6fe7-4f9f-8f0e-7bf8b69740ea", {} };
}

/** Init, then every patch of the factory bank in the order of their paths, read once. */
const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const auto presets = []
    {
        std::vector<FactoryPreset> result { { getInitInfo(), {}, {} } };
        const PresetBank bank (retrace::factoryBank_data, retrace::factoryBank_size);

        for (const auto& path : bank.getEntryPaths (".retrace"))
        {
            auto xml = bank.readEntry (path, SynthExample::maxFileBytes);
            const auto parsed = yup::parseXML (xml);

            if (parsed == nullptr)
                continue;

            const auto state = yup::DataTree::fromXml (*parsed);

            if (! isPatch (state))
                continue;

            auto info = readInfo (state);

            if (info.name.isEmpty())
                info.name = path.fromLastOccurrenceOf ("/", false, false).upToLastOccurrenceOf (".", false, false);

            result.push_back ({ std::move (info), path.upToLastOccurrenceOf ("/", false, false), std::move (xml) });
        }

        return result;
    }();

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

//==============================================================================
/** A range a random patch draws a value from. */
struct Between
{
    float low;
    float high;
};

float drawLinear (yup::Random& random, Between range)
{
    return range.low + random.nextFloat() * (range.high - range.low);
}

/** Draws evenly across octaves, for times and frequencies. */
float drawExponential (yup::Random& random, Between range)
{
    return range.low * std::pow (range.high / range.low, random.nextFloat());
}

bool chance (yup::Random& random, int percent)
{
    return random.nextInt (100) < percent;
}

/** Returns an index drawn with chances proportional to the weights. */
int drawWeighted (yup::Random& random, std::initializer_list<int> weights)
{
    auto draw = random.nextInt (std::accumulate (weights.begin(), weights.end(), 0));
    auto index = 0;

    for (const auto weight : weights)
    {
        if (draw < weight)
            break;

        draw -= weight;
        ++index;
    }

    return index;
}

template <typename Item, std::size_t size>
const Item& drawFrom (yup::Random& random, const Item (&items)[size])
{
    return items[static_cast<std::size_t> (random.nextInt (static_cast<int> (size)))];
}

/** The kind of sound a random patch aims for, which shapes its amplitude envelope. Times in seconds. */
struct RandomCharacter
{
    const char* name;
    Between attack;
    Between decay;
    Between sustain;
    Between release;
    int unisonPercent;
    bool inharmonic; /**< FM on inharmonic ratios, as bells are. */
    bool lowPitch;   /**< Plays an octave down. */
};

const RandomCharacter randomCharacters[] {
    { "pluck", { 0.001f, 0.005f }, { 0.15f, 0.6f }, { 0.0f, 0.15f }, { 0.1f, 0.4f }, 15, false, false },
    { "keys", { 0.001f, 0.01f }, { 0.8f, 2.5f }, { 0.2f, 0.6f }, { 0.2f, 0.6f }, 15, false, false },
    { "pad", { 0.3f, 1.5f }, { 0.5f, 2.0f }, { 0.6f, 0.9f }, { 1.0f, 3.0f }, 60, false, false },
    { "lead", { 0.003f, 0.05f }, { 0.2f, 0.8f }, { 0.6f, 1.0f }, { 0.08f, 0.3f }, 30, false, false },
    { "bass", { 0.001f, 0.01f }, { 0.2f, 0.8f }, { 0.4f, 0.9f }, { 0.05f, 0.2f }, 10, false, true },
    { "bell", { 0.001f, 0.005f }, { 2.0f, 5.0f }, { 0.0f, 0.0f }, { 1.5f, 3.0f }, 20, true, false }
};

/** A filter type with the cutoffs it sounds good at, in Hz. */
struct RandomFilter
{
    SynthFilterType type;
    Between cutoff;
};

/** The filters that shape the whole sound; lowpasses are repeated to be drawn more often. */
const RandomFilter shapingFilters[] {
    { SynthFilterType::lowpass12, { 600.0f, 6000.0f } },
    { SynthFilterType::lowpass24, { 400.0f, 5000.0f } },
    { SynthFilterType::lowpass24, { 400.0f, 5000.0f } },
    { SynthFilterType::moogLadder, { 300.0f, 4000.0f } },
    { SynthFilterType::moogLadder, { 300.0f, 4000.0f } },
    { SynthFilterType::korg35, { 400.0f, 5000.0f } },
    { SynthFilterType::diode, { 300.0f, 4000.0f } },
    { SynthFilterType::bandpass12, { 500.0f, 3000.0f } },
    { SynthFilterType::vowel, { 400.0f, 2500.0f } }
};

/** The filters that color what a shaping filter already shaped, in series after it. */
const RandomFilter coloringFilters[] {
    { SynthFilterType::highpass12, { 60.0f, 400.0f } },
    { SynthFilterType::peak, { 500.0f, 4000.0f } },
    { SynthFilterType::notch, { 500.0f, 4000.0f } },
    { SynthFilterType::bandpass12, { 600.0f, 3000.0f } },
    { SynthFilterType::lowpass12, { 2000.0f, 10000.0f } },
    { SynthFilterType::comb, { 200.0f, 1500.0f } }
};

/** Reads a parameter value from untrusted state, falling back when it is missing or not finite. */
float readParameterValue (const yup::DataTree& state, const yup::AudioParameter& parameter, float fallback)
{
    const auto value = static_cast<double> (state.getProperty (yup::Identifier (parameter.getID()), static_cast<double> (fallback)));

    if (! std::isfinite (value))
        return fallback;

    return static_cast<float> (yup::jlimit<double> (parameter.getMinimumValue(), parameter.getMaximumValue(), value));
}
} // namespace

//==============================================================================
PresetInfo readInfo (const yup::DataTree& state)
{
    PresetInfo info { state.getProperty (nameProperty, {}).toString(),
                      state.getProperty (bankProperty, {}).toString(),
                      state.getProperty (authorProperty, {}).toString(),
                      state.getProperty (tagsProperty, {}).toString(),
                      state.getProperty (categoryProperty, {}).toString(),
                      state.getProperty (descriptionProperty, {}).toString(),
                      state.getProperty (notesProperty, {}).toString(),
                      state.getProperty (presetIdProperty, {}).toString() };

    for (std::size_t macro = 0; macro < info.macroNames.size(); ++macro)
        info.macroNames[macro] = state.getProperty (getMacroNameProperty (macro), {}).toString();

    return info;
}

void writeInfo (yup::DataTree& state, const PresetInfo& info)
{
    auto transaction = state.beginTransaction();
    transaction.setProperty (nameProperty, info.name);
    transaction.setProperty (bankProperty, info.bank);
    transaction.setProperty (authorProperty, info.author);
    transaction.setProperty (tagsProperty, info.tags);
    transaction.setProperty (categoryProperty, info.category);
    transaction.setProperty (descriptionProperty, info.description);
    transaction.setProperty (notesProperty, info.notes);

    if (info.id.isNotEmpty())
        transaction.setProperty (presetIdProperty, info.id);
    else
        transaction.removeProperty (presetIdProperty);

    for (std::size_t macro = 0; macro < info.macroNames.size(); ++macro)
    {
        if (info.macroNames[macro].isNotEmpty())
            transaction.setProperty (getMacroNameProperty (macro), info.macroNames[macro]);
        else
            transaction.removeProperty (getMacroNameProperty (macro));
    }
}

bool isPatch (const yup::DataTree& state)
{
    return state.getType() == stateType;
}

yup::DataTree createState (yup::Span<const yup::AudioParameter::Ptr> parameters, const State& state, int presetIndex, const PresetInfo& info)
{
    auto tree = createEmptyState (presetIndex, info);
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
    builder.addChild (state.effectChain.createTree());
    builder.addChild (state.tuning.createTree());

    return tree;
}

yup::Result loadState (const yup::DataTree& tree, yup::Span<const yup::AudioParameter::Ptr> parameters, const State& state)
{
    if (! tree.isValid() || tree.getType() != stateType)
        return yup::Result::fail ("Not a Retrace patch");

    const auto version = static_cast<int> (tree.getProperty (versionProperty, 0));

    if (version > stateVersion)
        return yup::Result::fail ("The patch was saved by a newer version of Retrace");

    if (version < stateVersion)
        return yup::Result::fail ("The patch was saved by an older version of Retrace, which this version cannot load");

    for (const auto& parameter : parameters)
    {
        const yup::Identifier id (parameter->getID());

        if (! tree.hasProperty (id) && ! parameter->isAutomatable())
            continue;

        parameter->setValue (readParameterValue (tree, *parameter, parameter->getDefaultValue()));
    }

    for (std::size_t index = 0; index < state.partials.size(); ++index)
        state.partials[index].readFrom (tree.getChildWithName (getPartialsType (index)));

    state.curves.readFrom (tree.getChildWithName (SynthCurveState::treeType));
    state.routes.readFrom (tree.getChildWithName (SynthRoutes::treeType),
                           [parameters] (const yup::String& id) { return findRouteDestination (parameters, id); });
    state.effectChain.readFrom (tree.getChildWithName (SynthEffectChain::treeType));
    state.tuning.readFrom (tree.getChildWithName (SynthTuning::treeType));

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

        if (parameter->isStepped() || ! parameter->isAutomatable()
            || parameterID == "global.volume" || parameterID == "global.glide" || parameterID.startsWith ("route.")
            || parameterID.startsWith ("macro"))
            return -1;

        return parameter->getIndexInContainer();
    }

    return -1;
}

yup::DataTree createRandomState (const SynthParameters& parameters,
                                 yup::Random& random,
                                 const std::array<yup::String, SynthExample::macroCount>& macroNames)
{
    using Source = SynthModulationSource;
    using Shape = SynthLFOValues::Shape;
    using Parameter = yup::AudioParameter::Ptr;

    const auto& character = drawFrom (random, randomCharacters);

    PresetInfo info { "Random", {}, {}, yup::String ("random, ") + character.name };
    info.macroNames = macroNames;

    auto state = createEmptyState (-1, info);
    StateBuilder builder (state);

    for (const auto& parameter : { parameters.volume, parameters.playMode, parameters.glide, parameters.quality })
        builder.keep (parameter);

    // The macros are where the user's hands are, so a random patch leaves them in place.
    for (const auto& macro : parameters.macros)
        builder.keep (macro);

    // The effects are the user's mix, not part of the sound being drawn.
    for (const auto& parameter : parameters.effects.getParameters())
        builder.keep (parameter);

    // The operators in use are always the first ones. Operator 1 is heard, and every other
    // one is heard too or modulates an operator below it, so none is idle and none loops.
    const auto fmPatch = character.inharmonic || chance (random, 60);
    const auto numOperators = 1 + drawWeighted (random, { character.inharmonic ? 0 : 2, 4, 3, 3 });
    std::array<int, SynthExample::oscillatorCount> targets; // the operator each one modulates, -1 when heard
    targets.fill (-1);
    auto numCarriers = 1;

    for (int op = 1; op < numOperators; ++op)
    {
        if (chance (random, fmPatch ? 75 : 25))
            targets[static_cast<std::size_t> (op)] = chance (random, 70) ? op - 1 : random.nextInt (op);
        else
            ++numCarriers;
    }

    // A filter is on only when sound reaches it, and filter 2 only after filter 1.
    const auto numFilters = drawWeighted (random, { 1, 3, 1 });
    const auto parallel = numFilters == 2 && numCarriers > 1 && random.nextBool();
    builder.set (parameters.filterRouting, static_cast<float> (parallel ? SynthFilterRouting::parallel : SynthFilterRouting::serial));

    const float carrierRatios[] = { 0.5f, 1.0f, 1.0f, 1.0f, 2.0f };
    const float layerRatios[] = { 0.5f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 4.0f };
    const float modulatorRatios[] = { 0.5f, 1.0f, 1.0f, 2.0f, 2.0f, 3.0f, 4.0f, 5.0f, 7.0f };
    const float bellRatios[] = { 1.41f, 2.0f, 2.76f, 3.5f, 4.2f, 5.4f };
    const float spectralWaveforms[] = { sawtooth, sawtooth, square, triangle, pulse };
    const auto carrierLevel = 0.75f / std::sqrt (static_cast<float> (numCarriers));
    std::vector<Parameter> timbres; // what envelopes, velocity, controllers and sweeps move
    auto nextBus = random.nextInt (2);
    auto topModulator = -1;

    for (std::size_t op = 0; op < parameters.oscillators.size(); ++op)
    {
        const auto& oscillator = parameters.oscillators[op];
        const auto used = static_cast<int> (op) < numOperators;
        builder.set (oscillator.enabled, used ? 1.0f : 0.0f);

        if (! used)
        {
            builder.set (oscillator.level, 0.0f);
            continue;
        }

        const auto heard = targets[op] < 0;

        // Modulators are mostly sines, so their FM is predictable, and so are the carriers of an FM patch.
        const auto waveform = heard ? (chance (random, fmPatch ? 70 : 15) ? sine : drawFrom (random, spectralWaveforms))
                                    : (chance (random, 85) ? sine : triangle);
        builder.set (oscillator.waveform, waveform);

        if (! heard)
            builder.set (oscillator.ratio, character.inharmonic ? drawFrom (random, bellRatios) : drawFrom (random, modulatorRatios));
        else if (op == 0)
            builder.set (oscillator.ratio, character.lowPitch ? 0.5f : drawFrom (random, carrierRatios));
        else
            builder.set (oscillator.ratio, drawFrom (random, layerRatios));

        // Layers beat against operator 1, and a slightly detuned modulator makes FM move.
        const auto detune = heard ? (op > 0 ? drawLinear (random, { 3.0f, 12.0f }) : 0.0f)
                                  : (chance (random, 20) ? drawLinear (random, { 1.0f, 7.0f }) : 0.0f);
        builder.set (oscillator.fine, random.nextBool() ? detune : -detune);

        builder.set (oscillator.level, heard ? carrierLevel * drawLinear (random, { 0.8f, 1.0f }) : 0.0f);
        builder.set (oscillator.pan, heard && numCarriers > 1 ? drawLinear (random, { -0.35f, 0.35f }) : 0.0f);

        // Modulators send to the first filter, so raising their level is heard through it.
        const auto bus = parallel && heard ? nextBus++ % 2 : 0;
        builder.set (oscillator.toFilter1, numFilters > 0 && bus == 0 ? 1.0f : 0.0f);
        builder.set (oscillator.toFilter2, numFilters > 0 && bus == 1 ? 1.0f : 0.0f);
        builder.set (oscillator.direct, numFilters == 0 ? 1.0f : 0.0f);

        if (! heard)
        {
            topModulator = static_cast<int> (op);
            builder.set (parameters.fm.get (targets[op], static_cast<int> (op)),
                         drawExponential (random, character.inharmonic ? Between { 0.3f, 2.0f } : Between { 0.15f, 1.5f }));
            timbres.push_back (parameters.fm.get (targets[op], static_cast<int> (op)));
        }

        // Sines and triangles play as they are; the shaping is for the richer waveforms.
        if (waveform == sine || waveform == triangle)
        {
            builder.set (oscillator.color, 0.0f);
            continue;
        }

        builder.set (oscillator.color, random.nextBool() ? drawLinear (random, { 0.1f, 0.6f }) : 0.0f);
        builder.set (oscillator.ridgeSpacing, drawLinear (random, { 0.5f, 3.5f }));
        builder.set (oscillator.dispersion, drawLinear (random, { 0.2f, 0.8f }));
        builder.set (oscillator.tilt, drawLinear (random, { -1.0f, 0.5f }));
        builder.set (oscillator.oddEven, drawLinear (random, { 0.3f, 0.7f }));
        builder.set (oscillator.scatter, chance (random, 25) ? drawLinear (random, { 0.05f, 0.3f }) : 0.0f);
        timbres.push_back (oscillator.color);
        timbres.push_back (oscillator.tilt);

        if (chance (random, 15))
        {
            builder.set (oscillator.syncMode, static_cast<float> (yup::SyncMode::hard));
            builder.set (oscillator.syncRatio, drawLinear (random, { 1.5f, 4.0f }));
            timbres.push_back (oscillator.syncRatio);
        }
    }

    // Feedback, kept low: on the top modulator, or on operator 1 when an FM patch has none.
    const auto feedbackOp = topModulator >= 0 ? (chance (random, 40) ? topModulator : -1) : (fmPatch ? 0 : -1);

    if (feedbackOp >= 0)
    {
        builder.set (parameters.fm.get (feedbackOp, feedbackOp), drawLinear (random, topModulator >= 0 ? Between { 0.05f, 0.35f } : Between { 0.2f, 0.5f }));
        timbres.push_back (parameters.fm.get (feedbackOp, feedbackOp));
    }

    // Unison stops at four copies: eight voices of eight copies of four operators is too heavy.
    builder.set (parameters.unisonVoices, static_cast<float> (chance (random, character.unisonPercent) ? 2 + random.nextInt (3) : 1));
    builder.set (parameters.unisonDetune, drawLinear (random, { 0.05f, 0.25f }));
    builder.set (parameters.unisonSpread, drawLinear (random, { 0.4f, 0.9f }));

    std::array<const RandomFilter*, SynthExample::filterCount> filterChoices {};

    for (std::size_t index = 0; index < static_cast<std::size_t> (numFilters); ++index)
    {
        filterChoices[index] = index == 0 || parallel ? &drawFrom (random, shapingFilters) : &drawFrom (random, coloringFilters);
        timbres.push_back (parameters.filters[index].cutoff);
    }

    //==============================================================================
    struct Route
    {
        Source source;
        Parameter destination;
        float depth;
    };

    std::vector<Route> routes;

    const auto addRoute = [&routes] (Source source, const Parameter& destination, float depth)
    {
        const auto taken = std::any_of (routes.begin(), routes.end(), [&] (const Route& route) { return route.source == source && route.destination == destination; });

        if (! taken && routes.size() < static_cast<std::size_t> (SynthExample::modulationSlots))
            routes.push_back ({ source, destination, depth });
    };

    const auto drawTimbre = [&random, &timbres]
    {
        return timbres[static_cast<std::size_t> (random.nextInt (static_cast<int> (timbres.size())))];
    };

    // The envelopes and LFOs in use are the first ones, and each drives at least one route.
    const auto numEnvelopes = timbres.empty() ? 0 : drawWeighted (random, { 3, 4, 2, 1 });

    for (int index = 1; index <= numEnvelopes; ++index)
    {
        const auto source = static_cast<Source> (static_cast<int> (Source::env1) + index);

        for (int count = chance (random, 25) ? 2 : 1; count > 0; --count)
            addRoute (source, drawTimbre(), drawLinear (random, { 0.15f, 0.5f }));
    }

    const auto numLFOs = drawWeighted (random, { 3, 4, 2, 1, 1 });
    auto hasVibrato = false;

    for (int index = 0; index < numLFOs; ++index)
    {
        const auto& lfo = parameters.lfos[static_cast<std::size_t> (index)];
        const auto source = static_cast<Source> (static_cast<int> (Source::lfo1) + index);
        const auto role = drawWeighted (random, { hasVibrato ? 0 : 2, 1, timbres.empty() ? 0 : 3 });

        builder.set (lfo.phase, random.nextFloat());

        if (role == 0)
        {
            // Every operator moves by the same cents, so the FM ratios hold.
            hasVibrato = true;
            const auto depth = drawLinear (random, { 0.03f, 0.08f });

            for (int op = 0; op < numOperators; ++op)
                addRoute (source, parameters.oscillators[static_cast<std::size_t> (op)].fine, depth);

            builder.set (lfo.shape, static_cast<float> (random.nextBool() ? Shape::sine : Shape::triangle));
            builder.set (lfo.rate, drawLinear (random, { 4.5f, 6.5f }));
            builder.set (lfo.retrigger, static_cast<float> (SynthLFOValues::RetriggerMode::free));
        }
        else if (role == 1)
        {
            auto sign = random.nextBool() ? 1.0f : -1.0f;

            for (int op = 0; op < numOperators; ++op)
            {
                if (targets[static_cast<std::size_t> (op)] >= 0)
                    continue;

                addRoute (source, parameters.oscillators[static_cast<std::size_t> (op)].pan, sign * drawLinear (random, { 0.2f, 0.5f }));
                sign = -sign;
            }

            const Shape shapes[] = { Shape::sine, Shape::triangle, Shape::smoothRandom };
            builder.set (lfo.shape, static_cast<float> (drawFrom (random, shapes)));
            builder.set (lfo.rate, drawExponential (random, { 0.1f, 1.5f }));
            builder.set (lfo.retrigger, static_cast<float> (SynthLFOValues::RetriggerMode::free));
        }
        else
        {
            addRoute (source, drawTimbre(), (random.nextBool() ? 1.0f : -1.0f) * drawLinear (random, { 0.05f, 0.25f }));

            const Shape shapes[] = { Shape::sine, Shape::sine, Shape::triangle, Shape::triangle, Shape::sawtooth, Shape::sawDown,
                                     Shape::square, Shape::sampleAndHold, Shape::smoothRandom, Shape::smoothRandom };
            builder.set (lfo.shape, static_cast<float> (drawFrom (random, shapes)));
            builder.set (lfo.retrigger, static_cast<float> (random.nextBool() ? SynthLFOValues::RetriggerMode::retrigger : SynthLFOValues::RetriggerMode::free));

            // From eight bars to an eighth when synced.
            const auto synced = chance (random, 30);
            builder.set (lfo.tempoSync, synced ? 1.0f : 0.0f);
            builder.set (lfo.division, static_cast<float> (random.nextInt (7)));
            builder.set (lfo.rate, drawExponential (random, { 0.05f, 2.0f }));
        }
    }

    if (! timbres.empty())
    {
        if (chance (random, 50))
            addRoute (Source::velocity, drawTimbre(), drawLinear (random, { 0.1f, 0.3f }));

        if (chance (random, 40))
            addRoute (Source::modWheel, drawTimbre(), drawLinear (random, { 0.2f, 0.4f }));

        if (chance (random, 25))
            addRoute (Source::aftertouch, drawTimbre(), drawLinear (random, { 0.1f, 0.3f }));
    }

    std::vector<std::pair<Source, yup::String>> routeIDs;

    for (std::size_t slot = 0; slot < routes.size(); ++slot)
    {
        routeIDs.emplace_back (routes[slot].source, routes[slot].destination->getID());
        builder.set (parameters.modulation.depths[slot], routes[slot].depth);
    }

    builder.addChild (createRoutesTree (routeIDs));

    for (int index = 1; index < SynthExample::envelopeCount; ++index)
        builder.set (parameters.envelopes[static_cast<std::size_t> (index)].enabled, index <= numEnvelopes ? 1.0f : 0.0f);

    for (int index = 0; index < SynthExample::lfoCount; ++index)
        builder.set (parameters.lfos[static_cast<std::size_t> (index)].enabled, index < numLFOs ? 1.0f : 0.0f);

    //==============================================================================
    for (std::size_t index = 0; index < parameters.filters.size(); ++index)
    {
        const auto& filter = parameters.filters[index];
        builder.set (filter.enabled, filterChoices[index] != nullptr ? 1.0f : 0.0f);

        if (filterChoices[index] == nullptr)
            continue;

        // A cutoff a route sweeps up starts lower, so the sweep opens it.
        const auto sweptUp = std::any_of (routes.begin(), routes.end(), [&filter] (const Route& route) { return route.destination == filter.cutoff && route.depth > 0.0f; });

        builder.set (filter.type, static_cast<float> (filterChoices[index]->type));
        builder.set (filter.cutoff, drawExponential (random, filterChoices[index]->cutoff) * (sweptUp ? 0.5f : 1.0f));
        builder.set (filter.resonance, drawLinear (random, { 0.05f, 0.5f }));
        builder.set (filter.drive, chance (random, 30) ? drawLinear (random, { 0.05f, 0.3f }) : 0.0f);
        builder.set (filter.keytrack, random.nextBool() ? drawLinear (random, { 0.2f, 0.6f }) : 0.0f);
    }

    builder.set (parameters.envelopes[0].velocity, drawLinear (random, { 0.2f, 0.7f }));

    SynthCurveState curves;
    curves.setEnvelope (0, SynthCurveState::makeAdsr (drawExponential (random, character.attack), drawExponential (random, character.decay),
                                                      drawLinear (random, character.sustain), drawExponential (random, character.release),
                                                      drawLinear (random, { -0.7f, -0.3f })));

    for (int index = 1; index <= numEnvelopes; ++index)
    {
        builder.set (parameters.envelopes[static_cast<std::size_t> (index)].velocity, drawLinear (random, { 0.0f, 0.5f }));
        curves.setEnvelope (index, SynthCurveState::makeAdsr (drawExponential (random, { 0.001f, std::max (0.05f, character.attack.high) }),
                                                              drawExponential (random, { 0.1f, 1.5f }), drawLinear (random, { 0.0f, 0.5f }),
                                                              drawExponential (random, { 0.1f, 1.0f }), drawLinear (random, { -0.7f, -0.3f })));
    }

    builder.addChild (curves.createTree());

    return state;
}

void varyState (yup::DataTree& state, const SynthParameters& parameters, yup::Random& random)
{
    using Parameter = yup::AudioParameter::Ptr;

    // The most a control moves, in its normalized range; most moves are far smaller.
    constexpr float width = 0.15f;

    const auto valueOf = [&state] (const Parameter& parameter)
    {
        return readParameterValue (state, *parameter, 0.0f);
    };

    const auto isOn = [&valueOf] (const Parameter& enabled) { return enabled == nullptr || valueOf (enabled) >= 0.5f; };

    StateBuilder builder (state);

    const auto move = [&] (const Parameter& parameter)
    {
        const auto moved = parameter->convertToNormalizedValue (valueOf (parameter)) + (random.nextFloat() - random.nextFloat()) * width;
        builder.set (parameter, parameter->convertToDenormalizedValue (yup::jlimit (0.01f, 1.0f, moved)));
    };

    // A control at its minimum is switched off, like an unused FM link or send, and stays so.
    const auto nudge = [&] (const Parameter& parameter)
    {
        if (valueOf (parameter) > parameter->getMinimumValue())
            move (parameter);
    };

    // The spectral controls rest at their neutral values in the middle of their ranges, so
    // only a shape the patch already has moves; an unshaped waveform stays unshaped.
    const auto nudgeShape = [&] (const Parameter& parameter)
    {
        if (valueOf (parameter) != parameter->getDefaultValue())
            move (parameter);
    };

    for (int op = 0; op < SynthExample::oscillatorCount; ++op)
    {
        const auto& oscillator = parameters.oscillators[static_cast<std::size_t> (op)];

        if (! isOn (oscillator.enabled))
            continue;

        for (const auto& parameter : { oscillator.level, oscillator.pan, oscillator.toFilter1, oscillator.toFilter2, oscillator.direct })
            nudge (parameter);

        for (const auto& parameter : oscillator.getSpectralParameters())
            nudgeShape (parameter);

        for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
            if (isOn (parameters.oscillators[static_cast<std::size_t> (carrier)].enabled))
                nudge (parameters.fm.get (carrier, op));
    }

    for (const auto& filter : parameters.filters)
        if (isOn (filter.enabled))
            for (const auto& parameter : { filter.cutoff, filter.resonance, filter.drive, filter.keytrack })
                nudge (parameter);

    for (const auto& envelope : parameters.envelopes)
        if (isOn (envelope.enabled))
            nudge (envelope.timeScale);

    for (const auto& lfo : parameters.lfos)
        if (isOn (lfo.enabled))
            nudge (lfo.rate);

    // Depths scale by up to 30% either way, so a route in use never stops or turns around.
    for (const auto& depth : parameters.modulation.depths)
        if (const auto value = valueOf (depth); value != 0.0f)
            builder.set (depth, yup::jlimit (-1.0f, 1.0f, value * (1.0f + (random.nextFloat() - random.nextFloat()) * 0.3f)));

    if (valueOf (parameters.unisonVoices) > 1.0f)
    {
        nudge (parameters.unisonDetune);
        nudge (parameters.unisonSpread);
    }
}

//==============================================================================
int getNumFactoryPresets()
{
    return static_cast<int> (getFactoryPresets().size());
}

yup::String getFactoryPresetId (int index)
{
    return getFactoryPresetInfo (index).id;
}

yup::String getFactoryPresetName (int index)
{
    return getFactoryPresetInfo (index).name;
}

yup::String getFactoryPresetFolder (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    return getFactoryPresets()[static_cast<std::size_t> (index)].folder.replace ("/", " / ");
}

PresetInfo getFactoryPresetInfo (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    return getFactoryPresets()[static_cast<std::size_t> (index)].info;
}

yup::DataTree createFactoryState (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    const auto& preset = getFactoryPresets()[static_cast<std::size_t> (index)];

    if (preset.xml.isNotEmpty())
    {
        auto state = yup::DataTree::fromXml (*yup::parseXML (preset.xml));

        {
            auto transaction = state.beginTransaction();
            transaction.setProperty (presetProperty, index);
        }

        return state;
    }

    auto state = createEmptyState (index, preset.info);
    auto transaction = state.beginTransaction();

    // Quality is not automatable, so loading a patch without it would keep the current one.
    transaction.setProperty ("global.quality", static_cast<float> (SynthQuality::x2));

    // Only the first operator plays, and no envelope but the amplitude's and no LFO runs.
    for (int op = 2; op <= SynthExample::oscillatorCount; ++op)
        transaction.setProperty (yup::Identifier ("osc" + yup::String (op) + ".enabled"), 0.0f);

    for (int envelope = 2; envelope <= SynthExample::envelopeCount; ++envelope)
        transaction.setProperty (yup::Identifier ("env" + yup::String (envelope) + ".enabled"), 0.0f);

    for (int lfo = 1; lfo <= SynthExample::lfoCount; ++lfo)
        transaction.setProperty (yup::Identifier ("lfo" + yup::String (lfo) + ".enabled"), 0.0f);

    transaction.addChild (createRoutesTree ({}));
    return state;
}
} // namespace SynthPatch
