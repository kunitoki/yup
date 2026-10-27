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
#include "SynthAlgorithms.h"

#include <algorithm>
#include <numeric>
#include <string>
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
struct PresetValue
{
    std::string id;
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
    const char* id; /**< A fixed UUID, so the user's marks on the preset survive a new name. */
    const char* name;
    const char* category;
    const char* tags;
    const char* description;
    const char* notes;
    std::vector<PresetValue> values;
    std::vector<PresetRoute> routes;
    std::vector<PresetEnvelope> envelopes;
    std::vector<float> osc1Partials; /**< Sine magnitudes from the fundamental up; empty plays the waveform. */
};

constexpr float sine = static_cast<float> (yup::Waveform::sine);
constexpr float sawtooth = static_cast<float> (yup::Waveform::sawtooth);
constexpr float square = static_cast<float> (yup::Waveform::square);
constexpr float triangle = static_cast<float> (yup::Waveform::triangle);
constexpr float pulse = static_cast<float> (yup::Waveform::pulse);

/** Joins value lists, later lists overriding earlier ones. */
std::vector<PresetValue> join (std::initializer_list<std::vector<PresetValue>> lists)
{
    std::vector<PresetValue> result;

    for (const auto& list : lists)
        result.insert (result.end(), list.begin(), list.end());

    return result;
}

/** The values that wire a TX81Z style algorithm: sine operators, every link at one
    depth, operator 4's feedback, and the output operators at one level.
*/
std::vector<PresetValue> algorithm (int number, float depth, float feedback, float level)
{
    const auto& chosen = getSynthAlgorithms()[static_cast<std::size_t> (number - 1)];
    std::vector<PresetValue> values;

    const auto add = [&values] (const std::string& id, float value) { values.push_back ({ id, value }); };

    for (int op = 1; op <= SynthExample::oscillatorCount; ++op)
    {
        const auto isOutput = std::find (chosen.outputs.begin(), chosen.outputs.end(), op) != chosen.outputs.end();
        add ("osc" + std::to_string (op) + ".waveform", sine);
        add ("osc" + std::to_string (op) + ".color", 0.0f);
        add ("osc" + std::to_string (op) + ".level", isOutput ? level : 0.0f);
    }

    for (const auto& link : chosen.links)
        add ("fm." + std::to_string (link.carrier) + std::to_string (link.modulator), depth);

    add ("fm.44", feedback);
    return values;
}

/** Switches off the modules a factory preset does not use: silent operators that modulate
    nothing, and envelopes and LFOs no route reads. An operator a route makes heard or
    modulating counts as used. The first operator always plays, and the filters keep their
    defaults unless the preset sets them. */
std::vector<std::pair<std::string, bool>> getFactoryEnables (const FactoryPreset& preset)
{
    // Later values override earlier ones, as they do when the preset is applied.
    const auto valueOf = [&preset] (const std::string& id)
    {
        for (auto value = preset.values.rbegin(); value != preset.values.rend(); ++value)
            if (value->id == id)
                return value->value;

        return 0.0f;
    };

    const auto isRouted = [&preset] (const std::string& id)
    {
        return std::any_of (preset.routes.begin(), preset.routes.end(), [&id] (const auto& route) { return route.depth != 0.0f && id == route.destination; });
    };

    std::vector<std::pair<std::string, bool>> enables;

    for (int op = 2; op <= SynthExample::oscillatorCount; ++op)
    {
        const auto level = "osc" + std::to_string (op) + ".level";
        auto used = valueOf (level) > 0.0f || isRouted (level);

        for (int carrier = 1; carrier <= SynthExample::oscillatorCount; ++carrier)
        {
            const auto link = "fm." + std::to_string (carrier) + std::to_string (op);
            used = used || (carrier != op && (valueOf (link) != 0.0f || isRouted (link)));
        }

        enables.emplace_back ("osc" + std::to_string (op) + ".enabled", used);
    }

    const auto isSourced = [&preset] (SynthModulationSource source)
    {
        return std::any_of (preset.routes.begin(), preset.routes.end(), [source] (const auto& route) { return route.source == source; });
    };

    for (int envelope = 1; envelope < SynthExample::envelopeCount; ++envelope)
        enables.emplace_back ("env" + std::to_string (envelope + 1) + ".enabled", isSourced (static_cast<SynthModulationSource> (static_cast<int> (SynthModulationSource::env1) + envelope)));

    for (int lfo = 0; lfo < SynthExample::lfoCount; ++lfo)
        enables.emplace_back ("lfo" + std::to_string (lfo + 1) + ".enabled", isSourced (static_cast<SynthModulationSource> (static_cast<int> (SynthModulationSource::lfo1) + lfo)));

    return enables;
}

const std::vector<FactoryPreset>& getFactoryPresets()
{
    using Source = SynthModulationSource;

    const auto noFilter = std::vector<PresetValue> { { "flt1.enabled", 0.0f } };

    static const std::vector<FactoryPreset> presets {
        { "ed6a924c-6fe7-4f9f-8f0e-7bf8b69740ea", "Init", "Init", "init", "A single sine, the starting point.", "", {}, {}, {}, {} },

        { "bdb6e52d-e0a0-4613-aaaa-edc2d2df62b6", "FM E-Piano", "Keys", "fm, keys, electric piano", "Two-operator tine piano with a soft bark.", "Velocity opens the FM.",
          join ({ algorithm (5, 0.2f, 0.0f, 0.6f), noFilter,
                  { { "osc3.fine", 6.0f }, { "osc4.ratio", 14.0f }, { "fm.34", 0.08f } } }),
          { { Source::env2, "fm.12", 0.3f }, { Source::env3, "fm.34", 0.35f }, { Source::velocity, "fm.12", 0.15f } },
          { { 0, 0.002f, 2.5f, 0.0f, 0.4f, -0.6f }, { 1, 0.001f, 1.0f, 0.1f, 0.5f, -0.6f }, { 2, 0.001f, 0.15f, 0.0f, 0.1f, -0.7f } },
          {} },

        { "ee2ad62f-af1e-4ccf-81d8-af8cb9f7127a", "FM Bell", "Bell", "fm, bell, bright", "Inharmonic bell with a long ring.", "",
          join ({ algorithm (5, 0.5f, 0.0f, 0.6f), noFilter,
                  { { "osc2.ratio", 3.5f }, { "osc4.ratio", 3.5f }, { "osc3.fine", 4.0f } } }),
          { { Source::env2, "fm.12", 0.3f }, { Source::env2, "fm.34", 0.3f } },
          { { 0, 0.002f, 4.0f, 0.0f, 3.0f, -0.7f }, { 1, 0.001f, 2.0f, 0.0f, 2.0f, -0.6f } },
          {} },

        { "ffc5ad40-26c8-4522-b7dd-667aff608e6e", "FM Bass", "Bass", "fm, bass", "Punchy FM bass.", "",
          join ({ algorithm (3, 0.35f, 0.5f, 0.8f),
                  { { "global.mode", static_cast<float> (SynthPlayMode::mono) }, { "global.glide", 0.0f },
                    { "osc1.ratio", 0.5f }, { "osc2.ratio", 0.5f }, { "osc4.ratio", 0.5f },
                    { "flt1.type", static_cast<float> (SynthFilterType::lowpass24) }, { "flt1.cutoff", 1800.0f },
                    { "flt1.resonance", 0.25f } } }),
          { { Source::env2, "fm.12", 0.35f } },
          { { 0, 0.002f, 0.4f, 0.6f, 0.12f, -0.5f }, { 1, 0.001f, 0.25f, 0.0f, 0.1f, -0.6f } },
          {} },

        { "8c174bd5-ef1e-4a25-966a-ef3c4ab78895", "FM Brass", "Lead", "fm, brass, lead", "Swelling FM brass.", "",
          join ({ algorithm (3, 0.15f, 0.35f, 0.7f), noFilter, { { "lfo1.rate", 5.5f }, { "lfo1.retrigger", 0.0f } } }),
          { { Source::env2, "fm.12", 0.35f }, { Source::lfo1, "osc1.fine", 0.02f } },
          { { 0, 0.06f, 0.3f, 0.8f, 0.2f, -0.3f }, { 1, 0.08f, 0.4f, 0.6f, 0.2f, -0.3f } },
          {} },

        { "abc76eaa-3bab-4c1a-a5a4-4f83d822dae1", "Spectral Pad", "Pad", "spectral, pad, evolving", "Slowly evolving spectral pad.", "",
          { { "global.unisonVoices", 4.0f }, { "global.unisonDetune", 0.25f }, { "global.unisonSpread", 0.8f },
            { "osc1.ridges", 2.5f }, { "osc1.color", 0.6f }, { "osc1.dispersion", 0.3f }, { "osc1.scatter", 0.2f },
            { "osc2.waveform", sawtooth }, { "osc2.ratio", 0.5f }, { "osc2.level", 0.25f }, { "osc2.ridges", 3.0f },
            { "flt1.cutoff", 3500.0f }, { "flt1.resonance", 0.3f }, { "lfo1.rate", 0.3f } },
          { { Source::lfo1, "osc1.color", 0.3f }, { Source::env2, "flt1.cutoff", 0.2f } },
          { { 0, 0.8f, 1.2f, 0.8f, 2.0f, -0.4f } },
          {} },

        { "0bac86f1-810f-423d-9861-36831e1c2a78", "Drawn Organ", "Keys", "drawn, organ, keys", "Organ drawn from partials.", "",
          join ({ noFilter, { { "lfo1.rate", 6.0f } } }),
          { { Source::lfo1, "osc1.fine", 0.02f } },
          { { 0, 0.005f, 0.1f, 1.0f, 0.08f, 0.0f } },
          { 0.8f, 0.6f, 0.5f, 0.4f, 0.0f, 0.3f, 0.0f, 0.25f } },

        { "4340a4b7-9138-410a-9c4c-c3b1e22b063e", "Pluck Square", "Pluck", "pluck, square", "Short square pluck.", "",
          { { "osc1.waveform", square }, { "osc1.squeeze", 0.1f },
            { "flt1.type", static_cast<float> (SynthFilterType::moogLadder) }, { "flt1.cutoff", 400.0f },
            { "flt1.resonance", 0.35f }, { "flt1.keytrack", 0.3f } },
          { { Source::env2, "flt1.cutoff", 0.45f } },
          { { 0, 0.002f, 0.35f, 0.2f, 0.15f, -0.5f }, { 1, 0.001f, 0.25f, 0.0f, 0.15f, -0.6f } },
          {} },

        // The budget cases of the performance pass, at 4x with a sustained amplitude.
        { "0fef9e10-dfc6-43cb-aaab-117cdc6d9b6e", "Bench / 4 Saws", "Bench", "bench", "Benchmark: four saws.", "CPU measurement patch.",
          { { "global.quality", 1.0f }, { "osc1.level", 0.25f }, { "osc1.color", 0.0f },
            { "osc2.waveform", sawtooth }, { "osc2.level", 0.25f }, { "osc3.waveform", sawtooth }, { "osc3.level", 0.25f },
            { "osc4.waveform", sawtooth }, { "osc4.level", 0.25f } },
          {},
          { { 0, 0.005f, 0.1f, 1.0f, 0.1f, 0.0f } },
          {} },

        { "3f1091d9-1d44-45b4-84c9-cd365eceb072", "Bench / FM 4 Ops", "Bench", "bench, fm", "Benchmark: four FM operators.", "CPU measurement patch.",
          join ({ algorithm (1, 0.3f, 0.3f, 0.8f), { { "global.quality", 1.0f } } }),
          {},
          { { 0, 0.005f, 0.1f, 1.0f, 0.1f, 0.0f } },
          {} },

        { "9d53331f-f30a-4f90-942a-0659fa41c5db", "Bench / FM Unison 4", "Bench", "bench, fm, unison", "Benchmark: FM with unison.", "CPU measurement patch.",
          join ({ algorithm (1, 0.3f, 0.3f, 0.8f), { { "global.quality", 1.0f }, { "global.unisonVoices", 4.0f } } }),
          {},
          { { 0, 0.005f, 0.1f, 1.0f, 0.1f, 0.0f } },
          {} },

        { "01d0f112-366d-4f52-b76a-22dd5e20e174", "Bench / Spectral LFO", "Bench", "bench, spectral", "Benchmark: LFO-driven spectral tilt.", "CPU measurement patch.",
          { { "global.quality", 1.0f }, { "osc1.level", 0.25f }, { "osc1.color", 0.0f },
            { "osc2.waveform", sawtooth }, { "osc2.level", 0.25f }, { "osc3.waveform", sawtooth }, { "osc3.level", 0.25f },
            { "osc4.waveform", sawtooth }, { "osc4.level", 0.25f }, { "lfo1.rate", 2.0f } },
          { { Source::lfo1, "osc1.tilt", 0.25f }, { Source::lfo1, "osc2.tilt", 0.25f },
            { Source::lfo1, "osc3.tilt", 0.25f }, { Source::lfo1, "osc4.tilt", 0.25f } },
          { { 0, 0.005f, 0.1f, 1.0f, 0.1f, 0.0f } },
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

        // Settings that are not part of the sound, like the quality, stay as they are
        // when a patch does not mention them.
        if (! tree.hasProperty (id) && ! parameter->isAutomatable())
            continue;

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

        // Volume and glide act outside the voices, and route depths and macros are read
        // before any route applies, so modulating them would do nothing.
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
        return static_cast<float> (static_cast<double> (state.getProperty (yup::Identifier (parameter->getID()))));
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
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    return getFactoryPresets()[static_cast<std::size_t> (index)].id;
}

yup::String getFactoryPresetName (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    return getFactoryPresets()[static_cast<std::size_t> (index)].name;
}

PresetInfo getFactoryPresetInfo (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    const auto& preset = getFactoryPresets()[static_cast<std::size_t> (index)];
    return { preset.name, "Factory", "kunitoki", preset.tags, preset.category, preset.description, preset.notes, preset.id };
}

yup::DataTree createFactoryState (int index)
{
    if (! yup::isPositiveAndBelow (index, getNumFactoryPresets()))
        return {};

    const auto& preset = getFactoryPresets()[static_cast<std::size_t> (index)];
    auto state = createEmptyState (index, getFactoryPresetInfo (index));
    auto transaction = state.beginTransaction();

    for (const auto& value : preset.values)
        transaction.setProperty (yup::Identifier (value.id.c_str()), value.value);

    for (const auto& [id, enabled] : getFactoryEnables (preset))
        transaction.setProperty (yup::Identifier (id.c_str()), enabled ? 1.0f : 0.0f);

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
